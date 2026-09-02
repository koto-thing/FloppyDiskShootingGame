#pragma once

#include <array>

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
inline constexpr int WallClimbTransitionFrames = 120;
inline constexpr int WallClimbLowerFrames = 420;
inline constexpr int WallClimbMiddleFrames = 480;
inline constexpr int WallClimbUpperFrames = 540;
inline constexpr int RooftopArrivalFrames = 180;
inline constexpr int CarrierTransformationFrames = 120;
inline constexpr int TayamaCollapseFrames = 540;
inline constexpr int QuietFlightFrames = 60;
inline constexpr int SearchlightLockFrames = 45;
inline constexpr int SearchlightWarningFrames = 24;
inline constexpr int SearchlightVolleyCount = 3;
inline constexpr int SearchlightVolleyIntervalFrames = 10;
inline constexpr float SearchlightDetectionRadius = 0.27f;

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
