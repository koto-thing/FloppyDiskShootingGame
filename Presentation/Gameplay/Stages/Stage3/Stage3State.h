#pragma once

#include <array>

namespace ShooterStages::Stage3 {

/** @brief Phase3で展開する破壊可能な反射ファンネル */
struct ReflectFunnel {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    int hp = 0;
    int age = 0;
    int port = 0;
    int spinFrames = 0;
    bool active = false;
};

/** @brief Phase3で同時展開できる反射ファンネル数 */
inline constexpr int ReflectFunnelCount = 5;

/** @brief 反射ファンネル機雷が自然爆発するまでのフレーム数 */
inline constexpr int FunnelMineLifetimeFrames = 5 * 60;

/**
 * @brief 反射ファンネル機雷が自然爆発する時刻か判定する
 * @param age 機雷生成後の経過フレーム数
 * @return 5秒以上経過した場合true
 */
constexpr bool IsFunnelMineExpired(int age) {
    return age >= FunnelMineLifetimeFrames;
}

/** @brief Stage3固有の永続状態 */
struct State {
    std::array<ReflectFunnel, ReflectFunnelCount> reflectFunnels {};
    std::array<int, 3> funnelPortCooldowns {};
    float laserTargetX = 0.0f;
    float laserTargetY = 0.0f;
    float laserTargetZ = 0.0f;
    int nextFunnelPort = 0;
    bool laserTargetInitialized = false;
};

}
