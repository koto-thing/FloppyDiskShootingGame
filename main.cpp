#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include "Application/UseCases/SceneManager.h"
#include "Domain/ValueObjects/SceneSharedData.h"
#include "Domain/ValueObjects/SceneType.h"
#include "Infrastructure/ExternalServices/Win32WindowService.h"
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
    // ウィンドウを作成
    HWND hwnd = Win32WindowService::Create(
        hInstance, 800, 600, L"Floppy Disk Shooting Game - Clean Architecture", WindowProc
    );

    // ウィンドウの作成に失敗した場合は終了
    if (hwnd == nullptr) {
        return 0;
    }

    // ウィンドウを表示
    ShowWindow(hwnd, nCmdShow);

    // DirectX 12 レンダラーの初期化
    D3D12RenderingService renderer;
    if (!renderer.Initialize(hwnd, 800, 600)) {
        MessageBox(NULL, L"DirectX 12 Initializing Failed", L"Error", MB_OK);
        return 0;
    }
    
    // シーンマネージャを作成
    SceneManager<SceneType, SceneSharedData> app;
    
    // シーンを登録
    app.addScene<TitleScene>(SceneType::Title);
    app.addScene<TestStage>(SceneType::TestStage);
    
    // 初期シーンの設定
    app.init(SceneType::Title);
    
    MSG msg = { };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            app.ProcessInput();
            app.Tick();
            
            renderer.BeginFrame();
            app.Render(renderer);
            renderer.EndFrame();
        }
    }

    renderer.Cleanup();
    return 0;
}