#include "../Engine/Input/Input.h"

#include <stdexcept>

class InputTestAccess {
public:
    static void SetKey(int virtualKey, bool isPressed) {
        Input::SetNativeKeyState(static_cast<UINT>(virtualKey), isPressed);
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
}
