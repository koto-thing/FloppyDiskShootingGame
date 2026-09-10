# 楽器・声データの可逆圧縮比較

検証日: 2026-09-08

## 対象と方法

- `Infrastructure/ExternalServices/WavSamplesSource.h`: 楽器・打楽器40配列、71,079 bytes（導入前はWavSamples.h）
- `Presentation/Gameplay/Voices/VoiceSamplesSource.h`: 声14配列、61,833 bytes（導入前はVoiceSamples.h）
- 合計54配列、132,912 bytes（129.80 KiB）
- 使用頻度による選別や音源削除は行わず、両ヘッダーにある全uint8_t配列を宣言順に抽出
- 楽器の既存4bit差分符号化データと、声の既存IMA ADPCMデータに追加の可逆圧縮を適用
- C++ソースの文字列や展開後PCMではなく、実際の圧縮済みバイト列を比較
- 個別圧縮、カテゴリ別一括圧縮（楽器／声の2ストリーム）、全一括圧縮（楽器→声の1ストリーム）を比較
- 各方式のストリームヘッダーを含むサイズ。既存の名前、ループ位置、サンプル数、レート等のメタデータは変更せず、比較対象外

## 結果

単位はbytes。「削減率」は追加圧縮前132,912 bytesに対する全一括の削減率

| 方式 | 個別圧縮の合計 | 楽器一括 | 声一括 | カテゴリ別の合計 | 全一括 | 全一括の削減量 | 削減率 |
|---|---:|---:|---:|---:|---:|---:|---:|
| zlib / DEFLATE level 9 | 113,323 | 56,013 | 56,339 | 112,352 | 113,009 | 19,903 | 14.97% |
| bzip2 level 9 | 123,611 | 53,630 | 57,402 | 111,032 | 114,180 | 18,732 | 14.09% |
| LZMA2 / XZ preset 9 | 117,156 | 52,036 | 53,968 | **106,004** | **106,164** | **26,748** | **20.12%** |
| Windows MSZIP | 114,601 | 56,259 | 56,070 | 112,329 | 113,174 | 19,738 | 14.85% |
| Windows XPRESS | 133,931 | 71,107 | 61,861 | 132,968 | 132,940 | -28 | -0.02% |
| Windows XPRESS-HUFF | 122,899 | 57,972 | 56,198 | 114,170 | 115,350 | 17,562 | 13.21% |
| Windows LZMS | 124,329 | 56,738 | 56,792 | 113,530 | 113,864 | 19,048 | 14.33% |

最小はLZMA2のカテゴリ別圧縮で、26,908 bytes（26.28 KiB、20.24%）削減。全一括との差は160 bytesのみ

既存MML実装と同じWindows LZMSを使う場合、全一括で19,048 bytes（18.60 KiB）、カテゴリ別で19,382 bytes（18.93 KiB、14.58%）削減。Windows標準API方式の最小はMSZIPのカテゴリ別で20,583 bytes（20.10 KiB、15.49%）削減

全一括が常に最小になるわけではなく、今回の入力順・設定ではXPRESS以外はカテゴリ別の方が小さい

## 復元検証と速度

7方式すべてで、54配列の個別圧縮、2カテゴリの一括圧縮、全一括圧縮について展開後のバイト完全一致をassertで確認。全一括の復元データを元の境界で54配列へ切り分け、それぞれのSHA-256も一致確認済み。音源の追加劣化なし

全一括の展開時間の参考値（7回の中央値、Python 3.14、Windows、APIハンドル作成とバッファ確保を含む。ゲーム実装での測定ではない）:

| 方式 | 展開時間 ms |
|---|---:|
| zlib | 0.484 |
| bzip2 | 5.172 |
| LZMA2 | 4.670 |
| MSZIP | 0.713 |
| XPRESS | 0.010 |
| XPRESS-HUFF | 0.537 |
| LZMS | 1.949 |

## 実装判断に使える範囲

容量重視ならLZMA2、既存実装との共通化ならLZMSが候補。Windows標準APIだけで比較するとMSZIPも有力

上記の比較表は音源ペイロードの実測であり、実行ファイル全体の削減量ではない。LZMA2を採用する場合は復号器の実行ファイルへの追加容量も比較が必要。MSZIPの組み込み結果は次節のとおり

## 復号コードを含めた実装結果

追加ライブラリを必要としないWindows標準API方式の最小候補、MSZIPのカテゴリ別圧縮を採用。既存MMLと同じcabinet.libを利用し、楽器・声の2ストリームを初回参照時に一度だけ復元する。132,912 bytesの復元バッファを保持し、以降はオフセットから参照。PCM復号、ループ、声のレートや初期予測値は維持

同じ作業ツリーで変更前後をRelease / x64、MSVC v145、MinSpace設定でビルドして比較:

| 対象 | 変更前 bytes | 変更後 bytes | 削減 bytes |
|---|---:|---:|---:|
| 音源ペイロード | 132,912 | 112,329 | 20,583 |
| 実行ファイル（復号コード・索引・アラインメント込み） | **1,342,976** | **1,322,496** | **20,480（20 KiB、1.52%）** |

実行ファイル単体で1,440,000 bytesに対し117,504 bytesの余裕。復号器本体はWindowsのcabinet.dllを使用するため配布ファイルへの追加不要。LZMA2の組み込み比較は今回行っていない

生成元の全音源を`*SamplesSource.h`へ保存し、通常のゲームコードからは生成済み`WavSamples.h`・`VoiceSamples.h`と`PackedAudioSamples.h`を使用する。`PackedAudioSamples.cpp`がOS復号APIの呼び出しを担当。生成元はテストから直接参照し、全54配列の完全一致を検証

検証済み:

- ゲームとテストのRelease / x64ビルド成功
- 全40楽器・14声の圧縮前バイト列およびメタデータとC++側復元結果が一致
- 全楽器のPCMチェックサムと全声のADPCM復号後サンプル数を確認
- 破損・切り詰め・復元サイズ不一致・範囲外・空参照・不足ADPCMを拒否
- 既存の音声・ゲームテスト一式成功（`TimeTests passed`）
- 全14曲の事前生成とXAudio2の無音再生チェック成功
- 生成物の`--check`と差分の空白チェック成功

ゲーム画面での操作・聴感による試聴は未実施。テスト結果は`Temp/AudioPackedTestsRun.log`、変更前後の実行ファイルは`Temp/AudioPackedBuild/SpaceYakuza-before.exe`と`Temp/AudioPackedBuild/SpaceYakuza.exe`

## 再実行

リポジトリルートで以下を実行（Windows、Python標準ライブラリのみ、追加依存なし）:

```powershell
python Scripts/CompareAudioCompression.py
```

`Temp/AudioCompressionComparison/results.json`に比較表と全54配列の名前・元ファイル・オフセット・サイズ・SHA-256を出力。同じフォルダに各方式の全一括圧縮データを保存。入力ヘッダーとゲームコードは変更しない

組み込みデータの生成・検証:

```powershell
python Scripts/PackAudioSamples.py
python Scripts/PackAudioSamples.py --check
```

ゲームとテストのプロジェクトにはビルド前の生成処理を追加済み。ビルド環境ではPATH上のPythonが必要（標準ライブラリのみ）。音源編集は`WavSamplesSource.h`・`VoiceSamplesSource.h`を更新する

実行したビルドコマンド（PowerShell、リポジトリルート）:

```powershell
& 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe' FloppyDiskShootingGame.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:OutDir=D:/Pandd/FloppyDiskShootingGame/Temp/AudioPackedBuild/ /p:IntDir=D:/Pandd/FloppyDiskShootingGame/Temp/AudioPackedObj/ /v:minimal /nologo
& 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe' FloppyEngine.Tests.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:OutDir=D:/Pandd/FloppyDiskShootingGame/Temp/AudioPackedTests/ /p:IntDir=D:/Pandd/FloppyDiskShootingGame/Temp/AudioPackedTestsObj/ /v:minimal /nologo
& './Temp/AudioPackedTests/FloppyEngine.Tests.exe'
```

上記の比較用IntDir指定では参照プロジェクトと中間フォルダを共有するためMSB8028警告が出る。プロジェクト既定のIntDirはプロジェクト別に分離されており、通常ビルドではこの上書き指定は不要
