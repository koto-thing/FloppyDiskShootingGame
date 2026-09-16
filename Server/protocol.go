package main

import (
	"fmt"
	"strconv"
	"strings"
)

// numberは符号なしの10進数だけを受理する
// @param text 入力文字列
// @param max 最大値
// @return 数値と検証結果
func number(text string, max uint32) (uint32, bool) {
	if text == "" || strings.IndexFunc(text, func(r rune) bool { return r < '0' || r > '9' }) >= 0 {
		return 0, false
	}

	value, err := strconv.ParseUint(text, 10, 32)
	return uint32(value), err == nil && value <= uint64(max)
}

// commandは1行の操作を検証し、本人が変更できる状態だけを更新する
// @param p 送信者
// @param fields 空白で分けた操作
// @return エラーコード又は空文字
func (s *server) command(p *session, fields []string) string {
	if len(fields) == 0 {
		return "BAD_COMMAND"
	}

	r := p.room
	if r == nil {
		switch fields[0] {
		case "MATCH", "CREATE":
			if len(fields) != 1 {
				return "BAD_COMMAND"
			}

			if fields[0] == "MATCH" {
				for _, candidate := range s.rooms {
					if candidate.public && !candidate.started && candidate.players[1] == nil && candidate.players[0].build == p.build {
						candidate.players[1], p.room, p.player = p, candidate, 1
						return ""
					}
				}
			}

			if err := s.createRoom(p, fields[0] == "MATCH"); err != nil {
				return "SERVER_BUSY"
			}

			return ""

		case "JOIN":
			if len(fields) != 2 || len(fields[1]) != 6 {
				return "BAD_COMMAND"
			}

			if _, ok := number(fields[1], 999999); !ok {
				return "BAD_COMMAND"
			}

			candidate := s.rooms[fields[1]]

			if candidate == nil || candidate.started || candidate.players[1] != nil || candidate.players[0].build != p.build {
				return "ROOM_UNAVAILABLE"
			}

			candidate.players[1], p.room, p.player = p, candidate, 1
			return ""
		}

		return "NO_ROOM"
	}

	// 準備状態はサーバーが管理し、送信者が他人の設定を指定できないようにする
	switch fields[0] {
	case "SHOT", "DIFFICULTY", "READY":
		if len(fields) != 2 {
			return "BAD_COMMAND"
		}

		max := uint32(2)
		if fields[0] == "READY" {
			max = 1
		}

		value, ok := number(fields[1], max)
		if !ok {
			return "BAD_COMMAND"
		}

		if r.started {
			return ""
		}

		switch fields[0] {
		case "SHOT":
			r.shots[p.player], r.ready[p.player] = value, 0
		case "DIFFICULTY":
			if p.player != 0 {
				return "HOST_ONLY"
			}
			if r.ready[0] == 0 && r.ready[1] == 0 {
				r.difficulty = value
			}
		case "READY":
			r.ready[p.player] = value
		}

	case "START":
		if len(fields) != 1 || p.player != 0 {
			return "HOST_ONLY"
		}

		if r.players[1] != nil && r.ready == [2]uint32{1, 1} {
			r.started = true
		}

	case "FRAME":
		if len(fields) != 4 || !r.started {
			return "BAD_FRAME"
		}

		frame, a := number(fields[1], 4294967294)
		held, b := number(fields[2], 2047)
		pressed, c := number(fields[3], 2047)
		peer := r.players[1-p.player]
		if !a || !b || !c || frame != r.next[p.player] || peer == nil || len(peer.inbox) >= 256 || uint64(frame) > uint64(r.next[1-p.player])+256 {
			return "BAD_FRAME"
		}

		peer.inbox = append(peer.inbox, fmt.Sprintf("FRAME %d %d %d %d\n", p.player, frame, held, pressed))
		r.next[p.player]++

	default:
		return "BAD_COMMAND"
	}

	return ""
}

// exchangeは操作を適用して、状態と相手の入力を返す
// @param p 接続
// @param body 操作行
// @return 応答
func (s *server) exchange(p *session, body string) string {
	if p.fault != "" {
		return "ERROR " + p.fault + "\n"
	}

	lines := strings.Split(strings.TrimSuffix(body, "\n"), "\n")
	if len(lines) > 128 {
		return "ERROR BAD_COMMAND\n"
	}

	if body != "" {
		for _, line := range lines {
			if fault := s.command(p, strings.Fields(line)); fault != "" {
				s.leave(p)
				p.fault = fault
				return "ERROR " + fault + "\n"
			}
		}
	}

	r := p.room
	if r == nil {
		return "ERROR NO_ROOM\n"
	}

	members, started := 1, 0

	if r.players[1] != nil {
		members = 2
	}

	if r.started {
		started = 1
	}

	result := fmt.Sprintf("STATE %s %d %d %d %d %d %d %d %d %d\n", r.code, p.player, members, r.difficulty, r.shots[0], r.shots[1], r.ready[0], r.ready[1], started, r.seed)
	result += strings.Join(p.inbox, "")

	p.inbox = nil
	return result
}
