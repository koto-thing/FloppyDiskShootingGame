#include "Input.h"

#include "WindowsInputBackend.h"

std::array<bool, static_cast<std::size_t>(KeyCode::Count)> Input::m_currentKeys{};
std::array<bool, static_cast<std::size_t>(KeyCode::Count)> Input::m_keyDown{};
std::array<bool, static_cast<std::size_t>(KeyCode::Count)> Input::m_keyUp{};
std::array<bool, static_cast<std::size_t>(MouseButton::Count)> Input::m_currentMouseButtons{};
std::array<bool, static_cast<std::size_t>(MouseButton::Count)> Input::m_mouseButtonDown{};
std::array<bool, static_cast<std::size_t>(MouseButton::Count)> Input::m_mouseButtonUp{};
Vector2 Input::m_mousePosition{};
Vector2 Input::m_mouseDelta{};
float Input::m_mouseWheelDelta = 0.0f;

/**
 * @brief 入力状態を初期化してRaw Inputを登録する
 * @param hwnd 入力メッセージを受信するウィンドウハンドル
 * @return 初期化に成功した場合はtrue
 */
bool Input::Initialize(HWND hwnd) {
    // すべてのデジタル入力状態を未入力に戻す
    m_currentKeys.fill(false);
    m_keyDown.fill(false);
    m_keyUp.fill(false);
    m_currentMouseButtons.fill(false);
    m_mouseButtonDown.fill(false);
    m_mouseButtonUp.fill(false);

    // フレーム内に蓄積するアナログ入力を初期化する
    m_mousePosition = {};
    m_mouseDelta = {};
    m_mouseWheelDelta = 0.0f;

    return WindowsInputBackend::Initialize(hwnd);
}

/**
 * @brief 新しいフレームの入力受付を開始する
 */
void Input::BeginFrame() {
    // 前フレームで発生したデジタル入力イベントを消去する
    m_keyDown.fill(false);
    m_keyUp.fill(false);
    m_mouseButtonDown.fill(false);
    m_mouseButtonUp.fill(false);

    // フレーム単位で扱う移動量とホイール量をリセットする
    m_mouseDelta = {};
    m_mouseWheelDelta = 0.0f;
}

/**
 * @brief 指定したキーが押されているかを取得する
 * @param key 確認するキー
 * @return 押されている場合はtrue
 */
bool Input::GetKey(KeyCode key) {
    const auto index = static_cast<std::size_t>(key);
    return index < m_currentKeys.size() && m_currentKeys[index];
}

/**
 * @brief 指定したキーがこのフレームで押されたかを取得する
 * @param key 確認するキー
 * @return このフレームで押された場合はtrue
 */
bool Input::GetKeyDown(KeyCode key) {
    const auto index = static_cast<std::size_t>(key);
    return index < m_keyDown.size() && m_keyDown[index];
}

/**
 * @brief 指定したキーがこのフレームで離されたかを取得する
 * @param key 確認するキー
 * @return このフレームで離された場合はtrue
 */
bool Input::GetKeyUp(KeyCode key) {
    const auto index = static_cast<std::size_t>(key);
    return index < m_keyUp.size() && m_keyUp[index];
}

/**
 * @brief 指定したマウスボタンが押されているかを取得する
 * @param button 確認するマウスボタン
 * @return 押されている場合はtrue
 */
bool Input::GetMouseButton(MouseButton button) {
    const auto index = static_cast<std::size_t>(button);
    return index < m_currentMouseButtons.size() && m_currentMouseButtons[index];
}

/**
 * @brief 指定したマウスボタンがこのフレームで押されたかを取得する
 * @param button 確認するマウスボタン
 * @return このフレームで押された場合はtrue
 */
bool Input::GetMouseButtonDown(MouseButton button) {
    const auto index = static_cast<std::size_t>(button);
    return index < m_mouseButtonDown.size() && m_mouseButtonDown[index];
}

/**
 * @brief 指定したマウスボタンがこのフレームで離されたかを取得する
 * @param button 確認するマウスボタン
 * @return このフレームで離された場合はtrue
 */
bool Input::GetMouseButtonUp(MouseButton button) {
    const auto index = static_cast<std::size_t>(button);
    return index < m_mouseButtonUp.size() && m_mouseButtonUp[index];
}

/**
 * @brief クライアント領域を基準としたマウス座標を取得する
 * @return マウス座標
 */
Vector2 Input::GetMousePosition() {
    return m_mousePosition;
}

/**
 * @brief このフレームのマウス移動量を取得する
 * @return マウス移動量
 */
Vector2 Input::GetMouseDelta() {
    return m_mouseDelta;
}

/**
 * @brief このフレームのホイール回転量を取得する
 * @return WHEEL_DELTAを1としたホイール回転量
 */
float Input::GetMouseWheelDelta() {
    return m_mouseWheelDelta;
}

/**
 * @brief Win32メッセージを入力バックエンドへ渡す
 * @param message メッセージ識別子
 * @param wParam メッセージ固有の追加情報
 * @param lParam メッセージ固有の追加情報
 */
void Input::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    WindowsInputBackend::ProcessMessage(message, wParam, lParam);
}

/**
 * @brief Win32仮想キーの状態を反映する
 * @param virtualKey Win32仮想キーコード
 * @param isPressed 押下中の場合はtrue
 */
void Input::SetNativeKeyState(UINT virtualKey, bool isPressed) {
    // 対応する抽象キーを検索して状態を更新する
    for (std::size_t i = 1; i < m_currentKeys.size(); ++i) {
        if (ToVirtualKey(static_cast<KeyCode>(i)) == static_cast<int>(virtualKey)) {
            if (m_currentKeys[i] == isPressed) return;
            m_currentKeys[i] = isPressed;
            (isPressed ? m_keyDown : m_keyUp)[i] = true;
            return;
        }
    }
}

/**
 * @brief マウスボタンの状態を反映する
 * @param button 対象のマウスボタン
 * @param isPressed 押下中の場合はtrue
 */
void Input::SetMouseButtonState(MouseButton button, bool isPressed) {
    const auto index = static_cast<std::size_t>(button);
    if (index >= m_currentMouseButtons.size() || m_currentMouseButtons[index] == isPressed) return;
    m_currentMouseButtons[index] = isPressed;
    (isPressed ? m_mouseButtonDown : m_mouseButtonUp)[index] = true;
}

/**
 * @brief マウス移動量を加算する
 * @param x 水平方向の移動量
 * @param y 垂直方向の移動量
 */
void Input::AddMouseDelta(float x, float y) {
    m_mouseDelta.x += x;
    m_mouseDelta.y += y;
}

/**
 * @brief マウスホイール回転量を加算する
 * @param delta WHEEL_DELTAを1とした回転量
 */
void Input::AddMouseWheelDelta(float delta) {
    m_mouseWheelDelta += delta;
}

/**
 * @brief クライアント領域を基準としたマウス座標を設定する
 * @param x 水平座標
 * @param y 垂直座標
 */
void Input::SetMousePosition(float x, float y) {
    m_mousePosition = {x, y};
}

/**
 * @brief KeyCodeをWin32仮想キーコードへ変換する
 * @param key 変換するキー
 * @return Win32仮想キーコード、未対応の場合は0
 */
int Input::ToVirtualKey(KeyCode key) {
    // 連続している英数字キーをまとめて変換する
    if (key >= KeyCode::A && key <= KeyCode::Z) {
        return 'A' + static_cast<int>(key) - static_cast<int>(KeyCode::A);
    }
    if (key >= KeyCode::Alpha0 && key <= KeyCode::Alpha9) {
        return '0' + static_cast<int>(key) - static_cast<int>(KeyCode::Alpha0);
    }
    if (key >= KeyCode::F1 && key <= KeyCode::F12) {
        return VK_F1 + static_cast<int>(key) - static_cast<int>(KeyCode::F1);
    }
    if (key >= KeyCode::Numpad0 && key <= KeyCode::Numpad9) {
        return VK_NUMPAD0 + static_cast<int>(key) - static_cast<int>(KeyCode::Numpad0);
    }

    // 連続していない特殊キーを個別に変換する
    switch (key) {
        case KeyCode::UpArrow: return VK_UP;
        case KeyCode::DownArrow: return VK_DOWN;
        case KeyCode::LeftArrow: return VK_LEFT;
        case KeyCode::RightArrow: return VK_RIGHT;
        case KeyCode::Home: return VK_HOME;
        case KeyCode::End: return VK_END;
        case KeyCode::PageUp: return VK_PRIOR;
        case KeyCode::PageDown: return VK_NEXT;
        case KeyCode::Insert: return VK_INSERT;
        case KeyCode::Delete: return VK_DELETE;
        case KeyCode::Space: return VK_SPACE;
        case KeyCode::Enter: return VK_RETURN;
        case KeyCode::Escape: return VK_ESCAPE;
        case KeyCode::Tab: return VK_TAB;
        case KeyCode::Backspace: return VK_BACK;
        case KeyCode::CapsLock: return VK_CAPITAL;
        case KeyCode::NumLock: return VK_NUMLOCK;
        case KeyCode::ScrollLock: return VK_SCROLL;
        case KeyCode::LeftShift: return VK_LSHIFT;
        case KeyCode::RightShift: return VK_RSHIFT;
        case KeyCode::LeftControl: return VK_LCONTROL;
        case KeyCode::RightControl: return VK_RCONTROL;
        case KeyCode::LeftAlt: return VK_LMENU;
        case KeyCode::RightAlt: return VK_RMENU;
        case KeyCode::LeftWindows: return VK_LWIN;
        case KeyCode::RightWindows: return VK_RWIN;
        case KeyCode::NumpadAdd: return VK_ADD;
        case KeyCode::NumpadSubtract: return VK_SUBTRACT;
        case KeyCode::NumpadMultiply: return VK_MULTIPLY;
        case KeyCode::NumpadDivide: return VK_DIVIDE;
        case KeyCode::NumpadDecimal: return VK_DECIMAL;
        case KeyCode::NumpadEnter: return VK_SEPARATOR;
        case KeyCode::Minus: return VK_OEM_MINUS;
        case KeyCode::Equals: return VK_OEM_PLUS;
        case KeyCode::LeftBracket: return VK_OEM_4;
        case KeyCode::RightBracket: return VK_OEM_6;
        case KeyCode::Backslash: return VK_OEM_5;
        case KeyCode::Semicolon: return VK_OEM_1;
        case KeyCode::Quote: return VK_OEM_7;
        case KeyCode::Comma: return VK_OEM_COMMA;
        case KeyCode::Period: return VK_OEM_PERIOD;
        case KeyCode::Slash: return VK_OEM_2;
        case KeyCode::GraveAccent: return VK_OEM_3;
        case KeyCode::PrintScreen: return VK_SNAPSHOT;
        case KeyCode::Pause: return VK_PAUSE;
        default: return 0;
    }
}
