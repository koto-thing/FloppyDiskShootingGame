package main

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"strings"
	"sync"
	"time"
)

type session struct {
	build  string
	room   *room
	player int
	last   time.Time
	inbox  []string
	fault  string
}

type room struct {
	code       string
	public     bool
	players    [2]*session
	difficulty uint32
	shots      [2]uint32
	ready      [2]uint32
	started    bool
	seed       uint32
	next       [2]uint32
}

type allowance struct {
	at    time.Time
	count int
}

type server struct {
	mu          sync.Mutex
	sessions    map[string]*session
	rooms       map[string]*room
	limits      map[string]allowance
	scores      [2][3][5]uint32
	scoreFile   string
	storeFailed bool
}

// newServerは保持済みのスコアを検証して復元する
// @param path 保持ファイル
// @return サーバーとエラー
func newServer(path string) (*server, error) {
	s := &server{
		sessions:  make(map[string]*session),
		rooms:     make(map[string]*room),
		limits:    make(map[string]allowance),
		scoreFile: path,
	}

	file, err := os.Open(path)
	if errors.Is(err, os.ErrNotExist) {
		return s, nil
	}

	if err != nil {
		return nil, err
	}

	defer file.Close()
	input := bufio.NewReader(file)
	for {
		line, err := input.ReadString('\n')
		if err == io.EOF && line == "" {
			break
		}

		if err != nil {
			return nil, fmt.Errorf("score file incomplete: %w", err)
		}

		fields := strings.Fields(line)
		if len(fields) != 3 {
			return nil, errors.New("invalid score file")
		}

		d, a := number(fields[0], 2)
		mode, b := number(fields[1], 1)
		score, c := number(fields[2], 999999999)
		if !a || !b || !c || score == 0 {
			return nil, errors.New("invalid saved score")
		}

		insert(&s.scores[mode][d], score)
	}

	return s, nil
}
