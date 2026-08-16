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
#include "Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "Presentation/Scenes/TitleScene.h"
#include "Presentation/Scenes/TestStage.h"

/**
 * ウィンドウプロシージャ
 * @param hwnd ウィンドウハンドル
 * @param uMsg メッセージ識別子
 * @param wParam メッセージの最初のパラメータ
 * @param lParam メッセージの2番目のパラメータ
 * @return 
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // すべてのウィンドウメッセージを入力システムへ通知する
    Input::ProcessMessage(uMsg, wParam, lParam);

    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * エントリーポイント
 * @param hInstance インスタンスハンドル
 * @param nCmdShow ウィンドウの表示方法
 * @return 
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    Debug::Initialize();
    Debug::Log("Application starting");

    // ウィンドウを作成
    HWND hwnd = Win32WindowService::Create(
        hInstance, 1920, 1080, L"Floppy Disk Shooting Game - Clean Architecture", WindowProc
    );

    // ウィンドウの作成に失敗した場合は終了
    if (hwnd == nullptr) {
        Debug::LogError("Window creation failed");
        Debug::Shutdown();
        return 0;
    }

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
    if (!renderer.Initialize(hwnd, 1920, 1080)) {
        Debug::LogError("DirectX 12 initialization failed");
        MessageBox(NULL, L"DirectX 12 Initializing Failed", L"Error", MB_OK);
        Debug::Shutdown();
        return 0;
    }

    AudioService audio;
    if (!audio.Initialize()) {
        MessageBox(NULL, L"Audio Initializing Failed", L"Error", MB_OK);
        return 0;
    }

    if (!audio.PlayMMLBGMFromFile("test.mml", true)) {
        std::string sampleBGM = 
            "t140 o5 l8 @0 v12 c e g o6 c r g e c ; "
            "t140 o4 l8 @1 v9  e g o5 c e r c g e ; "
            "t140 o3 l4 @3 v14 c g c g ; "
            "t140 o4 l8 @5 v8  c r c r c c r r";
        audio.PlayMMLBGM(sampleBGM, true);
    }
    
    // シーンマネージャを作成
    SceneManager<SceneType, SceneSharedData> app;
    
    // シーンを登録
    app.addScene<TitleScene>(SceneType::Title);
    app.addScene<TestStage>(SceneType::TestStage);
    
    // 初期シーンの設定
    app.init(SceneType::Title);

    // 初期化処理にかかった時間をゲーム時間へ含めない
    Time::Initialize();
    
    // メインループ
    MSG msg = { };
    while (msg.message != WM_QUIT) {
        // 前フレームの状態を保存して新しい入力の受付を開始する
        Input::Update();

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        Time::BeginFrame();

        app.ProcessInput();

        constexpr int maxFixedStepsPerFrame = 8;
        int fixedStepCount = 0;

        while (Time::HasFixedStep() &&
               fixedStepCount < maxFixedStepsPerFrame) {
            Time::ConsumeFixedStep();
            app.Tick();
            ++fixedStepCount;
        }

        if (fixedStepCount >= maxFixedStepsPerFrame &&
            Time::HasFixedStep()) {
            Time::DiscardExcessFixedTime();
        }

        renderer.BeginFrame();
        app.Render(renderer);
        renderer.EndFrame();
    }

    renderer.Cleanup();
    Debug::Log("Application shutting down");
    Debug::Shutdown();
    return 0;
}
