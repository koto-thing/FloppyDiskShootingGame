#include "Win32WindowService.h"
/**
 * ウィンドウを作成する
 * @param hInstance インスタンスハンドル
 * @param width 幅
 * @param height 高さ
 * @param title ウィンドウタイトル
 * @param wndProc ウィンドウプロシージャ
 * @return ウィンドウハンドル
 */
HWND Win32WindowService::Create(HINSTANCE hInstance, int width, int height, const wchar_t* title, WNDPROC wndProc) {
    const wchar_t CLASS_NAME[] = L"DX12_Game_Window";
    const DWORD windowStyle = WS_POPUP;
    WNDCLASS wc = { };
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // 指定された解像度を画面全体へ表示するボーダーレスウィンドウを作成する
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, title, windowStyle,
        0, 0, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    return hwnd;
}
