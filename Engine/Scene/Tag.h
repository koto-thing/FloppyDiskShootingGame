#pragma once

#include <cstdint>

/**
 * @brief GameObjectを分類するタグ
 */
enum class Tag : std::uint8_t {
    Untagged,
    Player,
    Enemy,
    PlayerBullet,
    EnemyBullet,
    Item,
    MainCamera
};

/**
 * @brief GameObjectの衝突・描画分類に使うレイヤー
 */
enum class Layer : std::uint8_t {
    Default,
    Player,
    Enemy,
    PlayerBullet,
    EnemyBullet,
    Item,
    UI
};
