#pragma once

#include <cstdint>

enum class CollisionLayer : std::uint32_t {
    NONE        = 0,
    PLAYER      = 1 << 0,
    PLAYER_SHOT = 1 << 1,
    ENEMY       = 1 << 2,
    ENEMY_SHOT  = 1 << 3,
    WALL        = 1 << 4,
};

constexpr CollisionLayer operator | (
    CollisionLayer lhs,
    CollisionLayer rhs
) {
    return static_cast<CollisionLayer>(
        static_cast<std::uint32_t>(lhs) |
        static_cast<std::uint32_t>(rhs)
    );
}