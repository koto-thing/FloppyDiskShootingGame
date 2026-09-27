package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/gorilla/websocket"
)

// streamClientは実WebSocketの認証と接続を行う
// @param t テスト @param s サーバー @param url HTTPアドレス
// @return 接続とトークン
func streamClient(t *testing.T, s *server, url string) (*websocket.Conn, string) {
	t.Helper()
	code, body := request(s, "/v1/session", "", "2 same\n")
	if code != 200 {
		t.Fatal(code, body)
	}
	token := strings.TrimSpace(body)
	conn, _, err := websocket.DefaultDialer.Dial("ws"+strings.TrimPrefix(url, "http")+"/v2/stream", http.Header{"Authorization": {"Bearer " + token}})
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { conn.Close() })
	return conn, token
}

// streamWriteはテストメッセージを送信する
// @param t テスト @param c 接続 @param body メッセージ
// @return なし
func streamWrite(t *testing.T, c *websocket.Conn, body string) {
	t.Helper()
	c.SetWriteDeadline(time.Now().Add(2 * time.Second))
	if err := c.WriteMessage(websocket.TextMessage, []byte(body)); err != nil {
		t.Fatal(err)
	}
}

// streamUntilは期待するサーバープッシュまで読み進める
// @param t テスト @param c 接続 @param match 期待する部分文字列
// @return 一致したメッセージ
func streamUntil(t *testing.T, c *websocket.Conn, match string) string {
	t.Helper()
	c.SetReadDeadline(time.Now().Add(2 * time.Second))
	for {
		_, body, err := c.ReadMessage()
		if err != nil {
			t.Fatal(err)
		}
		if strings.Contains(string(body), match) {
			return string(body)
		}
	}
}

// TestStreamPushはポーリングなしの入力配信、遅延合意、突然の切断を検証する
// @param t テスト
// @return なし
func TestStreamPush(t *testing.T) {
	s := fresh(t)
	httpServer := httptest.NewServer(s)
	defer httpServer.Close()
	a, token := streamClient(t, s, httpServer.URL)
	b, _ := streamClient(t, s, httpServer.URL)
	streamWrite(t, a, "CREATE\n")
	state := strings.Fields(streamUntil(t, a, "STATE "))
	streamWrite(t, b, "JOIN "+state[1]+"\n")
	streamUntil(t, b, "STATE ")
	streamUntil(t, a, " 0 2 ")
	if code, _ := request(s, "/v1/exchange", token, ""); code != 409 {
		t.Fatal("HTTP accepted on stream session")
	}

	// 異なる回線の代表値を合成し、両者へ同じ15フレームを配信する
	streamWrite(t, a, "LATENCY 120\nLATENCY 160\nLATENCY 150\nREADY 1\n")
	streamUntil(t, a, "STATE ")
	streamWrite(t, b, "LATENCY 220\nLATENCY 240\nLATENCY 230\nREADY 1\n")
	streamUntil(t, a, " 6 1\n")
	streamWrite(t, a, "START\n")
	streamUntil(t, a, " 15 1\n")
	streamUntil(t, b, " 15 1\n")
	streamWrite(t, a, "FRAME 15 32 64\nFRAME 16 1 256\n")
	streamUntil(t, b, "FRAME 0 15 32 64\nFRAME 0 16 1 256\n")

	// 入力送信なしでもPINGへ応答し、退出操作なしの切断を相手へ通知する
	streamWrite(t, b, "PING 123\n")
	streamUntil(t, b, "PONG 123\n")
	a.Close()
	streamUntil(t, b, "ERROR PEER_LEFT\n")
}

// TestStreamValidationは認証、重複接続、メッセージ上限と送信キュー上限を検証する
// @param t テスト
// @return なし
func TestStreamValidation(t *testing.T) {
	s := fresh(t)
	httpServer := httptest.NewServer(s)
	defer httpServer.Close()
	url := "ws" + strings.TrimPrefix(httpServer.URL, "http") + "/v2/stream"
	if c, response, err := websocket.DefaultDialer.Dial(url, nil); err == nil {
		c.Close()
		t.Fatal("missing auth accepted")
	} else if response.StatusCode != 401 {
		t.Fatal(response.StatusCode)
	}
	a, token := streamClient(t, s, httpServer.URL)
	if c, response, err := websocket.DefaultDialer.Dial(url, http.Header{"Authorization": {"Bearer " + token}}); err == nil {
		c.Close()
		t.Fatal("duplicate connection accepted")
	} else if response.StatusCode != 409 {
		t.Fatal(response.StatusCode)
	}
	streamWrite(t, a, strings.Repeat("x", 8193))
	a.SetReadDeadline(time.Now().Add(2 * time.Second))
	if _, _, err := a.ReadMessage(); err == nil {
		t.Fatal("oversized message accepted")
	}
	peer := &socketPeer{out: make(chan string, 1), done: make(chan struct{})}
	peer.offer("first")
	peer.offer("second")
	select {
	case <-peer.done:
	default:
		t.Fatal("slow consumer not disconnected")
	}
}

// TestStreamDelayBoundsは開始前計測、上限、開始後の再STARTと旧版隔離を検証する
// @param t テスト
// @return なし
func TestStreamDelayBounds(t *testing.T) {
	for _, ms := range []uint32{0, 10000} {
		s := fresh(t)
		a, b := &session{streamRequired: true, build: "same"}, &session{streamRequired: true, build: "same"}
		s.command(a, []string{"MATCH"})
		legacy := &session{build: "same"}
		if fault := s.command(legacy, []string{"JOIN", a.room.code}); fault != "ROOM_UNAVAILABLE" {
			t.Fatal("mixed protocol private room accepted")
		}
		s.command(legacy, []string{"MATCH"})
		if legacy.room == a.room {
			t.Fatal("mixed protocol matchmaking")
		}
		s.command(b, []string{"MATCH"})
		s.command(a, []string{"READY", "1"})
		s.command(b, []string{"READY", "1"})
		s.command(a, []string{"START"})
		if a.room.started {
			t.Fatal("started before calibration")
		}
		for i := 0; i < 3; i++ {
			s.command(a, []string{"LATENCY", fmt.Sprint(ms)})
			s.command(b, []string{"LATENCY", fmt.Sprint(ms)})
		}
		s.command(a, []string{"START"})
		want := uint32(6)
		if ms != 0 {
			want = 30
		}
		if !a.room.started || a.room.delay != want || a.room.next != [2]uint32{want, want} {
			t.Fatal("wrong negotiated delay")
		}
		s.command(a, []string{"FRAME", fmt.Sprint(want), "0", "0"})
		s.command(a, []string{"START"})
		if a.room.next[0] != want+1 {
			t.Fatal("repeated START reset frame sequence")
		}
	}
}

// TestStreamLatencySpikeは一時的な遅延だけで試合全体の入力を重くしないことを検証する
// @param t テスト
// @return なし
func TestStreamLatencySpike(t *testing.T) {
	for _, samples := range [][]string{{"20", "800", "20"}, {"20", "20", "800", "20", "20", "700", "20", "20"}} {
		s := fresh(t)
		a, b := &session{streamRequired: true, build: "same"}, &session{streamRequired: true, build: "same"}
		s.command(a, []string{"MATCH"})
		s.command(b, []string{"MATCH"})
		for _, value := range samples {
			s.command(a, []string{"LATENCY", value})
			s.command(b, []string{"LATENCY", value})
		}
		s.command(a, []string{"READY", "1"})
		s.command(b, []string{"READY", "1"})
		s.command(a, []string{"START"})
		if !a.room.started || a.room.delay != 6 {
			t.Fatalf("spike inflated input delay: %d", a.room.delay)
		}
	}
}
