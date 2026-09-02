#pragma once

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
};

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
