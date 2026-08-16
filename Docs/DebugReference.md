# Debugリファレンス

## 概要

`Debug` は、エンジンとゲームから共通して利用できる静的ログAPIです。
Unityの `Debug.Log`、`Debug.LogWarning`、`Debug.LogError` に近い用途で使用します。

ログは次の場所へ同時に出力されます。

- Visual Studioのデバッグ出力ウィンドウ
- 実行ファイルと同じ場所に作られる `Logs/latest.log`

`latest.log` はアプリケーションを起動するたびに新しく作り直されます。

## ヘッダー

```cpp
#include "Engine/Diagnostics/Debug.h"
```

## 初期化と終了

ログをファイルへ出力するには、アプリケーションの開始時に `Initialize()` を呼び出します。
終了時には `Shutdown()` を呼び出します。

```cpp
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    Debug::Initialize();
    Debug::Log("Application starting");

    // ゲーム処理

    Debug::Log("Application shutting down");
    Debug::Shutdown();
    return 0;
}
```

`Initialize()` はログファイルを開けた場合に `true`、開けなかった場合に `false` を返します。
ファイルを開けなかった場合でも、Visual Studioへのデバッグ出力は利用できます。

現在は [main.cpp](../main.cpp) で初期化と終了が行われているため、通常のゲーム処理から個別に呼ぶ必要はありません。

## ログレベル

### Info

通常の動作状況や開発中の値を記録します。

```cpp
Debug::Log("Player initialized");
```

InfoログはDebugビルドでのみ出力されます。Releaseビルドでは呼び出しても出力されません。

### Warning

処理は継続できるものの、確認が必要な状態を記録します。

```cpp
Debug::LogWarning("Constant buffer capacity is nearly full");
```

WarningログはDebugビルドとReleaseビルドの両方で出力されます。

### Error

処理の失敗や、正常な動作を継続できない状態を記録します。

```cpp
Debug::LogError("Renderer initialization failed");
```

ErrorログはDebugビルドとReleaseビルドの両方で出力されます。

## HRESULTの記録

DirectXやWin32 APIの失敗結果を記録する場合は `LogHResult()` を使用します。

```cpp
HRESULT hr = D3D12CreateDevice(
    nullptr,
    D3D_FEATURE_LEVEL_11_0,
    IID_PPV_ARGS(&device));

if (FAILED(hr)) {
    Debug::LogHResult("D3D12CreateDevice failed", hr);
    return false;
}
```

HRESULTは8桁の16進数としてメッセージの末尾に追加されます。

```text
[2026-07-18 14:32:10.130] [ERROR] Renderer.cpp:42 D3D12CreateDevice failed (HRESULT: 0x887A0005)
```

## 出力形式

Infoログは日時、レベル、メッセージの順で出力されます。

```text
[2026-07-18 14:32:10.123] [INFO] Application starting
```

WarningとErrorには、呼び出し元のファイル名と行番号も追加されます。

```text
[2026-07-18 14:32:10.125] [WARN] D3D12RenderingService.cpp:594 RenderText: constant buffer capacity exceeded
[2026-07-18 14:32:10.130] [ERROR] main.cpp:63 DirectX 12 initialization failed
```

呼び出し元情報はC++20の `std::source_location` によって自動的に取得されます。
通常はファイル名や行番号を引数として渡す必要はありません。

## ビルド構成ごとの動作

| API | Debug | Release |
|---|---:|---:|
| `Debug::Log` | 出力する | 出力しない |
| `Debug::LogWarning` | 出力する | 出力する |
| `Debug::LogError` | 出力する | 出力する |
| `Debug::LogHResult` | 出力する | 出力する |

ReleaseビルドでもWarningとErrorはファイルへ記録されます。
ユーザー環境で発生した初期化失敗などの調査に利用できます。

## 日本語と文字コード

ソースコードとログメッセージにはUTF-8を使用します。

```cpp
Debug::Log("プレイヤーを初期化しました");
Debug::LogError("シェーダーの読み込みに失敗しました");
```

ログファイルにはUTF-8のまま保存されます。
Visual Studioへの出力時にはUTF-16へ変換されるため、日本語も表示できます。

## スレッドからの利用

ファイルへの書き込みは内部で排他制御されています。
複数のスレッドから同時にログAPIを呼び出すことができます。

ただし、大量のログを毎フレーム出力すると、ファイルの書き込みとフラッシュによって処理速度が低下します。
毎フレーム変化する情報は必要な期間だけ出力してください。

```cpp
// 毎フレーム常に出力する用途には向かない
Debug::Log("Player position updated");
```

## MessageBoxとの使い分け

`Debug` は開発者が問題を調査するための記録です。
`MessageBox` はユーザーの操作を止めて、起動不能などを通知する場合にだけ使用します。

起動に失敗した場合は、詳細をログへ記録してから簡潔なメッセージを表示します。

```cpp
if (!renderer.Initialize(hwnd, width, height)) {
    Debug::LogError("DirectX 12 initialization failed");
    MessageBox(hwnd, L"DirectX 12の初期化に失敗しました。", L"Error", MB_OK);
    return false;
}
```

フレーム中に繰り返し発生する可能性がある問題では、`MessageBox` を使用しないでください。

## 実装ファイル

- 公開API: [Engine/Diagnostics/Debug.h](../Engine/Diagnostics/Debug.h)
- 実装: [Engine/Diagnostics/Debug.cpp](../Engine/Diagnostics/Debug.cpp)
- 初期化処理: [main.cpp](../main.cpp)

## 現在の制限

- ログファイルは起動ごとに上書きされます。
- ログファイルのローテーション機能はありません。
- カテゴリによるフィルタリングには対応していません。
- `{}` 形式の可変長フォーマットには対応していません。
- 同じWarningやErrorの連続出力を抑制する機能はありません。

値を含むメッセージが必要な場合は、現在は文字列を作成してから渡します。

```cpp
std::string message = "Enemy count: " + std::to_string(enemyCount);
Debug::Log(message);
```
