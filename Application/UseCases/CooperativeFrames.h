#pragma once
#include <array>
#include <cstdint>
#include "../../Domain/ValueObjects/CooperativeInput.h"

/** @brief 同じ番号の入力が2人分揃った固定更新だけを取り出す */
class CooperativeFrames {
public:
    static constexpr std::uint32_t Delay = 6;
    static constexpr std::uint32_t Capacity = 256;

    /**
     * @brief 入力遅延分の空フレームで初期化する
     * @param delay 両者で合意した入力遅延フレーム数
     * @return なし
     */
    void Reset(std::uint32_t delay = Delay) {
        // フレームバッファと送信位置を初期化する
        m_frames = {};
        m_delay = delay >= Delay && delay < Capacity ? delay : Delay;
        m_next = {m_delay, m_delay};
        m_frame = 0;

        // 入力遅延分を空入力として消費済みにする
        for (std::uint32_t i = 0; i < m_delay; ++i) m_frames[i].ready = 3;
    }

    /**
     * @brief 次に送る入力番号を取得する
     * @param player プレイヤー番号
     * @return フレーム番号
     */
    std::uint32_t Next(int player) const { return m_next[player]; }

    /**
     * @brief 送信先行が遅延幅以内か調べる
     * @param player プレイヤー番号
     * @param lookahead 先行上限、0または範囲外なら開始時の遅延を使う
     * @return 送信可能ならtrue
     */
    bool CanSubmit(int player, std::uint32_t lookahead = 0) const {
        const auto limit = lookahead >= Delay && lookahead < Capacity ? lookahead : m_delay;
        return m_next[player] >= m_frame && m_next[player] - m_frame <= limit;
    }

    /**
     * @brief 順序とビットを検証して入力を保存する
     * @param player プレイヤー番号
     * @param frame 更新番号
     * @param input 操作
     * @return 正常ならtrue
     */
    bool Push(int player, std::uint32_t frame, CooperativeInput input) {
        // プレイヤー番号、フレーム番号、入力ビットを検証する
        if (player < 0 || player > 1 || frame != m_next[player] || frame < m_frame ||
            frame - m_frame >= Capacity || frame == UINT32_MAX ||
            ((input.held | input.pressed) & ~CooperativeInput::ValidButtons)) return false;

        // 入力をフレームへ保存して受信済み状態を進める
        auto& slot = m_frames[frame % Capacity];
        slot.inputs[player] = input;
        slot.ready |= static_cast<unsigned char>(1 << player);
        ++m_next[player];
        return true;
    }

    /**
     * @brief 両者が揃ったフレームを消費する
     * @param inputs 同期済み操作の出力先
     * @return 更新可能ならtrue
     */
    bool Pop(std::array<CooperativeInput, 2>& inputs) {
        // 両者の入力が揃うまで待機する
        auto& slot = m_frames[m_frame % Capacity];
        if (slot.ready != 3) return false;

        // 同期済み入力を出力して次のフレームへ進める
        inputs = slot.inputs;
        slot = {};
        ++m_frame;
        return true;
    }
private:
    struct Frame { std::array<CooperativeInput, 2> inputs {}; unsigned char ready = 0; };
    std::array<Frame, Capacity> m_frames {};
    std::array<std::uint32_t, 2> m_next {Delay, Delay};
    std::uint32_t m_frame = 0;
    std::uint32_t m_delay = Delay;
};
