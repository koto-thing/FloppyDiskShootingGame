package main

import (
	"log"
	"net/http"
	"os"
	"time"
)

// mainはローカル開発用または、TLSサーバーを起動する
// @return なし
func main() {
	path := os.Getenv("SPACEYAKUZA_SCORE_FILE")
	if path == "" {
		path = "scores.log"
	}

	state, err := newServer(path)
	if err != nil {
		log.Fatal(err)
	}

	address := os.Getenv("SPACEYAKUZA_LISTEN")
	if address == "" {
		address = "127.0.0.1:8080"
	}

	srv := &http.Server{Addr: address, Handler: state, ReadHeaderTimeout: 3 * time.Second,
		ReadTimeout: 5 * time.Second, WriteTimeout: 5 * time.Second, IdleTimeout: 45 * time.Second,
		MaxHeaderBytes: 8192,
	}

	// 操作が来ない時も切断とメモリを回収する
	go func() {
		ticker := time.NewTicker(time.Second)
		defer ticker.Stop()
		for now := range ticker.C {
			state.mu.Lock()
			state.expire(now)
			state.mu.Unlock()
		}
	}()

	log.Printf("Space Yakuza server listening on %s", address)

	cert, key := os.Getenv("SPACEYAKUZA_TLS_CERT"), os.Getenv("SPACEYAKUZA_TLS_KEY")
	if cert != "" || key != "" {
		log.Fatal(srv.ListenAndServeTLS(cert, key))
	}

	log.Fatal(srv.ListenAndServe())
}
