#include "Presentation/Scenes/OptionScene.h"
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include "Engine/Diagnostics/Debug.h"
#include "Engine/Input/Input.h"
#include "Engine/Time/Time.h"
#include "Application/UseCases/SceneManager.h"
#include "Domain/ValueObjects/SceneSharedData.h"
#include "Domain/ValueObjects/SceneType.h"
#include "Infrastructure/ExternalServices/Win32WindowService.h"
#include "Infrastructure/ExternalServices/AudioService.h"
#include "Infrastructure/Repositories/SettingsRepository.h"
#include "Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "Engine/Graphics/Renderer.h"
#include "Engine/UI/Button.h"
#include "Presentation/Scenes/TitleScene.h"
#include "Presentation/Scenes/TestStage.h"
#include "Presentation/Scenes/TutorialStage.h"
#include "Presentation/Scenes/GalleryScene.h"
#include "Presentation/Scenes/CreditScene.h"
#include "Presentation/Scenes/ModeSelectionScene.h"
#include "Presentation/Scenes/StoryScene.h"
#include "Presentation/Scenes/EndingScene.h"
#include "Presentation/Scenes/RankingScene.h"
#if defined(SPACEYAKUZA_EDITION_Steam)
#include "Infrastructure/ExternalServices/SteamCoopSession.h"
#include "Presentation/Scenes/SteamLobbyScene.h"
#elif defined(SPACEYAKUZA_EDITION_Online)
#include "Infrastructure/ExternalServices/OnlineCoopSession.h"
#include "Presentation/Scenes/OnlineLobbyScene.h"
#endif

constexpr WORD APP_ICON_RESOURCE_ID = 101;

/**
 * @brief ウィンドウプロシージャ
 * @param hwnd ウィンドウハンドル
 * @param uMsg メッセージ識別子
 * @param wParam メッセージの最初のパラメータ
 * @param lParam メッセージの2番目のパラメータ
 * @return ウィンドウメッセージの処理結果
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // すべてのウィンドウメッセージを入力システムへ通知する
    Input::ProcessMessage(uMsg, wParam, lParam);

    // ウィンドウの終了メッセージを処理する
    switch (uMsg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    // その他のウィンドウメッセージを既定処理へ渡す
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief エントリーポイント
 * @param hInstance インスタンスハンドル
 * @param hPrevInstance 前のインスタンスハンドル
 * @param pCmdLine コマンドライン引数
 * @param nCmdShow ウィンドウの表示方法
 * @return アプリケーションの終了コード
 */
int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);

    // COMとデバッグログを初期化する
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Debug::Initialize();
    Debug::Log("Application starting");

    // オンラインサービスを初期化する
#if defined(SPACEYAKUZA_EDITION_Steam)
    // オーバーレイが描画初期化を追跡できるようSteamを先に開始する
    SteamCoopSession steam;
    steam.Initialize(pCmdLine);
#elif defined(SPACEYAKUZA_EDITION_Online)
    OnlineCoopSession online;
#endif

    // プライマリモニターの解像度を取得する
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 画面全体を覆うボーダーレスウィンドウを作成する
    HWND hwnd = Win32WindowService::Create(
        hInstance, screenWidth, screenHeight, L"Space Yakuza", WindowProc
    );

    // ウィンドウの作成に失敗した場合は終了
    if (hwnd == nullptr) {
        Debug::LogError("Window creation failed");
        Debug::Shutdown();
        return 0;
    }

    // 埋め込みアイコンをウィンドウへ反映する
    const HICON appIcon = LoadIcon(hInstance, MAKEINTRESOURCE(APP_ICON_RESOURCE_ID));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(appIcon));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(appIcon));

    // ウィンドウへキーボードとマウスのRaw Inputを登録する
    if (!Input::Initialize(hwnd)) {
        Debug::LogError("Input initialization failed");
        DestroyWindow(hwnd);
        Debug::Shutdown();
        return 0;
    }

    // ウィンドウを表示
    ShowWindow(hwnd, nCmdShow);

    // DirectX 12 レンダラーの初期化
    D3D12RenderingService renderer;

    if (!renderer.Initialize(hwnd, screenWidth, screenHeight)) {
        Debug::LogError("DirectX 12 initialization failed");
        MessageBox(NULL, L"DirectX 12 Initializing Failed", L"Error", MB_OK);
        Debug::Shutdown();
        return 0;
    }

    Renderer renderFacade(renderer);

    // オーディオ設定を読み込む
    AudioService audio;
    const GameSettings settings = SettingsRepository().Load();

    // 保存済みの描画と音量設定を反映する
    renderer.SetRetroEffectEnabled(settings.retroEffectEnabled);
    audio.SetMasterVolume(settings.masterVolume);
    audio.SetBGMVolume(settings.bgmVolume);
    audio.SetSEVolume(settings.seVolume);

    // オーディオを初期化する
    if (!audio.Initialize()) {
        MessageBox(NULL, L"Audio Initializing Failed", L"Error", MB_OK);
        return 0;
    }

    // 通常決定音は高く、キャンセル音は低く鳴らす
    Button::SetClickSoundHandler([&audio](Button::ClickSound sound) {
        Audio::SfxrParams params = Audio::SfxrParams::CreatePreset(Audio::SfxrPreset::BlipSelect);
        params.startFrequency = sound == Button::ClickSound::Confirm ? 0.65f : 0.18f;
        audio.PlaySE(params);
    });

    // シーンマネージャを作成
    SceneManager<SceneType, SceneSharedData> app;
    app.getSharedData().audio = &audio;
#if defined(SPACEYAKUZA_EDITION_Steam)
    app.getSharedData().coop = &steam;
    app.AddScene<SteamLobbyScene>(SceneType::SteamLobby);
#elif defined(SPACEYAKUZA_EDITION_Online)
    app.getSharedData().coop = &online;
    app.AddScene<OnlineLobbyScene>(SceneType::OnlineLobby);
#endif

    // シーンを登録
    app.AddScene<TitleScene>(SceneType::Title);
    app.AddScene<ModeSelectionScene>(SceneType::ModeSelection);
    app.AddScene<StoryScene>(SceneType::Story);
    app.AddScene<TestStage>(SceneType::TestStage);
    app.AddScene<TutorialStage>(SceneType::TutorialStage);
    app.AddScene<GalleryScene>(SceneType::Gallery);
    app.AddScene<OptionScene>(SceneType::Option);
    app.AddScene<CreditScene>(SceneType::Credit);
    app.AddScene<EndingScene>(SceneType::Ending);
    app.AddScene<RankingScene>(SceneType::Ranking);

    // 初期シーンの設定
    app.Initialize(SceneType::Title);

    // 初期化処理にかかった時間をゲーム時間へ含めない
    Time::Initialize();

    // メインループ
    MSG msg = { };
    bool isRunning = true;

    while (isRunning) {
        // 前フレームの状態を保存して新しい入力の受付を開始する
        Input::BeginFrame();

        // ウィンドウメッセージを処理する
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                isRunning = false;
                break;
            }

            // ウィンドウメッセージを各処理へ振り分ける
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!isRunning) {
            break;
        }

        // キーボードとマウスのメッセージ後にゲームパッドを取得する
        Input::PollGamepad();

        // フレーム時間を更新する
        Time::BeginFrame();
#if defined(SPACEYAKUZA_EDITION_Steam)
        // 招待はプレイ中には受理せず、メニュー中だけロビーへ遷移する
        steam.Poll();
        if (steam.TakeLobbyRequest()) {
            app.RequestTransition(SceneType::SteamLobby);
            app.CommitTransitions();
        }
#elif defined(SPACEYAKUZA_EDITION_Online)
        online.Poll();
#endif

        // シーンへの入力を処理する
        app.ProcessInput();

#ifdef _DEBUG
        // デバッグ確認用に現在位置を問わずチュートリアルへ移動する
        if (Input::GetKeyDown(KeyCode::F7)
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
            && !app.getSharedData().onlineGame
#endif
        ) {
            app.RequestTransition(SceneType::TutorialStage);
        }
#endif

        // 固定時間ステップでゲームを更新する
        constexpr int maxFixedStepsPerFrame = 8;
        int fixedStepCount = 0;

        while (Time::HasFixedStep() &&
               fixedStepCount < maxFixedStepsPerFrame) {
            Time::ConsumeFixedStep();
            app.Tick();
            ++fixedStepCount;
        }

        // 処理落ち時に蓄積した余分なゲーム時間を破棄する
        if (fixedStepCount >= maxFixedStepsPerFrame &&
            Time::HasFixedStep()) {
            Time::DiscardExcessFixedTime();
        }

        // シーン遷移とオーディオを更新する
        app.CommitTransitions();
        audio.Update();

        // 現在のシーンを描画する
        renderFacade.BeginFrame();
        app.Render(renderFacade);
        renderFacade.EndFrame();
    }

    // ゲーム終了時のリソースを解放する
    app.Dispose();
    audio.Shutdown();
    renderer.Cleanup();

    // デバッグログとCOMを終了する
    Debug::Log("Application shutting down");
    Debug::Shutdown();
    CoUninitialize();
    return 0;
}
