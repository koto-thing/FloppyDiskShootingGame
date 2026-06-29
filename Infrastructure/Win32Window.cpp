#include "Win32Window.h"

HWND Win32Window::Create(HINSTANCE hInstance, int width, int height, const wchar_t* title, WNDPROC wndProc) {
    const wchar_t CLASS_NAME[] = L"DX12_Game_Window";
    WNDCLASS wc = { };
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, title,
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    return hwnd;
}
