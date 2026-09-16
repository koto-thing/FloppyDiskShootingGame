package main

import (
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"strings"
	"time"
)

// ServeHTTPはHTTPの要求の上限、認証、経路を管理する
// @param w 応答際
// @param req 要求
// @return なし
func (s *server) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Header().Set("Cache-Control", "no-store")
	w.Header().Set("X-Content-Type-Options", "nosniff")
	if req.Method != http.MethodPost && req.Method != http.MethodGet {
		http.Error(w, "method not allowed", 405)
		return
	}

	// 読み取りはロックの外で行い、遅い送信者で他のゲームを止めない
	body, err := io.ReadAll(http.MaxBytesReader(w, req.Body, 8192))
	if err != nil {
		http.Error(w, "body too large or incomplete", 413)
		return
	}

	s.mu.Lock()

	defer s.mu.Unlock()

	now := time.Now()
	s.expire(now)

	ip, _, err := net.SplitHostPort(req.RemoteAddr)
	if err != nil {
		http.Error(w, "invalid address", 400)
		return
	}

	limit, exists := s.limits[ip]
	if !exists && len(s.limits) >= 4096 {
		http.Error(w, "busy", 503)
		return
	}

	if now.Sub(limit.at) >= time.Second {
		limit = allowance{at: now}
	}

	limit.count++
	s.limits[ip] = limit
	if limit.count > 480 {
		http.Error(w, "rate limited", 429)
		return
	}

	if req.URL.Path == "/v1/rankings" && req.Method == http.MethodGet {
		mode, ok := number(req.URL.Query().Get("coop"), 1)
		if !ok {
			http.Error(w, "invalid mode", 400)
			return
		}

		result := "RANKS"
		for _, scores := range s.scores[mode] {
			for _, score := range scores {
				result += fmt.Sprintf(" %d", score)
			}
		}

		fmt.Fprintln(w, result)

		return
	}

	if req.Method != http.MethodPost {
		http.Error(w, "not found", 404)
		return
	}

	switch req.URL.Path {
	case "/v1/session":
		fields := strings.Fields(string(body))
		if len(fields) != 2 || fields[0] != "1" || len(fields[1]) > 96 ||
			strings.IndexFunc(fields[1], func(r rune) bool {
				return !(r >= 'a' && r <= 'z' || r >= 'A' && r <= 'Z' || r >= '0' && r <= '9' || r == '_' || r == '-' || r == '.')
			}) >= 0 {
			http.Error(w, "invalid protocol or build", 400)
			return
		}

		if len(s.sessions) >= 256 {
			http.Error(w, "busy", 503)
			return
		}

		var bytes [32]byte
		if _, err := rand.Read(bytes[:]); err != nil {
			http.Error(w, "random unavailable", 503)
			return
		}

		token := hex.EncodeToString(bytes[:])
		if s.sessions[token] != nil {
			http.Error(w, "retry", 503)
			return
		}

		s.sessions[token] = &session{build: fields[1], last: now}
		fmt.Fprintln(w, token)

	case "/v1/exchange", "/v1/leave":
		header := req.Header.Get("Authorization")
		token, ok := strings.CutPrefix(header, "Bearer ")
		p := s.sessions[token]
		if !ok || p == nil {
			http.Error(w, "unauthorized", 401)
			return
		}

		p.last = now
		if req.URL.Path == "/v1/leave" {
			s.leave(p)
			delete(s.sessions, token)
			fmt.Fprintln(w, "OK")
			return
		}

		fmt.Fprint(w, s.exchange(p, string(body)))

	case "/v1/scores":
		// このランキングは匿名、不正値のチェックはしない
		if err := s.saveScore(string(body)); err != nil {
			log.Printf("score rejected: %v", err)
			http.Error(w, "score rejected or storage unavailable", 400)
			return
		}

		fmt.Fprintln(w, "OK")

	default:
		http.Error(w, "not found", 404)
	}
}
