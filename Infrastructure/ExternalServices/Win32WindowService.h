#pragma once
#include <windows.h>

class Win32WindowService {
public:
    static HWND Create(HINSTANCE hInstance, int width, int height, const wchar_t* title, WNDPROC wndProc);
};
