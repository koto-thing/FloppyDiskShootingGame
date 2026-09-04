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
    TayamaCommandCore
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
inline constexpr float TayamaOrbitRadius = 115.0f;
inline constexpr float TayamaOrbitSpeed = 0.018f;
inline constexpr float TayamaCameraDistance = 18.0f;
inline constexpr float TayamaCameraHeight = 5.0f;
inline constexpr float TayamaCameraLookAhead = 32.0f;
inline constexpr float TayamaOverheadDistanceScale = 0.9f;
inline constexpr float TayamaFrontDistanceScale = 1.4f;
inline constexpr float TayamaPlayerMinY = -0.70f;
inline constexpr float TayamaPlayerMaxY = 55.0f;
inline constexpr float WallClimbHeight = 360.0f;
inline constexpr float Part2SideEnemyEntryY = 2.35f;
inline constexpr float Part2RailEnemyEntryY = 68.0f;
inline constexpr float Part2RailEnemyEntryStep = 0.45f;
inline constexpr float Part2RailEnemyFallSpeed = 0.16f;
inline constexpr float Part2RailEnemyPlaneZ = 35.0f;
inline constexpr float Part2RailEnemyExitY = -5.0f;
inline constexpr float Part2RailShotMinY = Part2RailEnemyExitY;
inline constexpr float Part2RailShotMaxY = Part2RailEnemyEntryY + Part2RailEnemyEntryStep * 2.0f;
inline constexpr float Part2RailPlayerMinY = 0.80f;
inline constexpr float Part2RailPlayerMaxY = 16.0f;
inline constexpr float Part2RailEnemyScale = 2.0f;
inline constexpr float Part2SideDroneBaseY = 0.18f;
inline constexpr float Part2SideDroneBaseStep = 0.18f;
inline constexpr float Part2RailDroneBaseY = 9.0f;
inline constexpr float Part2RailDroneBaseStep = 1.20f;
inline constexpr float Part2RailDroneEntryY = Part2RailPlayerMaxY + 2.0f;
inline constexpr int Part2RailDroneEntryFrames = 120;
inline constexpr float Part2SideSceneryFallSpeed = 0.64f;
inline constexpr float Part2RailSceneryFallSpeed = 0.96f;
inline constexpr float Part2SideItemFallSpeed = 0.014f;
inline constexpr float Part2RailItemFallSpeed = 0.06f;
inline constexpr int WallClimbLowerFrames = 600;
inline constexpr int WallClimbMiddleFrames = 600;
inline constexpr int WallClimbUpperFrames = 600;
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
inline constexpr int TayamaDragonMaxHp = 4000;
inline constexpr int TayamaDragonSegmentCount = 26;
inline constexpr float TayamaDragonShotFarZ = 108.0f;
inline constexpr int TayamaDragonBarrageIntervalFrames = 90;
inline constexpr int TayamaDragonSweepCycleFrames = 300;
inline constexpr int TayamaDragonSweepWarningFrames = 45;
inline constexpr int TayamaDragonSweepActiveFrames = 105;
inline constexpr int TayamaDragonRushCycleFrames = 480;
inline constexpr int TayamaDragonRushStartFrame = 150;
inline constexpr int TayamaDragonRushWarningFrames = 60;
inline constexpr int TayamaDragonRushActiveFrames = 30;
inline constexpr int TayamaDragonRushRecoveryFrames = 75;
inline constexpr float TayamaDragonRushWindupRate = 0.12f;
inline constexpr float TayamaDragonRushSideDistance = 1.35f;
inline constexpr float TayamaDragonRushRailDistance = 48.0f;
inline constexpr int TayamaDragonCollapseSegmentIntervalFrames = 9;
inline constexpr int TayamaDragonCollapseHeadExplosionFrame = 270;
inline constexpr int TayamaDragonCollapseFrames = 360;
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
inline constexpr float TayamaHeadLaserFrontDot = 0.92f;
inline constexpr float TayamaHeadLaserLength = 200.0f;
inline constexpr float TayamaHeadLaserHitRadius = 3.2f;

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
 * @brief TAYAMA龍突進の移動率を取得する
 * @param attackTimer 第2形態開始からの攻撃タイマー
 * @return 通常位置を0、突進先を1とする移動率
 */
constexpr float TayamaDragonRushProgress(int attackTimer) {
    const int frame = TayamaDragonRushFrame(attackTimer);
    if (frame < TayamaDragonRushWarningFrames) {
        return -TayamaDragonRushWindupRate * static_cast<float>(frame) /
            static_cast<float>(TayamaDragonRushWarningFrames);
    }
    const int activeEnd = TayamaDragonRushWarningFrames +
        TayamaDragonRushActiveFrames;
    if (frame < activeEnd) {
        const float charge = static_cast<float>(frame - TayamaDragonRushWarningFrames) /
            static_cast<float>(TayamaDragonRushActiveFrames - 1);
        return -TayamaDragonRushWindupRate +
            (1.0f + TayamaDragonRushWindupRate) * charge;
    }
    const int recoveryEnd = activeEnd + TayamaDragonRushRecoveryFrames;
    if (frame < recoveryEnd) {
        return 1.0f - static_cast<float>(frame - activeEnd) /
            static_cast<float>(TayamaDragonRushRecoveryFrames);
    }
    return 0.0f;
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
 * @return 腕へ適用するZ軸回転角度
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
static_assert(TayamaDragonRushProgress(TayamaDragonRushStartFrame +
    TayamaDragonRushWarningFrames + TayamaDragonRushActiveFrames) == 1.0f);

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
    int tayamaHp = 0;
    int tayamaMaxHp = 0;
    int tayamaDragonHitFlashFrames = 0;
    bool headLaserArmed = false;
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

static_assert(IsPart2RoutePhase(Phase::WallClimbLower));
static_assert(IsPart2RoutePhase(Phase::WallClimbMiddle));
static_assert(IsPart2RoutePhase(Phase::WallClimbUpper));
static_assert(!IsPart2RoutePhase(Phase::WallClimbTransition));
static_assert(!IsPart2RoutePhase(Phase::RooftopArrival));
static_assert(!IsPart2PlayerFlyingAway(Phase::WallClimbUpper,
    WallClimbUpperFrames - WallClimbExitFadeFrames - Part2PlayerFlyAwayFrames - 1));
static_assert(IsPart2PlayerFlyingAway(Phase::WallClimbUpper,
    WallClimbUpperFrames - WallClimbExitFadeFrames - Part2PlayerFlyAwayFrames));
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
static_assert(Part2RailDroneEntryY > Part2RailDroneBaseY + Part2RailDroneBaseStep * 2.0f);
static_assert(Part2RailDroneEntryProgress(0) == 0.0f);
static_assert(Part2RailDroneEntryProgress(Part2RailDroneEntryFrames / 2) == 0.5f);
static_assert(Part2RailDroneEntryProgress(Part2RailDroneEntryFrames) == 1.0f);
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
    if (phase == Phase::TayamaFireControl) {
        return weakpoint == TayamaWeakpoint::LeftSearchlight ||
            weakpoint == TayamaWeakpoint::RightSearchlight ||
            weakpoint == TayamaWeakpoint::FireControlRadar;
    }
    if (phase == Phase::TayamaLiftEngines) {
        return weakpoint == TayamaWeakpoint::LeftLiftEngine ||
            weakpoint == TayamaWeakpoint::RightLiftEngine;
    }
    return phase == Phase::TayamaCommandCore && weakpoint == TayamaWeakpoint::CommandCore;
}

}
