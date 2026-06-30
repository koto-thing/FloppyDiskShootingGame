#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include "Infrastructure/ExternalServices/Win32WindowService.h"
#include "Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "Application/GameManager.h"
#include "View/GameView.h"

/**
 * ウィンドウプロシージャ
 * @param hwnd ウィンドウ
 * @param uMsg メッセージ
 * @param wParam
 * @param lParam 
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
 * @param nCmdShow 
 * @return 
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    HWND hwnd = Win32WindowService::Create(
        hInstance, 800, 600, L"Floppy Disk Shooting Game - Clean Architecture", WindowProc
    );

    if (hwnd == nullptr)
        return 0;

    ShowWindow(hwnd, nCmdShow);

    D3D12RenderingService renderer;
    if (!renderer.Initialize(hwnd, 800, 600)) {
        MessageBox(hwnd, L"DirectX 12 Initializing Failed", L"Error", MB_OK);
        return 0;
    }

    GameManager gameManager;
    gameManager.Initialize();

    GameView gameView;

    MSG msg = { };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            gameManager.ProcessInput();
            gameManager.Update();
            
            renderer.BeginFrame();
            gameView.Render(renderer, gameManager);
            renderer.EndFrame();
        }
    }

    renderer.Cleanup();

    return 0;
}