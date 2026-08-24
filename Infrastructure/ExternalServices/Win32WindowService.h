#pragma once
#include <windows.h>

class Win32WindowService {
public:
    /** @brief 指定したクライアント領域サイズでウィンドウを作成する */
    static HWND Create(HINSTANCE hInstance, int width, int height, const wchar_t* title, WNDPROC wndProc);
};
