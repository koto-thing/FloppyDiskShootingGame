#pragma once

#include <cstdint>

namespace ShooterStages::Stage2 {

/** @brief Stage 2ボスの行動状態 */
enum class BossAction {
    Idle,
    MainGunCharge,
    MainGunFire,
    MainGunCooldown,
    Dive,
    Underground,
    Warning,
    Charge,
    Recover,
    Separating
};

/** @brief Stage 2ボスの永続状態 */
struct BossState {
    BossAction action = BossAction::Idle;
    int actionAge = 0;
    float landBattleshipOffsetX = 0.0f;
    float landBattleshipOffsetY = 0.0f;
    float landBattleshipOffsetZ = 0.0f;
    float sandSubmarineOffsetX = 0.0f;
    float sandSubmarineOffsetY = 0.0f;
    float sandSubmarineOffsetZ = 0.0f;
    int funnelLaunchCooldowns[12] {};
    int funnelLaunchCounts[12] {};
};

/**
 * @brief Phase 3ファンネルのハッチ別射出間隔を取得する
 * @param hatch ハッチ番号
 * @param launchCount そのハッチからの射出回数
 * @return 55から145フレームの決定的な疑似ランダム間隔
 */
constexpr int Phase3FunnelLaunchInterval(int hatch, int launchCount) {
    std::uint32_t value = static_cast<std::uint32_t>(hatch + 1) * 0x9e3779b9u ^
        static_cast<std::uint32_t>(launchCount + 1) * 0x85ebca6bu;
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    return 55 + static_cast<int>(value % 91u);
}

/**
 * @brief Phase 3主砲の予告中に使用する追従率を取得する
 * @param frame 予告開始からの経過フレーム
 * @param fireFrame 発射フレーム
 * @return 予告の前後で遅く中央で速い追従率
 */
constexpr float Phase3MainGunTrackingRate(int frame, int fireFrame) {
    const float progress = fireFrame > 0 ?
        static_cast<float>(frame) / static_cast<float>(fireFrame) : 1.0f;
    const float clamped = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    const float triangle = clamped < 0.5f ? clamped * 2.0f : (1.0f - clamped) * 2.0f;
    const float eased = triangle * triangle * (3.0f - 2.0f * triangle);
    return 0.012f + eased * 0.078f;
}

/** @brief Stage 2特殊弾の種類 */
enum class ShotKind {
    None,
    Funnel,
    Missile,
    ReflectPass,
    ReflectAttack,
    FunnelMissile
};

/** @brief Stage 2特殊弾の状態 */
struct ShotState {
    ShotKind kind = ShotKind::None;
    int dustAge = -1;
    float dustX = 0.0f;
    float dustY = 0.0f;
    float dustZ = 0.0f;
    float engineVx = 0.0f;
    float engineVy = 0.0f;
    float engineVz = 0.0f;
    bool delayedEngine = false;
};

/** @brief Stage 2特殊デブリの種類 */
enum class DebrisKind {
    None,
    Sink,
    Impact,
    ImpactPiece
};

/** @brief Stage 2特殊デブリの状態 */
struct DebrisState {
    DebrisKind kind = DebrisKind::None;
    int effectAge = -1;
};

/** @brief 骨アーチの最大HP */
inline constexpr int BoneArchMaxHp = 12000;

/** @brief Stage 2全体の永続状態 */
struct State {
    BossState boss {};
    int boneArchHp = BoneArchMaxHp;
    bool boneArchDestroyed = false;
};

}
