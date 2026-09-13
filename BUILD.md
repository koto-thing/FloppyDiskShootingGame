# 配布版のビルド

同じソースから `Distribution` プロパティで3版を生成する
未指定時は `Floppy` を使用する

| Distribution | 配布サイズ上限 | 用途 |
| --- | --- | --- |
| Floppy | 1,474,560 bytes | オフラインのフロッピー版 |
| Online | なし | 協力プレイ・共有ランキング向け |
| Steam | なし | Online版の機能とSteam連携向け |

現段階では3版とも同じゲーム内容で、通信・ランキング・Steam SDKは未実装
版のマクロは将来の機能選択用であり、実装済み機能を示すものではない

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
超過時はビルドを失敗させる（超過した生成物自体は調査用に残る）
現在の必須リソースは実行ファイルに埋め込み済みなので、配布対象はexeのみ
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
```
