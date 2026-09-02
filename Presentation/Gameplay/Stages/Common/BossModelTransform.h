#pragma once

#include "../../../../Engine/Math/Vector3.h"

/** @brief Stage 2とStage 4のボスモデルに共通する親座標と向きを表す */
struct BossModelTransform {
    Vector3 position {};
    Vector3 aimTarget {};
    float yaw = 0.0f;
    float scale = 1.0f;
    bool mainGunTracksTarget = false;
    bool secondaryGunsTrackTarget = false;
    Vector3 secondaryAimTarget {};
};
