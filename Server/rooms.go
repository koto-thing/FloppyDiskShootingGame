package main

import (
	"crypto/rand"
	"errors"
	"math/big"
	"strconv"
	"time"
)

// randomNumberは推測しにくい番号を生成する
// @param max 排他的上限
// @return 乱数とエラー
func randomNumber(max int64) (uint32, error) {
	n, err := rand.Int(rand.Reader, big.NewInt(max))
	if err != nil {
		return 0, err
	}

	return uint32(n.Int64()), nil
}

// leaveは部屋全体を閉じて、相手へ切断を通知する
// @param p 退出する接続
// @return なし
func (s *server) leave(p *session) {
	r := p.room
	if r == nil {
		return
	}

	delete(s.rooms, r.code)

	for _, peer := range r.players {
		if peer != nil {
			peer.room = nil
			peer.inbox = nil
			peer.fault = "PEER_LEFT"
		}
	}
}

// expireは通信が途切れた接続と古い制限カウンターを回収する
// @param now 現在時刻
// @return なし
func (s *server) expire(now time.Time) {
	for token, p := range s.sessions {
		if now.Sub(p.last) > 30*time.Second {
			s.leave(p)
			delete(s.sessions, token)
		}
	}

	for ip, limit := range s.limits {
		if now.Sub(limit.at) > time.Minute {
			delete(s.limits, ip)
		}
	}
}

// createRoomは自動検索用または、非公開の部屋を作る
// @param p ホスト
// @param public 自動検索対象か
// @return サーバーとエラー
func (s *server) createRoom(p *session, public bool) error {
	for tries := 0; tries < 32; tries++ {
		n, err := randomNumber(900000)
		if err != nil {
			return err
		}

		code := strconv.FormatUint(uint64(n+100000), 10)
		if s.rooms[code] != nil {
			continue
		}

		seed, err := randomNumber(4294967295)
		if err != nil {
			return err
		}

		r := &room{code: code, public: public, seed: seed + 1, next: [2]uint32{6, 6}}
		r.players[0], p.room, p.player = p, r, 0
		s.rooms[code] = r
		return nil
	}

	return errors.New("room capacity")
}
