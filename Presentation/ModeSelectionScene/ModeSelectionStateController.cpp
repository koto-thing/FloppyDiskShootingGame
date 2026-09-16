#include "ModeSelectionStateController.h"

void ModeSelectionStateController::UpdatePlayerInput(
    int player, bool connected, int direction, bool confirm, bool cancel) {
    if (player < 0 || player >= 2) return;

    // 切断時は再割り当てと両プレイヤーの再決定を要求する
    if (!connected) {
        m_joined[player] = false;
        m_ready.fill(false);
        if (m_currentState != ModeSelectionState::PlayerCountSelect)
            m_currentState = ModeSelectionState::ControllerSelect;
        return;
    }

    // 各コントローラーで決定してから難易度選択へ進む
    if (m_currentState == ModeSelectionState::ControllerSelect) {
        if (cancel) m_joined[player] = false;
        else if (confirm) m_joined[player] = true;
        if (m_joined[0] && m_joined[1]) m_currentState = ModeSelectionState::DifficultySelect;
        return;
    }
    if (m_currentState != ModeSelectionState::PlayerTypeSelect) return;

    // 各自が独立して選択でき、同じショットタイプの重複も許可する
    if (cancel) { m_ready[player] = false; return; }
    if (m_ready[player]) return;
    const int step = direction < 0 ? -1 : (direction > 0 ? 1 : 0);
    m_playerTypes[player] = static_cast<PlayerType>((static_cast<int>(m_playerTypes[player]) + step + 3) % 3);
    if (confirm) m_ready[player] = true;
}
