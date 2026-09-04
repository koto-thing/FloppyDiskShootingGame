#include "../Engine/Input/Input.h"
#include "../Engine/Input/WindowsInputBackend.h"

#include <stdexcept>

class InputTestAccess {
public:
    /**
     * @brief テスト用に物理キー状態を設定する
     * @param virtualKey Win32仮想キーコード
     * @param isPressed 押下中の場合はtrue
     * @return なし
     */
    static void SetKey(int virtualKey, bool isPressed) {
        Input::SetNativeKeyState(static_cast<UINT>(virtualKey), isPressed);
    }

    /**
     * @brief テスト用に物理マウスボタン状態を設定する
     * @param button 対象のマウスボタン
     * @param isPressed 押下中の場合はtrue
     * @return なし
     */
    static void SetMouseButton(MouseButton button, bool isPressed) {
        Input::SetMouseButtonState(button, isPressed);
    }

    /**
     * @brief テスト用に物理入力をクリックせず取り消す
     * @return なし
     */
    static void CancelNativeInput() {
        Input::CancelNativeInputState();
    }
};

class WindowsInputBackendTestAccess {
public:
    /**
     * @brief テスト用ゲームパッド状態を入力へ反映する
     * @param gamepad ゲームパッド状態、切断時はnullptr
     * @return なし
     */
    static void SetGamepad(const XINPUT_GAMEPAD* gamepad) {
        WindowsInputBackend::ProcessGamepad(gamepad, 1.0f / 60.0f);
    }

    /**
     * @brief テスト用に再接続判定を通してゲームパッド状態を反映する
     * @param gamepad ゲームパッド状態、切断時はnullptr
     * @return なし
     */
    static void SetPolledGamepad(const XINPUT_GAMEPAD* gamepad) {
        WindowsInputBackend::ProcessPolledGamepad(gamepad, 1.0f / 60.0f);
    }
};

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

/**
 * @brief 同時押しと同一フレーム内の短い押下を検証する
 * @return なし
 */
void RunInputTests() {
    // 射撃キーを押したまま次のフレームへ進める
    Input::BeginFrame();
    InputTestAccess::SetKey('Z', true);
    Input::BeginFrame();

    // Xの押下と解放が同一フレームに届いても押下イベントを保持する
    InputTestAccess::SetKey('X', true);
    InputTestAccess::SetKey('X', false);
    Require(Input::GetKey(KeyCode::Z), "Held fire key must remain pressed");
    Require(Input::GetKeyDown(KeyCode::X), "X tap must not be lost while fire is held");
    Require(Input::GetKeyUp(KeyCode::X), "X release must be reported in the same frame");

    // 次フレームでは一時イベントだけを消去する
    Input::BeginFrame();
    Require(Input::GetKey(KeyCode::Z), "BeginFrame must preserve held keys");
    Require(!Input::GetKeyDown(KeyCode::X), "Key-down event must last for one frame");
    Require(!Input::GetKeyUp(KeyCode::X), "Key-up event must last for one frame");
    InputTestAccess::SetKey('Z', false);

    // デッドゾーン境界の外側だけを左スティック入力として扱う
    Input::BeginFrame();
    XINPUT_GAMEPAD gamepad {};
    gamepad.sThumbLX = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Require(!Input::GetKey(KeyCode::D), "Stick deadzone must suppress movement");
    gamepad.sThumbLX = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + 1;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Require(Input::GetKeyDown(KeyCode::D), "Stick outside deadzone must press movement key");

    // 右スティックと同時に押したAも既存UIの左クリックへ合成する
    Input::BeginFrame();
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_A |
        XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_START |
        XINPUT_GAMEPAD_BACK;
    gamepad.sThumbLY = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + 1;
    gamepad.sThumbRX = 32767;
    gamepad.bLeftTrigger = XINPUT_GAMEPAD_TRIGGER_THRESHOLD + 1;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Require(Input::GetKey(KeyCode::LeftArrow), "D-pad must map to arrow movement");
    Require(Input::GetKey(KeyCode::W), "Left stick must map to movement keys");
    Require(Input::GetKey(KeyCode::Z) && Input::GetKey(KeyCode::Space),
        "A must retain gameplay actions while controlling the pointer");
    Require(Input::GetKey(KeyCode::X), "X must map to view toggle");
    Require(Input::GetKey(KeyCode::C), "Y must map to bomb");
    Require(Input::GetKey(KeyCode::LeftShift), "Left trigger must map to slow movement");
    Require(Input::GetKey(KeyCode::Escape), "Start and Back must map to pause or back");
    Require(Input::GetMouseButtonDown(MouseButton::Left),
        "A must click while the gamepad pointer moves");

    // Aを離しても右ショルダーからの射撃入力は維持する
    gamepad.wButtons &= ~XINPUT_GAMEPAD_A;
    gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Require(Input::GetKey(KeyCode::Z) && !Input::GetKey(KeyCode::Space),
        "Right shoulder must fire without pressing confirm");

    // ポインターを使わないAは従来の射撃と決定キーへ割り当てる
    gamepad = {};
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::BeginFrame();
    gamepad.wButtons = XINPUT_GAMEPAD_A;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Require(Input::GetKey(KeyCode::Z) && Input::GetKey(KeyCode::Space),
        "A without pointer mode must map to fire and confirm keys");
    Require(!Input::GetMouseButton(MouseButton::Left),
        "A without pointer mode must not click");
    gamepad = {};
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);

    // キーボードが同じ操作を保持中ならゲームパッド切断で解除しない
    InputTestAccess::SetKey('Z', true);
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetGamepad(nullptr);
    Require(Input::GetKey(KeyCode::Z), "Disconnect must preserve matching keyboard input");
    Require(!Input::GetKeyUp(KeyCode::Z), "Disconnect must not release a held keyboard key");
    InputTestAccess::SetKey('Z', false);
    Require(Input::GetKeyUp(KeyCode::Z), "Combined key must release after every source releases");

    // 物理マウスを保持中ならゲームパッド側の解放だけでは解除しない
    Input::BeginFrame();
    InputTestAccess::SetMouseButton(MouseButton::Left, true);
    gamepad = {};
    gamepad.sThumbRX = 32767;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_A;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetGamepad(nullptr);
    Require(Input::GetMouseButton(MouseButton::Left),
        "Gamepad release must preserve a held mouse button");
    Require(!Input::GetMouseButtonUp(MouseButton::Left),
        "Gamepad release must not report mouse-up while the mouse is held");
    InputTestAccess::SetMouseButton(MouseButton::Left, false);
    Require(Input::GetMouseButtonUp(MouseButton::Left),
        "Combined mouse button must release after every source releases");

    // 単独のゲームパッド入力は切断時に解放エッジを一度だけ生成する
    Input::BeginFrame();
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetGamepad(nullptr);
    Require(!Input::GetKey(KeyCode::X), "Disconnect must clear held gamepad input");
    Require(Input::GetKeyUp(KeyCode::X), "Disconnect must report gamepad release");
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetGamepad(nullptr);
    Require(!Input::GetKeyUp(KeyCode::X), "Disconnect release must last for one frame");

    // UI押下中の切断は解放イベントを発生させずクリックを取り消す
    gamepad = {};
    gamepad.sThumbRX = 32767;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_A;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetGamepad(nullptr);
    Require(!Input::GetMouseButton(MouseButton::Left),
        "Disconnect must clear a held gamepad pointer button");
    Require(!Input::GetMouseButtonUp(MouseButton::Left),
        "Disconnect must cancel rather than release a pointer click");

    // フォーカス喪失相当の取り消しは物理入力とイベントを残さない
    Input::BeginFrame();
    InputTestAccess::SetKey('Z', true);
    InputTestAccess::SetMouseButton(MouseButton::Left, true);
    InputTestAccess::CancelNativeInput();
    Require(!Input::GetKey(KeyCode::Z) && !Input::GetKeyUp(KeyCode::Z),
        "Native cancellation must clear keys without a release event");
    Require(!Input::GetMouseButton(MouseButton::Left) &&
        !Input::GetMouseButtonUp(MouseButton::Left),
        "Native cancellation must clear mouse buttons without a release event");

    // 復帰時に保持されていたパッド入力はニュートラルになるまで抑止する
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetPolledGamepad(nullptr);
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    WindowsInputBackendTestAccess::SetPolledGamepad(&gamepad);
    Require(!Input::GetKey(KeyCode::X) && !Input::GetKeyDown(KeyCode::X),
        "Held button on reconnect must not create an input edge");
    gamepad = {};
    WindowsInputBackendTestAccess::SetPolledGamepad(&gamepad);
    Input::BeginFrame();
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    WindowsInputBackendTestAccess::SetPolledGamepad(&gamepad);
    Require(Input::GetKeyDown(KeyCode::X),
        "Button press after reconnect neutral must be reported");
    WindowsInputBackendTestAccess::SetPolledGamepad(nullptr);

    // メッセージ入力を先に処理すれば入力元の持ち替えで偽エッジを作らない
    Input::BeginFrame();
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::BeginFrame();
    InputTestAccess::SetKey('X', true);
    WindowsInputBackendTestAccess::SetGamepad(nullptr);
    Require(Input::GetKey(KeyCode::X), "Keyboard handoff must preserve held input");
    Require(!Input::GetKeyDown(KeyCode::X) && !Input::GetKeyUp(KeyCode::X),
        "Keyboard handoff must not create false input edges");
    InputTestAccess::SetKey('X', false);

    // 逆向きの持ち替えも取得後の入力元比較で連続押下として扱う
    Input::BeginFrame();
    InputTestAccess::SetKey('X', true);
    Input::BeginFrame();
    InputTestAccess::SetKey('X', false);
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::PollGamepad();
    Require(Input::GetKey(KeyCode::X), "Gamepad handoff must preserve held input");
    Require(!Input::GetKeyDown(KeyCode::X) && !Input::GetKeyUp(KeyCode::X),
        "Gamepad handoff must not create false input edges");
    WindowsInputBackendTestAccess::SetGamepad(nullptr);

    // マウスからゲームパッドのUI押下へ持ち替えてもクリックを早期確定しない
    Input::BeginFrame();
    InputTestAccess::SetMouseButton(MouseButton::Left, true);
    Input::BeginFrame();
    InputTestAccess::SetMouseButton(MouseButton::Left, false);
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_A;
    gamepad.sThumbRX = 32767;
    WindowsInputBackendTestAccess::SetGamepad(&gamepad);
    Input::PollGamepad();
    Require(Input::GetMouseButton(MouseButton::Left),
        "Pointer handoff must preserve held input");
    Require(!Input::GetMouseButtonDown(MouseButton::Left) &&
        !Input::GetMouseButtonUp(MouseButton::Left),
        "Pointer handoff must not create false button edges");
    WindowsInputBackendTestAccess::SetGamepad(nullptr);

    // 保持中の物理キーを同一フレームで離して押し直した場合も両エッジを残す
    Input::BeginFrame();
    InputTestAccess::SetKey('X', true);
    Input::BeginFrame();
    InputTestAccess::SetKey('X', false);
    InputTestAccess::SetKey('X', true);
    Input::PollGamepad();
    Require(Input::GetKeyDown(KeyCode::X) && Input::GetKeyUp(KeyCode::X),
        "Rapid repress must preserve both release and press edges");
    InputTestAccess::SetKey('X', false);
}
