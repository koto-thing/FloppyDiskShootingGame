# Goサーバーの構成

同じフォルダーの `package main` を役割ごとに分けている
ファイル間の関数・型はそのまま参照でき、追加のパッケージやライブラリは不要

| ファイル | 役割 | 主な定義 |
| --- | --- | --- |
| `main.go` | 環境変数、HTTP/TLS起動、定期的な切断回収 | `main` |
| `server.go` | 共有データの型、サーバー初期化と保存済みスコアの復元 | `session`, `room`, `allowance`, `server`, `newServer` |
| `http.go` | HTTP経路、認証、要求サイズと頻度の制限 | `ServeHTTP` |
| `rooms.go` | 部屋の作成、乱数、退出、期限切れ接続の回収 | `createRoom`, `randomNumber`, `leave`, `expire` |
| `protocol.go` | 数値検証、マッチング・準備・入力コマンド、応答の組み立て | `number`, `command`, `exchange` |
| `scores.go` | 上位5件の更新とスコアの永続保存 | `insert`, `saveScore` |
| `main_test.go` | HTTP入口を使った部屋・入力同期・検証・保存のテスト | `TestRooms` など |

処理は `main.go → http.go → protocol.go → rooms.go / scores.go` の順に読む
例えば、自動マッチングの条件は `protocol.go` の `MATCH`、HTTPの受付先は `http.go`、スコア保存は `scores.go` を変更する
通信仕様、環境変数、保存形式は分割前と同じ

## 起動・テスト

`Server` フォルダーで、Windows用のビルド設定のPowerShellから実行する

```powershell
$go = "C:\Program Files\Go\bin\go.exe"
$env:GOOS = "windows"
$env:GOARCH = "amd64"
& $go test ./...
& $go run .
```

分割したすべてのファイルを含めるため、`go run main.go` ではなく `go run .` を使う

## ラズパイ向けビルド

64ビットLinux向けの例

```powershell
$go = "C:\Program Files\Go\bin\go.exe"
$env:GOOS = "linux"
$env:GOARCH = "arm64"
$env:CGO_ENABLED = "0"
& $go build -o space_yakuza_server .
```

ファイル分割後も配布する実行ファイルは1つ
ラズパイでの実行コマンド、CaddyやDNSの設定は変更不要
今回のファイル整理自体では、稼働中のラズパイへアップロード・再起動は行わない

## ラズパイへ更新を反映する

現在の手動起動方式で、配置先が `~/spaceyakuza` の場合の手順
別の場所で起動している場合は、その場所へ読み替える（`scores.log` の保存先を変えないため）

1. **WindowsのPowerShell**で再ビルドし、別名で転送する

   ```powershell
   cd D:\Pandd\FloppyDiskShootingGame\Server
   $go = "C:\Program Files\Go\bin\go.exe"
   $env:GOOS = "linux"
   $env:GOARCH = "arm64"
   $env:CGO_ENABLED = "0"
   & $go build -o space_yakuza_server .
   if ($LASTEXITCODE -ne 0) { throw "ビルド失敗" }
   scp .\space_yakuza_server pandd@192.168.10.102:~/spaceyakuza/space_yakuza_server.new
   if ($LASTEXITCODE -ne 0) { throw "転送失敗" }
   ```

2. 転送成功後、**ラズパイのサーバーを起動しているターミナル**で `Ctrl+C` を押し、差し替えて起動する
   更新中はプレイが切断されるので、利用者がいないときに行う

   ```bash
   cd ~/spaceyakuza
   chmod +x space_yakuza_server.new && cp -p space_yakuza_server space_yakuza_server.bak && mv space_yakuza_server.new space_yakuza_server && ./space_yakuza_server
   ```

   旧実行ファイルは `.bak` に残る。`scores.log` は削除・上書きしない
   起動したターミナルは開いたままにする。Caddy・DNS・ポート開放の再設定は不要

3. **WindowsのPowerShell**で接続を確認する

   ```powershell
   curl.exe --connect-timeout 10 "https://game.koto-thing.com/v1/rankings?coop=0"
   ```

   `RANKS` と数字が返れば更新完了
   元に戻す場合はサーバーを `Ctrl+C` で止め、ラズパイの同じフォルダーで実行する

   ```bash
   cp -p space_yakuza_server.bak space_yakuza_server && ./space_yakuza_server
   ```

最初の学習用コードと通信仕様は [制作ガイド](../Docs/OnlineServerTutorial.md) を参照する
制作ガイドの1ファイル版を、現在の分割済み `main.go` に上書きすると定義が重複するため、保守時はこのフォルダー内の対応ファイルを編集する
