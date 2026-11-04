#include "SdlInputBackend.h"
#include "Input.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
SDL_Window* inputWindow = nullptr;
std::array<SDL_GameController*, 2> controllers{};
std::array<bool, 2> needsNeutral{true, true};
Uint32 previousPoll = 0;
bool focused = false;
bool pointerActive = false;
bool primaryWasPressed = false;
bool refreshControllers = true;
constexpr int LeftDeadzone = 7849;
constexpr int RightDeadzone = 8689;
constexpr int TriggerThreshold = 3855;

/** @brief 未割当のコントローラーだけを空きプレイヤー枠へ追加する @return なし */
void RefreshControllers() {
    // 切断した枠だけ空け、残った2Pを1Pへ移動しない
    for (int slot = 0; slot < 2; ++slot) {
        if (controllers[slot] && !SDL_GameControllerGetAttached(controllers[slot])) {
            SDL_GameControllerClose(controllers[slot]);
            controllers[slot] = nullptr;
            needsNeutral[slot] = true;
        }
    }
    for (int device = 0; device < SDL_NumJoysticks(); ++device) {
        if (!SDL_IsGameController(device)) continue;
        const auto id = SDL_JoystickGetDeviceInstanceID(device);
        bool assigned = false;
        for (auto* controller : controllers) {
            assigned |= controller && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) == id;
        }
        if (assigned) continue;
        for (int slot = 0; slot < 2; ++slot) {
            if (controllers[slot]) continue;
            controllers[slot] = SDL_GameControllerOpen(device);
            needsNeutral[slot] = true;
            break;
        }
    }
}

/** @brief 復帰時の押しっぱなし操作を識別する @param controller 入力元 @return 全操作が中立時true */
bool IsNeutral(SDL_GameController* controller) {
    for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
        if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) return false;
    }
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
        const int threshold = axis < 2 ? LeftDeadzone : axis < 4 ? RightDeadzone : TriggerThreshold;
        if (std::abs(static_cast<int>(SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(axis)))) > threshold) return false;
    }
    return true;
}

/** @brief スティックの実測範囲とデッドゾーンを正規化する @param value 軸値 @return -1から1の操作量 */
float PointerAxis(int value) {
    const int magnitude = std::abs(value);
    if (magnitude <= RightDeadzone) return 0.0f;
    const float amount = static_cast<float>(magnitude - RightDeadzone) / ((value < 0 ? 32768 : 32767) - RightDeadzone);
    return value < 0 ? -amount : amount;
}
}

/** @brief SDL入力を初期化する @param window 入力先 @return 成功時true */
bool SdlInputBackend::Initialize(SDL_Window* window) {
    Shutdown();
    if (!window) return false;
    if (!(SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER) && SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return false;
    inputWindow = window;
    focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    previousPoll = SDL_GetTicks();
    refreshControllers = true;
    int x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    Input::SetMousePosition(static_cast<float>(x), static_cast<float>(y));
    return true;
}

/** @brief 入力ハンドルと残存操作を破棄する @return なし */
void SdlInputBackend::Shutdown() {
    for (int slot = 0; slot < 2; ++slot) {
        if (controllers[slot]) SDL_GameControllerClose(controllers[slot]);
        controllers[slot] = nullptr;
        ApplyGamepad(slot, nullptr, 0.0f);
    }
    needsNeutral = {true, true};
    Input::m_gamepadConnections = {};
    Input::m_gamepadConnected = false;
    Input::CancelNativeInputState();
    inputWindow = nullptr;
    focused = false;
    pointerActive = false;
    primaryWasPressed = false;
}

/** @brief SDLイベントを共通のフレーム状態へ反映する @param event イベント @return なし */
void SdlInputBackend::ProcessEvent(const SDL_Event& event) {
    if (!inputWindow) return;
    if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED) {
        refreshControllers = true;
        return;
    }
    // 対象ウィンドウのフォーカスだけを追跡する
    const Uint32 windowId = SDL_GetWindowID(inputWindow);
    if (event.type == SDL_WINDOWEVENT && event.window.windowID == windowId) {
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) focused = true;
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            focused = false;
            needsNeutral = {true, true};
            for (int slot = 0; slot < 2; ++slot) ApplyGamepad(slot, nullptr, 0.0f);
            Input::CancelNativeInputState();
        }
        return;
    }
    if (!focused) return;
    // 再送されたキー押下も共通Inputが重複エッジを抑止する
    if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && event.key.windowID == windowId) {
        Input::SetNativeKeyState(event.key.keysym.scancode, event.type == SDL_KEYDOWN);
    } else if (event.type == SDL_MOUSEMOTION && event.motion.windowID == windowId) {
        Input::SetMousePosition(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
        Input::AddMouseDelta(static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel));
    } else if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.windowID == windowId) {
        const MouseButton buttons[] = {MouseButton::Left, MouseButton::Middle, MouseButton::Right, MouseButton::Button4, MouseButton::Button5};
        if (event.button.button >= SDL_BUTTON_LEFT && event.button.button <= SDL_BUTTON_X2) {
            Input::SetMousePosition(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
            Input::SetMouseButtonState(buttons[event.button.button - 1], event.type == SDL_MOUSEBUTTONDOWN);
        }
    } else if (event.type == SDL_MOUSEWHEEL && event.wheel.windowID == windowId) {
        Input::AddMouseWheelDelta(static_cast<float>(event.wheel.y) * (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f));
    }
}

/** @brief 接続状態と中立復帰を確認してゲームパッドを反映する @return なし */
void SdlInputBackend::Update() {
    if (!inputWindow) return;
    const Uint32 now = SDL_GetTicks();
    const float seconds = std::min((now - previousPoll) * 0.001f, 0.05f);
    previousPoll = now;
    if (refreshControllers) { RefreshControllers(); refreshControllers = false; }
    SDL_GameControllerUpdate();
    for (int slot = 0; slot < 2; ++slot) {
        auto* controller = controllers[slot];
        const bool attached = controller && SDL_GameControllerGetAttached(controller);
        Input::m_gamepadConnections[slot] = attached;
        if (!attached || !focused) needsNeutral[slot] = true;
        if (attached && focused && needsNeutral[slot] && IsNeutral(controller)) needsNeutral[slot] = false;
        ApplyGamepad(slot, attached && focused && !needsNeutral[slot] ? controller : nullptr, seconds);
    }
    Input::m_gamepadConnected = Input::m_gamepadConnections[0];
    if (!focused) Input::CancelNativeInputState();
}

/** @brief パッド操作を既存の移動・攻撃・UI入力へ変換する @param slot プレイヤー番号 @param controller 入力元 @param seconds 経過秒数 @return なし */
void SdlInputBackend::ApplyGamepad(int slot, SDL_GameController* controller, float seconds) {
    const auto button = [controller](SDL_GameControllerButton code) { return controller && SDL_GameControllerGetButton(controller, code); };
    const auto axis = [controller](SDL_GameControllerAxis code) { return controller ? SDL_GameControllerGetAxis(controller, code) : 0; };
    Input::SetGamepadKeyState(slot, KeyCode::LeftArrow, button(SDL_CONTROLLER_BUTTON_DPAD_LEFT));
    Input::SetGamepadKeyState(slot, KeyCode::RightArrow, button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
    Input::SetGamepadKeyState(slot, KeyCode::UpArrow, button(SDL_CONTROLLER_BUTTON_DPAD_UP));
    Input::SetGamepadKeyState(slot, KeyCode::DownArrow, button(SDL_CONTROLLER_BUTTON_DPAD_DOWN));
    Input::SetGamepadKeyState(slot, KeyCode::A, axis(SDL_CONTROLLER_AXIS_LEFTX) < -LeftDeadzone);
    Input::SetGamepadKeyState(slot, KeyCode::D, axis(SDL_CONTROLLER_AXIS_LEFTX) > LeftDeadzone);
    Input::SetGamepadKeyState(slot, KeyCode::W, axis(SDL_CONTROLLER_AXIS_LEFTY) < -LeftDeadzone);
    Input::SetGamepadKeyState(slot, KeyCode::S, axis(SDL_CONTROLLER_AXIS_LEFTY) > LeftDeadzone);
    const bool primary = button(SDL_CONTROLLER_BUTTON_A);
    Input::SetGamepadKeyState(slot, KeyCode::Z, primary || button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) || axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > TriggerThreshold);
    Input::SetGamepadKeyState(slot, KeyCode::Space, primary);
    Input::SetGamepadKeyState(slot, KeyCode::X, button(SDL_CONTROLLER_BUTTON_X));
    Input::SetGamepadKeyState(slot, KeyCode::C, button(SDL_CONTROLLER_BUTTON_Y));
    Input::SetGamepadKeyState(slot, KeyCode::LeftShift, button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER) || axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT) > TriggerThreshold);
    Input::SetGamepadKeyState(slot, KeyCode::Escape, button(SDL_CONTROLLER_BUTTON_B) || button(SDL_CONTROLLER_BUTTON_START) || button(SDL_CONTROLLER_BUTTON_BACK));
    if (slot != 0) return;
    // 右スティックのUI操作だけを1Pへ割り当てる
    const float x = PointerAxis(axis(SDL_CONTROLLER_AXIS_RIGHTX));
    const float y = PointerAxis(axis(SDL_CONTROLLER_AXIS_RIGHTY));
    if (controller && inputWindow && (x != 0.0f || y != 0.0f)) {
        int width = 0, height = 0;
        SDL_GetWindowSize(inputWindow, &width, &height);
        const auto old = Input::GetMousePosition();
        const float px = std::clamp(old.x + x * 900.0f * seconds, 0.0f, static_cast<float>(std::max(0, width - 1)));
        const float py = std::clamp(old.y + y * 900.0f * seconds, 0.0f, static_cast<float>(std::max(0, height - 1)));
        Input::SetMousePosition(px, py);
        Input::AddMouseDelta(px - old.x, py - old.y);
        pointerActive = true;
    }
    if (controller) Input::SetGamepadMouseButtonState(MouseButton::Left, pointerActive && primary);
    else { Input::CancelGamepadMouseButtonState(MouseButton::Left); pointerActive = false; }
    if (primaryWasPressed && !primary) pointerActive = false;
    primaryWasPressed = primary;
}

/** @brief SDLの物理キー位置へ変換する @param key 抽象キー @return スキャンコード、不明時0 */
int SdlInputBackend::ToScancode(KeyCode key) {
    if (key >= KeyCode::A && key <= KeyCode::Z) return SDL_SCANCODE_A + static_cast<int>(key) - static_cast<int>(KeyCode::A);
    if (key >= KeyCode::Alpha1 && key <= KeyCode::Alpha9) return SDL_SCANCODE_1 + static_cast<int>(key) - static_cast<int>(KeyCode::Alpha1);
    if (key >= KeyCode::F1 && key <= KeyCode::F12) return SDL_SCANCODE_F1 + static_cast<int>(key) - static_cast<int>(KeyCode::F1);
    if (key >= KeyCode::Numpad1 && key <= KeyCode::Numpad9) return SDL_SCANCODE_KP_1 + static_cast<int>(key) - static_cast<int>(KeyCode::Numpad1);
    switch (key) {
        case KeyCode::Alpha0: return SDL_SCANCODE_0;
        case KeyCode::Numpad0: return SDL_SCANCODE_KP_0;
        case KeyCode::UpArrow: return SDL_SCANCODE_UP;
        case KeyCode::DownArrow: return SDL_SCANCODE_DOWN;
        case KeyCode::LeftArrow: return SDL_SCANCODE_LEFT;
        case KeyCode::RightArrow: return SDL_SCANCODE_RIGHT;
        case KeyCode::Home: return SDL_SCANCODE_HOME;
        case KeyCode::End: return SDL_SCANCODE_END;
        case KeyCode::PageUp: return SDL_SCANCODE_PAGEUP;
        case KeyCode::PageDown: return SDL_SCANCODE_PAGEDOWN;
        case KeyCode::Insert: return SDL_SCANCODE_INSERT;
        case KeyCode::Delete: return SDL_SCANCODE_DELETE;
        case KeyCode::Space: return SDL_SCANCODE_SPACE;
        case KeyCode::Enter: return SDL_SCANCODE_RETURN;
        case KeyCode::Escape: return SDL_SCANCODE_ESCAPE;
        case KeyCode::Tab: return SDL_SCANCODE_TAB;
        case KeyCode::Backspace: return SDL_SCANCODE_BACKSPACE;
        case KeyCode::CapsLock: return SDL_SCANCODE_CAPSLOCK;
        case KeyCode::NumLock: return SDL_SCANCODE_NUMLOCKCLEAR;
        case KeyCode::ScrollLock: return SDL_SCANCODE_SCROLLLOCK;
        case KeyCode::LeftShift: return SDL_SCANCODE_LSHIFT;
        case KeyCode::RightShift: return SDL_SCANCODE_RSHIFT;
        case KeyCode::LeftControl: return SDL_SCANCODE_LCTRL;
        case KeyCode::RightControl: return SDL_SCANCODE_RCTRL;
        case KeyCode::LeftAlt: return SDL_SCANCODE_LALT;
        case KeyCode::RightAlt: return SDL_SCANCODE_RALT;
        case KeyCode::LeftWindows: return SDL_SCANCODE_LGUI;
        case KeyCode::RightWindows: return SDL_SCANCODE_RGUI;
        case KeyCode::NumpadAdd: return SDL_SCANCODE_KP_PLUS;
        case KeyCode::NumpadSubtract: return SDL_SCANCODE_KP_MINUS;
        case KeyCode::NumpadMultiply: return SDL_SCANCODE_KP_MULTIPLY;
        case KeyCode::NumpadDivide: return SDL_SCANCODE_KP_DIVIDE;
        case KeyCode::NumpadDecimal: return SDL_SCANCODE_KP_PERIOD;
        case KeyCode::NumpadEnter: return SDL_SCANCODE_KP_ENTER;
        case KeyCode::Minus: return SDL_SCANCODE_MINUS;
        case KeyCode::Equals: return SDL_SCANCODE_EQUALS;
        case KeyCode::LeftBracket: return SDL_SCANCODE_LEFTBRACKET;
        case KeyCode::RightBracket: return SDL_SCANCODE_RIGHTBRACKET;
        case KeyCode::Backslash: return SDL_SCANCODE_BACKSLASH;
        case KeyCode::Semicolon: return SDL_SCANCODE_SEMICOLON;
        case KeyCode::Quote: return SDL_SCANCODE_APOSTROPHE;
        case KeyCode::Comma: return SDL_SCANCODE_COMMA;
        case KeyCode::Period: return SDL_SCANCODE_PERIOD;
        case KeyCode::Slash: return SDL_SCANCODE_SLASH;
        case KeyCode::GraveAccent: return SDL_SCANCODE_GRAVE;
        case KeyCode::PrintScreen: return SDL_SCANCODE_PRINTSCREEN;
        case KeyCode::Pause: return SDL_SCANCODE_PAUSE;
        default: return SDL_SCANCODE_UNKNOWN;
    }
}
