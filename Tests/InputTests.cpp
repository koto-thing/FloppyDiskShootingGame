#include "../Engine/Input/Input.h"
#include "../Engine/Input/WindowsInputBackend.h"
#include "../Engine/Input/Switch2ProInput.h"

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
    // PID 2009実機のUSBレポートを再生し、予約ビット0x80が中心補正を妨げないことを確認する
    std::array<unsigned char, 63> originalReport {
        0xc6,0x71,0,0x80,0,0x67,0xd8,0x76,0x86,0x78,0x88,0x0a
    };
    Switch2ProReport originalDecoder;
    XINPUT_GAMEPAD originalState {};
    for (int i = 0; i < 16; ++i) originalDecoder.DecodeSwitchPro(originalReport, originalState);
    Require(originalDecoder.calibrated && originalState.wButtons == 0 &&
        originalState.sThumbLX == 0 && originalState.sThumbLY == 0,
        "Captured PID 2009 neutral report must calibrate despite its reserved bit");
    Require(originalDecoder.center == std::array<int, 4>{2151,1901,2182,2183},
        "Captured original Pro stick packing must decode correctly");
    constexpr WORD originalFaceButtons[] = {XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_X,
        XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_A, 0, 0, XINPUT_GAMEPAD_RIGHT_SHOULDER, 0};
    constexpr WORD originalLeftButtons[] = {XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_UP,
        XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_LEFT, 0, 0, XINPUT_GAMEPAD_LEFT_SHOULDER, 0};
    for (unsigned bit = 0; bit < 8; ++bit) {
        originalReport[2] = static_cast<unsigned char>(1u << bit);
        originalReport[4] = 0;
        Require(originalDecoder.DecodeSwitchPro(originalReport, originalState) &&
            originalState.wButtons == originalFaceButtons[bit] && originalState.bRightTrigger == (bit == 7 ? 255 : 0),
            "Original Pro face buttons must preserve Nintendo labels");
        XINPUT_GAMEPAD bluetoothState {};
        Require(originalDecoder.DecodeSwitchPro(std::span(originalReport).first(48), bluetoothState) &&
            bluetoothState.wButtons == originalState.wButtons && bluetoothState.bRightTrigger == originalState.bRightTrigger,
            "USB and Bluetooth original Pro reports must map identically");
        originalReport[2] = 0;
        originalReport[4] = static_cast<unsigned char>(1u << bit);
        Require(originalDecoder.DecodeSwitchPro(originalReport, originalState) &&
            originalState.wButtons == originalLeftButtons[bit] && originalState.bLeftTrigger == (bit == 7 ? 255 : 0),
            "Original Pro D-pad and left trigger must map independently");
    }
    originalReport[4] = 0;
    originalReport[3] = 0x8f;
    Require(originalDecoder.DecodeSwitchPro(originalReport, originalState) &&
        originalState.wButtons == (XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_START |
            XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB), "Original Pro system buttons must map correctly");
    for (std::size_t length = 0; length < originalReport.size(); ++length) {
        if (length != 48) Require(!originalDecoder.DecodeSwitchPro(std::span(originalReport).first(length), originalState),
            "Invalid original Pro report lengths must be rejected");
    }

    // 実機と同じ0x09ペイロードで中心の個体差とNintendo印字の対応を検証する
    Switch2ProReport decoder;
    std::array<unsigned char, 63> report {};
    const auto setSticks = [&report](int lx, int ly, int rx, int ry) {
        report[5] = static_cast<unsigned char>(lx);
        report[6] = static_cast<unsigned char>((lx >> 8) | ((ly & 15) << 4));
        report[7] = static_cast<unsigned char>(ly >> 4);
        report[8] = static_cast<unsigned char>(rx);
        report[9] = static_cast<unsigned char>((rx >> 8) | ((ry & 15) << 4));
        report[10] = static_cast<unsigned char>(ry >> 4);
    };
    setSticks(2110, 1990, 2200, 2030);
    XINPUT_GAMEPAD decoded {};
    for (int i = 0; i < 15; ++i) Require(!decoder.Decode(report, decoded), "Calibration must wait for stable samples");
    Require(decoder.Decode(report, decoded), "Stable center must calibrate");
    Require(decoded.sThumbLX == 0 && decoded.sThumbLY == 0 &&
        decoded.sThumbRX == 0 && decoded.sThumbRY == 0, "Measured stick centers must map to zero");
    constexpr WORD faceMapping[] = {XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_A,
        XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_RIGHT_SHOULDER,
        0, XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_RIGHT_THUMB};
    constexpr WORD leftMapping[] = {XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_LEFT_SHOULDER,
        0, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB};
    for (unsigned bit = 0; bit < 8; ++bit) {
        report[2] = static_cast<unsigned char>(1u << bit);
        report[3] = 0;
        Require(decoder.Decode(report, decoded) && decoded.wButtons == faceMapping[bit] &&
            decoded.bRightTrigger == (bit == 5 ? 255 : 0), "Right buttons must match Nintendo labels");
        report[2] = 0;
        report[3] = static_cast<unsigned char>(1u << bit);
        Require(decoder.Decode(report, decoded) && decoded.wButtons == leftMapping[bit] &&
            decoded.bLeftTrigger == (bit == 5 ? 255 : 0), "Left buttons and ZL must map independently");
    }
    report[3] = 0;
    setSticks(0, 4095, 4095, 0);
    Require(decoder.Decode(report, decoded) && decoded.sThumbLX == -32768 &&
        decoded.sThumbLY == 32767 && decoded.sThumbRX == 32767 && decoded.sThumbRY == -32768,
        "Axis extremes must preserve right/up signs without overflow");
    for (std::size_t length = 0; length < report.size(); ++length) {
        Require(!decoder.Decode(std::span(report).first(length), decoded), "Truncated reports must be rejected");
    }
    Require(decoded.sThumbLX == -32768, "Invalid reports must not overwrite the last valid state");

    // 接続時の押しっぱなしと大きなスティック倒し込みを中心として登録しない
    decoder = {};
    for (int i = 0; i < 20; ++i) Require(!decoder.Decode(report, decoded), "Deflected stick must not calibrate");
    setSticks(2048, 2048, 2048, 2048);
    report[2] = 2;
    for (int i = 0; i < 20; ++i) Require(!decoder.Decode(report, decoded), "Held A must not calibrate");
    report[2] = 0;
    for (int i = 0; i < 16; ++i) decoder.Decode(report, decoded);
    Input::BeginFrame();
    WindowsInputBackendTestAccess::SetPolledGamepad(nullptr);
    WindowsInputBackendTestAccess::SetPolledGamepad(&decoded);
    report[2] = 2;
    Require(decoder.Decode(report, decoded), "A report must decode after calibration");
    WindowsInputBackendTestAccess::SetPolledGamepad(&decoded);
    Require(Input::GetKeyDown(KeyCode::Z) && Input::GetKeyDown(KeyCode::Space),
        "Nintendo A must reach existing fire and confirm actions");
    WindowsInputBackendTestAccess::SetPolledGamepad(nullptr);
    Require(!Input::GetKey(KeyCode::Z), "Native disconnect must release mapped actions");

    // 射撃キーを押したまま次のフレームへ進める
    Input::BeginFrame();
    InputTestAccess::SetKey('Z', true);
    Input::BeginFrame();

    // Xの押下と解放が同一フレームに届いても押下イベントを保持する
    InputTestAccess::SetKey('X', true);
    InputTestAccess::SetKey('X', false);
    Require(Input::GetKey(KeyCode::Z), "Held fire key must remain pressed");
    Require(Input::GetKeyDown(KeyCode::X), "X tap must not be lost while fire is held");
    Require(Input::GetAnyKeyDown(), "Any-key event must report a new key press");
    Require(Input::GetKeyUp(KeyCode::X), "X release must be reported in the same frame");

    // 次フレームでは一時イベントだけを消去する
    Input::BeginFrame();
    Require(Input::GetKey(KeyCode::Z), "BeginFrame must preserve held keys");
    Require(!Input::GetKeyDown(KeyCode::X), "Key-down event must last for one frame");
    Require(!Input::GetAnyKeyDown(), "Held keys must not repeat the any-key event");
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
    Require(!Input::IsGamepadConnected(),
        "Disconnected gamepad must select keyboard UI hints");
    gamepad = {};
    gamepad.wButtons = XINPUT_GAMEPAD_X;
    WindowsInputBackendTestAccess::SetPolledGamepad(&gamepad);
    Require(Input::IsGamepadConnected(),
        "Connected gamepad must select gamepad UI hints");
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
    Require(!Input::IsGamepadConnected(),
        "Gamepad disconnect must restore keyboard UI hints");

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
