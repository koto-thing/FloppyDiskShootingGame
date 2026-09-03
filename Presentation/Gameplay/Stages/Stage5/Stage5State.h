#pragma once

#include <array>
#include <cstdint>

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
    float lockedX = 0.0f;
    float lockedY = 0.0f;
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
inline constexpr int EastsourceMaxHp = 1200;
inline constexpr int EastsourceNoseHp = 180;
inline constexpr int EastsourceWingHp = 240;
inline constexpr int EastsourceEngineHp = 210;
inline constexpr int EastsourceIntroFrames = 210;
inline constexpr int EastsourceFallFrames = 180;
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
inline constexpr float TayamaBossScale = 1.18f;
inline constexpr float WallClimbHeight = 360.0f;
inline constexpr float Part2SideEnemyEntryY = 2.35f;
inline constexpr float Part2RailEnemyEntryY = 68.0f;
inline constexpr float Part2RailEnemyEntryStep = 0.45f;
inline constexpr float Part2RailEnemyFallSpeed = 0.32f;
inline constexpr float Part2RailEnemyPlaneZ = 35.0f;
inline constexpr float Part2RailEnemyExitY = -5.0f;
inline constexpr float Part2SideSceneryFallSpeed = 0.64f;
inline constexpr float Part2RailSceneryFallSpeed = 0.96f;
inline constexpr float Part2SideItemFallSpeed = 0.014f;
inline constexpr float Part2RailItemFallSpeed = 0.06f;
inline constexpr int WallClimbLowerFrames = 420;
inline constexpr int WallClimbMiddleFrames = 480;
inline constexpr int WallClimbUpperFrames = 540;
inline constexpr int RooftopArrivalFrames = 240;
inline constexpr int CarrierTransformationFrames = 180;
inline constexpr int TayamaCollapseFrames = 540;
inline constexpr int QuietFlightFrames = 60;
inline constexpr int SearchlightLockFrames = 45;
inline constexpr int SearchlightWarningFrames = 24;
inline constexpr int SearchlightVolleyCount = 3;
inline constexpr int SearchlightVolleyIntervalFrames = 10;
inline constexpr float SearchlightDetectionRadius = 0.27f;
inline constexpr int DroneMachineGunBurstFrames = 45;
inline constexpr int DroneMachineGunShotIntervalFrames = 5;
inline constexpr int DroneMachineGunCooldownFrames = 75;
inline constexpr float DroneSearchlightDetectionRadius = 0.12f;

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
    float checkpointPower = 0.0f;
    float coreTargetX = 0.0f;
    float coreTargetY = 0.0f;
    int phaseTimer = 0;
    int checkpointScore = 0;
    int checkpointKills = 0;
    int soundCooldown = 0;
    int attackTimer = 0;
    int guardSpawnCooldown = 0;
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
        (from == Phase::TayamaCollapse && to == Phase::EndingReady);
}

/**
 * @brief EASTSOURCE撃破後のムービー区間か判定する
 * @param phase 判定するStage 5状態
 * @return 操作を停止するムービー区間の場合true
 */
constexpr bool IsCinematicPhase(Phase phase) {
    return phase == Phase::EastsourceFall ||
        phase == Phase::WallClimbTransition ||
        phase == Phase::RooftopArrival ||
        phase == Phase::CarrierTransformation;
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
 * @brief 超巨大ビル屋上を舞台にする状態か判定する
 * @param phase 現在のStage 5状態
 * @return 屋上背景を描画する場合true
 */
constexpr bool IsRooftopPhase(Phase phase) {
    return phase >= Phase::RooftopArrival && phase <= Phase::EndingReady;
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

static_assert(IsPart2RoutePhase(Phase::WallClimbLower));
static_assert(IsPart2RoutePhase(Phase::WallClimbMiddle));
static_assert(IsPart2RoutePhase(Phase::WallClimbUpper));
static_assert(!IsPart2RoutePhase(Phase::WallClimbTransition));
static_assert(!IsPart2RoutePhase(Phase::RooftopArrival));
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
static_assert(Part2SideSceneryFallSpeed > 0.0f);

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
 * @brief EASTSOURCE撃破後ムービーの暗転率を取得する
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
