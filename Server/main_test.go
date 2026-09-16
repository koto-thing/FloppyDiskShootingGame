package main

import (
    "net/http/httptest"
    "os"
    "path/filepath"
    "strings"
    "testing"
    "time"
)

// requestはHTTP入口をメモリ内で呼ぶ
// @param s 対象 
// @param path 経路 
// @param token 接続トークン 
// @param body 本文 
// @return HTTPステータスと応答
func request(s *server, path, token, body string) (int, string) {
    method := "POST"
    if strings.HasPrefix(path, "/v1/rankings") { 
        method = "GET" 
    }

    req := httptest.NewRequest(method, path, strings.NewReader(body))
    if token != "" { 
        req.Header.Set("Authorization", "Bearer "+token) 
    }

    out := httptest.NewRecorder()
    s.ServeHTTP(out, req)
    return out.Code, out.Body.String()
}

// connectはテスト接続を作る
// @param t テスト 
// @param s 対象 
// @param build ビルド識別 
// @return トークン
func connect(t *testing.T, s *server, build string) string {
    t.Helper()
    
    code, body := request(s, "/v1/session", "", "1 "+build+"\n")
    if code != 200 || len(body) != 65 { 
        t.Fatalf("session: %d %q", code, body) 
    }

    return strings.TrimSpace(body)
}

// freshは一時保存先でサーバーを作る
// @param t テスト 
// @return サーバー
func fresh(t *testing.T) *server {
    t.Helper()
    
    s, err := newServer(filepath.Join(t.TempDir(), "scores.log"))
    if err != nil { 
        t.Fatal(err) 
    }

    return s
}

// TestRoomsは自動検索、非公開部屋、ビルド不一致と満室を検証する
// @param t テスト
// @return なし
func TestRooms(t *testing.T) {
    s := fresh(t)
    a, b, c := connect(t, s, "same"), connect(t, s, "same"), connect(t, s, "same")
    request(s, "/v1/exchange", a, "CREATE\n")
    private := s.sessions[a].room
    request(s, "/v1/exchange", b, "MATCH\n")
    if s.sessions[b].room == private { 
        t.Fatal("private room entered by matchmaking") 
    }

    request(s, "/v1/exchange", c, "MATCH\n")
    if s.sessions[b].room != s.sessions[c].room || s.sessions[b].player == s.sessions[c].player { 
        t.Fatal("match failed") 
    }

    outsider := connect(t, s, "other-build")
    _, body := request(s, "/v1/exchange", outsider, "JOIN "+private.code+"\n")
    if body != "ERROR ROOM_UNAVAILABLE\n" { 
        t.Fatal(body) 
    }

    d := connect(t, s, "same")
    request(s, "/v1/exchange", d, "JOIN "+private.code+"\n")
    if s.sessions[d].room != private || s.sessions[d].player != 1 { 
        t.Fatal("join failed") 
    }

    e := connect(t, s, "same")
    _, body = request(s, "/v1/exchange", e, "JOIN "+private.code+"\n")
    if body != "ERROR ROOM_UNAVAILABLE\n" { 
        t.Fatal("full room accepted") 
    }

    request(s, "/v1/leave", a, "")
    _, body = request(s, "/v1/exchange", d, "")
    if body != "ERROR PEER_LEFT\n" { 
        t.Fatal("disconnect not notified") 
    }
}

// TestFramesは準備条件、送信者の割当てとフレーム順序を検証する
// @param t テスト 
// @return なし
func TestFrames(t *testing.T) {
    s := fresh(t)
    a, b := connect(t, s, "same"), connect(t, s, "same")
    request(s, "/v1/exchange", a, "MATCH\n")
    request(s, "/v1/exchange", b, "MATCH\n")
    r := s.sessions[a].room
    request(s, "/v1/exchange", a, "START\n")
    if r.started { 
        t.Fatal("started before ready") 
    }

    request(s, "/v1/exchange", a, "DIFFICULTY 2\nSHOT 2\nREADY 1\n")
    request(s, "/v1/exchange", b, "READY 1\n")
    request(s, "/v1/exchange", a, "START\nFRAME 6 32 64\n")
    if !r.started || r.difficulty != 2 || r.shots[0] != 2 { 
        t.Fatal("start configuration") 
    }

    _, body := request(s, "/v1/exchange", b, "FRAME 6 1 256\n")
    if !strings.Contains(body, "FRAME 0 6 32 64\n") { 
        t.Fatal(body) 
    }

    _, body = request(s, "/v1/exchange", a, "")
    if !strings.Contains(body, "FRAME 1 6 1 256\n") { 
        t.Fatal(body) 
    }

    _, body = request(s, "/v1/exchange", a, "FRAME 6 0 0\n")
    if body != "ERROR BAD_FRAME\n" { 
        t.Fatal("duplicate frame accepted") 
    }

    if len(s.rooms) != 0 { 
        t.Fatal("invalid sender left room running")
    }
}

// TestValidationは境界値と認証を検証する
// @param t テスト @return なし
func TestValidation(t *testing.T) {
    for _, word := range []string{ "-1", "+1", "1x", "4294967296", "", "1.0" } {
        if _, ok := number(word, 4294967295); ok { 
            t.Fatalf("accepted %q", word) 
        }
    }

    s := fresh(t)
    if code, _ := request(s, "/v1/exchange", "wrong", "MATCH\n"); code != 401 { t.Fatal("missing auth") }
    if code, _ := request(s, "/v1/session", "", "2 same\n"); code != 400 { t.Fatal("protocol mismatch") }
    if code, _ := request(s, "/v1/session", "", strings.Repeat("x", 8193)); code != 413 { t.Fatal("large body") }
    for _, command := range []string{"FRAME 6 2048 0\n", "FRAME -1 0 0\n", "FRAME 8 0 0\n", "FRAME 6 0 0 extra\n"} {
        state := fresh(t)
        a, b := connect(t, state, "same"), connect(t, state, "same")
        request(state, "/v1/exchange", a, "MATCH\nREADY 1\n")
        request(state, "/v1/exchange", b, "MATCH\nREADY 1\n")
        request(state, "/v1/exchange", a, "START\n")
        _, body := request(state, "/v1/exchange", a, command)
        if body != "ERROR BAD_FRAME\n" { 
            t.Fatal(command, body) 
        }
    }

    a, b := connect(t, s, "same"), connect(t, s, "same")
    request(s, "/v1/exchange", a, "MATCH\n")
    request(s, "/v1/exchange", b, "MATCH\n")
    _, body := request(s, "/v1/exchange", b, "DIFFICULTY 2\n")
    if body != "ERROR HOST_ONLY\n" { 
        t.Fatal("guest changed host setting") 
    }
}

// TestExpiryは切断者の回収と相手通知を検証する
// @param t テスト @return なし
func TestExpiry(t *testing.T) {
    s := fresh(t)
    a, b := connect(t, s, "same"), connect(t, s, "same")
    request(s, "/v1/exchange", a, "MATCH\n")
    request(s, "/v1/exchange", b, "MATCH\n")
    s.sessions[a].last = time.Now().Add(-31*time.Second)
    s.expire(time.Now())
    if s.sessions[a] != nil || len(s.rooms) != 0 || s.sessions[b].fault != "PEER_LEFT" {
        t.Fatal("expiry failed") 
    }
}

// TestScoresはモード分離、再起動、保存失敗と破損検出を検証する
// @param t テスト @return なし
func TestScores(t *testing.T) {
    s := fresh(t)
    for _, body := range []string{ "0 0 100\n", "0 0 50\n", "0 0 200\n", "2 1 500\n" } {
        if code, _ := request(s, "/v1/scores", "", body); code != 200 { 
            t.Fatal("save failed") 
        }
    }

    if s.scores[0][0] != [5]uint32{200, 100, 50, 0, 0} || s.scores[1][2][0] != 500 { 
        t.Fatal("ranking sort or mode") 
    }

    restored, err := newServer(s.scoreFile)
    if err != nil || restored.scores != s.scores { 
        t.Fatal("restore failed", err) 
    }

    for _, body := range []string{"3 0 1", "0 2 1", "0 0 -1", "0 0 0", "0 0 1000000000", "0 0 1 extra"} {
        if code, _ := request(s, "/v1/scores", "", body); code == 200 { 
            t.Fatal("bad score accepted", body)
        }
    }

    before := s.scores
    s.scoreFile = filepath.Join(t.TempDir(), "missing", "scores.log")
    if err := s.saveScore("0 0 999"); err == nil || s.scores != before { 
        t.Fatal("failed write changed ranking")
    }

    path := filepath.Join(t.TempDir(), "broken.log")
    if err := os.WriteFile(path, []byte("0 0 100\n0 0 20"), 0600); err != nil { 
        t.Fatal(err) 
    }

    if _, err := newServer(path); err == nil { 
        t.Fatal("partial record silently accepted") 
    }
}