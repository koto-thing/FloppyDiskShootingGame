package main

import (
	"errors"
	"fmt"
	"io"
	"os"
	"strings"
)

// insertは上位5剣だけを保持する
// @param scores 変更対象
// @param score スコア
// @return なし
func insert(scores *[5]uint32, score uint32) {
	for i := range scores {
		if score > scores[i] {
			score, scores[i] = scores[i], score
		}
	}
}

// saveScoreは永続化の成功後だけランキングを変更する
// @param body 難易度、協力フラグ、得点
// @return エラー
func (s *server) saveScore(body string) error {
	fields := strings.Fields(body)
	if len(fields) != 3 {
		return errors.New("invalid score")
	}

	d, a := number(fields[0], 2)
	mode, b := number(fields[1], 1)
	score, c := number(fields[2], 999999999)
	if !a || !b || !c || score == 0 {
		return errors.New("invalid score")
	}

	if s.storeFailed {
		return errors.New("score storage unavailable")
	}

	file, err := os.OpenFile(s.scoreFile, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0600)
	if err != nil {
		return err
	}

	line := fmt.Sprintf("%d %d %d\n", d, mode, score)
	n, err := io.WriteString(file, line)

	if err == nil && n != len(line) {
		err = io.ErrShortWrite
	}

	if err == nil {
		err = file.Sync()
	}

	closeErr := file.Close()

	if err == nil {
		err = closeErr
	}

	if err != nil {
		s.storeFailed = true
		return err
	}

	insert(&s.scores[mode][d], score)

	return nil
}
