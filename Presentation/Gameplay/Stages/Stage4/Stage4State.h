#pragma once

namespace ShooterStages::Stage4 {

/** @brief Stage 4特殊弾の種類 */
enum class ShotKind {
    None,
    Cannonball
};

/** @brief Stage 4特殊弾の状態 */
struct ShotState {
    ShotKind kind = ShotKind::None;
    bool gravity = false;
};

}
