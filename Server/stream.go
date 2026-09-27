package main

import (
	"fmt"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

type socketPeer struct {
	out  chan string
	done chan struct{}
	once sync.Once
}

// stopは送受信ループへ終了を通知する
// @return なし
func (p *socketPeer) stop() { p.once.Do(func() { close(p.done) }) }

// offerは共有ロックを通信で止めずに応答を予約する
// @param body 応答
// @return なし
func (p *socketPeer) offer(body string) {
	select {
	case <-p.done:
	case p.out <- body:
	default:
		// 遅い受信者は切断し、入力を捨てたままゲームを続けない
		p.stop()
	}
}

// serveStreamは認証済みの接続へ入力とロビー変更を即時配信する
// @param w 応答
// @param req 要求
// @return なし
func (s *server) serveStream(w http.ResponseWriter, req *http.Request) {
	if req.Method != http.MethodGet {
		http.Error(w, "method not allowed", 405)
		return
	}
	token, ok := strings.CutPrefix(req.Header.Get("Authorization"), "Bearer ")
	peer := &socketPeer{out: make(chan string, 256), done: make(chan struct{})}
	s.mu.Lock()
	p := s.sessions[token]
	if !ok || p == nil || !p.streamRequired || time.Since(p.last) > 30*time.Second {
		s.mu.Unlock()
		http.Error(w, "unauthorized", 401)
		return
	}
	if p.stream != nil {
		s.mu.Unlock()
		http.Error(w, "already connected", 409)
		return
	}
	p.stream = peer
	s.mu.Unlock()

	// 接続予約と部屋は失敗や切断時にも必ず回収する
	defer func() {
		peer.stop()
		s.mu.Lock()
		s.leave(p)
		delete(s.sessions, token)
		s.mu.Unlock()
	}()
	upgrader := websocket.Upgrader{HandshakeTimeout: 3 * time.Second, ReadBufferSize: 4096, WriteBufferSize: 4096}
	conn, err := upgrader.Upgrade(w, req, nil)
	if err != nil {
		return
	}
	defer conn.Close()
	conn.SetReadLimit(8192)

	// 書き込みは接続ごとの単一ループで行い、遅い相手を他の部屋から隔離する
	writerDone := make(chan struct{})
	go func() {
		defer close(writerDone)
		defer conn.Close()
		for {
			select {
			case <-peer.done:
				return
			case body := <-peer.out:
				if conn.SetWriteDeadline(time.Now().Add(3*time.Second)) != nil || conn.WriteMessage(websocket.TextMessage, []byte(body)) != nil {
					return
				}
			}
		}
	}()
	defer func() { peer.stop(); <-writerDone }()
	started, window := time.Now(), time.Now()
	count, messages := 0, 0
	var processing, slowest time.Duration
	defer func() {
		log.Printf("stream closed duration=%s messages=%d processing_total=%s processing_max=%s", time.Since(started).Round(time.Millisecond), messages, processing, slowest)
	}()
	for {
		// サイズ、無通信時間、送信頻度を接続単位で制限する
		if conn.SetReadDeadline(time.Now().Add(15*time.Second)) != nil {
			return
		}
		kind, body, err := conn.ReadMessage()
		if err != nil || kind != websocket.TextMessage {
			return
		}
		now := time.Now()
		if now.Sub(window) >= time.Second {
			window, count = now, 0
		}
		count++
		if count > 240 {
			return
		}
		messages++
		// 期限切れやHTTP退出後の接続から部屋を作り直させない
		s.mu.Lock()
		if s.sessions[token] != p {
			s.mu.Unlock()
			return
		}
		p.last = now
		if strings.HasPrefix(string(body), "PING ") {
			s.mu.Unlock()
			fields := strings.Fields(string(body))
			if len(fields) != 2 {
				return
			}
			id, valid := number(fields[1], 4294967295)
			if !valid {
				return
			}
			peer.offer(fmt.Sprintf("PONG %d\n", id))
			continue
		}

		// 状態更新と配信順だけをロックし、ネットワークI/Oは含めない
		result := s.exchange(p, string(body))
		peer.offer(result)
		if p.room != nil {
			other := p.room.players[1-p.player]
			if other != nil && other.stream != nil {
				other.stream.offer(s.exchange(other, ""))
			}
		}
		s.mu.Unlock()
		elapsed := time.Since(now)
		processing += elapsed
		slowest = max(slowest, elapsed)
	}
}
