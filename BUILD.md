# 配布版のビルド

同じソースから `Distribution` プロパティで3版を生成する
未指定時は `Floppy` を使用する

| Distribution | 配布サイズ上限 | 用途 |
| --- | --- | --- |
| Floppy | 1,474,560 bytes | オフラインのフロッピー版 |
| Online | なし | 協力プレイ・共有ランキング向け |
| Steam | なし | Online版の機能とSteam連携向け |

Floppy版の2人用はローカル協力プレイ
Steam版の2人用はSteamフレンド招待ロビーとP2P入力同期を使用する
Online版の2人用はGo HTTPサーバー経由の入力同期を使用し、自動マッチング（初期ON、切替・保存可能）と6桁の部屋コードに対応する
Online版のランキングは匿名の共有ランキング（難易度別、1人用と協力用の上位5件）

## Online版の準備

Go側を自分で実装する手順と省略のないコードは [OnlineServerTutorial.md](Docs/OnlineServerTutorial.md) を参照する
サーバーを起動してから `OnlineRelease` または `OnlineDebug` のゲームを起動する
既定接続先は `http://127.0.0.1:8080`、外部サーバーは環境変数 `SPACEYAKUZA_SERVER_URL` でHTTPSのoriginを設定する
例: `$env:SPACEYAKUZA_SERVER_URL = 'https://自分のドメイン'`
パス・クエリ・ユーザー情報付きURL、外部への平文HTTP、リダイレクトは受理しない
公開サーバーのホスティングとHTTPS証明書は別途必要で、この変更だけでは公開URLは発行されない

`START GAME → 2 PLAYERS ONLINE` で自動検索し、OFFにすると非公開部屋の作成と部屋コード参加が使える
両者が `READY`、1Pが `START GAME` で開始する
同じビルドのexeを2台へ配布し、片方が退出・切断した場合は検索または部屋作成をやり直す
HTTP交換は少人数用で、高遅延時には入力待ちで進行が遅くなる
全ステージクリア時のスコアはローカル保存後に送信し、協力時はホストのみ送信する
得点はクライアント申告値であり、アカウント認証・不正スコア検証は含まない

```powershell
# 教材から一時領域へGoコードを抽出し、GoテストとC++の実通信テストを実行する
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RunOnlineTests.ps1
```

## Steam版の準備と2人プレイ

Steamworks SDKを別途取得し、`Steamworks.local.props`（Git管理対象外）に配置先を設定する

```xml
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <SteamworksSdkRoot>D:\ProgramFiles\Steamworks_sdk</SteamworksSdkRoot>
  </PropertyGroup>
</Project>
```

`/p:SteamworksSdkRoot=...` または環境変数 `STEAMWORKS_SDK_ROOT` でも指定できる
既定のApp IDは開発テスト用Spacewarの480
本番App ID取得後は `/p:SteamAppId=取得したID` を指定する
480の配布フォルダーは開発テスト専用で、本番Steamへアップロードしない

1. `SteamRelease` をビルドし、`dist/Steam/x64/SteamRelease` フォルダー全体を相手へ渡す
2. 2台のPCで、それぞれ別のSteamアカウントにログインする（Steam上でフレンドになる）
3. 両者が同じ配布物を起動し、ホストが `START GAME → 2 PLAYERS ONLINE → INVITE FRIEND` を選ぶ
4. 相手がSteamの招待を承認する
5. ホストが難易度、各自が機体を選び、両者が `READY`、ホストが `START GAME` を選ぶ

各PCではキーボードまたは1台目のゲームパッドを使用する
会話送りと視点切替はホストが操作する
どちらかがポーズを開くと両者のゲームが止まり、両者が閉じると再開する
切断時は停止してエラーを表示するため、タイトルに戻って招待し直す
対戦中の途中参加・再接続・ホスト移行は未対応
初回ストーリーとチュートリアルはオンライン開始時には省略する

入力を6固定フレーム先へ送り、両者の入力が揃うまで更新を待つ方式
高遅延時は進行が待機する（予測・ロールバックは未実装）
ビルド番号とCPU構成が一致しない相手は開始できないため、各自で再ビルドせず同じ配布物を使う
SteamNetworkingMessagesを使用し、接続経路はSteamが直接通信またはリレーを選択する
自動テストは入力の順序・待機と同一入力によるゲーム進行一致を検証する
実際の招待・通信・切断の最終確認には2台・2アカウントが必要

```powershell
# 同期入力と既存ゲームの回帰テスト
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RunCoopTests.ps1
# Steamへログインして実行する単独ロビーの接続テスト（招待は送らない）
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RunSteamCoopSmokeTests.ps1
```

## Rider / Visual Studio

`FloppyDiskShootingGame.sln` を開く（既に開いている場合はソリューションを再読み込みする）
新しいIDEでは既存の `FloppyDiskShootingGame.slnx` も使用可能で、両形式の構成は同一
RiderのSLNX対応は2024.2.6以降のため、旧バージョンでは `.sln` を使用する（[JetBrains公式](https://www.jetbrains.com/pages/rider/whatsnew/2024-2/)）
両IDEで共通のビルド構成を使用するため、個別のプロファイル作成や環境変数の設定は不要

| 開発・デバッグ用 | 配布用 |
| --- | --- |
| FloppyDebug | FloppyRelease |
| OnlineDebug | OnlineRelease |
| SteamDebug | SteamRelease |

- Visual Studio: 上部の「ソリューション構成」で上記の構成を選び、プラットフォームを `x64` にしてビルド
- Rider: 上部のビルド構成セレクター（または Build → Change Solution Configuration）で上記の構成と `x64` を選んでビルド
- 起動・デバッグの対象プロジェクトは `FloppyDiskShootingGame` を選択

IDE構成の出力例: `bin/Steam/x64/SteamDebug/SpaceYakuza.exe`
Release構成の配布例: `dist/Steam/x64/SteamRelease/`
通常の `Debug` / `Release` も互換性のため残してあり、版未指定ならFloppy版になる
構成名と矛盾する `/p:Distribution=...` を同時指定するとエラーになる

## コマンドライン

Visual StudioのDeveloper PowerShellでリポジトリ直下から実行する

```powershell
msbuild FloppyDiskShootingGame.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:Distribution=Floppy
msbuild FloppyDiskShootingGame.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:Distribution=Online
msbuild FloppyDiskShootingGame.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:Distribution=Steam
```

`Debug` も同じプロパティで選択可能
IDEと同じ構成を使う場合は `/p:Configuration=SteamRelease /p:Platform=x64` のように指定し、`Distribution` の指定は省略する
x64/Win32の構成は従来どおり使用する（テストプロジェクトはx64のみ）

- 実行ファイル: `bin/<版>/<Platform>/<Configuration>/SpaceYakuza.exe`
- 中間生成物: `obj/<版>/<Platform>/<Configuration>/<Project>/`
- Release配布物: `dist/<版>/<Platform>/Release/`

Releaseビルドでは配布物を自動コピーし、Floppy版のみフォルダー全体の容量を検査する
Releaseのリンクは通常のLTCGを使用し、増分LTCGのキャッシュは再利用しない
MSVCで連続ビルド時に発生するLNK1143を回避するためで、変更のないソースのオブジェクトは再利用する
超過時はビルドを失敗させる（超過した生成物自体は調査用に残る）
ゲームリソースは実行ファイルに埋め込み済み
Steam版のみSDKの `steam_api64.dll`（Win32では `steam_api.dll`）も配布する
開発App ID 480のSteam版には `steam_appid.txt` も同梱する
PDB・静的ライブラリ・編集用MMLは配布しない
配布先に手動追加されたファイルもFloppy版の容量に含まれる

今後必須DLLや外部リソースを追加するときは、ゲームプロジェクトの `DistributionFile` 項目へ必要な版だけ条件付きで登録する
追加するソースやリンクライブラリも `Distribution` 条件で選び、Floppy版にオンライン・Steam依存を含めない
C++から版を判別するときは `SPACEYAKUZA_EDITION_Floppy`、`SPACEYAKUZA_EDITION_Online`、`SPACEYAKUZA_EDITION_Steam` のうち定義されたマクロを使用する
ゲーム全体に条件分岐を散らさず、各機能の接続部分に限定する
エンジンとテストも同じ版で生成され、生成物が混ざらない
プロジェクト・構成を追加するときは `.sln` と `.slnx` の両方を更新する

回帰チェック:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RunDistributionTests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RunReleaseLinkTests.ps1 -Distribution Steam
```
