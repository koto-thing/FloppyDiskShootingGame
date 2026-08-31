#pragma once

enum class ModeSelectionState {
    DifficultySelect,
    PlayerTypeSelect,
};

/** @brief モードセレクト画面の選択段階を管理するクラス */
class ModeSelectionStateController {
public:
    /** @brief 現在の選択段階を取得する */
    ModeSelectionState GetCurrentState() const { return m_currentState; }

    /** @brief 指定した選択段階へ切り替える */
    void SetCurrentState(ModeSelectionState state) { m_currentState = state; }

private:
    ModeSelectionState m_currentState = ModeSelectionState::DifficultySelect;
};
