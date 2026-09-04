#pragma once

#include <array>
#include <cstdint>

#include "Stage5ModelTypes.h"

namespace ShooterStages::Stage5 {

/** @brief Stage 5専用の進行状態 */
enum class Phase {
    Approach,
    EastsourceIntro,
    EastsourceBattle,
    EastsourceFall,
    WallClimbTransition,
    WallClimbLower,
    WallClimbMiddle,
    WallClimbUpper,
    RooftopArrival,
    CarrierTransformation,
    TayamaFireControl,
    TayamaLiftEngines,
    TayamaCommandCore,
    TayamaCollapse,
    CloudSea,
    TayamaDragonBattle,
    TayamaDragonCollapse,
    EndingReady
};

/** @brief Stage 5の被弾復帰地点 */
enum class Checkpoint {
    Chapter1,
    Chapter2,
    Chapter3,
    Eastsource,
    WallClimbLower,
    WallClimbMiddle,
    WallClimbUpper,
    TayamaFireControl,
    TayamaLiftEngines,
    TayamaCommandCore,
    TayamaDragonBattle
};

/** @brief サーチライトの索敵状態 */
enum class SearchlightPhase {
    Searching,
    Detecting,
    Locked,
    Firing,
    Cooldown
};

/** @brief TAYAMAの破壊可能な弱点 */
enum class TayamaWeakpoint {
    LeftSearchlight,
    RightSearchlight,
    FireControlRadar,
    LeftLiftEngine,
    RightLiftEngine,
    CommandCore,
    Count
};

/** @brief 壁面およびTAYAMAのサーチライト状態 */
struct SearchlightState {
    SearchlightPhase phase = SearchlightPhase::Searching;
    float beamX = 0.0f;
    float beamY = 0.0f;
    float beamZ = 8.0f;
    float lockedX = 0.0f;
    float lockedY = 0.0f;
    float lockedZ = 8.0f;
    int detectionFrames = 0;
    int timer = 0;
    int volley = 0;
    int hp = 0;
    bool destroyed = false;
};

/** @brief TAYAMA弱点の固定長状態 */
struct TayamaWeakpointState {
    TayamaWeakpoint type = TayamaWeakpoint::LeftSearchlight;
    int hp = 0;
    int maxHp = 0;
    bool active = false;
    bool destroyed = false;
    int hitFlashFrames = 0;
};

inline constexpr int SearchlightCount = 3;
inline constexpr int TayamaWeakpointCount = static_cast<int>(TayamaWeakpoint::Count);
inline constexpr int TayamaMaxHp = 6000;
inline constexpr int TayamaPartBreakDamage = 500;
static_assert(TayamaMaxHp > 3600);
static_assert(TayamaPartBreakDamage * 10 >= TayamaMaxHp / 2);
inline constexpr int EastsourceMaxHp = 1200;
inline constexpr int EastsourceNoseHp = 180;
inline constexpr int EastsourceWingHp = 240;
inline constexpr int EastsourceEngineHp = 210;
inline constexpr int EastsourceIntroFrames = 210;
inline constexpr int EastsourceFallFrames = 180;
inline constexpr int EastsourceNoseAttackCycleFrames = 72;
inline constexpr int EastsourceDamagedNoseAttackCycleFrames = 96;
inline constexpr int EastsourceNoseWarningFrames = 24;
inline constexpr int EastsourceDamagedNoseWarningFrames = 36;
inline constexpr int EastsourceNoseShotCount = 7;
inline constexpr int EastsourceDamagedNoseShotCount = 3;
inline constexpr int EastsourceNoseShotIntervalFrames = 5;
inline constexpr int EastsourceWingBarrageCycleFrames = 60;
inline constexpr int EastsourcePursuitCycleFrames = 120;
inline constexpr int EastsourcePursuitRetreatFrames = 48;
inline constexpr int EastsourcePursuitWarningEndFrame = 66;
inline constexpr int EastsourcePursuitPoweredPassFrames = 24;
inline constexpr int EastsourcePursuitDamagedPassFrames = 34;
inline constexpr int EastsourcePursuitAimFrame = 52;
inline constexpr int EastsourcePursuitShotStartFrame = 72;
inline constexpr int EastsourcePursuitShotEndFrame = 96;
static_assert(EastsourceNoseWarningFrames +
    (EastsourceNoseShotCount - 1) * EastsourceNoseShotIntervalFrames <
    EastsourceNoseAttackCycleFrames);
static_assert(EastsourcePursuitWarningEndFrame +
    EastsourcePursuitDamagedPassFrames < EastsourcePursuitCycleFrames);
static_assert(EastsourcePursuitAimFrame < EastsourcePursuitShotStartFrame &&
    EastsourcePursuitShotStartFrame < EastsourcePursuitShotEndFrame);
inline constexpr int WallClimbTransitionFrames = 660;
inline constexpr int WallClimbFadeFrames = 60;
inline constexpr int WallClimbApproachFrames = 210;
inline constexpr int WallClimbExitFadeFrames = 45;
inline constexpr float WallApproachStartZ = 8.0f;
inline constexpr float WallApproachEndZ = 32.0f;
inline constexpr float PandDBuildingWidth = 144.0f;
inline constexpr float PandDBuildingHeight = 1200.0f;
inline constexpr float PandDBuildingCapScale = 1.08f;
inline constexpr int PandDBuildingWindowColumns = 6;
inline constexpr float RooftopSurfaceY = -3.35f;
inline constexpr float TayamaBossScale = 8.0f;
inline constexpr float TayamaArenaCenterZ = 57.0f;
inline constexpr float TayamaOrbitRadius = 115.0f * 1.5f;
inline constexpr float TayamaOrbitSpeed = 0.018f;
inline constexpr float TayamaCameraDistance = 18.0f;
inline constexpr float TayamaCameraHeight = 5.0f;
inline constexpr float TayamaCameraLookAhead = 32.0f;
inline constexpr float TayamaCameraFarClip = TayamaOrbitRadius * 2.0f + 110.0f;
inline constexpr float TayamaOverheadDistanceScale = 0.9f;
inline constexpr float TayamaFrontDistanceScale = 1.4f;
inline constexpr float TayamaPlayerMinY = -0.70f;
inline constexpr float TayamaPlayerMaxY = 55.0f;
inline constexpr float WallClimbHeight = 360.0f;
inline constexpr float Part2SideEnemyEntryY = 2.35f;
inline constexpr float Part2RailEnemyEntryY = 68.0f;
inline constexpr float Part2RailEnemyEntryStep = 0.45f;
inline constexpr float Part2RailEnemyFallSpeed = 0.16f;
inline constexpr float Part2SideEnemyExitSpeed = 0.10f;
inline constexpr float Part2RailEnemyExitSpeed = 1.4f;
inline constexpr float Part2RailEnemyPlaneZ = 35.0f;
inline constexpr float Part2RailEnemyExitY = -5.0f;
inline constexpr float Part2RailShotMinY = Part2RailEnemyExitY;
inline constexpr float Part2RailShotMaxY = Part2RailEnemyEntryY + Part2RailEnemyEntryStep * 2.0f;
inline constexpr float Part2RailPlayerMinY = 0.80f;
inline constexpr float Part2RailPlayerMaxY = 16.0f;
inline constexpr float Part2RailEnemyScale = 2.0f;
inline constexpr int Part2DroneColumnCount = 5;
inline constexpr int Part2DroneRowCount = 6;
inline constexpr float Part2SideDroneBaseY = -0.62f;
inline constexpr float Part2SideDroneBaseStep = 0.30f;
inline constexpr float Part2RailDroneBaseY = 3.0f;
inline constexpr float Part2RailDroneBaseStep = 2.35f;
inline constexpr float Part2RailDroneEntryY = Part2RailPlayerMaxY + 2.0f;
inline constexpr int Part2RailDroneEntryFrames = 120;
inline constexpr float Part2SideSceneryFallSpeed = 0.64f;
inline constexpr float Part2RailSceneryFallSpeed = 0.96f;
inline constexpr float Part2SideItemFallSpeed = 0.014f;
inline constexpr float Part2RailItemFallSpeed = 0.06f;
inline constexpr int WallClimbLowerFrames = 900;
inline constexpr int WallClimbMiddleFrames = 900;
inline constexpr int WallClimbUpperFrames = 900;
inline constexpr int Part2PlayerFlyAwayFrames = 30;
inline constexpr float Part2PlayerFlyAwaySpeed = 0.12f;
inline constexpr int RooftopArrivalFrames = 240;
inline constexpr int CarrierTransformationFrames = 180;
inline constexpr int CarrierCameraMoveFrames = 45;
inline constexpr int TayamaIntroductionWarningFrames = 150;
inline constexpr int TayamaCollapseFrames = 540;
inline constexpr int FinalEscapeHeadStartFrames = 30;
inline constexpr int FinalEscapeHeadDurationFrames = 180;
inline constexpr int FinalEscapeDragonStartFrames = 120;
inline constexpr int FinalEscapeDragonDurationFrames = 300;
inline constexpr int FinalEscapePlayerStartFrames = 300;
inline constexpr int FinalEscapePlayerDurationFrames = 150;
inline constexpr int FinalEscapeFadeStartFrames = 450;
inline constexpr int CloudSeaFadeFrames = 90;
inline constexpr int CloudSeaAssemblyFrames = 240;
inline constexpr int RooftopStormCloudCount = 192;
inline constexpr int TayamaDragonMaxHp = 4000;
inline constexpr int TayamaDragonSegmentCount = 40;
inline constexpr float TayamaDragonShotFarZ = 108.0f;
inline constexpr int TayamaDragonBarrageIntervalFrames = 90;
inline constexpr int TayamaDragonBarrageFireFrame = 45;
inline constexpr int TayamaDragonBarrageChargeFrames = 30;
inline constexpr int TayamaDragonBarrageRecoveryFrames = 15;
inline constexpr int TayamaDragonBarrageSourceCount = 4;
inline constexpr int TayamaDragonBarrageShotsPerSource = 25;

/**
 * @brief 胴体弾幕の現在フレームに発射する弾番号を取得する
 * @param attackTimer 胴体弾幕開始後の経過フレーム数
 * @return 0以上24以下の弾番号、発射しないフレームは-1
 */
constexpr int TayamaDragonBarrageShotIndex(int attackTimer) {
    const int frame = attackTimer % TayamaDragonBarrageIntervalFrames;
    return frame >= TayamaDragonBarrageFireFrame &&
        frame < TayamaDragonBarrageFireFrame + TayamaDragonBarrageShotsPerSource ?
        frame - TayamaDragonBarrageFireFrame : -1;
}
inline constexpr int TayamaDragonSweepCycleFrames = 300;
inline constexpr int TayamaDragonSweepWarningFrames = 45;
inline constexpr int TayamaDragonSweepActiveFrames = 105;
inline constexpr int TayamaDragonSweepRecoveryFrames = 45;
inline constexpr int TayamaDragonRushCycleFrames = 480;
inline constexpr int TayamaDragonRushStartFrame = 150;
inline constexpr int TayamaDragonRushWarningFrames = 60;
inline constexpr int TayamaDragonRushActiveFrames = 30;
inline constexpr int TayamaDragonRushRecoveryFrames = 75;
inline constexpr float TayamaDragonRushWindupRate = 0.12f;
inline constexpr float TayamaDragonRushSideDistance = 1.35f;
inline constexpr float TayamaDragonRushRailDistance = 48.0f;
inline constexpr int TayamaDragonOrbitStartFrame = 330;
inline constexpr int TayamaDragonOrbitFrames = 120;
inline constexpr float TayamaDragonOrbitRadius = 10.5f;
inline constexpr float TayamaDragonOrbitSegmentAngle = 0.16f;
inline constexpr int TayamaDragonOrbitShotIntervalFrames = 12;
inline constexpr int TayamaDragonOrbitMoveFrames = 30;
inline constexpr int TayamaDragonRomanceCannonIntervalFrames = 900;
inline constexpr int TayamaDragonRomanceCannonSequenceFrames = 180;
inline constexpr int TayamaDragonRomanceCannonMoveFrames = 30;
inline constexpr int TayamaDragonRomanceCannonFireFrame = 75;
inline constexpr float TayamaDragonSideCenterX = 0.98f;
inline constexpr int TayamaDragonCollapseSegmentIntervalFrames = 10;
inline constexpr int TayamaDragonHeadPartCount = 34;
inline constexpr int TayamaDragonHeadBreakStartFrame = 285;
inline constexpr int TayamaDragonHeadPartIntervalFrames = 8;
inline constexpr int TayamaDragonCollapseHeadExplosionFrame =
    TayamaDragonHeadBreakStartFrame +
    TayamaDragonHeadPartCount * TayamaDragonHeadPartIntervalFrames;
inline constexpr int TayamaDragonCollapseFrames =
    TayamaDragonCollapseHeadExplosionFrame + 120;
inline constexpr int SearchlightLockFrames = 45;
inline constexpr int SearchlightWarningFrames = 24;
inline constexpr int SearchlightVolleyCount = 3;
inline constexpr int SearchlightVolleyIntervalFrames = 10;
inline constexpr float SearchlightDetectionRadius = 0.27f;
inline constexpr int DroneMachineGunBurstFrames = 45;
inline constexpr int DroneMachineGunShotIntervalFrames = 5;
inline constexpr int DroneMachineGunCooldownFrames = 75;
inline constexpr float DroneSearchlightDetectionRadius = 0.12f;
inline constexpr int TayamaArmSpinWarningFrames = 45;
inline constexpr int TayamaArmSpinActiveFrames = 180;
inline constexpr int TayamaArmSpinRecoveryFrames = 30;
inline constexpr int TayamaArmSpinCycleFrames = 300;
inline constexpr int TayamaArmSpinTurns = 3;
inline constexpr float TayamaArmSpinHitRadius = 4.0f;
inline constexpr int TayamaBattleReadFrames = 76;
inline constexpr int TayamaHangarSpawnIntervalFrames = 240;
inline constexpr int TayamaRadarBurstIntervalFrames = 180;
inline constexpr int TayamaRadarBurstBulletCount = 16;
inline constexpr int TayamaHeadLaserCycleFrames = 240;
inline constexpr int TayamaHeadLaserWarningFrames = 45;
inline constexpr int TayamaHeadLaserActiveFrames = 60;
inline constexpr int TayamaHeadLaserFadeFrames = 8;
inline constexpr float TayamaHeadLaserFrontDot = 0.92f;
inline constexpr float TayamaHeadLaserLength = 200.0f;
inline constexpr float TayamaHeadLaserHitRadius = 3.2f;
inline constexpr int TayamaReflectFunnelCount = 3;
inline constexpr int TayamaReflectFunnelLaunchIntervalFrames = 5 * 60;
inline constexpr int TayamaReflectFunnelLaunchFrames = 60;
inline constexpr int TayamaReflectFunnelShotIntervalFrames = 5 * 60;
inline constexpr int TayamaReflectFunnelOrbitFrames = 10 * 60;
inline constexpr int TayamaReflectFunnelHp = 30;
inline constexpr float TayamaReflectShotSpeed = 0.11f;

/**
 * @brief TAYAMA龍第2形態の反射ファンネル旋回角度を取得する
 * @param age ファンネル生成後の経過フレーム数
 * @return 配置完了位置を0とする時計回りの角度
 */
constexpr float TayamaReflectFunnelOrbitAngle(int age) {
    return age < TayamaReflectFunnelLaunchFrames ? 0.0f :
        -static_cast<float>(age - TayamaReflectFunnelLaunchFrames) /
            static_cast<float>(TayamaReflectFunnelOrbitFrames) * Math::TwoPi;
}

/** @brief TAYAMA龍第2形態の排他的な攻撃 */
enum class TayamaDragonAttack {
    None,
    HeadLaser,
    BodyBarrage,
    BodySweep,
    Rush,
    Orbit,
    RomanceCannon,
    Count
};

inline constexpr int TayamaDragonAttackCooldownFrames = 45;

/**
 * @brief 攻撃固有タイマーを既存の演出タイムラインへ変換する
 * @param attack 選択中の攻撃
 * @param timer 攻撃開始からのフレーム数
 * @return 既存ヘルパーへ渡すタイマー
 */
constexpr int TayamaDragonAttackTimeline(TayamaDragonAttack attack, int timer) {
    if (attack == TayamaDragonAttack::Rush) return TayamaDragonRushStartFrame + timer;
    if (attack == TayamaDragonAttack::Orbit) {
        return TayamaDragonRushStartFrame + TayamaDragonOrbitStartFrame + timer;
    }
    if (attack == TayamaDragonAttack::RomanceCannon) {
        return TayamaDragonRomanceCannonIntervalFrames + timer;
    }
    return timer;
}

/**
 * @brief 選択攻撃の継続フレーム数を取得する
 * @param attack 対象攻撃
 * @return 継続フレーム数
 */
constexpr int TayamaDragonAttackDuration(TayamaDragonAttack attack) {
    if (attack == TayamaDragonAttack::HeadLaser) {
        return TayamaHeadLaserWarningFrames + TayamaHeadLaserActiveFrames;
    }
    if (attack == TayamaDragonAttack::BodyBarrage) {
        return TayamaDragonBarrageIntervalFrames * TayamaDragonBarrageSourceCount;
    }
    if (attack == TayamaDragonAttack::BodySweep) return TayamaDragonSweepWarningFrames +
        TayamaDragonSweepActiveFrames + TayamaDragonSweepRecoveryFrames;
    if (attack == TayamaDragonAttack::Rush) return TayamaDragonRushWarningFrames +
        TayamaDragonRushActiveFrames + TayamaDragonRushRecoveryFrames;
    if (attack == TayamaDragonAttack::Orbit) return TayamaDragonOrbitFrames;
    if (attack == TayamaDragonAttack::RomanceCannon) return TayamaDragonRomanceCannonSequenceFrames;
    return TayamaDragonAttackCooldownFrames;
}

/** @brief TAYAMA龍第2形態が射出する反射ファンネル */
struct TayamaReflectFunnel {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    int hp = 0;
    int age = 0;
    int spinFrames = 0;
    bool active = false;
};

/**
 * @brief TAYAMA龍第2形態の反射ファンネル射出時刻か判定する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 5秒間隔の射出時刻の場合true
 */
constexpr bool IsTayamaReflectFunnelLaunchFrame(int attackTimer) {
    return attackTimer > 0 &&
        attackTimer % TayamaReflectFunnelLaunchIntervalFrames == 0;
}

/**
 * @brief TAYAMA龍第2形態の反射ファンネルが射撃する時刻か判定する
 * @param age ファンネル生成後の経過フレーム数
 * @return 配置完了後または5秒間隔の射撃時刻の場合true
 */
constexpr bool IsTayamaReflectFunnelShotFrame(int age) {
    return age >= TayamaReflectFunnelLaunchFrames &&
        (age - TayamaReflectFunnelLaunchFrames) %
            TayamaReflectFunnelShotIntervalFrames == 0;
}
inline constexpr int TayamaRearMissileCount = 10;
inline constexpr float TayamaRearMissileBackDot = 0.0f;
inline constexpr float TayamaRearMissileSpeed = 0.32f;
inline constexpr float TayamaRearMissileTurnRate = 0.035f;
inline constexpr float TayamaStompTriggerMaxWorldY = RooftopSurfaceY + 12.0f;
inline constexpr int TayamaStompRaiseFrames = 24;
inline constexpr int TayamaStompHoldFrames = 12;
inline constexpr int TayamaStompDropFrames = 6;
inline constexpr int TayamaStompImpactFrame =
    TayamaStompRaiseFrames + TayamaStompHoldFrames + TayamaStompDropFrames;
inline constexpr int TayamaStompRecoveryFrames = 36;
inline constexpr int TayamaStompSequenceFrames =
    TayamaStompImpactFrame + TayamaStompRecoveryFrames;
inline constexpr int TayamaStompCooldownFrames = 240;
inline constexpr float TayamaStompLiftLocalY = 3.5f;
inline constexpr int TayamaStompDebrisCount = 9;
static_assert(TayamaRearMissileCount == 10);

/**
 * @brief 自機がTAYAMAの足元を狙える低高度にいるか判定する
 * @param playerWorldY 自機中心のワールドY座標
 * @return 踏みつけ攻撃を開始できる高さの場合true
 */
constexpr bool IsTayamaStompRange(float playerWorldY) {
    return playerWorldY <= TayamaStompTriggerMaxWorldY;
}

/**
 * @brief 踏みつけ中の脚部上昇量を取得する
 * @param stompTimer 踏みつけ開始からの経過フレーム
 * @return モデルローカルY軸の上昇量
 */
constexpr float TayamaStompLiftOffset(int stompTimer) {
    if (stompTimer <= 0 || stompTimer >= TayamaStompImpactFrame) return 0.0f;
    if (stompTimer <= TayamaStompRaiseFrames) {
        return TayamaStompLiftLocalY * static_cast<float>(stompTimer) /
            static_cast<float>(TayamaStompRaiseFrames);
    }
    const int dropStart = TayamaStompRaiseFrames + TayamaStompHoldFrames;
    if (stompTimer <= dropStart) return TayamaStompLiftLocalY;
    return TayamaStompLiftLocalY *
        static_cast<float>(TayamaStompImpactFrame - stompTimer) /
        static_cast<float>(TayamaStompDropFrames);
}

static_assert(!IsTayamaStompRange(TayamaStompTriggerMaxWorldY + 0.01f));
static_assert(IsTayamaStompRange(TayamaStompTriggerMaxWorldY));
static_assert(TayamaStompLiftOffset(0) == 0.0f);
static_assert(TayamaStompLiftOffset(TayamaStompRaiseFrames) == TayamaStompLiftLocalY);
static_assert(TayamaStompLiftOffset(TayamaStompImpactFrame) == 0.0f);

/**
 * @brief 0から1の進捗へ加減速を付ける
 * @param progress 線形進捗
 * @return 始点と終点の速度が0になる進捗
 */
constexpr float TayamaDragonSmoothProgress(float progress) {
    return progress * progress * (3.0f - 2.0f * progress);
}

/**
 * @brief TAYAMA龍胴体弾幕の発光率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 消灯を0、最大発光を1とする補間率
 */
constexpr float TayamaDragonBarrageGlow(int attackTimer) {
    const int frame = attackTimer % TayamaDragonBarrageIntervalFrames;
    const int chargeStart = TayamaDragonBarrageFireFrame -
        TayamaDragonBarrageChargeFrames;
    if (frame < chargeStart) return 0.0f;
    if (frame < TayamaDragonBarrageFireFrame) {
        return static_cast<float>(frame - chargeStart) /
            static_cast<float>(TayamaDragonBarrageChargeFrames);
    }
    const int recoveryEnd = TayamaDragonBarrageFireFrame +
        TayamaDragonBarrageRecoveryFrames;
    return frame < recoveryEnd ? 1.0f -
        static_cast<float>(frame - TayamaDragonBarrageFireFrame) /
            static_cast<float>(TayamaDragonBarrageRecoveryFrames) : 0.0f;
}

/**
 * @brief TAYAMA龍薙ぎ払いの振幅率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 通常振幅を0、最大振幅を1とする補間率
 */
constexpr float TayamaDragonSweepProgress(int attackTimer) {
    const int frame = attackTimer % TayamaDragonSweepCycleFrames;
    if (frame < TayamaDragonSweepWarningFrames) {
        return static_cast<float>(frame) /
            static_cast<float>(TayamaDragonSweepWarningFrames);
    }
    const int activeEnd = TayamaDragonSweepWarningFrames +
        TayamaDragonSweepActiveFrames;
    if (frame < activeEnd) return 1.0f;
    const int recoveryEnd = activeEnd + TayamaDragonSweepRecoveryFrames;
    return frame < recoveryEnd ? 1.0f - static_cast<float>(frame - activeEnd) /
        static_cast<float>(TayamaDragonSweepRecoveryFrames) : 0.0f;
}

/**
 * @brief TAYAMA龍頭部レーザーの太さ補間率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 予告線相当を0、最大幅を1とする補間率
 */
constexpr float TayamaDragonLaserWidthProgress(int attackTimer) {
    const int frame = attackTimer % TayamaHeadLaserCycleFrames -
        TayamaHeadLaserWarningFrames;
    if (frame < 0 || frame >= TayamaHeadLaserActiveFrames) return 0.0f;
    if (frame < TayamaHeadLaserFadeFrames) {
        return static_cast<float>(frame) /
            static_cast<float>(TayamaHeadLaserFadeFrames);
    }
    const int fadeStart = TayamaHeadLaserActiveFrames - TayamaHeadLaserFadeFrames;
    return frame < fadeStart ? 1.0f :
        static_cast<float>(TayamaHeadLaserActiveFrames - frame) /
            static_cast<float>(TayamaHeadLaserFadeFrames);
}

/**
 * @brief TAYAMA龍突進攻撃の周期内フレームを取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 予告開始を0とする周期内フレーム
 */
constexpr int TayamaDragonRushFrame(int attackTimer) {
    const int frame = (attackTimer - TayamaDragonRushStartFrame) %
        TayamaDragonRushCycleFrames;
    return frame < 0 ? frame + TayamaDragonRushCycleFrames : frame;
}

/**
 * @brief TAYAMA龍が突進の予告、攻撃、復帰中か判定する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 突進シーケンス中の場合true
 */
constexpr bool IsTayamaDragonRushSequence(int attackTimer) {
    return TayamaDragonRushFrame(attackTimer) < TayamaDragonRushWarningFrames +
        TayamaDragonRushActiveFrames + TayamaDragonRushRecoveryFrames;
}

/**
 * @brief TAYAMA龍突進が接触判定を持つフレームか判定する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 突進中の場合true
 */
constexpr bool IsTayamaDragonRushActive(int attackTimer) {
    const int frame = TayamaDragonRushFrame(attackTimer);
    return frame >= TayamaDragonRushWarningFrames &&
        frame < TayamaDragonRushWarningFrames + TayamaDragonRushActiveFrames;
}

/**
 * @brief TAYAMA龍突進の予備動作進行率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 予備動作開始を0、突進直前を1とする進行率、予備動作外は0
 */
constexpr float TayamaDragonRushWarningProgress(int attackTimer) {
    const int frame = TayamaDragonRushFrame(attackTimer);
    return frame < TayamaDragonRushWarningFrames ?
        TayamaDragonSmoothProgress(static_cast<float>(frame) /
            static_cast<float>(TayamaDragonRushWarningFrames - 1)) : 0.0f;
}

/**
 * @brief TAYAMA龍突進の移動率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 通常位置を0、突進先を1とする移動率
 */
constexpr float TayamaDragonRushProgress(int attackTimer) {
    const int frame = TayamaDragonRushFrame(attackTimer);
    if (frame < TayamaDragonRushWarningFrames) {
        const float windup = static_cast<float>(frame) /
            static_cast<float>(TayamaDragonRushWarningFrames);
        return -TayamaDragonRushWindupRate * TayamaDragonSmoothProgress(windup);
    }
    const int activeEnd = TayamaDragonRushWarningFrames +
        TayamaDragonRushActiveFrames;
    if (frame < activeEnd) {
        const float charge = static_cast<float>(frame - TayamaDragonRushWarningFrames) /
            static_cast<float>(TayamaDragonRushActiveFrames - 1);
        return -TayamaDragonRushWindupRate +
            (1.0f + TayamaDragonRushWindupRate) *
                TayamaDragonSmoothProgress(charge);
    }
    const int recoveryEnd = activeEnd + TayamaDragonRushRecoveryFrames;
    if (frame < recoveryEnd) {
        const float recovery = static_cast<float>(frame - activeEnd) /
            static_cast<float>(TayamaDragonRushRecoveryFrames);
        return 1.0f - TayamaDragonSmoothProgress(recovery);
    }
    return 0.0f;
}

/**
 * @brief TAYAMA龍が自機周囲を旋回する周期内フレームを取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 旋回開始を0とするフレーム、旋回外の場合-1
 */
constexpr int TayamaDragonOrbitFrame(int attackTimer) {
    const int frame = TayamaDragonRushFrame(attackTimer);
    return frame >= TayamaDragonOrbitStartFrame &&
        frame < TayamaDragonOrbitStartFrame + TayamaDragonOrbitFrames ?
        frame - TayamaDragonOrbitStartFrame : -1;
}

/**
 * @brief TAYAMA龍が自機周囲を旋回中か判定する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 旋回中の場合true
 */
constexpr bool IsTayamaDragonOrbitActive(int attackTimer) {
    return TayamaDragonOrbitFrame(attackTimer) >= 0;
}

/**
 * @brief TAYAMA龍の旋回角度を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 旋回開始位置を0とする角度
 */
constexpr float TayamaDragonOrbitAngle(int attackTimer) {
    const int frame = TayamaDragonOrbitFrame(attackTimer);
    return frame < 0 ? 0.0f : static_cast<float>(frame) /
        static_cast<float>(TayamaDragonOrbitFrames) * Math::TwoPi;
}

/**
 * @brief TAYAMA龍が通常位置と自機包囲位置を行き来する補間率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 通常位置を0、包囲位置を1とする補間率
 */
constexpr float TayamaDragonOrbitBlend(int attackTimer) {
    const int frame = TayamaDragonOrbitFrame(attackTimer);
    if (frame < 0) return 0.0f;
    if (frame < TayamaDragonOrbitMoveFrames) {
        return static_cast<float>(frame) / static_cast<float>(TayamaDragonOrbitMoveFrames);
    }
    const int exitStart = TayamaDragonOrbitFrames - TayamaDragonOrbitMoveFrames;
    if (frame >= exitStart) {
        return static_cast<float>(TayamaDragonOrbitFrames - frame) /
            static_cast<float>(TayamaDragonOrbitMoveFrames);
    }
    return 1.0f;
}

/**
 * @brief TAYAMA龍が包囲位置への移動を終えて攻撃中か判定する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 包囲射撃中の場合true
 */
constexpr bool IsTayamaDragonOrbitAttacking(int attackTimer) {
    const int frame = TayamaDragonOrbitFrame(attackTimer);
    return frame >= TayamaDragonOrbitMoveFrames &&
        frame < TayamaDragonOrbitFrames - TayamaDragonOrbitMoveFrames;
}

/**
 * @brief TAYAMA龍の頭部切り離し攻撃内の経過フレームを取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 攻撃中は0以上、攻撃外は-1
 */
constexpr int TayamaDragonRomanceCannonFrame(int attackTimer) {
    if (attackTimer < TayamaDragonRomanceCannonIntervalFrames) return -1;
    const int frame = (attackTimer - TayamaDragonRomanceCannonIntervalFrames) %
        TayamaDragonRomanceCannonIntervalFrames;
    return frame < TayamaDragonRomanceCannonSequenceFrames ? frame : -1;
}

/**
 * @brief TAYAMA頭部と龍胴体の切り離し率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 結合を0、完全分離を1とする補間率
 */
constexpr float TayamaDragonHeadSeparationRate(int attackTimer) {
    const int frame = TayamaDragonRomanceCannonFrame(attackTimer);
    if (frame < 0) return 0.0f;
    if (frame < TayamaDragonRomanceCannonMoveFrames) {
        return static_cast<float>(frame) /
            static_cast<float>(TayamaDragonRomanceCannonMoveFrames);
    }
    const int attachStart = TayamaDragonRomanceCannonSequenceFrames -
        TayamaDragonRomanceCannonMoveFrames;
    return frame < attachStart ? 1.0f :
        static_cast<float>(TayamaDragonRomanceCannonSequenceFrames - frame) /
            static_cast<float>(TayamaDragonRomanceCannonMoveFrames);
}

/**
 * @brief 戦闘開始時の読ませ時間を除いた腕攻撃タイマーを取得する
 * @param attackTimer フェーズ開始からの攻撃タイマー
 * @return 腕攻撃開始後の経過フレーム
 */
constexpr int TayamaArmAttackTimer(int attackTimer) {
    return attackTimer > TayamaBattleReadFrames ?
        attackTimer - TayamaBattleReadFrames : 0;
}

/**
 * @brief TAYAMA頭部レーザーが照射中のフレームか判定する
 * @param attackTimer 読ませ時間を除いた攻撃タイマー
 * @return レーザー照射中の場合true
 */
constexpr bool IsTayamaHeadLaserActive(int attackTimer) {
    const int frame = attackTimer % TayamaHeadLaserCycleFrames;
    return frame >= TayamaHeadLaserWarningFrames &&
        frame < TayamaHeadLaserWarningFrames + TayamaHeadLaserActiveFrames;
}

/**
 * @brief TAYAMA腕回転攻撃が接触判定を持つフレームか判定する
 * @param attackTimer 攻撃タイマー
 * @return 回転中の場合true
 */
constexpr bool IsTayamaArmSpinActive(int attackTimer) {
    const int frame = attackTimer % TayamaArmSpinCycleFrames;
    return frame >= TayamaArmSpinWarningFrames &&
        frame < TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames;
}

/**
 * @brief TAYAMA腕回転攻撃の角度を取得する
 * @param attackTimer 攻撃タイマー
 * @return 腕へ適用する水平X軸の時計回り回転角度
 */
constexpr float TayamaArmSpinAngle(int attackTimer) {
    const int frame = attackTimer % TayamaArmSpinCycleFrames;
    if (frame < TayamaArmSpinWarningFrames) return 0.0f;
    if (frame >= TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames) return 0.0f;
    return static_cast<float>(frame - TayamaArmSpinWarningFrames) /
        static_cast<float>(TayamaArmSpinActiveFrames) *
        Math::TwoPi * static_cast<float>(TayamaArmSpinTurns);
}

static_assert(!IsTayamaArmSpinActive(TayamaArmSpinWarningFrames - 1));
static_assert(IsTayamaArmSpinActive(TayamaArmSpinWarningFrames));
static_assert(!IsTayamaArmSpinActive(
    TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames));
static_assert(TayamaArmSpinAngle(0) == 0.0f);
static_assert(TayamaArmSpinAngle(TayamaArmSpinWarningFrames) == 0.0f);
static_assert(TayamaArmAttackTimer(TayamaBattleReadFrames) == 0);
static_assert(TayamaArmAttackTimer(TayamaBattleReadFrames + 1) == 1);
static_assert(!IsTayamaHeadLaserActive(TayamaHeadLaserWarningFrames - 1));
static_assert(IsTayamaHeadLaserActive(TayamaHeadLaserWarningFrames));
static_assert(!IsTayamaHeadLaserActive(
    TayamaHeadLaserWarningFrames + TayamaHeadLaserActiveFrames));
static_assert(TayamaDragonRushFrame(TayamaDragonRushStartFrame) == 0);
static_assert(!IsTayamaDragonRushActive(
    TayamaDragonRushStartFrame + TayamaDragonRushWarningFrames - 1));
static_assert(IsTayamaDragonRushActive(
    TayamaDragonRushStartFrame + TayamaDragonRushWarningFrames));
static_assert(!IsTayamaDragonRushActive(TayamaDragonRushStartFrame +
    TayamaDragonRushWarningFrames + TayamaDragonRushActiveFrames));
static_assert(TayamaDragonRushProgress(TayamaDragonRushStartFrame) == 0.0f);
static_assert(TayamaDragonOrbitFrame(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame) == 0);
static_assert(IsTayamaDragonOrbitActive(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame + TayamaDragonOrbitFrames - 1));
static_assert(!IsTayamaDragonOrbitActive(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame + TayamaDragonOrbitFrames));
static_assert(TayamaDragonOrbitBlend(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame) == 0.0f);
static_assert(TayamaDragonOrbitBlend(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame + TayamaDragonOrbitMoveFrames) == 1.0f);
static_assert(!IsTayamaDragonOrbitAttacking(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame + TayamaDragonOrbitMoveFrames - 1));
static_assert(IsTayamaDragonOrbitAttacking(TayamaDragonRushStartFrame +
    TayamaDragonOrbitStartFrame + TayamaDragonOrbitMoveFrames));
static_assert(TayamaDragonRushProgress(TayamaDragonRushStartFrame +
    TayamaDragonRushWarningFrames + TayamaDragonRushActiveFrames) == 1.0f);
static_assert(TayamaDragonRomanceCannonFrame(
    TayamaDragonRomanceCannonIntervalFrames - 1) == -1);
static_assert(TayamaDragonRomanceCannonFrame(
    TayamaDragonRomanceCannonIntervalFrames) == 0);
static_assert(TayamaDragonHeadSeparationRate(
    TayamaDragonRomanceCannonIntervalFrames + TayamaDragonRomanceCannonMoveFrames) == 1.0f);

/**
 * @brief 屋上到着演出で斜め上から正面へ回り込む進捗を取得する
 * @param phaseTimer 屋上到着状態の経過フレーム数
 * @return 斜め上を0、正面を1とする進捗
 */
constexpr float RooftopCameraFrontProgress(int phaseTimer) {
    const float progress = static_cast<float>(phaseTimer - RooftopArrivalFrames / 2) /
        static_cast<float>(RooftopArrivalFrames / 2);
    return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

/**
 * @brief 変形終盤に戦闘カメラへ移る進捗を取得する
 * @param phaseTimer 変形状態の経過フレーム数
 * @return 正面全景を0、戦闘位置を1とする進捗
 */
constexpr float CarrierCameraBattleProgress(int phaseTimer) {
    const float progress = static_cast<float>(phaseTimer -
        (CarrierTransformationFrames - CarrierCameraMoveFrames)) /
        static_cast<float>(CarrierCameraMoveFrames);
    return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

static_assert(RooftopCameraFrontProgress(0) == 0.0f);
static_assert(RooftopCameraFrontProgress(RooftopArrivalFrames) == 1.0f);
static_assert(CarrierCameraBattleProgress(0) == 0.0f);
static_assert(CarrierCameraBattleProgress(CarrierTransformationFrames) == 1.0f);

/**
 * @brief 終幕ムービー内の指定区間を0から1へ正規化する
 * @param phaseTimer 終幕ムービーの経過フレーム数
 * @param startFrame 区間の開始フレーム
 * @param durationFrames 区間の長さ
 * @return 開始前を0、終了後を1とする進捗
 */
constexpr float FinalEscapeProgress(int phaseTimer, int startFrame, int durationFrames) {
    const float progress = static_cast<float>(phaseTimer - startFrame) /
        static_cast<float>(durationFrames);
    return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

/**
 * @brief 雲海でTAYAMA頭部が龍の首へ合体する進捗を取得する
 * @param phaseTimer 雲海演出の経過フレーム数
 * @return 暗転復帰直後を0、合体完了を1とする進捗
 */
constexpr float CloudSeaAssemblyProgress(int phaseTimer) {
    return FinalEscapeProgress(phaseTimer, CloudSeaFadeFrames,
        CloudSeaAssemblyFrames - CloudSeaFadeFrames);
}

/**
 * @brief 第2形態撃破演出で尻尾側から消えた節数を取得する
 * @param phaseTimer 撃破演出の経過フレーム数
 * @return 0から全節数までの破壊済み節数
 */
constexpr int TayamaDragonDestroyedSegmentCount(int phaseTimer) {
    const int count = phaseTimer / TayamaDragonCollapseSegmentIntervalFrames;
    return count < 0 ? 0 :
        (count > TayamaDragonSegmentCount ? TayamaDragonSegmentCount : count);
}

/**
 * @brief 第2形態撃破演出で外れた頭部パーツ数を取得する
 * @param phaseTimer 撃破演出の経過フレーム数
 * @return 0から全頭部パーツ数までの脱落済みパーツ数
 */
constexpr int TayamaDragonDestroyedHeadPartCount(int phaseTimer) {
    if (phaseTimer < TayamaDragonHeadBreakStartFrame) return 0;
    const int count =
        (phaseTimer - TayamaDragonHeadBreakStartFrame) /
        TayamaDragonHeadPartIntervalFrames + 1;
    return count > TayamaDragonHeadPartCount ? TayamaDragonHeadPartCount : count;
}

static_assert(FinalEscapeFadeStartFrames + CloudSeaFadeFrames == TayamaCollapseFrames);
static_assert(FinalEscapeProgress(FinalEscapeHeadStartFrames - 1,
    FinalEscapeHeadStartFrames, FinalEscapeHeadDurationFrames) == 0.0f);
static_assert(FinalEscapeProgress(FinalEscapeHeadStartFrames +
    FinalEscapeHeadDurationFrames, FinalEscapeHeadStartFrames,
    FinalEscapeHeadDurationFrames) == 1.0f);
static_assert(FinalEscapeProgress(FinalEscapeDragonStartFrames +
    FinalEscapeDragonDurationFrames, FinalEscapeDragonStartFrames,
    FinalEscapeDragonDurationFrames) == 1.0f);
static_assert(FinalEscapeProgress(FinalEscapePlayerStartFrames +
    FinalEscapePlayerDurationFrames, FinalEscapePlayerStartFrames,
    FinalEscapePlayerDurationFrames) == 1.0f);
static_assert(CloudSeaAssemblyProgress(CloudSeaFadeFrames) == 0.0f);
static_assert(CloudSeaAssemblyProgress(CloudSeaAssemblyFrames) == 1.0f);
static_assert(TayamaDragonDestroyedSegmentCount(0) == 0);
static_assert(TayamaDragonDestroyedSegmentCount(
    TayamaDragonSegmentCount * TayamaDragonCollapseSegmentIntervalFrames) ==
    TayamaDragonSegmentCount);
static_assert(TayamaDragonDestroyedHeadPartCount(
    TayamaDragonHeadBreakStartFrame - 1) == 0);
static_assert(TayamaDragonDestroyedHeadPartCount(
    TayamaDragonCollapseHeadExplosionFrame) == TayamaDragonHeadPartCount);
static_assert(TayamaDragonCollapseHeadExplosionFrame < TayamaDragonCollapseFrames);

/**
 * @brief 壁面警備ドローンのレーザーポインターへ自機が触れたか判定する
 * @param playerX 自機X座標
 * @param playerY 自機Y座標
 * @param pointerX ポインターX座標
 * @param pointerY ポインターY座標
 * @param playerRadius 自機の判定半径
 * @return ポインターの検知円と自機が重なる場合true
 */
constexpr bool DroneSearchlightTouches(float playerX, float playerY,
    float pointerX, float pointerY, float playerRadius) {
    const float dx = playerX - pointerX;
    const float dy = playerY - pointerY;
    const float radius = DroneSearchlightDetectionRadius + playerRadius;
    return dx * dx + dy * dy <= radius * radius;
}

/**
 * @brief 壁面警備ドローンの連射残り時間が発射フレームか判定する
 * @param remainingFrames 連射の残りフレーム数
 * @return マシンガンを発射するフレームの場合true
 */
constexpr bool IsDroneMachineGunFireFrame(int remainingFrames) {
    return remainingFrames > 0 && remainingFrames <= DroneMachineGunBurstFrames &&
        (DroneMachineGunBurstFrames - remainingFrames) %
            DroneMachineGunShotIntervalFrames == 0;
}

static_assert(DroneMachineGunBurstFrames % DroneMachineGunShotIntervalFrames == 0);
static_assert(IsDroneMachineGunFireFrame(DroneMachineGunBurstFrames));
static_assert(!IsDroneMachineGunFireFrame(DroneMachineGunBurstFrames - 1));

/** @brief Stage 5専用の効果音種別 */
enum Cue {
    DistantThunder,
    Thunder,
    SearchlightDetect,
    SearchlightLocked,
    BarrageWarning,
    EastsourceEntrance,
    SignalLost,
    Transformation,
    WeakpointDestroyed,
    CoreWarning,
    ChainExplosion,
    FinalExplosion
};

/** @brief Stage 5全体の永続状態 */
struct State {
    std::array<SearchlightState, SearchlightCount> searchlights {};
    std::array<TayamaWeakpointState, TayamaWeakpointCount> tayamaWeakpoints {};
    std::array<TayamaReflectFunnel, TayamaReflectFunnelCount> tayamaReflectFunnels {};
    float tayamaTransformation = 0.0f;
    float tayamaOrbitAngle = 0.0f;
    float tayamaSideViewAngle = 0.0f;
    float checkpointPower = 0.0f;
    float coreTargetX = 0.0f;
    float coreTargetY = 0.0f;
    float coreTargetZ = 8.0f;
    Vector3 headLaserTarget {};
    int phaseTimer = 0;
    int checkpointScore = 0;
    int checkpointKills = 0;
    int soundCooldown = 0;
    int attackTimer = 0;
    int guardSpawnCooldown = 0;
    int tayamaStompTimer = 0;
    int tayamaStompCooldown = 0;
    int tayamaHp = 0;
    int tayamaMaxHp = 0;
    int tayamaDragonHitFlashFrames = 0;
    int tayamaDragonAttackTimer = 0;
    TayamaDragonAttack tayamaDragonAttack = TayamaDragonAttack::None;
    TayamaDragonAttack previousTayamaDragonAttack = TayamaDragonAttack::None;
    bool headLaserArmed = false;
    bool tayamaStompLeftFoot = true;
    int tayamaCollisionBoundsFrame = -1;
    std::array<Stage5GroupBounds, TayamaCollisionGroupCount> tayamaCollisionBounds {};
    Phase phase = Phase::Approach;
    Checkpoint checkpoint = Checkpoint::Chapter1;
};

/**
 * @brief Stage 5の状態遷移が正規経路か判定する
 * @param from 遷移元
 * @param to 遷移先
 * @return 正規経路の場合true、許可しない遷移の場合false
 */
constexpr bool IsValidTransition(Phase from, Phase to) {
    return (from == Phase::Approach && to == Phase::EastsourceIntro) ||
        (from == Phase::EastsourceIntro && to == Phase::EastsourceBattle) ||
        (from == Phase::EastsourceBattle && to == Phase::EastsourceFall) ||
        (from == Phase::EastsourceFall && to == Phase::WallClimbTransition) ||
        (from == Phase::WallClimbTransition && to == Phase::WallClimbLower) ||
        (from == Phase::WallClimbLower && to == Phase::WallClimbMiddle) ||
        (from == Phase::WallClimbMiddle && to == Phase::WallClimbUpper) ||
        (from == Phase::WallClimbUpper && to == Phase::RooftopArrival) ||
        (from == Phase::RooftopArrival && to == Phase::CarrierTransformation) ||
        (from == Phase::CarrierTransformation && to == Phase::TayamaFireControl) ||
        (from == Phase::TayamaFireControl && to == Phase::TayamaLiftEngines) ||
        (from == Phase::TayamaLiftEngines && to == Phase::TayamaCommandCore) ||
        (from == Phase::TayamaCommandCore && to == Phase::TayamaCollapse) ||
        (from == Phase::TayamaCollapse && to == Phase::CloudSea) ||
        (from == Phase::CloudSea && to == Phase::TayamaDragonBattle) ||
        (from == Phase::TayamaDragonBattle && to == Phase::TayamaDragonCollapse) ||
        (from == Phase::TayamaDragonCollapse && to == Phase::EndingReady);
}

/**
 * @brief Stage 5の操作を停止するムービー区間か判定する
 * @param phase 判定するStage 5状態
 * @return 操作を停止するムービー区間の場合true
 */
constexpr bool IsCinematicPhase(Phase phase) {
    return phase == Phase::EastsourceFall ||
        phase == Phase::WallClimbTransition ||
        phase == Phase::RooftopArrival ||
        phase == Phase::CarrierTransformation ||
        phase == Phase::TayamaCollapse ||
        phase == Phase::CloudSea ||
        phase == Phase::TayamaDragonCollapse;
}

/**
 * @brief NEO AIZU上空を進む第2部道中か判定する
 * @param phase 判定するStage 5状態
 * @return 第2部道中の場合true
 */
constexpr bool IsPart2RoutePhase(Phase phase) {
    return phase >= Phase::WallClimbLower && phase <= Phase::WallClimbUpper;
}

/**
 * @brief 第2部道中クリア後の自機上昇演出中か判定する
 * @param phase 現在のStage 5状態
 * @param phaseTimer 現在状態の経過フレーム数
 * @return 自機上昇演出中の場合true
 */
constexpr bool IsPart2PlayerFlyingAway(Phase phase, int phaseTimer) {
    return phase == Phase::WallClimbUpper &&
        phaseTimer >= WallClimbUpperFrames - WallClimbExitFadeFrames -
            Part2PlayerFlyAwayFrames;
}

/**
 * @brief 第2部道中終了時に敵を画面下へ退避させたY座標を取得する
 * @param y 現在のY座標
 * @param railMode 3Dレール表示の場合true
 * @return 1フレーム退避後のY座標
 */
constexpr float Part2EnemyExitY(float y, bool railMode) {
    return y - (railMode ? Part2RailEnemyExitSpeed : Part2SideEnemyExitSpeed);
}

/**
 * @brief 超巨大ビル屋上を舞台にする状態か判定する
 * @param phase 現在のStage 5状態
 * @return 屋上背景を描画する場合true
 */
constexpr bool IsRooftopPhase(Phase phase) {
    return phase >= Phase::RooftopArrival && phase <= Phase::TayamaCollapse;
}

/**
 * @brief 終幕ムービー後の雲海待機状態か判定する
 * @param phase 現在のStage 5状態
 * @return 雲海を描画する場合true
 */
constexpr bool IsCloudSeaPhase(Phase phase) {
    return phase >= Phase::CloudSea && phase <= Phase::EndingReady;
}

/**
 * @brief 雲海のTAYAMA龍第2形態が戦闘中か判定する
 * @param phase 現在のStage 5状態
 * @return 第2形態を操作可能な場合true
 */
constexpr bool IsTayamaDragonBattlePhase(Phase phase) {
    return phase == Phase::TayamaDragonBattle;
}

/**
 * @brief TAYAMAとの操作可能な最終戦か判定する
 * @param phase 現在のStage 5状態
 * @return TAYAMA戦闘中の場合true
 */
constexpr bool IsTayamaBattlePhase(Phase phase) {
    return phase >= Phase::TayamaFireControl && phase <= Phase::TayamaCommandCore;
}

/**
 * @brief TAYAMA攻略開始時に自機位置を戦闘開始地点へ戻すか判定する
 * @param phase 開始する攻略状態
 * @param restarting チェックポイントから再開する場合true
 * @return 初回開始または再開時に戻す場合true
 */
constexpr bool ShouldResetTayamaPlayer(Phase phase, bool restarting) {
    return phase == Phase::TayamaFireControl || restarting;
}

static_assert(ShouldResetTayamaPlayer(Phase::TayamaFireControl, false));
static_assert(ShouldResetTayamaPlayer(Phase::TayamaLiftEngines, true));
static_assert(ShouldResetTayamaPlayer(Phase::TayamaCommandCore, true));
static_assert(!ShouldResetTayamaPlayer(Phase::TayamaLiftEngines, false));

/**
 * @brief ボス弾の発射元グループに対する接触除外を更新する
 * @param ignoreMask 弾がまだ外へ出ていない発射元グループのビット列
 * @param groupBit 判定対象グループのビット
 * @param insideGroup 現在の弾位置がグループ内の場合true
 * @return このフレームで対象グループとの接触を判定する場合true
 */
constexpr bool CanBossShotHitGroup(std::uint16_t& ignoreMask,
    std::uint16_t groupBit, bool insideGroup) {
    if ((ignoreMask & groupBit) == 0) return true;
    if (!insideGroup) ignoreMask &= static_cast<std::uint16_t>(~groupBit);
    return false;
}

/**
 * @brief 第2部道中フェーズに対応するチャプター番号を取得する
 * @param phase 現在のStage 5状態
 * @return 第2部道中は1から3、道中外は0
 */
constexpr int Part2ChapterNumber(Phase phase) {
    if (phase == Phase::WallClimbLower) return 1;
    if (phase == Phase::WallClimbMiddle) return 2;
    if (phase == Phase::WallClimbUpper) return 3;
    return 0;
}

/**
 * @brief 第2部道中開始からの連続経過フレーム数を取得する
 * @param phase 現在のStage 5状態
 * @param phaseTimer 現在状態の経過フレーム数
 * @return 第2部道中外は0、道中内は区画をまたいだ経過フレーム数
 */
constexpr int Part2RouteElapsedFrames(Phase phase, int phaseTimer) {
    if (phase == Phase::WallClimbLower) return phaseTimer < 0 ? 0 : phaseTimer;
    if (phase == Phase::WallClimbMiddle) {
        return WallClimbLowerFrames + (phaseTimer < 0 ? 0 : phaseTimer);
    }
    if (phase == Phase::WallClimbUpper) {
        return WallClimbLowerFrames + WallClimbMiddleFrames +
            (phaseTimer < 0 ? 0 : phaseTimer);
    }
    return 0;
}

/**
 * @brief 第2部頭上敵の降下率を保ったまま視点別Y座標へ変換する
 * @param y 変換元のY座標
 * @param fromEntryY 変換元の出現Y座標
 * @param fromExitY 変換元の消滅Y座標
 * @param toEntryY 変換先の出現Y座標
 * @param toExitY 変換先の消滅Y座標
 * @return 変換先のY座標
 */
constexpr float RemapPart2EnemyY(float y, float fromEntryY, float fromExitY,
    float toEntryY, float toExitY) {
    const float range = fromEntryY - fromExitY;
    if (range <= 0.0f) return toEntryY;
    const float progress = (fromEntryY - y) / range;
    const float clampedProgress = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    return toEntryY + (toExitY - toEntryY) * clampedProgress;
}

/**
 * @brief 第2部壁面ドローンの巡回基準Yを視点別座標へ変換する
 * @param y 変換元の巡回基準Y
 * @param fromBaseY 変換元の最下段Y
 * @param fromStep 変換元の段間隔
 * @param toBaseY 変換先の最下段Y
 * @param toStep 変換先の段間隔
 * @return 変換先の巡回基準Y
 */
constexpr float RemapPart2DroneBaseY(float y, float fromBaseY, float fromStep,
    float toBaseY, float toStep) {
    return fromStep <= 0.0f ? toBaseY : toBaseY + (y - fromBaseY) / fromStep * toStep;
}

/**
 * @brief 第2部の視点補間率から敵モデル倍率を取得する
 * @param railWeight 3D視点の補間率
 * @return 2Dでは1、3DではPart2RailEnemyScale
 */
constexpr float Part2EnemyScaleMultiplier(float railWeight) {
    return 1.0f + (Part2RailEnemyScale - 1.0f) * railWeight;
}

/**
 * @brief 第2部3Dの壁面ドローン走査波を画面内Y座標へ変換する
 * @param wave -1から1の走査波
 * @return 自機移動範囲内の照準Y座標
 */
constexpr float Part2RailDroneAimY(float wave) {
    constexpr float Margin = 0.35f;
    constexpr float Minimum = Part2RailPlayerMinY + Margin;
    constexpr float Maximum = Part2RailPlayerMaxY - Margin;
    return (Minimum + Maximum) * 0.5f + wave * (Maximum - Minimum) * 0.5f;
}

/**
 * @brief 第2部3Dの壁面ドローン登場補間率を取得する
 * @param age 生成後の経過フレーム数
 * @return 生成直後を0、壁面巡回開始時を1とする補間率
 */
constexpr float Part2RailDroneEntryProgress(int age) {
    const float progress = static_cast<float>(age) /
        static_cast<float>(Part2RailDroneEntryFrames);
    const float clamped = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

/**
 * @brief 第2部壁面ドローンの配置区画を重複なしの巡回順で取得する
 * @param waveIndex ドローンウェーブ番号
 * @param phaseSeed 壁面区画番号
 * @return 左右列と上下段を直列化した配置区画番号
 */
constexpr int Part2DronePlacementIndex(int waveIndex, int phaseSeed) {
    constexpr int PlacementCount = Part2DroneColumnCount * Part2DroneRowCount;
    const int index = waveIndex * 11 + phaseSeed * 7;
    return (index % PlacementCount + PlacementCount) % PlacementCount;
}

/**
 * @brief 第2部3D壁面ドローンの巡回基準Xを取得する
 * @param placementIndex 配置区画番号
 * @return 壁面中央を0とする巡回基準X
 */
constexpr float Part2RailDroneBaseX(int placementIndex) {
    constexpr float ColumnStep = 0.34f;
    const int column = placementIndex % Part2DroneColumnCount;
    return static_cast<float>(column - Part2DroneColumnCount / 2) * ColumnStep;
}

static_assert(IsPart2RoutePhase(Phase::WallClimbLower));
static_assert(IsPart2RoutePhase(Phase::WallClimbMiddle));
static_assert(IsPart2RoutePhase(Phase::WallClimbUpper));
static_assert(!IsPart2RoutePhase(Phase::WallClimbTransition));
static_assert(!IsPart2RoutePhase(Phase::RooftopArrival));
static_assert(!IsPart2PlayerFlyingAway(Phase::WallClimbUpper,
    WallClimbUpperFrames - WallClimbExitFadeFrames - Part2PlayerFlyAwayFrames - 1));
static_assert(IsPart2PlayerFlyingAway(Phase::WallClimbUpper,
    WallClimbUpperFrames - WallClimbExitFadeFrames - Part2PlayerFlyAwayFrames));
static_assert(Part2EnemyExitY(10.0f, false) < 10.0f);
static_assert(Part2EnemyExitY(10.0f, true) <
    Part2EnemyExitY(10.0f, false));
static_assert(Part2ChapterNumber(Phase::WallClimbLower) == 1);
static_assert(Part2ChapterNumber(Phase::WallClimbMiddle) == 2);
static_assert(Part2ChapterNumber(Phase::WallClimbUpper) == 3);
static_assert(Part2ChapterNumber(Phase::Approach) == 0);
static_assert(Part2RouteElapsedFrames(Phase::WallClimbLower, 0) == 0);
static_assert(Part2RouteElapsedFrames(Phase::WallClimbMiddle, 0) == WallClimbLowerFrames);
static_assert(Part2RouteElapsedFrames(Phase::WallClimbUpper, 0) ==
    WallClimbLowerFrames + WallClimbMiddleFrames);
static_assert(Part2RailEnemyEntryY > 60.0f);
static_assert(Part2SideEnemyEntryY > 2.0f);
static_assert(Part2RailEnemyFallSpeed > 0.0f);
static_assert(Part2RailShotMinY < Part2RailPlayerMinY);
static_assert(Part2RailShotMaxY > Part2RailPlayerMaxY);
static_assert(Part2RailPlayerMinY < Part2RailPlayerMaxY);
static_assert(Part2EnemyScaleMultiplier(0.0f) == 1.0f);
static_assert(Part2EnemyScaleMultiplier(1.0f) == Part2RailEnemyScale);
static_assert(Part2RailDroneAimY(-1.0f) > Part2RailPlayerMinY);
static_assert(Part2RailDroneAimY(1.0f) < Part2RailPlayerMaxY);
static_assert(Part2RailDroneEntryY >
    Part2RailDroneBaseY + Part2RailDroneBaseStep * (Part2DroneRowCount - 1));
static_assert(Part2RailDroneEntryProgress(0) == 0.0f);
static_assert(Part2RailDroneEntryProgress(Part2RailDroneEntryFrames / 2) == 0.5f);
static_assert(Part2RailDroneEntryProgress(Part2RailDroneEntryFrames) == 1.0f);
static_assert(Part2DronePlacementIndex(0, 0) != Part2DronePlacementIndex(1, 0));
static_assert(Part2DronePlacementIndex(0, 0) == Part2DronePlacementIndex(30, 0));
static_assert(Part2RailDroneBaseX(0) < 0.0f);
static_assert(Part2RailDroneBaseX(Part2DroneColumnCount - 1) > 0.0f);
static_assert(Part2SideSceneryFallSpeed > 0.0f);
static_assert(RemapPart2EnemyY(2.35f, 2.35f, -1.87f, 68.0f, -5.0f) == 68.0f);
static_assert(RemapPart2EnemyY(-1.87f, 2.35f, -1.87f, 68.0f, -5.0f) == -5.0f);
static_assert(RemapPart2DroneBaseY(Part2SideDroneBaseY + Part2SideDroneBaseStep,
    Part2SideDroneBaseY, Part2SideDroneBaseStep,
    Part2RailDroneBaseY, Part2RailDroneBaseStep) ==
    Part2RailDroneBaseY + Part2RailDroneBaseStep);

/**
 * @brief EASTSOURCE撃破後に自機が画面奥へ飛ぶ進行率を取得する
 * @param phaseTimer EASTSOURCE墜落状態の経過フレーム数
 * @return 撃破直後を0、暗転開始時を1とする進行率
 */
constexpr float EastsourceFlyAwayProgress(int phaseTimer) {
    constexpr int FlyAwayFrames = EastsourceFallFrames - WallClimbFadeFrames;
    const float progress = static_cast<float>(phaseTimer) /
        static_cast<float>(FlyAwayFrames);
    return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

/**
 * @brief 暗転解除後に道路を進むムービーの進行率を取得する
 * @param phaseTimer 外壁上昇状態の経過フレーム数
 * @return 暗転解除時を0、ビル前到達時を1とする進行率
 */
constexpr float WallApproachProgress(int phaseTimer) {
    constexpr int ApproachFrames = WallClimbApproachFrames - WallClimbFadeFrames;
    const float progress = static_cast<float>(phaseTimer - WallClimbFadeFrames) /
        static_cast<float>(ApproachFrames);
    return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

/**
 * @brief 外壁上昇ムービーの上昇進行率を取得する
 * @param phaseTimer 外壁上昇状態の経過フレーム数
 * @return 接近完了を0、屋上到達を1とする進行率
 */
constexpr float WallClimbProgress(int phaseTimer) {
    constexpr int ClimbFrames = WallClimbTransitionFrames -
        WallClimbApproachFrames - WallClimbExitFadeFrames;
    const float progress = static_cast<float>(phaseTimer - WallClimbApproachFrames) /
        static_cast<float>(ClimbFrames);
    return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

/**
 * @brief 第2部の敵ウェーブ用に再現可能な疑似乱数を生成する
 * @param waveIndex ウェーブ番号
 * @param phaseSeed 壁面区画番号
 * @return 敵種、出現位置、奥行きの選択に使用する32bit値
 */
constexpr std::uint32_t WallWaveHash(int waveIndex, int phaseSeed) {
    std::uint32_t value = static_cast<std::uint32_t>(waveIndex + 1) * 0x9E3779B9u +
        static_cast<std::uint32_t>(phaseSeed + 1) * 0x85EBCA6Bu;
    value ^= value >> 16;
    value *= 0x7FEB352Du;
    value ^= value >> 15;
    return value;
}

/**
 * @brief Stage 5ムービーの暗転率を取得する
 * @param phase 現在のStage 5状態
 * @param phaseTimer 現在状態の経過フレーム数
 * @return 透明を0、完全な暗転を1とする不透明度
 */
constexpr float CinematicFadeAlpha(Phase phase, int phaseTimer) {
    if (phase == Phase::EastsourceFall) {
        const float progress = static_cast<float>(phaseTimer -
            (EastsourceFallFrames - WallClimbFadeFrames)) /
            static_cast<float>(WallClimbFadeFrames);
        return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    }
    if (phase == Phase::WallClimbTransition) {
        if (phaseTimer < WallClimbFadeFrames) {
            return 1.0f - static_cast<float>(phaseTimer) /
                static_cast<float>(WallClimbFadeFrames);
        }
        const float progress = static_cast<float>(phaseTimer -
            (WallClimbTransitionFrames - WallClimbExitFadeFrames)) /
            static_cast<float>(WallClimbExitFadeFrames);
        return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    }
    if (phase == Phase::RooftopArrival && phaseTimer < WallClimbFadeFrames) {
        return 1.0f - static_cast<float>(phaseTimer) /
            static_cast<float>(WallClimbFadeFrames);
    }
    if (phase == Phase::WallClimbUpper) {
        const float progress = static_cast<float>(phaseTimer -
            (WallClimbUpperFrames - WallClimbExitFadeFrames)) /
            static_cast<float>(WallClimbExitFadeFrames);
        return progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
    }
    if (phase == Phase::WallClimbLower && phaseTimer < WallClimbFadeFrames) {
        return 1.0f - static_cast<float>(phaseTimer) /
            static_cast<float>(WallClimbFadeFrames);
    }
    if (phase == Phase::TayamaCollapse) {
        return FinalEscapeProgress(phaseTimer, FinalEscapeFadeStartFrames,
            CloudSeaFadeFrames);
    }
    if (phase == Phase::CloudSea && phaseTimer < CloudSeaFadeFrames) {
        return 1.0f - static_cast<float>(phaseTimer) /
            static_cast<float>(CloudSeaFadeFrames);
    }
    return 0.0f;
}

/**
 * @brief 指定弱点が現在フェーズで有効か判定する
 * @param weakpoint 判定する弱点
 * @param phase 現在のStage 5状態
 * @return ダメージを受ける場合true、無効な弱点の場合false
 */
constexpr bool IsWeakpointActiveForPhase(TayamaWeakpoint weakpoint, Phase phase) {
    return weakpoint < TayamaWeakpoint::Count && IsTayamaBattlePhase(phase);
}

}
