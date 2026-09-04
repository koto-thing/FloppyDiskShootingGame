#include "WindowsInputBackend.h"

#include <algorithm>
#include <cstddef>
#include <vector>
#include <windowsx.h>

#include "Input.h"

#pragma comment(lib, "xinput.lib")

namespace {
HWND inputWindow = nullptr;
ULONGLONG previousGamepadPollTime = 0;
ULONGLONG nextGamepadSearchTime = 0;
DWORD activeGamepadIndex = XUSER_MAX_COUNT;
bool gamepadNeedsNeutral = true;
bool gamepadPointerActive = false;
bool gamepadPrimaryWasPressed = false;
bool nativeInputEnabled = true;

constexpr ULONGLONG GamepadSearchIntervalMilliseconds = 1000;

/**
 * @brief スティック軸へデッドゾーンを適用して-1から1へ正規化する
 * @param value XInputの軸値
 * @param deadzone XInputのデッドゾーン
 * @return デッドゾーン適用後の正規化値
 */
float NormalizeThumbAxis(SHORT value, SHORT deadzone) {
    const int signedValue = static_cast<int>(value);
    const int magnitude = signedValue < 0 ? -signedValue : signedValue;
    if (magnitude <= static_cast<int>(deadzone)) return 0.0f;

    const int maximum = signedValue < 0 ? 32768 : 32767;
    const float normalized = static_cast<float>(magnitude - deadzone) /
        static_cast<float>(maximum - deadzone);
    return signedValue < 0 ? -normalized : normalized;
}

/**
 * @brief ゲームパッドが割り当て済み操作を入力していないか判定する
 * @param gamepad 判定するXInput状態
 * @return ニュートラルの場合はtrue
 */
bool IsGamepadNeutral(const XINPUT_GAMEPAD& gamepad) {
    return gamepad.wButtons == 0 &&
        gamepad.bLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
        gamepad.bRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
        std::abs(static_cast<int>(gamepad.sThumbLX)) <= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
        std::abs(static_cast<int>(gamepad.sThumbLY)) <= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
        std::abs(static_cast<int>(gamepad.sThumbRX)) <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
        std::abs(static_cast<int>(gamepad.sThumbRY)) <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}
}

/**
 * @brief キーボードとマウスのRaw InputおよびXInputを初期化する
 * @param hwnd 入力メッセージを受信するウィンドウハンドル
 * @return 登録に成功した場合はtrue
 */
bool WindowsInputBackend::Initialize(HWND hwnd) {
    inputWindow = hwnd;
    previousGamepadPollTime = GetTickCount64();
    nextGamepadSearchTime = 0;
    activeGamepadIndex = XUSER_MAX_COUNT;
    gamepadNeedsNeutral = true;
    gamepadPointerActive = false;
    gamepadPrimaryWasPressed = false;
    nativeInputEnabled = true;

    // 標準的なマウスとキーボードを対象ウィンドウへ登録する
    RAWINPUTDEVICE devices[] = {
        {0x01, 0x02, 0, hwnd},
        {0x01, 0x06, 0, hwnd}
    };

    if (RegisterRawInputDevices(
        devices,
        static_cast<UINT>(std::size(devices)),
        sizeof(RAWINPUTDEVICE)
    ) == FALSE) {
        return false;
    }

    // 実カーソル位置からゲームパッド用ポインターの開始位置を揃える
    POINT cursorPosition {};
    if (GetCursorPos(&cursorPosition) && ScreenToClient(hwnd, &cursorPosition)) {
        Input::SetMousePosition(
            static_cast<float>(cursorPosition.x),
            static_cast<float>(cursorPosition.y));
    }
    return true;
}

/**
 * @brief 接続中のXInputゲームパッドを取得して入力状態へ反映する
 * @return なし
 */
void WindowsInputBackend::Update() {
    if (inputWindow == nullptr) return;

    // 実時間差を短く制限して復帰直後のポインター飛びを防ぐ
    const ULONGLONG currentTime = GetTickCount64();
    const float elapsedSeconds = (std::min)(
        static_cast<float>(currentTime - previousGamepadPollTime) * 0.001f, 0.05f);
    previousGamepadPollTime = currentTime;

    // バックグラウンド中は操作を解放して復帰後のニュートラルを待つ
    const bool isForeground = GetForegroundWindow() == inputWindow;
    nativeInputEnabled = isForeground;
    if (!isForeground) {
        ProcessPolledGamepad(nullptr, elapsedSeconds);
        Input::CancelNativeInputState();
        return;
    }

    // 使用中のパッドは切断されるまで固定して毎フレーム取得する
    XINPUT_STATE state {};
    bool connected = activeGamepadIndex < XUSER_MAX_COUNT &&
        XInputGetState(activeGamepadIndex, &state) == ERROR_SUCCESS;
    if (activeGamepadIndex < XUSER_MAX_COUNT && !connected) {
        activeGamepadIndex = XUSER_MAX_COUNT;
        nextGamepadSearchTime = 0;
        ProcessPolledGamepad(nullptr, elapsedSeconds);
    }

    // 空スロットは毎フレーム走査せず1秒ごとに再検出する
    if (!connected && currentTime >= nextGamepadSearchTime) {
        nextGamepadSearchTime = currentTime + GamepadSearchIntervalMilliseconds;
        for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index) {
            if (XInputGetState(index, &state) == ERROR_SUCCESS) {
                activeGamepadIndex = index;
                connected = true;
                break;
            }
        }
    }
    ProcessPolledGamepad(connected ? &state.Gamepad : nullptr, elapsedSeconds);
}

/**
 * @brief Win32メッセージから入力状態を更新する
 * @param message メッセージ識別子
 * @param wParam メッセージ固有の追加情報
 * @param lParam メッセージ固有の追加情報
 * @return なし
 */
void WindowsInputBackend::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && wParam == FALSE)) {
        // フォーカス外で届かない解放を補い、途中のUI操作はクリックせず破棄する
        nativeInputEnabled = false;
        ProcessPolledGamepad(nullptr, 0.0f);
        Input::CancelNativeInputState();
        return;
    }

    // フォーカス喪失前からキューに残っていた入力を再登録しない
    const bool isNativeInputMessage = message == WM_INPUT ||
        message == WM_MOUSEMOVE || message == WM_MOUSEWHEEL;
    if (isNativeInputMessage &&
        (!nativeInputEnabled || GetForegroundWindow() != inputWindow)) return;

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

/**
 * @brief 再接続時の押下を抑止して取得済みゲームパッド状態を反映する
 * @param gamepad ゲームパッド状態、未接続の場合はnullptr
 * @param elapsedSeconds 前回取得からの秒数
 * @return なし
 */
void WindowsInputBackend::ProcessPolledGamepad(
    const XINPUT_GAMEPAD* gamepad, float elapsedSeconds) {
    // HUD案内が物理的な接続状態へ追従できるよう保存する
    Input::m_gamepadConnected = gamepad != nullptr;
    if (gamepad == nullptr) {
        gamepadNeedsNeutral = true;
        ProcessGamepad(nullptr, elapsedSeconds);
        return;
    }

    // 復帰や接続直後に保持されていたボタンを新規押下として扱わない
    if (gamepadNeedsNeutral) {
        if (!IsGamepadNeutral(*gamepad)) {
            ProcessGamepad(nullptr, elapsedSeconds);
            return;
        }
        gamepadNeedsNeutral = false;
    }
    ProcessGamepad(gamepad, elapsedSeconds);
}

/**
 * @brief XInput状態を既存のキーとポインター操作へ割り当てる
 * @param gamepad ゲームパッド状態、未接続の場合はnullptr
 * @param elapsedSeconds 前回取得からの秒数
 * @return なし
 */
void WindowsInputBackend::ProcessGamepad(const XINPUT_GAMEPAD* gamepad, float elapsedSeconds) {
    const XINPUT_GAMEPAD emptyState {};
    const XINPUT_GAMEPAD& state = gamepad != nullptr ? *gamepad : emptyState;
    const auto isPressed = [&state](WORD button) { return (state.wButtons & button) != 0; };

    // 切断またはフォーカス喪失時は古いUIポインター操作を引き継がない
    if (gamepad == nullptr) gamepadPointerActive = false;

    // D-padと左スティックを既存の移動キーへ割り当てる
    Input::SetGamepadKeyState(KeyCode::LeftArrow, isPressed(XINPUT_GAMEPAD_DPAD_LEFT));
    Input::SetGamepadKeyState(KeyCode::RightArrow, isPressed(XINPUT_GAMEPAD_DPAD_RIGHT));
    Input::SetGamepadKeyState(KeyCode::UpArrow, isPressed(XINPUT_GAMEPAD_DPAD_UP));
    Input::SetGamepadKeyState(KeyCode::DownArrow, isPressed(XINPUT_GAMEPAD_DPAD_DOWN));
    Input::SetGamepadKeyState(KeyCode::A,
        state.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    Input::SetGamepadKeyState(KeyCode::D,
        state.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    Input::SetGamepadKeyState(KeyCode::W,
        state.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    Input::SetGamepadKeyState(KeyCode::S,
        state.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

    // 右スティックをマウス専用UIでも使えるポインターへ割り当てる
    const bool primaryPressed = isPressed(XINPUT_GAMEPAD_A);
    const float pointerX = NormalizeThumbAxis(
        state.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    const float pointerY = NormalizeThumbAxis(
        state.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    if (pointerX != 0.0f || pointerY != 0.0f) {
        MoveGamepadPointer(pointerX, pointerY, elapsedSeconds);
        gamepadPointerActive = true;
    }

    // フェイスボタンとトリガーを既存ゲーム操作へ割り当てる
    const bool pointerClickPressed = primaryPressed && gamepadPointerActive;
    const bool firePressed = primaryPressed || isPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER) ||
        state.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    Input::SetGamepadKeyState(KeyCode::Z, firePressed);
    Input::SetGamepadKeyState(KeyCode::Space, primaryPressed);
    Input::SetGamepadKeyState(KeyCode::X, isPressed(XINPUT_GAMEPAD_X));
    Input::SetGamepadKeyState(KeyCode::C, isPressed(XINPUT_GAMEPAD_Y));
    Input::SetGamepadKeyState(KeyCode::LeftShift,
        isPressed(XINPUT_GAMEPAD_LEFT_SHOULDER) ||
        state.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
    Input::SetGamepadKeyState(KeyCode::Escape,
        isPressed(XINPUT_GAMEPAD_B) || isPressed(XINPUT_GAMEPAD_START) ||
        isPressed(XINPUT_GAMEPAD_BACK));

    // Aは右スティックでポインターを動かした後だけUIクリックにも使用する
    if (gamepad != nullptr) {
        Input::SetGamepadMouseButtonState(
            MouseButton::Left, pointerClickPressed);
    } else {
        Input::CancelGamepadMouseButtonState(MouseButton::Left);
    }
    // ponytail: UIとゲーム操作の混同を避けるためクリックごとに解除し、必要なら入力コンテキストへ置き換える
    if (gamepadPrimaryWasPressed && !primaryPressed) gamepadPointerActive = false;
    gamepadPrimaryWasPressed = primaryPressed;
}

/**
 * @brief 右スティックでUI用ポインターを移動する
 * @param x 水平方向の正規化入力
 * @param y 垂直方向の正規化入力
 * @param elapsedSeconds 前回取得からの秒数
 * @return なし
 */
void WindowsInputBackend::MoveGamepadPointer(float x, float y, float elapsedSeconds) {
    if (inputWindow == nullptr || elapsedSeconds <= 0.0f) return;

    // 解像度によらず約1.25秒で画面端から端へ移動する速度にする
    RECT clientRect {};
    if (!GetClientRect(inputWindow, &clientRect)) return;
    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;
    if (width <= 0 || height <= 0) return;

    Vector2 position = Input::GetMousePosition();
    const float deltaX = x * static_cast<float>(width) * 0.8f * elapsedSeconds;
    const float deltaY = -y * static_cast<float>(height) * 0.8f * elapsedSeconds;
    position.x = (std::clamp)(
        position.x + deltaX,
        0.0f, static_cast<float>(width - 1));
    position.y = (std::clamp)(
        position.y + deltaY,
        0.0f, static_cast<float>(height - 1));
    Input::SetMousePosition(position.x, position.y);

    // 仮想ドラッグでもギャラリーの軌道カメラを同じ入力経路で動かす
    Input::AddMouseDelta(deltaX, deltaY);

    // 既存のOSカーソルも同じ位置へ動かしてホバー位置を可視化する
    POINT screenPosition {
        static_cast<LONG>(position.x),
        static_cast<LONG>(position.y)
    };
    if (ClientToScreen(inputWindow, &screenPosition)) {
        SetCursorPos(screenPosition.x, screenPosition.y);
    }
}
