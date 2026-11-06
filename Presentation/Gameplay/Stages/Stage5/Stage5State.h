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

/** @brief ビル内部の落下物の種類 */
enum class ShaftObstacleKind {
    WallJut,
    Debris,
    Beam,
    Bulkhead
};

/** @brief ビル内部の配置パターン */
enum class ShaftPattern {
    LeftJut,
    RightJut,
    CeilingJut,
    FloorJut,
    Debris,
    Count,
    Beam,
    Bulkhead
};

/** @brief ビル内部を落下する箱型障害物 */
struct ShaftObstacle {
    float x = 0.0f;
    float y = 0.0f;
    float halfWidth = 0.0f;
    float halfHeight = 0.0f;
    float travel = 0.0f;
    int hp = 0;
    int hitFlashFrames = 0;
    ShaftObstacleKind kind = ShaftObstacleKind::WallJut;
    bool active = false;
};

inline constexpr int ShaftObstacleCount = 40;
inline constexpr int ShaftPatternMaxPieces = 4;
inline constexpr int ShaftDebrisHp = 45;
inline constexpr float ShaftFarZ = 68.0f;
inline constexpr float ShaftNearZ = -6.0f;
inline constexpr float ShaftTopSideY = 1.95f;
inline constexpr float ShaftBottomSideY = -2.05f;
inline constexpr float ShaftObstacleHalfDepth = 2.0f;
inline constexpr float ShaftObstacleSideHalfThickness = 0.16f;
inline constexpr float ShaftHalfWidth = 2.2f;
inline constexpr float ShaftHalfHeight = 1.75f;
inline constexpr float ShaftBulkheadSlitHalfHeight = 0.05f;
inline constexpr int ShaftGateFirstFrame = 360;
inline constexpr int ShaftGateIntervalFrames = 600;
inline constexpr int ShaftGateClearFrames = 54;
inline constexpr int ShaftSpawnStopFrames = 230;
inline constexpr float ShaftObstacleRevealTravel = 0.22f;

/**
 * @brief 落下進行率から3D視点の奥行きを取得する
 * @param travel 出現を0、通過完了を1とする進行率
 * @return ワールドZ座標
 */
constexpr float ShaftObstacleZ(float travel) {
    return ShaftFarZ + (ShaftNearZ - ShaftFarZ) * travel;
}

/**
 * @brief 落下進行率から2D視点の画面Yを取得する
 * @param travel 出現を0、通過完了を1とする進行率
 * @return ゲーム座標のY
 */
constexpr float ShaftObstacleSideY(float travel) {
    return ShaftTopSideY + (ShaftBottomSideY - ShaftTopSideY) * travel;
}

/**
 * @brief 落下物の出現ディザに渡すアルファを取得する
 * @param travel 出現を0、通過完了を1とする進行率
 * @return 出現中は不透明度を負にしたディザ指定、出現後は通常の不透明を表す1
 */
constexpr float ShaftObstacleRevealAlpha(float travel) {
    const float reveal = travel / ShaftObstacleRevealTravel;
    return reveal >= 1.0f ? 1.0f : -(reveal < 0.01f ? 0.01f : reveal);
}

/**
 * @brief 障害物が2D視点の断面に含まれるか判定する
 * @param obstacle 判定する障害物
 * @return 2D視点で実体を持つ場合true
 */
constexpr bool ShaftObstacleInSideSlice(const ShaftObstacle& obstacle) {
    const float distance = obstacle.y < 0.0f ? -obstacle.y : obstacle.y;
    return distance < obstacle.halfHeight;
}

/**
 * @brief 円形の判定が障害物へ接触するか判定する
 * @param obstacle 判定する障害物
 * @param rail 3D視点の場合true
 * @param x 判定中心のゲーム座標X
 * @param y 判定中心のゲーム座標Y
 * @param z 判定中心のワールドZ、2D視点では使用しない
 * @param radius 判定半径
 * @return 接触する場合true
 */
constexpr bool ShaftObstacleHits(const ShaftObstacle& obstacle, bool rail,
    float x, float y, float z, float radius) {
    const auto within = [](float delta, float limit) {
        return delta < limit && -delta < limit;
    };
    if (!obstacle.active || !within(x - obstacle.x, obstacle.halfWidth + radius)) return false;
    if (rail) {
        return within(y - obstacle.y, obstacle.halfHeight + radius) &&
            within(z - ShaftObstacleZ(obstacle.travel), ShaftObstacleHalfDepth);
    }
    // 2D視点は高さ0の断面を見るため、断面に掛からない部品は判定を持たない
    return ShaftObstacleInSideSlice(obstacle) &&
        within(y - ShaftObstacleSideY(obstacle.travel),
            ShaftObstacleSideHalfThickness + radius);
}

/**
 * @brief 関門の配置パターンを取得する
 * @param tier 階層番号、0が下層で2が上層
 * @param elapsed 暗転解除後の経過フレーム数
 * @return 関門を出す時刻ならBeamまたはBulkhead、それ以外はCount
 */
constexpr ShaftPattern ShaftGatePatternAt(int tier, int elapsed) {
    const int offset = elapsed - ShaftGateFirstFrame;
    if (offset < 0 || offset % ShaftGateIntervalFrames != 0) return ShaftPattern::Count;
    const int index = offset / ShaftGateIntervalFrames;
    if (index > tier + 1) return ShaftPattern::Count;
    return (index + tier) % 2 == 0 ? ShaftPattern::Beam : ShaftPattern::Bulkhead;
}

/**
 * @brief 階層ごとの落下物の進行速度を取得する
 * @param tier 階層番号、0が下層で2が上層
 * @return 1フレームあたりの進行率
 */
constexpr float ShaftTravelPerFrame(int tier) {
    return tier == 0 ? 1.0f / 180.0f : (tier == 1 ? 1.0f / 160.0f : 1.0f / 140.0f);
}

/**
 * @brief 階層と難易度に応じた通常障害物の生成間隔を取得する
 * @param tier 階層番号、0が下層で2が上層
 * @param difficulty 難易度番号、0がEasyで2がHard
 * @return 生成間隔のフレーム数
 */
constexpr int ShaftObstacleInterval(int tier, int difficulty) {
    return 84 - tier * 12 + (difficulty == 0 ? 24 : (difficulty == 2 ? -12 : 0));
}

/**
 * @brief 配置パターンを箱型障害物へ展開する
 * @param pattern 配置パターン
 * @param hash 配置を決める疑似乱数
 * @param out 展開先
 * @return 展開した部品数
 */
constexpr int BuildShaftPattern(ShaftPattern pattern, std::uint32_t hash,
    std::array<ShaftObstacle, ShaftPatternMaxPieces>& out) {
    constexpr float Edge = ShaftHalfWidth + 0.2f;
    constexpr float Depth = ShaftHalfHeight + 0.1f;
    const float side = (hash & 1u) != 0u ? 1.0f : -1.0f;
    const float lane = (static_cast<float>((hash >> 4) % 15u) - 7.0f) * 0.2f;
    const auto piece = [&](int index, float x, float y, float halfWidth, float halfHeight,
        ShaftObstacleKind kind, int hp) {
        ShaftObstacle obstacle {};
        obstacle.x = x;
        obstacle.y = y;
        obstacle.halfWidth = halfWidth;
        obstacle.halfHeight = halfHeight;
        obstacle.kind = kind;
        obstacle.hp = hp;
        obstacle.active = true;
        out[static_cast<std::size_t>(index)] = obstacle;
    };
    switch (pattern) {
    case ShaftPattern::LeftJut:
    case ShaftPattern::RightJut: {
        // 壁の出っ張りは2Dでも3Dでも通路が残る幅に抑え、3Dでは上下の片方にも隙間を残す
        const float halfWidth = 0.8f + static_cast<float>((hash >> 8) % 8u) * 0.05f;
        const float wall = pattern == ShaftPattern::LeftJut ? -1.0f : 1.0f;
        piece(0, wall * (Edge - halfWidth), side * 0.5f, halfWidth, 1.35f,
            ShaftObstacleKind::WallJut, 0);
        return 1;
    }
    case ShaftPattern::CeilingJut:
    case ShaftPattern::FloorJut: {
        const float halfWidth = 0.55f + static_cast<float>((hash >> 8) % 6u) * 0.05f;
        const float wall = pattern == ShaftPattern::CeilingJut ? 1.0f : -1.0f;
        piece(0, lane, wall * 0.75f, halfWidth, 1.1f, ShaftObstacleKind::WallJut, 0);
        return 1;
    }
    case ShaftPattern::Debris: {
        const float second = lane + side * (1.1f + static_cast<float>((hash >> 12) % 4u) * 0.2f);
        const float tilt = static_cast<float>((hash >> 16) % 5u) * 0.05f - 0.1f;
        piece(0, lane, tilt, 0.3f, 0.3f, ShaftObstacleKind::Debris, ShaftDebrisHp);
        piece(1, second > Edge || second < -Edge ? -second * 0.5f : second, -tilt, 0.3f, 0.3f,
            ShaftObstacleKind::Debris, ShaftDebrisHp);
        return 2;
    }
    case ShaftPattern::Beam:
        // 全幅の梁は2Dの断面を完全に塞ぎ、3Dでは上下の片方だけ抜けられる
        piece(0, 0.0f, side * 0.85f, Edge, 1.0f, ShaftObstacleKind::Beam, 0);
        return 1;
    case ShaftPattern::Bulkhead: {
        // 隔壁の細い隙間は2Dの断面にだけ開き、3Dの機体幅では通れない
        constexpr float GapHalfWidth = 0.34f;
        constexpr float Slit = ShaftBulkheadSlitHalfHeight;
        const float halfHeight = (Depth - Slit) * 0.5f;
        const float leftWidth = (lane - GapHalfWidth + Edge) * 0.5f;
        const float rightWidth = (Edge - lane - GapHalfWidth) * 0.5f;
        piece(0, -Edge + leftWidth, 0.0f, leftWidth, Depth, ShaftObstacleKind::Bulkhead, 0);
        piece(1, Edge - rightWidth, 0.0f, rightWidth, Depth, ShaftObstacleKind::Bulkhead, 0);
        piece(2, lane, Slit + halfHeight, GapHalfWidth, halfHeight, ShaftObstacleKind::Bulkhead, 0);
        piece(3, lane, -Slit - halfHeight, GapHalfWidth, halfHeight, ShaftObstacleKind::Bulkhead, 0);
        return 4;
    }
    default:
        return 0;
    }
}

static_assert(ShaftObstacleZ(0.0f) == ShaftFarZ);
static_assert(ShaftObstacleRevealAlpha(0.0f) < 0.0f);
static_assert(ShaftObstacleRevealAlpha(ShaftObstacleRevealTravel) == 1.0f);
static_assert(ShaftObstacleZ(1.0f) == ShaftNearZ);
static_assert(ShaftGatePatternAt(0, ShaftGateFirstFrame) == ShaftPattern::Beam);
static_assert(ShaftGatePatternAt(1, ShaftGateFirstFrame) == ShaftPattern::Bulkhead);
static_assert(ShaftGatePatternAt(0, ShaftGateFirstFrame + ShaftGateIntervalFrames) ==
    ShaftPattern::Bulkhead);
static_assert(ShaftGatePatternAt(0, ShaftGateFirstFrame + ShaftGateIntervalFrames * 2) ==
    ShaftPattern::Count);
static_assert(ShaftGatePatternAt(0, ShaftGateFirstFrame + 1) == ShaftPattern::Count);

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
    Vector3 wreckPosition {};
};

inline constexpr int SearchlightCount = 3;
inline constexpr int TayamaWeakpointCount = static_cast<int>(TayamaWeakpoint::Count);
inline constexpr int TayamaMaxHp = 3000;
inline constexpr int TayamaPartBreakDamage = 500;
static_assert(TayamaMaxHp == 3000);
static_assert(TayamaPartBreakDamage * 10 >= TayamaMaxHp / 2);
inline constexpr int EastsourceMaxHp = 1200;
inline constexpr int EastsourceNoseHp = 180;
inline constexpr int EastsourceWingHp = 240;
inline constexpr int EastsourceEngineHp = 210;
inline constexpr int EastsourceIntroFrames = 210;
inline constexpr int EastsourceFallFrames = 180;
inline constexpr int EastsourceNoseAttackCycleFrames = 60;
inline constexpr int EastsourceDamagedNoseAttackCycleFrames = 96;
inline constexpr int EastsourceNoseWarningFrames = 24;
inline constexpr int EastsourceDamagedNoseWarningFrames = 36;
inline constexpr int EastsourceNoseShotCount = 11;
inline constexpr int EastsourceDamagedNoseShotCount = 3;
inline constexpr int EastsourceNoseShotIntervalFrames = 3;
inline constexpr int EastsourceWingBarrageCycleFrames = 42;
inline constexpr int EastsourcePursuitCycleFrames = 120;
inline constexpr int EastsourcePursuitRetreatFrames = 48;
inline constexpr int EastsourcePursuitWarningEndFrame = 66;
inline constexpr int EastsourcePursuitPoweredPassFrames = 24;
inline constexpr int EastsourcePursuitDamagedPassFrames = 34;
inline constexpr int EastsourcePursuitAimFrame = 52;
inline constexpr int EastsourcePursuitShotStartFrame = 72;
inline constexpr int EastsourcePursuitShotEndFrame = 96;
inline constexpr int EastsourceBombStartFrames = 180;
inline constexpr int EastsourceBombIntervalFrames = 600;
inline constexpr int EastsourceMissileHomingFrames = 12;
inline constexpr float EastsourceMissileSpeed = 0.32f;
inline constexpr int EastsourceLaserWarningFrames = 180;
inline constexpr int EastsourceLaserActiveFrames = 90;
inline constexpr float EastsourceLaserRadius = 2.4f;
inline constexpr int EastsourceShieldHp = 180;
inline constexpr float EastsourceShieldRadius = 5.5f;
static_assert(EastsourceNoseWarningFrames +
    (EastsourceNoseShotCount - 1) * EastsourceNoseShotIntervalFrames <
    EastsourceNoseAttackCycleFrames);
static_assert(EastsourcePursuitWarningEndFrame +
    EastsourcePursuitDamagedPassFrames < EastsourcePursuitCycleFrames);
static_assert(EastsourcePursuitAimFrame < EastsourcePursuitShotStartFrame &&
    EastsourcePursuitShotStartFrame < EastsourcePursuitShotEndFrame);
inline constexpr int WallClimbTransitionFrames = 510;
inline constexpr int WallClimbFadeFrames = 60;
inline constexpr int WallClimbApproachFrames = 210;
inline constexpr int WallClimbExitFadeFrames = 45;
inline constexpr float WallApproachStartZ = 8.0f;
inline constexpr float WallApproachEndZ = 32.0f;
inline constexpr int EntranceMissileFireFrame = 225;
inline constexpr int EntranceBlastFrame = 255;
inline constexpr int EntranceBlastBurstFrames = 54;
inline constexpr int EntranceFlyInFrame = 375;
inline constexpr float EntranceDoorWidth = 18.0f;
inline constexpr float EntranceDoorHeight = 5.2f;
inline constexpr float EntranceDoorZ = 41.6f;
static_assert(WallClimbApproachFrames < EntranceMissileFireFrame &&
    EntranceMissileFireFrame < EntranceBlastFrame &&
    EntranceBlastFrame + EntranceBlastBurstFrames < EntranceFlyInFrame &&
    EntranceFlyInFrame < WallClimbTransitionFrames - WallClimbExitFadeFrames);
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
inline constexpr float Part2SideEnemyExitSpeed = 0.10f;
// 第2部2Dの縦スクロールで自機弾が弾プールを占有し続けない速度倍率
inline constexpr float Part2PlayerShotSpeedScale = 4.0f;
inline constexpr float Part2RailPlayerMinY = -1.5f;
inline constexpr float Part2RailPlayerMaxY = 1.5f;
inline constexpr float Part2RailEnemyScale = 2.0f;
inline constexpr int Part2DroneColumnCount = 5;
inline constexpr int Part2DroneRowCount = 6;
inline constexpr float Part2SideDroneBaseY = 0.5f;
inline constexpr float Part2SideDroneBaseStep = 0.125f;
inline constexpr float Part2RailDroneBaseY = -1.0f;
inline constexpr float Part2RailDroneBaseStep = 0.375f;
inline constexpr int Part2RailDroneEntryFrames = 120;
inline constexpr float Part2SideItemFallSpeed = 0.014f;
inline constexpr int WallClimbLowerFrames = 1380;
inline constexpr int WallClimbMiddleFrames = 1980;
inline constexpr int WallClimbUpperFrames = 2640;
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
inline constexpr int TayamaDragonMaxHp = 3000;
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
inline constexpr int TayamaDragonRushActiveFrames = 120;
inline constexpr int TayamaDragonRushRecoveryFrames = 75;
// 全40節が左端またはカメラ後方へ抜けるまで進み、予備動作の後退量を維持する
inline constexpr float TayamaDragonRushWindupRate = 0.03f;
inline constexpr float TayamaDragonRushSideDistance = 5.4f;
inline constexpr float TayamaDragonRushRailDistance = 192.0f;
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
inline constexpr float TayamaDragonSideCenterX = 1.3f;
inline constexpr int TayamaDragonCollapseSegmentIntervalFrames = 10;
inline constexpr int TayamaDragonHeadPartCount = 34;
inline constexpr int TayamaDragonHeadBreakStartFrame = 285;
inline constexpr int TayamaDragonHeadPartIntervalFrames = 8;
inline constexpr int TayamaDragonCollapseHeadExplosionFrame =
    TayamaDragonHeadBreakStartFrame +
    TayamaDragonHeadPartCount * TayamaDragonHeadPartIntervalFrames;
inline constexpr int TayamaDragonFinalChargeFrames = 72;
inline constexpr int TayamaDragonFinalBurstIntervalFrames = 18;
inline constexpr int TayamaDragonFinalBurstCount = 5;
inline constexpr int TayamaDragonFinalAfterglowFrames = 240;
inline constexpr int TayamaDragonCollapseFrames =
    TayamaDragonCollapseHeadExplosionFrame + TayamaDragonFinalAfterglowFrames;
inline constexpr int SearchlightLockFrames = 45;
inline constexpr int SearchlightWarningFrames = 24;
inline constexpr int SearchlightVolleyCount = 3;
inline constexpr int SearchlightVolleyIntervalFrames = 10;
inline constexpr float SearchlightDetectionRadius = 0.27f;
inline constexpr int DroneMachineGunBurstFrames = 45;
inline constexpr int DroneMachineGunShotIntervalFrames = 5;
inline constexpr int DroneMachineGunCooldownFrames = 75;
inline constexpr float DroneSearchlightDetectionRadius = 0.12f;
inline constexpr int TayamaArmSpinWarningFrames = 60;
inline constexpr int TayamaArmSpinActiveFrames = 150;
inline constexpr int TayamaArmSpinRecoveryFrames = 30;
inline constexpr float TayamaArmSpinWindup = 0.35f;
inline constexpr float TayamaArmSpinHitRadius = 6.0f;
inline constexpr int TayamaHangarSpawnIntervalFrames = 240;
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
 * @param frame 腕回転攻撃の開始からの経過フレーム
 * @return 回転中の場合true
 */
constexpr bool IsTayamaArmSpinActive(int frame) {
    return frame >= TayamaArmSpinWarningFrames &&
        frame < TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames;
}

/**
 * @brief TAYAMA腕回転攻撃の胴体軸まわりの角度を取得する
 * @param frame 腕回転攻撃の開始からの経過フレーム
 * @return 予告中は逆方向へ振りかぶり、発動中に一周する角度
 */
constexpr float TayamaArmSpinAngle(int frame) {
    if (frame < TayamaArmSpinWarningFrames) {
        return -TayamaArmSpinWindup * TayamaDragonSmoothProgress(
            static_cast<float>(frame) / static_cast<float>(TayamaArmSpinWarningFrames));
    }
    if (frame >= TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames) return 0.0f;
    return -TayamaArmSpinWindup + (Math::TwoPi + TayamaArmSpinWindup) * TayamaDragonSmoothProgress(
        static_cast<float>(frame - TayamaArmSpinWarningFrames) /
        static_cast<float>(TayamaArmSpinActiveFrames));
}

static_assert(!IsTayamaArmSpinActive(TayamaArmSpinWarningFrames - 1));
static_assert(IsTayamaArmSpinActive(TayamaArmSpinWarningFrames));
static_assert(!IsTayamaArmSpinActive(
    TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames));
static_assert(TayamaArmSpinAngle(0) == 0.0f);
static_assert(TayamaArmSpinAngle(TayamaArmSpinWarningFrames) == -TayamaArmSpinWindup);
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

/** @brief TAYAMA第一形態の排他的な攻撃 */
enum class TayamaAttack {
    None,
    HeadLaser,
    ArmSpin,
    RadarBurst,
    ChestSweep,
    LegVolley,
    CoreBarrage,
    CoreCannon,
    RearMissile,
    Stomp,
    Count
};

inline constexpr int TayamaAttackCooldownFrames = 30;
inline constexpr int TayamaSelectableAttackCount = static_cast<int>(TayamaAttack::Count) - 1;
inline constexpr int TayamaHeadLaserRecoveryFrames = 15;
inline constexpr int TayamaRadarBurstFireFrame = 72;
inline constexpr int TayamaRadarBurstVolleys = 3;
inline constexpr int TayamaRadarBurstIntervalFrames = 12;
inline constexpr int TayamaChestSweepFireFrame = 36;
inline constexpr int TayamaChestSweepVolleys = 3;
inline constexpr int TayamaChestSweepIntervalFrames = 8;
inline constexpr int TayamaLegVolleyFireFrame = 32;
inline constexpr int TayamaLegVolleyVolleys = 2;
inline constexpr int TayamaLegVolleyIntervalFrames = 12;
inline constexpr int TayamaCoreCannonChargeFrames = 120;
inline constexpr int TayamaCoreCannonActiveFrames = 75;
inline constexpr int TayamaCoreCannonRecoveryFrames = 30;
inline constexpr int TayamaCoreCannonBurstIntervalFrames = 15;
inline constexpr float TayamaCoreCannonHitRadius = 16.0f;
inline constexpr float TayamaHeadLaserSweepSpan = 30.0f;
inline constexpr float TayamaBeamOvershoot = 60.0f;
inline constexpr float TayamaArmBladeReach = TayamaOrbitRadius + 40.0f;
inline constexpr int TayamaStompRingBullets = 24;
inline constexpr float TayamaStompRingBulletSpeed = 1.4f;
inline constexpr float TayamaShockwaveSpeed = 3.0f;
inline constexpr float TayamaShockwaveBand = 5.0f;
inline constexpr float TayamaShockwaveHeight = 14.0f;
inline constexpr int TayamaShockwaveFrames = 72;
inline constexpr int TayamaRearMissileFireFrame = 36;
inline constexpr int TayamaCoreSalvoFireFrame = 42;
inline constexpr int TayamaCoreSalvoIntervalFrames = 10;
inline constexpr int TayamaCoreRadialChargeFrame = 72;
inline constexpr int TayamaCoreRadialFireFrame = 104;
inline constexpr int TayamaCoreFanChargeFrame = 112;
inline constexpr int TayamaCoreFanFireFrame = 138;

/**
 * @brief 第一形態の攻撃の継続フレーム数を取得する
 * @param attack 対象攻撃
 * @return 予告、発動、硬直を含むフレーム数
 */
constexpr int TayamaAttackDuration(TayamaAttack attack) {
    switch (attack) {
    case TayamaAttack::HeadLaser:
        return TayamaHeadLaserWarningFrames + TayamaHeadLaserActiveFrames +
            TayamaHeadLaserRecoveryFrames;
    case TayamaAttack::ArmSpin:
        return TayamaArmSpinWarningFrames + TayamaArmSpinActiveFrames +
            TayamaArmSpinRecoveryFrames;
    case TayamaAttack::RadarBurst:
        return TayamaRadarBurstFireFrame + TayamaRadarBurstVolleys * TayamaRadarBurstIntervalFrames + 24;
    case TayamaAttack::ChestSweep:
        return TayamaChestSweepFireFrame + TayamaChestSweepVolleys * TayamaChestSweepIntervalFrames + 24;
    case TayamaAttack::LegVolley:
        return TayamaLegVolleyFireFrame + TayamaLegVolleyVolleys * TayamaLegVolleyIntervalFrames + 28;
    case TayamaAttack::CoreBarrage: return TayamaCoreFanFireFrame + 32;
    case TayamaAttack::CoreCannon:
        return TayamaCoreCannonChargeFrames + TayamaCoreCannonActiveFrames + TayamaCoreCannonRecoveryFrames;
    case TayamaAttack::RearMissile: return TayamaRearMissileFireFrame + 54;
    case TayamaAttack::Stomp: return TayamaStompSequenceFrames;
    default: return TayamaAttackCooldownFrames;
    }
}

/**
 * @brief 難易度に応じた攻撃間の待機時間を取得する
 * @param difficulty 難易度番号、0がEasyで2がHard
 * @return 待機フレーム数
 */
constexpr int TayamaDifficultyCooldown(int difficulty) {
    return difficulty == 0 ? 60 : (difficulty == 2 ? 12 : TayamaAttackCooldownFrames);
}

/**
 * @brief 難易度に応じた全周弾の弾数を取得する
 * @param difficulty 難易度番号、0がEasyで2がHard
 * @return 一度に発射する弾数
 */
constexpr int TayamaDifficultyRingBullets(int difficulty) {
    return difficulty == 0 ? 12 : (difficulty == 2 ? 24 : TayamaRadarBurstBulletCount);
}

/**
 * @brief 難易度に応じて連射回数を増減する
 * @param volleys Normalの連射回数
 * @param difficulty 難易度番号、0がEasyで2がHard
 * @return 連射回数
 */
constexpr int TayamaDifficultyVolleys(int volleys, int difficulty) {
    return volleys + difficulty - 1;
}

/**
 * @brief 攻撃開始時の警告音を取得する
 * @param attack 開始する攻撃
 * @return 攻撃ごとに聞き分けられるMML
 */
constexpr const char* TayamaAttackWarningMml(TayamaAttack attack) {
    switch (attack) {
    case TayamaAttack::HeadLaser: return "t240 o5 l32 v12 c e g >c";
    case TayamaAttack::ArmSpin: return "t160 o3 l16 v12 c d e f g";
    case TayamaAttack::RadarBurst: return "t220 o6 l32 v9 c r c r c";
    case TayamaAttack::LegVolley: return "t200 o3 l32 v11 g g >c";
    case TayamaAttack::CoreBarrage: return "t240 o3 l16 v12 c > c < c > c";
    case TayamaAttack::CoreCannon: return "t70 o2 l4 v14 c d f a";
    case TayamaAttack::RearMissile: return "t240 o5 l32 v9 g e g e";
    case TayamaAttack::Stomp: return "t100 o2 l8 v13 c <g";
    default: return "t180 o4 l32 v10 c c c";
    }
}

// 連射を増やしてもHardの最後の一発が攻撃の継続時間内に収まる
static_assert(TayamaRadarBurstFireFrame + TayamaDifficultyVolleys(TayamaRadarBurstVolleys, 2) *
    TayamaRadarBurstIntervalFrames < TayamaAttackDuration(TayamaAttack::RadarBurst));
static_assert(TayamaChestSweepFireFrame + TayamaDifficultyVolleys(TayamaChestSweepVolleys, 2) *
    TayamaChestSweepIntervalFrames < TayamaAttackDuration(TayamaAttack::ChestSweep));
static_assert(TayamaLegVolleyFireFrame + TayamaDifficultyVolleys(TayamaLegVolleyVolleys, 2) *
    TayamaLegVolleyIntervalFrames < TayamaAttackDuration(TayamaAttack::LegVolley));
static_assert(TayamaDifficultyVolleys(TayamaLegVolleyVolleys, 0) >= 1);

/**
 * @brief 連続発射の各発射フレームか判定する
 * @param frame 攻撃開始からの経過フレーム
 * @param first 最初の発射フレーム
 * @param interval 発射間隔
 * @param count 発射回数
 * @return 発射するフレームの場合true
 */
constexpr bool IsTayamaVolleyFrame(int frame, int first, int interval, int count) {
    return frame >= first && frame < first + interval * count && (frame - first) % interval == 0;
}

/**
 * @brief 光線攻撃が判定を持つフレームか判定する
 * @param attack 選択中の攻撃
 * @param frame 攻撃開始からの経過フレーム
 * @return 照射中の場合true、予告中や他の攻撃ではfalse
 */
constexpr bool IsTayamaBeamActive(TayamaAttack attack, int frame) {
    if (attack == TayamaAttack::HeadLaser) return IsTayamaHeadLaserActive(frame);
    if (attack == TayamaAttack::ArmSpin) return IsTayamaArmSpinActive(frame);
    return attack == TayamaAttack::CoreCannon && frame >= TayamaCoreCannonChargeFrames &&
        frame < TayamaCoreCannonChargeFrames + TayamaCoreCannonActiveFrames;
}

/**
 * @brief 攻撃の抽選候補を表すビットを取得する
 * @param attack 対象攻撃
 * @return 候補マスク用のビット
 */
constexpr unsigned TayamaAttackBit(TayamaAttack attack) {
    return 1u << static_cast<int>(attack);
}

/**
 * @brief 候補から直前と異なる攻撃を選ぶ
 * @param eligible 発動可能な攻撃のビット列
 * @param previous 直前の攻撃
 * @param roll 0以上の乱数値
 * @return 選んだ攻撃、候補が直前の攻撃だけならその攻撃、候補がなければNone
 */
constexpr TayamaAttack SelectTayamaAttack(unsigned eligible, TayamaAttack previous, int roll) {
    for (int offset = 0; offset < TayamaSelectableAttackCount; ++offset) {
        const TayamaAttack candidate = static_cast<TayamaAttack>(
            1 + (roll + offset) % TayamaSelectableAttackCount);
        if (candidate != previous && (eligible & TayamaAttackBit(candidate))) return candidate;
    }
    return (eligible & TayamaAttackBit(previous)) ? previous : TayamaAttack::None;
}

/**
 * @brief 第一形態の段階で抽選する攻撃の候補を取得する
 * @param phase 第一形態の段階
 * @return 攻撃候補のビット列
 */
constexpr unsigned TayamaStageAttacks(Phase phase) {
    if (phase == Phase::TayamaFireControl) {
        return TayamaAttackBit(TayamaAttack::RadarBurst) |
            TayamaAttackBit(TayamaAttack::ChestSweep) |
            TayamaAttackBit(TayamaAttack::RearMissile);
    }
    if (phase == Phase::TayamaLiftEngines) {
        return TayamaAttackBit(TayamaAttack::ArmSpin) |
            TayamaAttackBit(TayamaAttack::Stomp) |
            TayamaAttackBit(TayamaAttack::LegVolley) |
            TayamaAttackBit(TayamaAttack::ChestSweep) |
            TayamaAttackBit(TayamaAttack::RearMissile);
    }
    return TayamaAttackBit(TayamaAttack::HeadLaser) |
        TayamaAttackBit(TayamaAttack::CoreBarrage) |
        TayamaAttackBit(TayamaAttack::CoreCannon) |
        TayamaAttackBit(TayamaAttack::RadarBurst) |
        TayamaAttackBit(TayamaAttack::RearMissile);
}

/**
 * @brief 第一形態の段階を開始する時の本体HPを取得する
 * @param phase 第一形態の段階
 * @param maxHp 本体の最大HP
 * @return 段階開始時の本体HP
 */
constexpr int TayamaStageStartHp(Phase phase, int maxHp) {
    return phase == Phase::TayamaLiftEngines ? maxHp * 2 / 3 :
        (phase == Phase::TayamaCommandCore ? maxHp / 3 : maxHp);
}

/**
 * @brief 第一形態の段階で本体HPが下回らない値を取得する
 * @param phase 第一形態の段階
 * @param maxHp 本体の最大HP
 * @return 次の段階の開始HP、最終段階はコア破壊まで残す1
 */
constexpr int TayamaStageFloorHp(Phase phase, int maxHp) {
    return phase == Phase::TayamaFireControl ? TayamaStageStartHp(Phase::TayamaLiftEngines, maxHp) :
        (phase == Phase::TayamaLiftEngines ? TayamaStageStartHp(Phase::TayamaCommandCore, maxHp) : 1);
}

static_assert(TayamaStageFloorHp(Phase::TayamaFireControl, 3000) == 2000);
static_assert(TayamaStageFloorHp(Phase::TayamaLiftEngines, 3000) == 1000);
static_assert(TayamaStageFloorHp(Phase::TayamaCommandCore, 3000) == 1);
static_assert(TayamaStageStartHp(Phase::TayamaFireControl, 3000) == 3000);
static_assert(TayamaStageStartHp(Phase::TayamaCommandCore, 3000) == 1000);
static_assert((TayamaStageAttacks(Phase::TayamaFireControl) &
    TayamaAttackBit(TayamaAttack::CoreBarrage)) == 0);
static_assert((TayamaStageAttacks(Phase::TayamaCommandCore) &
    TayamaAttackBit(TayamaAttack::CoreBarrage)) != 0);

/**
 * @brief 発射元へ重ねる予告光の進捗を取得する
 * @param attack 選択中の攻撃
 * @param frame 攻撃開始からの経過フレーム
 * @return 予告中は0から1へ増える進捗、予告外は0
 */
constexpr float TayamaAttackCharge(TayamaAttack attack, int frame) {
    int start = 0;
    int end = 0;
    switch (attack) {
    case TayamaAttack::HeadLaser: end = TayamaHeadLaserWarningFrames; break;
    case TayamaAttack::ArmSpin: end = TayamaArmSpinWarningFrames; break;
    case TayamaAttack::RadarBurst: end = TayamaRadarBurstFireFrame; break;
    case TayamaAttack::ChestSweep: end = TayamaChestSweepFireFrame; break;
    case TayamaAttack::LegVolley: end = TayamaLegVolleyFireFrame; break;
    case TayamaAttack::RearMissile: end = TayamaRearMissileFireFrame; break;
    case TayamaAttack::CoreCannon: end = TayamaCoreCannonChargeFrames; break;
    case TayamaAttack::Stomp: end = TayamaStompRaiseFrames; break;
    case TayamaAttack::CoreBarrage:
        start = frame >= TayamaCoreFanChargeFrame ? TayamaCoreFanChargeFrame :
            (frame >= TayamaCoreRadialChargeFrame ? TayamaCoreRadialChargeFrame : 0);
        end = frame >= TayamaCoreFanChargeFrame ? TayamaCoreFanFireFrame :
            (frame >= TayamaCoreRadialChargeFrame ? TayamaCoreRadialFireFrame :
                TayamaCoreSalvoFireFrame);
        break;
    default: break;
    }
    return frame >= start && frame < end ?
        static_cast<float>(frame - start) / static_cast<float>(end - start) : 0.0f;
}

// 全攻撃が発射前に予告を持ち、発射や判定の終了後に硬直して次の攻撃と重ならない
static_assert(TayamaSelectableAttackCount == 9);
static_assert(TayamaCoreSalvoFireFrame + TayamaCoreSalvoIntervalFrames * 2 < TayamaCoreRadialChargeFrame);
static_assert(TayamaCoreRadialFireFrame < TayamaCoreFanChargeFrame);
static_assert(TayamaCoreFanFireFrame < TayamaAttackDuration(TayamaAttack::CoreBarrage));
static_assert(TayamaAttackCharge(TayamaAttack::RadarBurst, TayamaRadarBurstFireFrame - 1) > 0.9f);
static_assert(TayamaAttackCharge(TayamaAttack::RadarBurst, TayamaRadarBurstFireFrame) == 0.0f);
static_assert(TayamaAttackCharge(TayamaAttack::CoreBarrage, TayamaCoreRadialFireFrame - 1) > 0.9f);
static_assert(TayamaAttackCharge(TayamaAttack::CoreBarrage, TayamaCoreFanFireFrame - 1) > 0.9f);
static_assert(TayamaAttackCharge(TayamaAttack::Stomp, TayamaStompRaiseFrames) == 0.0f);
static_assert(!IsTayamaBeamActive(TayamaAttack::CoreCannon, TayamaCoreCannonChargeFrames - 1));
static_assert(IsTayamaBeamActive(TayamaAttack::CoreCannon, TayamaCoreCannonChargeFrames));
static_assert(!IsTayamaBeamActive(TayamaAttack::RadarBurst, TayamaCoreCannonChargeFrames));
static_assert(IsTayamaVolleyFrame(24, 8, 8, 3) && !IsTayamaVolleyFrame(32, 8, 8, 3));
// 光刃の先端が最高速でも判定幅を飛び越えず、周回軌道の自機を素通りしない
static_assert((Math::TwoPi + TayamaArmSpinWindup) * 1.5f / TayamaArmSpinActiveFrames * TayamaOrbitRadius <
    (TayamaArmSpinHitRadius + 0.38f) * 2.0f);
// 高度移動の速度で予告中に光刃の判定幅から抜けられる
static_assert(TayamaArmSpinWarningFrames * 0.058f * 4.4f > TayamaArmSpinHitRadius + 0.38f);
static_assert(TayamaAttackCharge(TayamaAttack::None, 1) == 0.0f);
static_assert(SelectTayamaAttack(TayamaAttackBit(TayamaAttack::ArmSpin) |
    TayamaAttackBit(TayamaAttack::LegVolley), TayamaAttack::ArmSpin, 0) == TayamaAttack::LegVolley);
static_assert(SelectTayamaAttack(TayamaAttackBit(TayamaAttack::ArmSpin),
    TayamaAttack::ArmSpin, 3) == TayamaAttack::ArmSpin);
static_assert(SelectTayamaAttack(0u, TayamaAttack::None, 0) == TayamaAttack::None);
static_assert(SelectTayamaAttack(TayamaAttackBit(TayamaAttack::Stomp),
    TayamaAttack::None, 0) == TayamaAttack::Stomp);

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
    int eastsourceShieldHp = 0;
    int eastsourceLaserTimer = -1;
    Vector3 eastsourceLaserDirection {};
    bool eastsourceLaserRail = false;
    std::array<SearchlightState, SearchlightCount> searchlights {};
    std::array<TayamaWeakpointState, TayamaWeakpointCount> tayamaWeakpoints {};
    std::array<TayamaWeakpointState, TayamaWeakpointCount> checkpointWeakpoints {};
    std::array<TayamaReflectFunnel, TayamaReflectFunnelCount> tayamaReflectFunnels {};
    std::array<ShaftObstacle, ShaftObstacleCount> shaftObstacles {};
    float tayamaTransformation = 0.0f;
    float tayamaOrbitAngle = 0.0f;
    float tayamaSideViewAngle = 0.0f;
    float checkpointPower = 0.0f;
    float coreTargetX = 0.0f;
    float coreTargetY = 0.0f;
    float coreTargetZ = 8.0f;
    Vector3 headLaserTarget {};
    Vector3 tayamaDragonExplosionCenter {};
    int phaseTimer = 0;
    int checkpointScore = 0;
    int checkpointKills = 0;
    int soundCooldown = 0;
    int attackTimer = 0;
    int guardSpawnCooldown = 0;
    int tayamaStompTimer = 0;
    int tayamaStompCooldown = 0;
    int tayamaShockwaveTimer = 0;
    Vector3 tayamaShockwaveCenter {};
    int tayamaHp = 0;
    int tayamaMaxHp = 0;
    int tayamaDragonHitFlashFrames = 0;
    int tayamaDragonAttackTimer = 0;
    TayamaDragonAttack tayamaDragonAttack = TayamaDragonAttack::None;
    TayamaDragonAttack previousTayamaDragonAttack = TayamaDragonAttack::None;
    TayamaAttack tayamaAttack = TayamaAttack::None;
    TayamaAttack previousTayamaAttack = TayamaAttack::None;
    bool headLaserArmed = false;
    bool tayamaStageShifted = false;
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

inline constexpr int TayamaStageShiftFrames = 150;
inline constexpr int TayamaStageShiftBlastFrames = 50;
inline constexpr int TayamaBattleReadFrames = 75;

/**
 * @brief 第一形態の段階移行演出の最中か判定する
 * @param phase 現在のStage 5状態
 * @param phaseTimer 現在状態の経過フレーム数
 * @param stageShifted 被弾復帰ではなくHP減少で段階へ入った場合true
 * @return 段階移行直後ならtrue
 */
constexpr bool IsTayamaStageShift(Phase phase, int phaseTimer, bool stageShifted) {
    return stageShifted && IsTayamaBattlePhase(phase) && phaseTimer <= TayamaStageShiftFrames;
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
    constexpr float ColumnStep = 0.65f;
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
static_assert(Part2ChapterNumber(Phase::WallClimbLower) == 1);
static_assert(Part2ChapterNumber(Phase::WallClimbMiddle) == 2);
static_assert(Part2ChapterNumber(Phase::WallClimbUpper) == 3);
static_assert(Part2ChapterNumber(Phase::Approach) == 0);
static_assert(Part2RouteElapsedFrames(Phase::WallClimbLower, 0) == 0);
static_assert(Part2RouteElapsedFrames(Phase::WallClimbMiddle, 0) == WallClimbLowerFrames);
static_assert(Part2RouteElapsedFrames(Phase::WallClimbUpper, 0) ==
    WallClimbLowerFrames + WallClimbMiddleFrames);
static_assert(Part2RailPlayerMinY < Part2RailPlayerMaxY);
static_assert(Part2EnemyScaleMultiplier(0.0f) == 1.0f);
static_assert(Part2EnemyScaleMultiplier(1.0f) == Part2RailEnemyScale);
static_assert(Part2RailDroneEntryProgress(0) == 0.0f);
static_assert(Part2RailDroneEntryProgress(Part2RailDroneEntryFrames / 2) == 0.5f);
static_assert(Part2RailDroneEntryProgress(Part2RailDroneEntryFrames) == 1.0f);
static_assert(Part2DronePlacementIndex(0, 0) != Part2DronePlacementIndex(1, 0));
static_assert(Part2DronePlacementIndex(0, 0) == Part2DronePlacementIndex(30, 0));
static_assert(Part2RailDroneBaseX(0) < 0.0f);
static_assert(Part2RailDroneBaseX(Part2DroneColumnCount - 1) > 0.0f);
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
 * @brief 爆破後に自機が入口へ突入する進行率を取得する
 * @param phaseTimer 入口爆破状態の経過フレーム数
 * @return 突入開始を0、暗転開始時を1とする進行率
 */
constexpr float EntranceFlyInProgress(int phaseTimer) {
    constexpr int FlyInFrames = WallClimbTransitionFrames -
        WallClimbExitFadeFrames - EntranceFlyInFrame;
    const float progress = static_cast<float>(phaseTimer - EntranceFlyInFrame) /
        static_cast<float>(FlyInFrames);
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
    return weakpoint < TayamaWeakpoint::CommandCore ? IsTayamaBattlePhase(phase) :
        weakpoint == TayamaWeakpoint::CommandCore && phase == Phase::TayamaCommandCore;
}

}
