#pragma once

#include <array>
#include "../../Domain/ValueObjects/PlayerType.h"

enum class ModeSelectionState {
    PlayerCountSelect,
    ControllerSelect,
    DifficultySelect,
    PlayerTypeSelect,
};

/** @brief モードセレクト画面の選択段階を管理するクラス */
class ModeSelectionStateController {
public:
    /** @brief 現在の選択段階を取得する */
    ModeSelectionState GetCurrentState() const { return m_currentState; }

    /** @brief 指定した選択段階へ切り替える */
    void SetCurrentState(ModeSelectionState state) {
        m_currentState = state;
        if (state == ModeSelectionState::PlayerTypeSelect) m_ready.fill(false);
    }

    /**
     * @brief 2人用の接続確認と各自のショット選択を更新する
     * @param player プレイヤー番号、0または1
     * @param connected 対応コントローラーが接続中ならtrue
     * @param direction 選択移動、前なら-1、次なら1、移動なしなら0
     * @param confirm 決定を押したならtrue
     * @param cancel 決定解除を押したならtrue
     * @return なし
     */
    void UpdatePlayerInput(int player, bool connected, int direction, bool confirm, bool cancel);

    /** @brief 割り当て済みか取得する @param player プレイヤー番号 @return 割り当て済みならtrue */
    bool IsControllerJoined(int player) const { return m_joined[player]; }
    /** @brief 選択を確定済みか取得する @param player プレイヤー番号 @return 確定済みならtrue */
    bool IsPlayerReady(int player) const { return m_ready[player]; }
    /** @brief 選択中のショットを取得する @param player プレイヤー番号 @return ショットタイプ */
    PlayerType GetPlayerType(int player) const { return m_playerTypes[player]; }
    /** @brief 2人とも開始できるか取得する @return 割り当てと選択が完了ならtrue */
    bool ArePlayersReady() const { return m_joined[0] && m_joined[1] && m_ready[0] && m_ready[1]; }

private:
    ModeSelectionState m_currentState = ModeSelectionState::PlayerCountSelect;
    std::array<bool, 2> m_joined {};
    std::array<bool, 2> m_ready {};
    std::array<PlayerType, 2> m_playerTypes {Homing, Homing};
};
