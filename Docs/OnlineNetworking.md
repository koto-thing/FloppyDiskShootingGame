# Online版の通信と更新手順

Online版のロビーと入力同期は、`/v2/stream` の認証済みWebSocket接続を使用する。ランキング・スコア・セッション作成は従来のHTTPを使用する

## 配布とサーバー更新

この変更はクライアントとサーバーの両方の更新が必要。旧サーバーでは新版クライアントを接続できないため、サーバーを先に更新し、次に新版Online版を配布する。サーバー再起動中の試合は切断されるため、プレイしていない時間帯に更新する

新版サーバーは旧HTTPプロトコルも受け付ける。旧版と新版は同じ部屋へ入れない。2人とも同じビルドの新版Online版を使用する

Go 1.22以降で、必ずソースからサーバーを再ビルドする。リポジトリ内の既存バイナリ `Server/space_yakuza_server` はこの変更で更新していない

```sh
cd Server
go mod download
go test ./...
go build -o space_yakuza_server .
```

既存のスコアファイルと `SPACEYAKUZA_SCORE_FILE`、`SPACEYAKUZA_LISTEN`、TLS関連の環境変数はそのまま引き継ぐ。スコアデータを実行ファイルと一緒に置き換えない

TLSをリバースプロキシで終端する場合、`/v2/stream` のWebSocket Upgradeを許可し、接続タイムアウトを60秒以上にする。Nginxの専用locationの例は次のとおり。上流ポートは既存構成に合わせる

参考: [NginxのWebSocket転送設定](https://nginx.org/en/docs/http/websocket.html)

```nginx
location = /v2/stream {
    proxy_pass http://127.0.0.1:8080;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_set_header Host $host;
    proxy_read_timeout 60s;
    proxy_send_timeout 60s;
}
```

クライアントは `Configuration=OnlineRelease`、`Platform=x64` でビルドする。接続先は既定の `https://game.koto-thing.com`、開発時は `SPACEYAKUZA_SERVER_URL=http://127.0.0.1:8080` で変更できる

### 現在のRaspberry Pi環境への転送

確認済みの構成はCPUが `aarch64`、SSH接続先が `pi`、サービスが `space-yakuza.service`。作成済みのLinux ARM64版をWindowsのPowerShellから転送する

```powershell
scp "D:\Pandd\FloppyDiskShootingGame\dist\OnlineServer\linux-arm64\space_yakuza_server" pi:~/space_yakuza_server/space_yakuza_server.new
ssh pi
```

サーバー側では、サービスのExecStartが `~/space_yakuza_server/space_yakuza_server` を指していることを確認してから更新する

```bash
systemctl show space-yakuza.service -p ExecStart -p WorkingDirectory
```

```bash
cd ~/space_yakuza_server &&
chmod +x space_yakuza_server.new &&
backup="space_yakuza_server.bak.$(date +%Y%m%d-%H%M%S)" &&
cp -p space_yakuza_server "$backup" &&
sudo systemctl stop space-yakuza.service &&
mv space_yakuza_server.new space_yakuza_server &&
sudo systemctl start space-yakuza.service

systemctl status space-yakuza.service --no-pager
sudo journalctl -u space-yakuza.service -n 50 --no-pager
```

失敗した場合は同じSSHシェル内で戻す

```bash
sudo systemctl stop space-yakuza.service &&
cp -p "$backup" space_yakuza_server &&
sudo systemctl start space-yakuza.service
```

## 入力遅延の合意

以下の可変先行送信はWebSocket対応済みサーバーに対してクライアント更新だけで利用できる。2人とも同じ新版クライアントを使用する

- 各クライアントはロビーで250ms間隔、試合中は1秒間隔にPINGを送信し、通信スレッドの送信開始からPONG受信完了までを計測する。受信後の描画スレッドの待ち時間はRTTに含めない
- 両者が最低3回計測するまで開始ボタンを有効にしない
- 開始時に各プレイヤーの直近8回のRTTの中央値（偶数件は中央の大きい側）を採用し、`ceil((((RTT_A + RTT_B) / 2) + 50ms) × 60 / 1000)` フレームを両者へ通知する。一時的な大きな遅延を試合全体の固定入力遅延にしない
- 遅延は6〜30フレームに制限する。Steam版の既定値は6フレームのまま
- 開始時に埋める空フレーム数と入力番号は試合中も変えない。Onlineクライアントは待機して送信枠が埋まった時だけ、先行送信の上限を1フレームずつ増やす。追加は最大6フレーム（100ms）、合計は最大30フレーム
- 待機または50msを超える入力到着間隔を検出したら、先行量の縮小を5秒間見送る。バッファで揺らぎを吸収できている間も必要な余裕を保持する。揺らぎが収まれば0.5秒ごとに1フレームずつ開始値へ戻す。縮小時は既存の入力を残したまま送信を1回見送り、押下入力は次の送信まで保持する
- 先行量は各クライアントで調整するが、既存の連続した入力番号を使い、受け取った同じ番号の入力だけで進める。サーバーの通信形式はそのまま使用できる。通信が上限より大幅に悪化した場合は待機が発生し得る。入力を推測して進める処理やロールバックは追加していない
- 相手の入力が届く前にゲームを進めない。送受信キューが上限を超えた場合も、入力を捨てずに接続を失敗扱いにする

操作は取得した固定更新から通信スレッドへ直ちに渡し、次の描画フレームのPollを待たない。送信は応答待ちに依存せず、HTTPランキング通信とは別スレッドで進む。サーバーも入力を受け取った時点で相手へプッシュし、相手の次の要求を待たない

## 計測結果

試合の退出またはアプリ終了時に `%LOCALAPPDATA%/SpaceYakuza/online-network.log` へ1行保存する

- `delay_frames`: 合意した入力遅延
- `logged_at` / `build`: 保存したUTC日時／クライアントのビルド名
- `rtt_ms` / `rtt_max_ms`: 直近／接続中の最大往復時間
- `main_thread_lag_max_ms`: 受信完了から描画スレッドが処理するまでの最大待ち時間
- `input_gap_max_ms`: クライアントが相手入力を取り込む間隔の最大値
- `send_queue_max_ms`: 入力やPINGを予約してから通信スレッドが取り出すまでの最大時間
- `send_max_ms`: WebSocket送信を開始してからWindowsの送信完了通知までの最大時間。相手への到着確認時間ではない
- `send_ahead_frames` / `send_ahead_max_frames`: 現在／試合中最大の先行送信上限。`delay_frames`は開始時の合意値のまま
- `waits`: 入力待ちの発生回数
- `wait_total_ms` / `wait_max_ms`: 入力待ちの合計／最長時間
- `status`: 退出時の接続状態
- `failure_reason`: 最初の失敗原因。`none`、`server_error`、`transport_error`、`local_error`、`input_timeout`、`receive_timeout`、`ping_timeout`、`rtt_timeout`

ログは1MiBを超えた後の保存時に切り替える。認証トークンや部屋コードは記録しない。サーバー側には接続終了時のメッセージ数と処理時間の合計・最大値を記録する。最大処理時間には共有ロックの待ち時間も含む

入力の30秒監視は実際に入力を要求するプレイ中だけ行う。クリア後に入力送信を終えても、PING/PONGが正常ならタイムアウト扱いにしない。最初の失敗原因を後続の監視で上書きしない

失敗が確定した時点で進行中の待機時間も確定する。切断画面でタイトルへ戻るまでの時間は入力待ちに加算しない

## 検証

WindowsでVisual Studio C++ Build Tools、Go、Python 3.11以降を使用する。Pythonは標準ライブラリだけを使用するテスト用で、ゲーム配布には不要

```powershell
pwsh -NoProfile -File Tests/RunOnlineTests.ps1
```

実際の `Server/*.go` を隔離先でビルドし、認証・部屋・サーバープッシュ・切断・入力遅延の上下限を検証する。その後、C++クライアント2台で通常接続と各方向40〜65msの遅延・揺らぎを加えた接続を検証する。各試行で60Hz・360フレーム分の入力一致、待機回数、試合中のランキング通信、退出後の再参加を確認する

最後にTCP接続を開いたまま通信を途絶させ、タイムアウトと2秒以内の退出を検証する

入力遅延の回帰テストとして、描画相当の処理を600ms停止してもRTTに加算しないこと、送信側の次のPollを待たず相手へ操作が届くこと、単発の800ms遅延で入力バッファが最大値に固定されないことを確認する

入力送信終了後に31秒間接続を保てること、その後の相手退出がタイムアウトで上書きされないことも確認する

周期的に下り通信へ120msの遅延を追加する試験では、1,440フレームの入力一致、先行量の増加と上限、回線回復後の開始値への復帰を確認する。v020の固定バッファでは待機235更新、可変先行では15更新だった（両者合計、開始10フレーム、先行最大15フレーム）

3秒間隔で120msの遅延を追加する試験では、2,160フレームの入力一致と、最初の6秒後に待機が繰り返されないこと、回線回復後の先行量復帰も確認する

公開経路だけを調べる場合は次を実行する。2台分のクライアントを同じPCで起動し、専用部屋で60秒分の入力を交換する。公開マッチングやスコア送信は行わず、保存先も試験用に分離する。回線の変動による待機回数は合否判定にしない

```powershell
pwsh -NoProfile -File Tests/RunOnlineTests.ps1 -ProbeServerUrl https://game.koto-thing.com
```

### 2026-09-26の実経路調査

- ユーザーの最新記録は遅延8フレーム（約133ms）、待機3,219〜3,252回、待機合計約245〜249秒。サーバーの処理最長は約0.84msだが、この値にネットワーク送信・到着の時間は含まれない
- 描画のない実クライアント2台でも、公開経路では60秒分の進行に対して約239〜285回、合計約16〜21秒の待機を再現した（開始遅延7フレーム）。追加計測では送信キュー0〜1ms、Windows送信完了は1ms未満
- 別実装のGoクライアントでも、各方向1,200入力の到着時間は中央値約61ms、95パーセンタイル約170〜173ms、最大約264〜288msだった。ゲーム固有の描画・送信キューだけでは説明できず、現在の固定バッファを実経路の揺らぎが超えている
- 同一PC内の接続、および各方向40〜65msに制限した遅延プロキシでは360フレームの待機は0回だった。この試験は実経路で観測した170〜288msの入力到着を再現していない
- ユーザー確認ではCaddyとGoサーバーは同じRaspberry Pi内で接続し、Piは有線LANを使用。回線・ルーター・Caddyのどの区間が揺らいでいるかは未特定。サーバーの処理時間だけを根拠にCaddyを含む配信経路全体が正常とは判断できない
- 旧ログの`PARTNER TIMED OUT`はクリア後の入力監視やエラー上書きで付くため、実際の切断を示すとは限らない。この調査で両方の不具合を修正した

### 可変先行送信の改善比較

2026-09-26 23:58〜23:59 JSTに、同じPCからv020相当と改善後のクライアント各2台を別々の専用部屋へ接続し、各試行で60秒分の入力を処理した。開始遅延はいずれも7フレーム。実回線の変動を含むため、この改善率を他の試行に保証するものではない

| 指標 | v020相当 | 改善後 |
|---|---:|---:|
| 待機合計（各クライアント） | 2,579 / 2,631ms | 399 / 384ms |
| 待機回数 | 41 / 42回 | 16 / 14回 |
| 最長待機 | 131 / 197ms | 51 / 46ms |
| 先行送信上限 | 固定7フレーム | 最大13フレーム、終了時11 / 9フレーム |

両者が消費した3,600フレームの入力内容と順序は一致した。周期的な遅延を与えるローカル比較でも待機235更新から15更新へ減少し、回線回復後は両者とも開始時の先行量へ戻った

### v022の断続的な待機対策

v021は待機が2秒間なくなると先行量を減らしていたため、バッファで吸収できた揺らぎを安定と判断していた。v022では入力到着間隔が50msを超えた場合も縮小を5秒間延期し、安定後は0.5秒ごとに1フレーム戻す。追加先行の上限は従来どおり6フレーム（100ms）、合計30フレームまで

2026-09-27の3秒間隔・120ms追加遅延の比較では、2,160フレームに対する両者合計の待機更新はv021の21から14へ、最初の6秒を除く遅延継続中の待機更新は7から0へ減った。修正版は全入力が一致し、先行量は最大15から回復後10へ戻った。通常接続、頻繁な遅延、切断監視を含むオンライン試験も通過した

同日06:34〜06:35 JSTの実サーバー比較では、同じPCから別々の専用部屋で60秒分を処理した。回線が比較的安定しており、v021とv022の待機合計は各クライアント52msと51ms、最長はいずれも35msで、実経路での改善差は確認できなかった。両版とも入力一致を確認した。この結果は別回線での実プレイの無停止を保証しない

サーバーのデータ競合チェックは、Cコンパイラのある環境で `cd Server` の後に `go test -race ./...` を実行する

本番更新後は、別回線の2台で接続とゲーム開始を確認し、試合後の両者のログを比較する。ローカルでの遅延再現テストは、本番のTLS・プロキシ設定や実回線の動作を保証するものではない
