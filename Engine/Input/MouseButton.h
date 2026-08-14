#pragma once

#include <cstdint>

enum class MouseButton : std::uint8_t
{
    Left = 0,
    Right,
    Middle,

    Button4,
    Button5,

    Count
};