#include "WindowsInputBackend.h"

#include <cstddef>
#include <vector>
#include <windowsx.h>

#include "Input.h"

/**
 * @brief キーボードとマウスのRaw Inputを登録する
 * @param hwnd 入力メッセージを受信するウィンドウハンドル
 * @return 登録に成功した場合はtrue
 */
bool WindowsInputBackend::Initialize(HWND hwnd) {
    // 標準的なマウスとキーボードを対象ウィンドウへ登録する
    RAWINPUTDEVICE devices[] = {
        {0x01, 0x02, 0, hwnd},
        {0x01, 0x06, 0, hwnd}
    };

    return RegisterRawInputDevices(
        devices,
        static_cast<UINT>(std::size(devices)),
        sizeof(RAWINPUTDEVICE)
    ) != FALSE;
}

/**
 * @brief Win32メッセージから入力状態を更新する
 * @param message メッセージ識別子
 * @param wParam メッセージ固有の追加情報
 * @param lParam メッセージ固有の追加情報
 */
void WindowsInputBackend::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INPUT) {
        // Raw Inputデータの必要サイズを取得する
        UINT size = 0;
        if (GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_INPUT,
                nullptr,
                &size,
                sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) || size == 0) {
            return;
        }

        // 可変長データを確保して入力内容を取得する
        std::vector<std::byte> buffer(size);
        if (GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_INPUT,
                buffer.data(),
                &size,
                sizeof(RAWINPUTHEADER)) != size) {
            return;
        }

        // デバイス種別に応じた入力処理へ振り分ける
        const auto* rawInput = reinterpret_cast<const RAWINPUT*>(buffer.data());
        if (rawInput->header.dwType == RIM_TYPEKEYBOARD) {
            ProcessKeyboard(rawInput->data.keyboard);
        } else if (rawInput->header.dwType == RIM_TYPEMOUSE) {
            ProcessMouse(rawInput->data.mouse);
        }
        return;
    }

    if (message == WM_MOUSEMOVE) {
        // ウィンドウのクライアント領域を基準とした座標を保存する
        Input::SetMousePosition(
            static_cast<float>(GET_X_LPARAM(lParam)),
            static_cast<float>(GET_Y_LPARAM(lParam))
        );
        return;
    }

    if (message == WM_MOUSEWHEEL) {
        // Windowsのホイール単位を1ノッチ基準へ変換する
        const auto wheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        Input::AddMouseWheelDelta(wheelDelta / static_cast<float>(WHEEL_DELTA));
    }
}

/**
 * @brief Raw Inputのキーボード情報を反映する
 * @param keyboard キーボード入力情報
 */
void WindowsInputBackend::ProcessKeyboard(const RAWKEYBOARD& keyboard) {
    // 無効な疑似キーイベントを除外する
    if (keyboard.VKey == 255) {
        return;
    }

    // 左右を区別したキーコードと押下状態を反映する
    const UINT virtualKey = NormalizeVirtualKey(keyboard);
    const bool isPressed = (keyboard.Flags & RI_KEY_BREAK) == 0;
    Input::SetNativeKeyState(virtualKey, isPressed);
}

/**
 * @brief Raw Inputのマウス情報を反映する
 * @param mouse マウス入力情報
 */
void WindowsInputBackend::ProcessMouse(const RAWMOUSE& mouse) {
    // 相対移動の場合だけフレーム内の移動量へ加算する
    if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
        Input::AddMouseDelta(
            static_cast<float>(mouse.lLastX),
            static_cast<float>(mouse.lLastY)
        );
    }

    // 各ボタンの押下と解放を入力状態へ反映する
    const USHORT flags = mouse.usButtonFlags;
    if ((flags & RI_MOUSE_LEFT_BUTTON_DOWN) != 0) Input::SetMouseButtonState(MouseButton::Left, true);
    if ((flags & RI_MOUSE_LEFT_BUTTON_UP) != 0) Input::SetMouseButtonState(MouseButton::Left, false);
    if ((flags & RI_MOUSE_RIGHT_BUTTON_DOWN) != 0) Input::SetMouseButtonState(MouseButton::Right, true);
    if ((flags & RI_MOUSE_RIGHT_BUTTON_UP) != 0) Input::SetMouseButtonState(MouseButton::Right, false);
    if ((flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) != 0) Input::SetMouseButtonState(MouseButton::Middle, true);
    if ((flags & RI_MOUSE_MIDDLE_BUTTON_UP) != 0) Input::SetMouseButtonState(MouseButton::Middle, false);
    if ((flags & RI_MOUSE_BUTTON_4_DOWN) != 0) Input::SetMouseButtonState(MouseButton::Button4, true);
    if ((flags & RI_MOUSE_BUTTON_4_UP) != 0) Input::SetMouseButtonState(MouseButton::Button4, false);
    if ((flags & RI_MOUSE_BUTTON_5_DOWN) != 0) Input::SetMouseButtonState(MouseButton::Button5, true);
    if ((flags & RI_MOUSE_BUTTON_5_UP) != 0) Input::SetMouseButtonState(MouseButton::Button5, false);
}

/**
 * @brief 左右を区別した仮想キーコードへ変換する
 * @param keyboard キーボード入力情報
 * @return 正規化したWin32仮想キーコード
 */
UINT WindowsInputBackend::NormalizeVirtualKey(const RAWKEYBOARD& keyboard) {
    const bool isExtended = (keyboard.Flags & RI_KEY_E0) != 0;

    // 左右で共通の仮想キーをスキャンコードと拡張フラグから分離する
    switch (keyboard.VKey) {
        case VK_SHIFT:
            return MapVirtualKeyW(keyboard.MakeCode, MAPVK_VSC_TO_VK_EX);
        case VK_CONTROL:
            return isExtended ? VK_RCONTROL : VK_LCONTROL;
        case VK_MENU:
            return isExtended ? VK_RMENU : VK_LMENU;
        case VK_RETURN:
            return isExtended ? VK_SEPARATOR : VK_RETURN;
        default:
            return keyboard.VKey;
    }
}
