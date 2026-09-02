#pragma once

#include <array>

namespace ShooterStages::Stage1 {

/** @brief ステージ1を横切る破壊可能な隕石 */
struct Meteor {
    float travel = 0.0f;
    float scale = 1.0f;
    float yaw = 0.0f;
    float spin = 0.0f;
    int hp = 0;
    bool destroyed = false;
};

/** @brief ステージ1に配置する隕石数 */
inline constexpr int MeteorCount = 6;

/** @brief ステージ1固有の永続状態 */
struct State {
    std::array<Meteor, MeteorCount> meteors {};
};

}
