#pragma once
#include <cstdint>

/** @brief 固定更新1回分の協力プレイ操作 */
struct CooperativeInput {
    enum Button : std::uint16_t {
        Left = 1, Right = 2, Up = 4, Down = 8, Slow = 16,
        Fire = 32, Bomb = 64, View = 128, Confirm = 256, Restart = 512, Pause = 1024
    };
    std::uint16_t held = 0;
    std::uint16_t pressed = 0;
    static constexpr std::uint16_t ValidButtons = 2047;
};
