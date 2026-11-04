#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include "../Engine/Input/Input.h"
#include "../Infrastructure/ExternalServices/MMLData.h"
#include "../Infrastructure/ExternalServices/PackedAudioSamples.h"
#include <cassert>
#include <cstdio>
#include <limits>

void RunAudioServiceTests();

/** @brief ウィンドウへテスト用フォーカスイベントを送る @param id ウィンドウID @param focused フォーカス状態 @return なし */
static void Focus(Uint32 id, bool focused) {
    SDL_Event event{};
    event.type = SDL_WINDOWEVENT;
    event.window.windowID = id;
    event.window.event = focused ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
    Input::ProcessEvent(event);
}

/** @brief キーイベントを既存入力へ送る @param id ウィンドウID @param code 物理キー @param down 押下状態 @return なし */
static void Key(Uint32 id, SDL_Scancode code, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.windowID = id;
    event.key.keysym.scancode = code;
    Input::ProcessEvent(event);
}

/** @brief SDLダミーデバイスとオフライン音声で移植基盤を検証する @return 成功時0 */
int main() {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) == 0);
    auto* window = SDL_CreateWindow("Input check", 0, 0, 1280, 720, SDL_WINDOW_HIDDEN);
    assert(window && Input::Initialize(window));
    const auto id = SDL_GetWindowID(window);
    Focus(id, true);
    // 同フレームの短い押下とリピートを取りこぼさない
    Input::BeginFrame();
    Key(id, SDL_SCANCODE_Z, true);
    Key(id, SDL_SCANCODE_Z, false);
    assert(!Input::GetKey(KeyCode::Z) && Input::GetKeyDown(KeyCode::Z) && Input::GetKeyUp(KeyCode::Z));
    Input::BeginFrame();
    Key(id, SDL_SCANCODE_Z, true);
    Input::BeginFrame();
    Key(id, SDL_SCANCODE_Z, true);
    assert(Input::GetKey(KeyCode::Z) && !Input::GetKeyDown(KeyCode::Z));
    Key(id, SDL_SCANCODE_KP_ENTER, true);
    assert(Input::GetKey(KeyCode::NumpadEnter) && !Input::GetKey(KeyCode::Enter));
    Key(id + 1, SDL_SCANCODE_X, true);
    assert(!Input::GetKey(KeyCode::X));
    Focus(id, false);
    Key(id, SDL_SCANCODE_Z, true);
    assert(!Input::GetAnyKeyDown() && !Input::GetKey(KeyCode::Z));
    assert(!Input::GetKey(static_cast<KeyCode>(65535)));
    Focus(id, true);
    // コントローラー実機なしで接続時の中立待ちとプレイヤー分離を確認する
    const int first = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    const int second = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    assert(first >= 0 && second >= 0);
    auto* firstJoystick = SDL_JoystickOpen(first);
    auto* secondJoystick = SDL_JoystickOpen(second);
    assert(firstJoystick && secondJoystick);
    SDL_JoystickSetVirtualButton(firstJoystick, SDL_CONTROLLER_BUTTON_A, 1);
    SDL_JoystickSetVirtualAxis(firstJoystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
    SDL_JoystickSetVirtualAxis(firstJoystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    SDL_JoystickSetVirtualAxis(secondJoystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
    SDL_JoystickSetVirtualAxis(secondJoystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    SDL_Event added{};
    added.type = SDL_CONTROLLERDEVICEADDED;
    Input::ProcessEvent(added);
    Input::PollGamepad();
    assert(Input::IsGamepadConnected(0) && Input::IsGamepadConnected(1));
    assert(!Input::GetGamepadKey(0, KeyCode::Z));
    SDL_JoystickSetVirtualButton(firstJoystick, SDL_CONTROLLER_BUTTON_A, 0);
    Input::BeginFrame();
    Input::PollGamepad();
    SDL_JoystickSetVirtualButton(secondJoystick, SDL_CONTROLLER_BUTTON_A, 1);
    Input::BeginFrame();
    Input::PollGamepad();
    assert(Input::GetGamepadKeyDown(1, KeyCode::Z) && !Input::GetKey(KeyCode::Z));
    SDL_JoystickDetachVirtual(first);
    Input::ProcessEvent(added);
    Input::BeginFrame();
    Input::PollGamepad();
    assert(!Input::IsGamepadConnected(0) && Input::IsGamepadConnected(1));
    assert(Input::GetGamepadKey(1, KeyCode::Z) && !Input::GetKey(KeyCode::Z));
    Focus(id, false);
    assert(!Input::GetGamepadKey(1, KeyCode::Z) && !Input::GetGamepadKeyUp(1, KeyCode::Z));
    Input::Shutdown();
    SDL_JoystickClose(firstJoystick);
    SDL_JoystickClose(secondJoystick);
    SDL_DestroyWindow(window);
    SDL_Quit();
    // 全曲の参照と音源の範囲検証はWindows圧縮APIなしで成功する
    assert(!MMLData::GetTitleBgm().empty() && !MMLData::GetEndingBgm().empty());
    for (int stage = 1; stage <= 6; ++stage) {
        assert(!MMLData::GetStageBgm(stage).empty() && !MMLData::GetBossBgm(stage).empty());
    }
    assert(MMLData::GetByName("missing").empty());
    using namespace Audio::PackedAudioSamples;
    assert(Get(sample_AcousticGrandPiano.offset, sample_AcousticGrandPiano.size)[0] == 0x8f);
    assert(Get(tayamaDeathData.offset, tayamaDeathData.size)[0] == 0x77);
    assert(!Get(TotalSize, 1) && !Get(0, 0) && !Get(0, std::numeric_limits<std::size_t>::max()));
    RunAudioServiceTests();
    std::puts("Portable input, audio and resource tests passed");
    return 0;
}
