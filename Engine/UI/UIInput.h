#pragma once

#include "../Input/Input.h"
#include "../Math/Vector2.h"
#if defined(SPACEYAKUZA_PORTABLE)
#include <SDL.h>
#endif

/**
 * @brief UI向けに正規化したポインター入力状態
 *
 * position はRendererと同じNDC座標系（左下=-1,-1、右上=1,1）を使用する
 */
struct UIInputState {
    Vector2 position {};
    bool primaryDown = false;
    bool primaryPressed = false;
    bool primaryReleased = false;
};

/** @brief クライアント座標のInputをUI座標へ変換する補助関数 */
class UIInput {
public:
    /** @brief ゲームウィンドウの論理サイズで現在の入力を正規化する @return UI入力状態 */
    static UIInputState Current() {
        // マウス座標と同じ論理ピクセル単位を使い、HiDPIでも位置を一致させる
        int width = 1280, height = 720;
#if defined(SPACEYAKUZA_PORTABLE)
        if (SDL_Window* window = SDL_GetKeyboardFocus()) SDL_GetWindowSize(window, &width, &height);
#else
        RECT rect {};
        if (const HWND window = GetForegroundWindow(); window && GetClientRect(window, &rect)) {
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
#endif
        return Current(width, height);
    }

    /** @brief 指定した描画領域に合わせて現在のInputをUI入力状態へ変換する */
    static UIInputState Current(int viewportWidth, int viewportHeight) {
        const Vector2 mousePosition = Input::GetMousePosition();
        const float width = viewportWidth > 0 ? static_cast<float>(viewportWidth) : 1.0f;
        const float height = viewportHeight > 0 ? static_cast<float>(viewportHeight) : 1.0f;

        return {
            {mousePosition.x / width * 2.0f - 1.0f, 1.0f - mousePosition.y / height * 2.0f},
            Input::GetMouseButton(MouseButton::Left),
            Input::GetMouseButtonDown(MouseButton::Left),
            Input::GetMouseButtonUp(MouseButton::Left)
        };
    }
};
