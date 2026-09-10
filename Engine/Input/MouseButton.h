#pragma once

#include <cstdint>

/** @brief マウスボタンの識別子 */
enum class MouseButton : std::uint8_t
{
    Left = 0,
    Right,
    Middle,

    Button4,
    Button5,

    Count
};
