#include "../Presentation/ModeSelectionScene/ModeSelectionStateController.h"
#include "../Domain/ValueObjects/SceneSharedData.h"

#include <cassert>
#include <cstdio>

/**
 * @brief 2人用の割り当て、個別選択、切断後の再確認を検証する
 * @return 全検証成功なら0
 */
int main() {
    // 既定は1人用で、2人用は両コントローラーの決定を必要とする
    assert(SceneSharedData {}.playerCount == 1);
    ModeSelectionStateController selection;
    assert(selection.GetCurrentState() == ModeSelectionState::PlayerCountSelect);
    selection.SetCurrentState(ModeSelectionState::ControllerSelect);
    selection.UpdatePlayerInput(0, true, 0, true, false);
    selection.UpdatePlayerInput(1, false, 0, true, false);
    assert(selection.GetCurrentState() == ModeSelectionState::ControllerSelect);
    assert(selection.IsControllerJoined(0) && !selection.IsControllerJoined(1));
    assert(!selection.ArePlayersReady());
    selection.UpdatePlayerInput(1, true, 0, true, false);
    assert(selection.GetCurrentState() == ModeSelectionState::DifficultySelect);

    // 一方の選択と決定で他方のショットや決定状態が変化しない
    selection.SetCurrentState(ModeSelectionState::PlayerTypeSelect);
    selection.UpdatePlayerInput(0, true, -1, true, false);
    assert(selection.GetPlayerType(0) == Spread);
    assert(selection.GetPlayerType(1) == Homing);
    assert(selection.IsPlayerReady(0) && !selection.IsPlayerReady(1));
    selection.UpdatePlayerInput(0, true, 1, false, false);
    assert(selection.GetPlayerType(0) == Spread);
    selection.UpdatePlayerInput(1, true, -1, true, false);
    assert(selection.GetPlayerType(0) == selection.GetPlayerType(1));
    assert(selection.ArePlayersReady());

    // 決定解除で選び直せ、選択肢の末尾から先頭へ循環する
    selection.UpdatePlayerInput(1, true, 0, false, true);
    assert(!selection.ArePlayersReady() && selection.IsPlayerReady(0));
    selection.UpdatePlayerInput(1, true, 1, true, false);
    assert(selection.GetPlayerType(1) == Homing && selection.ArePlayersReady());

    // 選択完了後でも切断を見逃さず、再接続だけで開始しない
    selection.UpdatePlayerInput(0, false, 0, false, false);
    assert(selection.GetCurrentState() == ModeSelectionState::ControllerSelect);
    assert(!selection.IsPlayerReady(0) && !selection.IsPlayerReady(1));
    selection.UpdatePlayerInput(0, true, 0, false, false);
    assert(selection.GetCurrentState() == ModeSelectionState::ControllerSelect);
    selection.UpdatePlayerInput(0, true, 0, true, false);
    assert(selection.GetCurrentState() == ModeSelectionState::DifficultySelect);
    selection.SetCurrentState(ModeSelectionState::PlayerTypeSelect);
    assert(!selection.ArePlayersReady());
    selection.UpdatePlayerInput(0, true, 0, true, false);
    selection.UpdatePlayerInput(1, true, 0, true, false);
    assert(selection.ArePlayersReady());

    std::puts("Cooperative setup tests passed");
    return 0;
}
