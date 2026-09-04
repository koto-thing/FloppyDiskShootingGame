#include "Stage5Module.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../SideScrollingShooterEnemies.h"
#include "../../SideScrollingShooterShared.h"
#include "../Common/CityBackgroundModule.h"
#include "../Common/StageDefinition.h"
#include "../Stage4/Stage4BossModelView.h"
#include "../Stage3/Stage3FunnelModelView.h"
#include "Stage5ModelView.h"
#include "Stage5CityModelView.h"
#include "WallSecurityDroneModelView.h"

namespace {
constexpr float TowerFacadeColor[4] = { 0.10f, 0.13f, 0.24f, 1.0f };
constexpr float SatelliteLightColor[4] = { 0.82f, 0.94f, 1.0f, 1.0f };
constexpr float SearchlightColor[4] = { 1.00f, 0.82f, 0.20f, 0.24f };
constexpr float SearchlightLockedColor[4] = { 1.00f, 0.08f, 0.08f, 0.50f };
constexpr float DronePointerColor[4] = { 1.00f, 0.03f, 0.02f, 0.72f };
constexpr float StormCloudColor[4] = { 0.05f, 0.07f, 0.13f, 1.0f };
constexpr float BuildingWindowColor[4] = { 0.20f, 0.78f, 0.92f, 1.0f };
constexpr float MegaBuildingSilhouetteColor[4] = { 0.035f, 0.045f, 0.075f, 1.0f };
constexpr float WallLowerFacadeColor[4] = { 0.075f, 0.085f, 0.11f, 1.0f };
constexpr float WallMiddleFacadeColor[4] = { 0.16f, 0.035f, 0.045f, 1.0f };
constexpr float WallUpperFacadeColor[4] = { 0.025f, 0.022f, 0.030f, 1.0f };
constexpr float WallLowerAccentColor[4] = { 0.62f, 0.43f, 0.16f, 1.0f };
constexpr float WallMiddleAccentColor[4] = { 0.88f, 0.62f, 0.18f, 1.0f };
constexpr float WallUpperAccentColor[4] = { 1.00f, 0.77f, 0.24f, 1.0f };
constexpr float WallLowerLightColor[4] = { 1.00f, 0.48f, 0.12f, 1.0f };
constexpr float WallMiddleLightColor[4] = { 1.00f, 0.16f, 0.08f, 1.0f };
constexpr float WallUpperLightColor[4] = { 1.00f, 0.82f, 0.34f, 1.0f };
constexpr unsigned char PandDBuildingSignGlyphs[][7] = {
    {30, 17, 17, 30, 16, 16, 16}, // P
    {14, 17, 17, 31, 17, 17, 17}, // A
    {17, 25, 21, 19, 17, 17, 17}, // N
    {30, 17, 17, 17, 17, 17, 30}, // D
    {30, 17, 17, 17, 17, 17, 30}, // D
    {0, 0, 0, 31, 0, 0, 0},       // -
    {17, 18, 20, 24, 20, 18, 17}, // K
    {14, 17, 17, 31, 17, 17, 17}, // A
    {31, 4, 4, 4, 4, 4, 31}       // I
};
constexpr int PandDBuildingSignCharacterCount =
    static_cast<int>(std::size(PandDBuildingSignGlyphs));
static_assert(PandDBuildingSignCharacterCount == 9);
constexpr int TayamaGoldenLaserGlowEffect = 4;
constexpr int TayamaGoldenLaserCoreEffect = 5;
constexpr int RainCycle = 240;
constexpr int RainFallSpeed = 4;
constexpr int RainSplashDuration = 32;
constexpr float RainViewMargin = 1.14f;
constexpr float RainViewTravel = RainViewMargin * 2.0f;
constexpr int CloudSeaTravelCycleFrames = 720;
constexpr float CloudSeaTravelSpeed = 0.28f;
constexpr float CloudSeaDepth =
    static_cast<float>(CloudSeaTravelCycleFrames) * CloudSeaTravelSpeed;
constexpr int CloudSeaStarCount = 96;
constexpr int RooftopStarCount = 96;
static_assert(CloudSeaStarCount > 0);
static_assert(RooftopStarCount > 0);
constexpr int Part2StarCount = 72;
constexpr int Part2CloudCount = 18;
static_assert(Part2StarCount > Part2CloudCount);

/**
 * @brief 屋上の嵐雲をシーン内の固定座標へ配置する
 * @param index 雲番号
 * @return 屋上全体を覆うワールド座標
 */
constexpr Vector3 RooftopCloudPosition(int index) {
    constexpr float TayamaHeadY = TayamaModelView::GroundedRootY(
        ShooterStages::Stage5::RooftopSurfaceY,
        ShooterStages::Stage5::TayamaBossScale) +
        TayamaModelView::MechaHead.position.y * ShooterStages::Stage5::TayamaBossScale;
    return {
        -68.0f + static_cast<float>((index * 47) % 137),
        TayamaHeadY + 28.0f + static_cast<float>(index % 4) * 1.1f,
        -24.0f + static_cast<float>((index * 31) % 149)
    };
}

static_assert(RooftopCloudPosition(0).x == -68.0f);
static_assert(RooftopCloudPosition(1).z != RooftopCloudPosition(0).z);
static_assert(RooftopCloudPosition(137).x == RooftopCloudPosition(0).x);
static_assert(RooftopCloudPosition(0).y > 230.0f);

/**
 * @brief 雲海がカメラから奥へ進む循環移動量を取得する
 * @param frame 現在フレーム
 * @return 0以上CloudSeaDepth未満の+Z移動量
 */
constexpr float CloudSeaTravelOffset(int frame) {
    const int wrapped = frame % CloudSeaTravelCycleFrames;
    return static_cast<float>(wrapped < 0 ?
        wrapped + CloudSeaTravelCycleFrames : wrapped) * CloudSeaTravelSpeed;
}

static_assert(CloudSeaTravelOffset(0) == 0.0f);
static_assert(CloudSeaTravelOffset(1) > 0.0f);
static_assert(CloudSeaTravelOffset(CloudSeaTravelCycleFrames) == 0.0f);

/**
 * @brief TAYAMA龍第2形態のHPから夜明け進行率を取得する
 * @param phase 現在のStage 5状態
 * @param hp 第2形態の現在HP
 * @param maxHp 第2形態の最大HP
 * @return 夜を0、朝を1とする補間率
 */
constexpr float TayamaDragonDawnProgress(
    ShooterStages::Stage5::Phase phase, int hp, int maxHp) {
    if (phase == ShooterStages::Stage5::Phase::CloudSea || maxHp <= 0) return 0.0f;
    if (phase >= ShooterStages::Stage5::Phase::TayamaDragonCollapse) return 1.0f;
    const float progress = 1.0f - static_cast<float>((std::clamp)(hp, 0, maxHp)) /
        static_cast<float>(maxHp);
    return progress * progress * (3.0f - 2.0f * progress);
}

static_assert(TayamaDragonDawnProgress(
    ShooterStages::Stage5::Phase::CloudSea, 0, 0) == 0.0f);
static_assert(TayamaDragonDawnProgress(
    ShooterStages::Stage5::Phase::TayamaDragonBattle, 4000, 4000) == 0.0f);
static_assert(TayamaDragonDawnProgress(
    ShooterStages::Stage5::Phase::TayamaDragonBattle, 0, 4000) == 1.0f);

/**
 * @brief 雨粒の落下位相を周期内へ折り返す
 * @param index 雨粒番号
 * @param frame 現在フレーム
 * @return 0が接地、RainCycle未満が上空となる落下位相
 */
constexpr int RainFallPhase(int index, int frame) {
    return (index * 83 + RainCycle - (frame * RainFallSpeed) % RainCycle) % RainCycle;
}

/**
 * @brief 接地直後の経過位相を跳ね返り進行率へ変換する
 * @param fallPhase 現在の落下位相
 * @return 表示期間中は0から1、期間外は負数
 */
constexpr float RainSplashProgress(int fallPhase) {
    const int age = (RainCycle - fallPhase) % RainCycle;
    return age < RainSplashDuration ?
        static_cast<float>(age) / static_cast<float>(RainSplashDuration) : -1.0f;
}
constexpr int CityBuildingCount = 30;
constexpr float CityBuildingNdcSpacing = 2.0f / CityBuildingCount;
constexpr float Stage5CityBuildingScale = 2.0f;
constexpr int Stage5CityBuildingCount = static_cast<int>(Stage5BuildingType::Count) * 2;
static_assert(Stage5CityBuildingCount == 14);

/**
 * @brief 第2部道中の進行状態を壁面階層番号へ変換する
 * @param phase 現在の進行状態
 * @return 下層は0、中層は1、上層は2
 */
constexpr int WallTier(ShooterStages::Stage5::Phase phase) {
    if (phase == ShooterStages::Stage5::Phase::WallClimbMiddle) return 1;
    if (phase == ShooterStages::Stage5::Phase::WallClimbUpper) return 2;
    return 0;
}

static_assert(WallTier(ShooterStages::Stage5::Phase::WallClimbLower) == 0);
static_assert(WallTier(ShooterStages::Stage5::Phase::WallClimbMiddle) == 1);
static_assert(WallTier(ShooterStages::Stage5::Phase::WallClimbUpper) == 2);

/**
 * @brief 値を横画面の循環範囲へ収める
 * @param value 循環前の値
 * @return -1以上1未満の値
 */
float WrapCityNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) wrapped += 2.0f;
    return wrapped - 1.0f;
}

/**
 * @brief 値を正の距離範囲へ収める
 * @param value 循環前の値
 * @param length 循環距離
 * @return 0以上length未満の値
 */
float WrapCityDistance(float value, float length) {
    float wrapped = std::fmod(value, length);
    if (wrapped < 0.0f) wrapped += length;
    return wrapped;
}

/**
 * @brief カメラ前方のXZ平面へ天候エフェクト座標を配置する
 * @param camera 基準カメラ
 * @param lateral カメラ右方向の距離
 * @param depth カメラ前方向の距離
 * @return Yを0に固定したワールド座標
 */
Vector3 WeatherPosition(const Camera3D& camera, float lateral, float depth) {
    Vector3 forward = camera.Forward();
    forward.y = 0.0f;
    const float length = forward.Length();
    forward = length > Math::Epsilon ? forward / length : Vector3::Forward;
    const Vector3 right {forward.z, 0.0f, -forward.x};
    Vector3 position = camera.Position() + right * lateral + forward * depth;
    position.y = 0.0f;
    return position;
}

/**
 * @brief Stage 5進行状態から雨量を取得する
 * @param phase 現在の進行状態
 * @param chapter 現在のチャプター番号
 * @param tayamaTransformation TAYAMA変形率
 * @param phaseTimer 現在状態の経過フレーム数
 * @return 0から1の雨量
 */
constexpr float RainIntensity(ShooterStages::Stage5::Phase phase, int chapter,
    float tayamaTransformation, int phaseTimer) {
    (void)chapter;
    (void)tayamaTransformation;
    (void)phaseTimer;
    return phase <= ShooterStages::Stage5::Phase::TayamaCollapse ? 1.0f : 0.0f;
}

static_assert(RainIntensity(ShooterStages::Stage5::Phase::Approach, 1, 0.0f, 0) == 1.0f);
static_assert(RainIntensity(ShooterStages::Stage5::Phase::EastsourceBattle, 3, 0.0f, 0) == 1.0f);
static_assert(RainIntensity(ShooterStages::Stage5::Phase::WallClimbTransition, 3, 0.0f, 0) > 0.0f);
static_assert(RainIntensity(ShooterStages::Stage5::Phase::CarrierTransformation, 3, 0.5f, 90) > 0.0f);
static_assert(RainIntensity(ShooterStages::Stage5::Phase::TayamaCommandCore, 3, 1.0f, 180) == 1.0f);
static_assert(RainFallPhase(0, 0) == 0);
static_assert(RainFallPhase(0, 1) == RainCycle - RainFallSpeed);
static_assert(RainSplashProgress(RainCycle - RainFallSpeed) == 0.125f);
static_assert(RainSplashProgress(RainCycle - RainSplashDuration) < 0.0f);
static_assert(RainViewMargin > 1.0f);
static_assert(RainViewMargin - RainViewTravel < -1.0f);
}

/**
 * @brief Stage 5の壁面上昇と崩壊に合わせてレールカメラを補正する
 * @param shooter 判定対象
 * @param railPosition 補正するカメラ位置
 * @param railTarget 補正する注視点
 * @return なし
 */
void SideScrollingShooter::Stage5Module::ApplyCameraCorrection(
    const SideScrollingShooter& shooter, Vector3& railPosition, Vector3& railTarget) {
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition) {
        // 暗転解除後は道路を進み、ビル直前から壁面上方へカメラを向ける
        const float approach = SmoothStep(ShooterStages::Stage5::WallApproachProgress(
            shooter.m_stage5.phaseTimer));
        const float climb = SmoothStep(ShooterStages::Stage5::WallClimbProgress(
            shooter.m_stage5.phaseTimer));
        const float turn = SmoothStep(Math::Clamp01(climb * 4.0f));
        const Vector3 playerPosition {
            ToWorldX(shooter.m_playerX),
            -0.65f + climb * ShooterStages::Stage5::WallClimbHeight,
            Math::Lerp(ShooterStages::Stage5::WallApproachStartZ,
                ShooterStages::Stage5::WallApproachEndZ, approach)
        };
        const Vector3 roadCamera = playerPosition + Vector3 {0.0f, 1.2f, -9.0f};
        const Vector3 climbCamera = playerPosition + Vector3 {0.0f, 0.5f, -16.0f};
        const Vector3 roadTarget = playerPosition + Vector3 {0.0f, 0.0f, 14.0f};
        constexpr float BuildingTopY = -3.65f + ShooterStages::Stage5::PandDBuildingHeight;
        const Vector3 climbTarget {
            playerPosition.x,
            (std::min)(playerPosition.y + 48.0f, BuildingTopY),
            42.0f
        };
        railPosition = Vector3::Lerp(roadCamera, climbCamera, turn);
        railTarget = Vector3::Lerp(roadTarget, climbTarget, turn);
        return;
    }

    // 第2部道中は自機を画面内に保ちつつ、後方から壁面の進行方向を見る
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) {
        railPosition.y += 0.72f;
        railPosition.z -= 6.0f;
        railTarget.z += 8.0f;
        return;
    }

    // 屋上到着後は斜め上から建物全体を見せ、変形開始までに正面へ回り込む
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        const float front = shooter.m_stage5.phase == Stage5Phase::RooftopArrival ?
            SmoothStep(ShooterStages::Stage5::RooftopCameraFrontProgress(
                shooter.m_stage5.phaseTimer)) : 1.0f;
        const float shake = shooter.m_stage5.phase == Stage5Phase::RooftopArrival ?
            std::sin(static_cast<float>(shooter.m_stage5.phaseTimer) * 0.72f) * 0.16f : 0.0f;
        const Stage5ModelTransform boss = TayamaTransform(shooter);
        const float towerCenterY = boss.position.y +
            (TayamaModelView::TowerBoundsMin.y + TayamaModelView::TowerBoundsMax.y) *
                0.5f * boss.scale;
        const float towerHalfHeight = TayamaModelView::TowerSize.y * boss.scale * 0.5f;
        const float overheadDistance = TayamaModelView::TowerSize.y * boss.scale *
            ShooterStages::Stage5::TayamaOverheadDistanceScale;
        const float frontDistance = TayamaModelView::TowerSize.y * boss.scale *
            ShooterStages::Stage5::TayamaFrontDistanceScale;
        const float targetY = Math::Lerp(towerCenterY, boss.position.y + 0.8f,
            shooter.m_stage5.tayamaTransformation);
        const Vector3 overheadPosition {
            overheadDistance * 0.55f,
            towerCenterY + towerHalfHeight + 30.0f,
            boss.position.z - overheadDistance
        };
        const Vector3 frontPosition {
            0.0f, towerCenterY, boss.position.z - frontDistance
        };
        railPosition = Vector3::Lerp(overheadPosition, frontPosition, front) +
            Vector3 {shake, -shake * 0.45f, 0.0f};
        railTarget = {0.0f, targetY, 57.0f};

        // 変形終盤だけ正面全景から戦闘開始時の自機追従位置へ接続する
        if (shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
            const float battle = SmoothStep(ShooterStages::Stage5::CarrierCameraBattleProgress(
                shooter.m_stage5.phaseTimer));
            const Vector2 playerOrbit = SideScrollingShooter::TayamaOrbitXZ(0.0f, 0.0f);
            const Vector3 player {playerOrbit.x, ToWorldY(shooter.m_playerY), playerOrbit.y};
            const Vector3 radial {0.0f, 0.0f, -1.0f};
            const Vector3 battlePosition = player + radial * ShooterStages::Stage5::TayamaCameraDistance +
                Vector3 {0.0f, ShooterStages::Stage5::TayamaCameraHeight, 0.0f};
            const Vector3 battleTarget = player - radial * ShooterStages::Stage5::TayamaCameraLookAhead;
            railPosition = Vector3::Lerp(railPosition, battlePosition, battle);
            railTarget = Vector3::Lerp(railTarget, battleTarget, battle);
        }
        return;
    }

    // 戦闘中は自機の横移動に引かれない正面構図で巨大メカを中央へ固定する
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore) {
        const Stage5ModelTransform boss = TayamaTransform(shooter);
        railPosition = {ToWorldX(shooter.m_playerX) * 0.08f, 1.5f, -9.0f};
        railTarget = {0.0f, boss.position.y + 0.8f, boss.position.z};
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) {
        // 頭部、龍、自機の順に上昇する全景を一つの上向きパンで追う
        const float progress = SmoothStep(ShooterStages::Stage5::FinalEscapeProgress(
            shooter.m_stage5.phaseTimer,
            ShooterStages::Stage5::FinalEscapeHeadStartFrames,
            ShooterStages::Stage5::FinalEscapePlayerStartFrames +
                ShooterStages::Stage5::FinalEscapePlayerDurationFrames -
                ShooterStages::Stage5::FinalEscapeHeadStartFrames));
        railPosition = {0.0f, Math::Lerp(58.0f, 150.0f, progress), -74.0f};
        railTarget = {0.0f, Math::Lerp(102.0f, 232.0f, progress), 68.0f};
        return;
    }
    if (ShooterStages::Stage5::IsCloudSeaPhase(shooter.m_stage5.phase)) {
        railPosition = {0.0f, 12.0f, -24.0f};
        railTarget = {0.0f, 9.0f, 62.0f};
    }
}

/**
 * @brief Stage 5演出用のレールカメラ画角を取得する
 * @param shooter 判定対象
 * @param defaultFieldOfView 通常の画角
 * @return 適用する画角
 */
float SideScrollingShooter::Stage5Module::CameraFieldOfView(
    const SideScrollingShooter& shooter, float defaultFieldOfView) {
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) {
        // 広角化して中央外壁の上方と下方の市街を同時に画面へ残す
        return Math::Lerp(defaultFieldOfView, 70.0f, shooter.RailBlend());
    }
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        const float battle = shooter.m_stage5.phase == Stage5Phase::CarrierTransformation ?
            SmoothStep(ShooterStages::Stage5::CarrierCameraBattleProgress(
                shooter.m_stage5.phaseTimer)) : 0.0f;
        return Math::Lerp(70.0f, 42.0f, battle);
    }
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore) {
        return 42.0f;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) return 62.0f;
    if (ShooterStages::Stage5::IsCloudSeaPhase(shooter.m_stage5.phase)) return 54.0f;
    if (shooter.m_stage5.phase != Stage5Phase::WallClimbTransition) {
        return defaultFieldOfView;
    }
    const float climb = SmoothStep(ShooterStages::Stage5::WallClimbProgress(
        shooter.m_stage5.phaseTimer));
    return Math::Lerp(70.0f, 132.0f,
        SmoothStep(Math::Clamp01(climb * 4.0f)));
}

/**
 * @brief 現在の演出状態で敵を描画するか判定する
 * @param shooter 判定対象
 * @param enemy 描画候補
 * @return 描画する場合true、格納庫内に隠す場合false
 */
bool SideScrollingShooter::Stage5Module::ShouldDrawEnemy(
    const SideScrollingShooter& shooter, const Enemy& enemy) {
    return !(shooter.m_stage5.phase == Stage5Phase::EastsourceIntro &&
        shooter.m_stage5.phaseTimer < 58 && enemy.type == Stage::BossEnemy);
}

/**
 * @brief Stage 5の進行状態に対応する空を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawSky(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    if (!ShooterStages::Stage5::IsCloudSeaPhase(shooter.m_stage5.phase)) {
        CityBackgroundModule::DrawSky(renderer);
        return;
    }

    // 第2形態のHP減少に合わせ、暗い夜空から朝焼けの青空へ滑らかに変える
    const float dawn = TayamaDragonDawnProgress(shooter.m_stage5.phase,
        shooter.m_stage5.tayamaHp, shooter.m_stage5.tayamaMaxHp);
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}},
        {Math::Lerp(0.025f, 0.38f, dawn), Math::Lerp(0.035f, 0.62f, dawn),
            Math::Lerp(0.10f, 0.86f, dawn), 1.0f});
    renderer.Draw(Rect {{0.0f, -0.58f}, {2.0f, 0.84f}},
        {Math::Lerp(0.10f, 0.96f, dawn), Math::Lerp(0.07f, 0.58f, dawn),
            Math::Lerp(0.16f, 0.30f, dawn), 1.0f});
}

/**
 * @brief Stage 5専用2D画面エフェクトを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawOverlay2D(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    DrawScreenEffects(shooter, renderer, nullptr);
}

/**
 * @brief Stage 5ムービー用の自機描画Transformを適用する
 * @param shooter 判定対象
 * @param position 補正する自機ワールド座標
 * @param pitch 補正する自機X軸回転
 * @return なし
 */
void SideScrollingShooter::Stage5Module::ApplyPlayerRenderCorrection(
    const SideScrollingShooter& shooter, Vector3& position, float& pitch) {
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceFall) {
        // カメラを残し、自機だけを画面奥へ飛ばしてから暗転する
        const float progress = SmoothStep(ShooterStages::Stage5::EastsourceFlyAwayProgress(
            shooter.m_stage5.phaseTimer));
        position.z = Math::Lerp(PlayerRailZ, 42.0f, progress);
        position.y += progress * 1.8f;
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition) {
        const float approach = SmoothStep(ShooterStages::Stage5::WallApproachProgress(
            shooter.m_stage5.phaseTimer));
        const float climb = SmoothStep(ShooterStages::Stage5::WallClimbProgress(
            shooter.m_stage5.phaseTimer));
        const float turn = SmoothStep(Math::Clamp01(climb * 4.0f));
        position.y = -0.65f + climb * ShooterStages::Stage5::WallClimbHeight;
        position.z = Math::Lerp(ShooterStages::Stage5::WallApproachStartZ,
            ShooterStages::Stage5::WallApproachEndZ, approach);
        pitch = Math::Lerp(0.0f, -Math::HalfPi, turn);
        return;
    }
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) {
        // 2Dから3Dへ切り替わる間も機首を連続的に壁面上方へ向ける
        pitch = Math::Lerp(0.0f, -Math::HalfPi, shooter.RailBlend());
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) {
        // 胴体前面で待機した自機を龍に続け、奥へ潜らせず画面上方へ抜く
        const float progress = SmoothStep(ShooterStages::Stage5::FinalEscapeProgress(
            shooter.m_stage5.phaseTimer,
            ShooterStages::Stage5::FinalEscapePlayerStartFrames,
            ShooterStages::Stage5::FinalEscapePlayerDurationFrames));
        const Stage5ModelTransform boss = TayamaTransform(shooter);
        const float bossFrontZ = boss.position.z +
            TayamaModelView::TowerBoundsMin.z * boss.scale - 4.0f;
        position = Vector3::Lerp(
            {0.0f, boss.position.y, bossFrontZ},
            {8.0f, boss.position.y + 480.0f, bossFrontZ}, progress);
        pitch = Math::Lerp(0.0f, -Math::HalfPi, progress);
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::CloudSea) {
        const float assembly = SmoothStep(ShooterStages::Stage5::CloudSeaAssemblyProgress(
            shooter.m_stage5.phaseTimer));
        position = Vector3::Lerp({0.0f, 8.0f, 18.0f},
            {0.0f, 0.0f, PlayerRailZ}, assembly);
    }
}

/**
 * @brief Stage 5専用3D画面エフェクトを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawOverlay3D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    DrawScreenEffects(shooter, renderer, &camera);
}

/**
 * @brief Stage 5専用HUDを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @return 専用HUDが共通ボスHUDを置き換える場合true、共通ボスHUDを描画する場合false
 */
bool SideScrollingShooter::Stage5Module::DrawHud(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    // Approach以外は現行と同じく空表示の終幕も共通ボスHUDへ戻さない
    if (shooter.m_stage5.phase == Stage5Phase::Approach) return false;
    DrawStage5Hud(shooter, renderer);
    return true;
}

/**
 * @brief EASTSOURCE専用攻撃予告を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param enemy 攻撃予告を持つ敵
 * @param size 共通処理で算出した予告サイズ
 * @return 専用予告を描画して共通予告を省略する場合true、共通予告を使用する場合false
 */
bool SideScrollingShooter::Stage5Module::DrawSpecialAttackWarning3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float size) {
    if (enemy.type != Stage::BossEnemy ||
        shooter.m_stage5.phase < Stage5Phase::EastsourceIntro ||
        shooter.m_stage5.phase > Stage5Phase::EastsourceBattle) return false;

    // 固定した照準地点をプレイヤー面へ表示して発射後の追尾と誤認させない
    constexpr float FlashColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
    shooter.DrawModelPrimitive(renderer, camera, 1,
        ToWorldX(enemy.attackWarningTargetX), ToWorldY(enemy.attackWarningTargetY),
        PlayerRailZ + 0.3f, size, size, size, FlashColor);
    return true;
}

/**
 * @brief EASTSOURCEと外壁警備ドローンの専用モデルを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param enemy 描画する敵
 * @return 専用モデルを描画して共通敵モデルを省略する場合true
 */
bool SideScrollingShooter::Stage5Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy) {
    if (enemy.type == Stage::WallSecurityDroneEnemy) {
        // サーチライトと機関銃を走査地点または接触時に固定した地点へ向ける
        WallSecurityDronePose pose;
        pose.sensorYaw = (std::clamp)((enemy.turretAimX - enemy.x) * 0.72f, -0.68f, 0.68f);
        pose.sensorPitch = (std::clamp)((enemy.turretAimY - enemy.y) * 0.34f, -0.32f, 0.32f);
        pose.searchLightYaw = pose.sensorYaw;
        pose.searchLightPitch = pose.sensorPitch;
        pose.machineGunYaw = pose.sensorYaw;
        pose.machineGunPitch = pose.sensorPitch;
        pose.machineGunDeployment = SmoothStep(Math::Clamp01(
            (EnemyRailFarZ - enemy.z) / 12.0f));
        pose.contactExtension = pose.machineGunDeployment;
        pose.warningLightColor = enemy.motionAge > 0 ?
            WallSecurityDroneModelView::AttackWarning : (enemy.recoilAge > 0 ?
                WallSecurityDroneModelView::DetectedWarning :
                WallSecurityDroneModelView::PatrolWarning);
        const Stage5ModelTransform transform {
            {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z}, {},
            1.45f * ShooterStages::Stage5::Part2EnemyScaleMultiplier(shooter.RailBlend())};
        WallSecurityDroneModelView::DrawAll(transform, pose,
            [&](PrimitiveShape shape, const Matrix4x4& world,
                const ColorF& color, WallSecurityDronePartGroup) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });

        // 巡回中に壁面を擦る磁着パッドから短命な火花を交互に散らす
        constexpr int SparkCycleFrames = 8;
        const int sparkContact = (enemy.age / SparkCycleFrames) & 3;
        const float sparkProgress = static_cast<float>(enemy.age % SparkCycleFrames) /
            static_cast<float>(SparkCycleFrames);
        const Vector3 sparkPosition = Stage5ModelDetail::Matrix(transform).TransformPoint(
            WallSecurityDroneModelView::WallContactLocalPosition(
                (sparkContact & 1) == 0 ? -1.0f : 1.0f,
                (sparkContact & 2) == 0 ? -1.0f : 1.0f,
                pose.contactExtension));
        const float sparkSize = 0.34f + sparkProgress * 0.42f;
        const Matrix4x4 sparkWorld = Matrix4x4::Translation(sparkPosition) *
            Matrix4x4::Scale({sparkSize, sparkSize, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * sparkWorld,
            sparkProgress});

        // ビル上空から壁面へ到着するまではレーザーポインターを停止する
        if (enemy.entersWallFromTop) return true;

        // 灯体レンズから走査地点まで細い赤色光軸と着弾点を描画する
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);
        const Vector3 source = root.TransformPoint(
            WallSecurityDroneModelView::SearchLightOriginLocalPosition(
                pose.searchLightYaw, pose.searchLightPitch));
        const Vector3 target {ToWorldX(enemy.turretAimX), ToWorldY(enemy.turretAimY),
            shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ};
        const Vector3 delta = target - source;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float beamYaw = std::atan2(direction.z, -direction.x);
        const float beamPitch = -std::asin(direction.y);
        const Matrix4x4 beamWorld = Matrix4x4::Translation(
            source + direction * (length * 0.5f)) *
            Matrix4x4::RotationY(beamYaw) * Matrix4x4::RotationZ(beamPitch) *
            Matrix4x4::Scale({length, enemy.motionAge > 0 ? 0.055f : 0.025f, 0.025f});
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box), beamWorld, DronePointerColor);
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Sphere), target.x, target.y, target.z,
            0.16f, 0.16f, 0.05f, DronePointerColor);
        return true;
    }
    if (enemy.type != Stage::BossEnemy) return false;

    const Stage5ModelTransform transform = EastsourceTransform(shooter, enemy);
    const EastsourceModelState state = EastsourceState(enemy);

    // 参照ブランチdrawBoss1の26パーツとXYZ回転を変更せず描画する
    EastsourceModelView::VisitParts(transform, state,
        [&](PrimitiveShape shape, const Matrix4x4& world,
            const ColorF& color, EastsourcePartGroup) {
            const float partColor[] = {color.r, color.g, color.b, color.a};
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(shape), world, partColor);
        });

    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };
    for (int part = BossNose; part <= BossRightEngine; ++part) {
        const int maxHp = enemy.bossPartMaxHp[part];
        if (maxHp <= 0 || enemy.bossPartHp[part] <= 0 ||
            enemy.bossPartHp[part] * 100 > maxHp * 35) continue;
        const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
            transform, state, Groups[part]);
        if (!bounds.valid) continue;
        const Matrix4x4 world = Matrix4x4::Translation(bounds.center) *
            Matrix4x4::Scale({0.82f, 0.82f * 1.7f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
            static_cast<float>(enemy.age) / 30.0f + static_cast<float>(part) * 0.37f, 1});
    }
    return true;
}

/**
 * @brief EASTSOURCEの破壊部位を専用デブリへ変換する
 * @param shooter 更新対象
 * @param enemy デブリ生成元
 * @param bossPart 破壊部位
 * @return 専用デブリを生成して共通デブリ生成を省略する場合true、共通デブリ生成を使用する場合false
 */
bool SideScrollingShooter::Stage5Module::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& enemy, int bossPart) {
    if (enemy.type != Stage::BossEnemy) return false;

    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };
    const EastsourcePartGroup detached = bossPart >= BossNose && bossPart <= BossRightEngine ?
        Groups[bossPart] : EastsourcePartGroup::Body;
    EastsourceModelState intact;
    const Stage5ModelTransform transform = EastsourceTransform(shooter, enemy);
    int pieceNumber = 0;

    // 破壊グループの実モデルパーツだけを既存の小型Debrisプールへ送る
    EastsourceModelView::VisitParts(transform, intact,
        [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color,
            EastsourcePartGroup group) {
            if (group != detached) return;
            const Vector3 center = world.TransformPoint(Vector3::Zero);
            const float radius = (std::max)(0.24f,
                (std::min)(1.2f, Stage5ModelDetail::WorldPartRadius(world) * 0.55f));
            const float direction = center.x < ToWorldX(enemy.x) ? -1.0f : 1.0f;
            const float pieceColor[] = {color.r, color.g, color.b, color.a};
            const int debrisShape = shape == PrimitiveShape::Box ? 1 :
                (shape == PrimitiveShape::Cylinder ? 2 :
                    (shape == PrimitiveShape::Cone ? 3 :
                        (shape == PrimitiveShape::Prism ? 4 : 5)));
            shooter.SpawnDebrisPiece(center.x, center.y, center.z,
                direction * (0.035f + static_cast<float>(pieceNumber % 3) * 0.008f),
                0.018f + static_cast<float>(pieceNumber % 2) * 0.012f,
                -0.025f + static_cast<float>(pieceNumber % 3) * 0.018f,
                0.0f, direction * 0.10f, debrisShape,
                radius, radius * 0.65f, radius, pieceColor, 90, 64, false);
            ++pieceNumber;
        });
    return true;
}

/**
 * @brief Stage5ではStage4の約2倍へ拡大した専用ビル群を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawCityBuildings(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    if (ShooterStages::Stage5::IsRooftopPhase(shooter.m_stage5.phase)) return;

    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideHalfHeight =
        (SideBackgroundZ - SideScrollingShooterShared::SideCameraZ) *
        std::tan(Math::ToRadians(SideScrollingShooterShared::SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideHalfWidth = sideHalfHeight * renderer.AspectRatio();
    const float buildingScale = shooter.m_stageNumber == 5 ? Stage5CityBuildingScale : 1.0f;
    const int buildingCount = shooter.m_stageNumber == 5 ?
        Stage5CityBuildingCount : CityBuildingCount;
    const float buildingSpacing = shooter.m_stageNumber == 5 ?
        2.0f / Stage5CityBuildingCount : CityBuildingNdcSpacing;

    // Stage5は2倍モデル7種を2巡させ、形状の大きさを保ったまま街並みを詰める
    for (int index = 0; index < buildingCount; ++index) {
        const bool leftSide = index % 2 == 0;
        const float sideX = WrapCityNdcX(
            index * buildingSpacing - shooter.m_scroll * 0.18f) *
            (sideHalfWidth + 2.0f);
        const float sideWidth = 3.65f + static_cast<float>((index * 11) % 3) * 0.38f;
        const float sideHeight = 3.8f + static_cast<float>((index * 17) % 5) * 1.18f;
        const float railHeight = 9.0f + static_cast<float>((index * 17) % 5) * 2.4f;
        const float x = Math::Lerp(sideX, leftSide ? -18.0f : 18.0f, railWeight);
        const float groundY = Math::Lerp(-6.0f, -3.65f, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.7f,
            10.0f + WrapCityDistance(
                static_cast<float>(index * 29) - shooter.m_scroll * 36.0f, 100.0f),
            railWeight);
        const Stage5BuildingType building = static_cast<Stage5BuildingType>(
            index % static_cast<int>(Stage5BuildingType::Count));
        const Vector3 modelSize = Stage5CityModelView::ModelSize(building);
        const float railScale = railHeight * buildingScale / modelSize.y;
        const Vector3 modelScale {
            Math::Lerp(sideWidth * buildingScale / modelSize.x, railScale, railWeight),
            Math::Lerp(sideHeight * buildingScale / modelSize.y, railScale, railWeight),
            Math::Lerp(0.42f * buildingScale / modelSize.z, railScale, railWeight)
        };

        // 1部から2部へのムービー中は横道路と交差する市街ビルを除外する
        constexpr float CrossRoadNearZ = 24.0f;
        constexpr float CrossRoadFarZ = 42.0f;
        constexpr float CrossRoadClearance = 4.0f;
        const float modelHalfDepth = modelSize.z * modelScale.z * 0.5f;
        if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition &&
            z + modelHalfDepth > CrossRoadNearZ - CrossRoadClearance &&
            z - modelHalfDepth < CrossRoadFarZ + CrossRoadClearance) {
            continue;
        }

        const Matrix4x4 root = Matrix4x4::Translation({x, groundY, z}) *
            Matrix4x4::Scale(modelScale);

        // 既存の暗紺壁とシアン／マゼンタ系アクセントを保ったまま形状だけ差し替える
        Stage5CityModelView::VisitBuilding(building, root,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });

        // 密度を崩さないよう3棟ごとに一つだけ独立広告をマウントする
        if (index % 3 != 0) continue;
        const Stage5SignMount mount = Stage5CityModelView::SignMount(building, 0);
        const Matrix4x4 adWorld = root * Stage5ModelDetail::Matrix(mount.transform);
        const Stage5AdType ad = static_cast<Stage5AdType>(
            (index / 3) % static_cast<int>(Stage5AdType::Count));
        Stage5CityModelView::VisitAd(ad, adWorld,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });
    }
}

/**
 * @brief 第2部道中のNEO AIZU市街とPANDD会ビルを描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawPart2Background(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    constexpr float GroundTopY = -3.65f;
    constexpr float BuildingZ = 45.0f;
    constexpr float EdgeColor[] = {0.20f, 0.34f, 0.48f, 1.0f};
    const float weight = SmoothStep(Math::Clamp01(railWeight));
    const float moveWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f));
    const float expandWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f - 1.0f));
    const float sideHalfHeight =
        (SideBackgroundZ - SideScrollingShooterShared::SideCameraZ) *
        std::tan(Math::ToRadians(SideScrollingShooterShared::SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideHalfWidth = sideHalfHeight * renderer.AspectRatio();
    const int routeFrame = ShooterStages::Stage5::Part2RouteElapsedFrames(
        shooter.m_stage5.phase, shooter.m_stage5.phaseTimer);
    const int wallTier = WallTier(shooter.m_stage5.phase);
    const float* facadeColor = wallTier == 0 ? WallLowerFacadeColor :
        (wallTier == 1 ? WallMiddleFacadeColor : WallUpperFacadeColor);
    const float* accentColor = wallTier == 0 ? WallLowerAccentColor :
        (wallTier == 1 ? WallMiddleAccentColor : WallUpperAccentColor);
    const float* lightColor = wallTier == 0 ? WallLowerLightColor :
        (wallTier == 1 ? WallMiddleLightColor : WallUpperLightColor);

    // ムービー終端と同じ嵐空を道中でも継続する
    DrawPart2StormSky(shooter, renderer, camera);

    // 2Dでは右三分の一、3Dでは画面中央奥へ同じ外壁を連続移動する
    const float sideBuildingWidth = sideHalfWidth * 0.68f;
    const float sideBuildingHeight = sideHalfHeight * 2.18f;
    const float sideBuildingX = sideHalfWidth - sideBuildingWidth * 0.5f;
    constexpr float RailBuildingBottomY = -36.0f;
    const float compactHeight = Math::Lerp(sideBuildingHeight, 32.0f, moveWeight);
    const float expandedHeight = ShooterStages::Stage5::PandDBuildingHeight +
        GroundTopY - RailBuildingBottomY;
    const float buildingHeight = Math::Lerp(compactHeight,
        expandedHeight, expandWeight);
    const float compactY = Math::Lerp(0.0f, GroundTopY + 16.0f, moveWeight);
    const float buildingY = Math::Lerp(compactY,
        RailBuildingBottomY + expandedHeight * 0.5f, expandWeight);
    const float buildingX = Math::Lerp(sideBuildingX, 0.0f, moveWeight);
    const float buildingZ = Math::Lerp(SidePlaneZ + 12.8f, BuildingZ, moveWeight);
    const float buildingWidth = Math::Lerp(sideBuildingWidth,
        ShooterStages::Stage5::PandDBuildingWidth, weight);
    const float buildingDepth = Math::Lerp(0.55f, 6.0f, moveWeight);
    // 超巨大部分は単純なビル躯体とし、新規モデルを縦に引き延ばさない
    shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
        buildingX, buildingY, buildingZ,
        buildingWidth, buildingHeight, buildingDepth, facadeColor);

    // PANDD会建造物は元の縦横比のまま超巨大ビルの屋上へ載せる
    const Vector3 capSize = TayamaModelView::TowerSize *
        ShooterStages::Stage5::PandDBuildingCapScale;
    const Matrix4x4 capRoot = TayamaModelView::BuildingRoot(
        buildingX, buildingY + buildingHeight * 0.5f, buildingZ, capSize);
    TayamaModelView::VisitParts(capRoot, 0.0f, TayamaModelState {},
        [&](PrimitiveShape shape, const Matrix4x4& world,
            const ColorF& color, TayamaPartGroup) {
            const float partColor[] = {color.r, color.g, color.b, color.a};
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(shape), world, partColor);
        });

    // 外壁両端の縦線で3D時に上方へ収束する輪郭を強調する
    for (int side = -1; side <= 1; side += 2) {
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            buildingX + static_cast<float>(side) * buildingWidth * 0.495f,
            buildingY, buildingZ - buildingDepth * 0.51f,
            Math::Lerp(0.12f, 0.42f, weight), buildingHeight,
            Math::Lerp(0.08f, 0.22f, weight), EdgeColor);
    }

    // 窓列を高速で上から下へ循環させ、外壁に沿った上昇速度を見せる
    constexpr int WindowRowCount = 12;
    const float sideWindowSpacing = sideHalfHeight * 0.24f;
    const float sideWindowScroll = std::fmod(
        static_cast<float>(routeFrame) *
            ShooterStages::Stage5::Part2SideSceneryFallSpeed,
        sideWindowSpacing);
    constexpr float RailWindowSpacing = 12.0f;
    constexpr int WindowStartRowOffset = 4;
    const float railWindowScroll = std::fmod(
        static_cast<float>(routeFrame) *
            ShooterStages::Stage5::Part2RailSceneryFallSpeed,
        RailWindowSpacing);
    for (int row = 0; row < WindowRowCount; ++row) {
        for (int column = 0;
            column < ShooterStages::Stage5::PandDBuildingWindowColumns; ++column) {
            const float columnOffset =
                (static_cast<float>(column) /
                    static_cast<float>(ShooterStages::Stage5::PandDBuildingWindowColumns - 1) -
                    0.5f) * 0.75f;
            const float sideWindowX = sideBuildingX + columnOffset * sideBuildingWidth;
            const float sideWindowY = -sideHalfHeight - sideWindowSpacing +
                static_cast<float>(row + WindowStartRowOffset) * sideWindowSpacing - sideWindowScroll;
            const float railWindowX = columnOffset *
                ShooterStages::Stage5::PandDBuildingWidth;
            const float railWindowY = GroundTopY - RailWindowSpacing +
                static_cast<float>(row + WindowStartRowOffset) * RailWindowSpacing - railWindowScroll;
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                Math::Lerp(sideWindowX, railWindowX, weight),
                Math::Lerp(sideWindowY, railWindowY, weight),
                Math::Lerp(SidePlaneZ + 12.45f, 41.82f, weight),
                Math::Lerp(sideBuildingWidth * 0.10f,
                    ShooterStages::Stage5::PandDBuildingWidth * 0.08f, weight),
                Math::Lerp(0.32f, 0.52f, weight),
                Math::Lerp(0.08f, 0.18f, weight), lightColor);
        }
    }

    // 階層ごとに真鍮帯、深紅の組格子、黒漆と金の紋章へ豪華さを増す
    constexpr int OrnamentRowCount = 6;
    const float sideOrnamentSpacing = sideHalfHeight * 0.48f;
    const float sideOrnamentScroll = std::fmod(
        static_cast<float>(routeFrame) *
            ShooterStages::Stage5::Part2SideSceneryFallSpeed,
        sideOrnamentSpacing);
    constexpr float RailOrnamentSpacing = 24.0f;
    const float railOrnamentScroll = std::fmod(
        static_cast<float>(routeFrame) *
            ShooterStages::Stage5::Part2RailSceneryFallSpeed,
        RailOrnamentSpacing);
    for (int row = 0; row < OrnamentRowCount; ++row) {
        const float sideY = -sideHalfHeight - sideOrnamentSpacing +
            static_cast<float>(row) * sideOrnamentSpacing - sideOrnamentScroll;
        const float railY = GroundTopY - RailOrnamentSpacing +
            static_cast<float>(row) * RailOrnamentSpacing - railOrnamentScroll;
        const float ornamentY = Math::Lerp(sideY, railY, weight);
        const float ornamentZ = Math::Lerp(SidePlaneZ + 12.38f, 41.70f, weight);

        // 全階層を貫く横帯は上層ほど太く明るくする
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            buildingX, ornamentY, ornamentZ,
            buildingWidth * 0.92f,
            Math::Lerp(0.10f + wallTier * 0.05f, 0.30f + wallTier * 0.12f, weight),
            Math::Lerp(0.10f, 0.24f, weight), accentColor);

        // 中層は菱形の組格子、上層は金縁の代紋を左右対称に配置する
        if (wallTier == 0) continue;
        for (int side = -1; side <= 1; side += 2) {
            const float sideX = sideBuildingX + static_cast<float>(side) *
                sideBuildingWidth * 0.27f;
            const float railX = static_cast<float>(side) *
                ShooterStages::Stage5::PandDBuildingWidth * 0.28f;
            const float emblemSize = wallTier == 1 ? 0.72f : 1.05f;
            for (int diagonal = -1; diagonal <= 1; diagonal += 2) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    {Math::Lerp(sideX, railX, weight), ornamentY, ornamentZ - 0.06f},
                    {Math::Lerp(emblemSize, emblemSize * 3.2f, weight),
                        Math::Lerp(0.08f, 0.20f, weight),
                        Math::Lerp(0.08f, 0.20f, weight)},
                    {0.0f, 0.0f, static_cast<float>(diagonal) * Math::Pi * 0.25f},
                    accentColor);
            }
            if (wallTier < 2) continue;
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                Math::Lerp(sideX, railX, weight), ornamentY, ornamentZ - 0.14f,
                Math::Lerp(0.32f, 1.10f, weight),
                Math::Lerp(0.32f, 1.10f, weight),
                Math::Lerp(0.10f, 0.28f, weight), lightColor);
        }
    }
}

/**
 * @brief 第1部から第2部へのムービーと第2部道中へ嵐雲と星を描画する
 * @param shooter 描画対象
 * @param renderer 描画先
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawPart2StormSky(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    constexpr float StarColor[] = {0.62f, 0.76f, 1.0f, 0.92f};
    constexpr float CloudColor[] = {0.045f, 0.060f, 0.11f, 0.84f};

    // 星はカメラ前方の視錐台へ決定的に分散して視点旋回中も空を覆う
    for (int index = 0; index < Part2StarCount; ++index) {
        const std::uint32_t depthHash = ShooterStages::Stage5::WallWaveHash(index, 307);
        const std::uint32_t lateralHash = ShooterStages::Stage5::WallWaveHash(index, 101);
        const std::uint32_t verticalHash = ShooterStages::Stage5::WallWaveHash(index, 211);
        const float depth = 190.0f + static_cast<float>(depthHash % 61u);
        const float halfHeight = depth * std::tan(camera.FieldOfView() * 0.5f);
        const float halfWidth = halfHeight * renderer.AspectRatio();
        const float lateral = (-0.96f + static_cast<float>(lateralHash & 0xffffu) /
            65535.0f * 1.92f) * halfWidth;
        const float vertical = (-0.82f + static_cast<float>(verticalHash & 0xffffu) /
            65535.0f * 1.68f) * halfHeight;
        const Vector3 position = camera.Position() + camera.Forward() * depth +
            camera.Right() * lateral + camera.Up() * vertical;
        const float size = index % 11 == 0 ? 0.36f : 0.18f;
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            position, {size, size, size * 0.32f}, {}, StarColor);
    }

    // 厚い雲群を左右へ流し、三つのCubeで荒れた輪郭を作る
    for (int cloud = 0; cloud < Part2CloudCount; ++cloud) {
        const std::uint32_t depthHash = ShooterStages::Stage5::WallWaveHash(cloud, 401);
        const std::uint32_t lateralHash = ShooterStages::Stage5::WallWaveHash(cloud, 503);
        const std::uint32_t verticalHash = ShooterStages::Stage5::WallWaveHash(cloud, 601);
        const float depth = 145.0f + static_cast<float>(depthHash % 46u);
        const float halfHeight = depth * std::tan(camera.FieldOfView() * 0.5f);
        const float halfWidth = halfHeight * renderer.AspectRatio();
        const float start = static_cast<float>(lateralHash & 0xffffu) / 65535.0f *
            halfWidth * 2.4f;
        const float travel = WrapCityDistance(start +
            static_cast<float>(shooter.m_frame) * 0.055f, halfWidth * 2.4f);
        const float lateral = -halfWidth * 1.2f + travel;
        const float vertical = (0.12f + static_cast<float>(verticalHash & 0xffffu) /
            65535.0f * 0.72f) * halfHeight;
        const Vector3 center = camera.Position() + camera.Forward() * depth +
            camera.Right() * lateral + camera.Up() * vertical;
        for (int lobe = 0; lobe < 3; ++lobe) {
            const float offset = static_cast<float>(lobe - 1);
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                center + camera.Right() * offset * (2.8f + cloud % 3) +
                    camera.Up() * (lobe == 1 ? 0.8f : 0.0f),
                {lobe == 1 ? 22.0f : 16.5f, lobe == 1 ? 7.2f : 5.4f,
                    10.0f + static_cast<float>(cloud % 4) * 2.0f}, {}, CloudColor);
        }
    }
}

/**
 * @brief 終幕ムービーでビル背後から上昇する頭部なしのメカ龍を描画する
 * @param shooter 描画対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawFinalEscapeDragon(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    if (shooter.m_stage5.phase != Stage5Phase::TayamaCollapse) return;
    const float progress = SmoothStep(ShooterStages::Stage5::FinalEscapeProgress(
        shooter.m_stage5.phaseTimer,
        ShooterStages::Stage5::FinalEscapeDragonStartFrames,
        ShooterStages::Stage5::FinalEscapeDragonDurationFrames));
    if (progress <= 0.0f) return;

    constexpr int SegmentCount = 26;
    constexpr float BodyColor[] = {0.36f, 0.18f, 0.025f, 1.0f};
    constexpr float ArmorColor[] = {0.95f, 0.56f, 0.08f, 1.0f};
    constexpr float EdgeColor[] = {1.55f, 0.92f, 0.16f, 1.0f};
    constexpr float CoreColor[] = {1.80f, 0.62f, 0.06f, 1.0f};
    const auto SegmentPosition = [&](int index) {
        const float distance = static_cast<float>(index) * 8.6f;
        const float wave = progress * 4.4f - static_cast<float>(index) * 0.46f;
        return Vector3 {
            std::sin(wave) * (18.0f + static_cast<float>(index % 3)),
            -70.0f + progress * 640.0f - distance,
            92.0f + std::cos(wave * 0.72f) * 9.0f +
                static_cast<float>(index) * 0.52f
        };
    };

    // 箱型の装甲節と球形関節を連ね、先頭には頭部でなく露出した接続環だけを置く
    for (int index = 0; index < SegmentCount; ++index) {
        const Vector3 head = SegmentPosition(index);
        const Vector3 tail = SegmentPosition(index + 1);
        const Vector3 delta = tail - head;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float yaw = std::atan2(direction.z, -direction.x);
        const float pitch = -std::asin(direction.y);
        const float segmentWave = progress * 4.4f - static_cast<float>(index) * 0.46f;
        const float thickness = 5.2f + static_cast<float>((index * 5) % 4) * 0.55f;
        const Matrix4x4 bodyWorld = Matrix4x4::Translation((head + tail) * 0.5f) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale({length, thickness, thickness});
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box), bodyWorld, BodyColor);
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Sphere), head,
            {thickness * 1.12f, thickness * 1.12f, thickness * 1.12f}, {}, ArmorColor);

        // 側面装甲と発光節を一定間隔で置き、巨大な機械構造として輪郭を読ませる
        if (index % 2 == 0) {
            for (int side = -1; side <= 1; side += 2) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    head + Vector3 {static_cast<float>(side) * thickness * 0.95f, 0.0f, 0.0f},
                    {thickness * 0.78f, 1.1f, thickness * 1.35f},
                    {0.0f, segmentWave * 0.08f, static_cast<float>(side) * 0.34f}, EdgeColor);
            }
        }
        if (index % 3 == 0) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Sphere),
                head + Vector3 {0.0f, 0.0f, -thickness * 0.62f},
                {1.2f, 1.2f, 0.55f}, {}, CoreColor);
        }
    }

    const Vector3 collar = SegmentPosition(0);
    shooter.DrawModelPrimitive(renderer, camera,
        static_cast<int>(PrimitiveShape::Cylinder), collar,
        {9.0f, 3.0f, 9.0f}, {}, EdgeColor);
    shooter.DrawModelPrimitive(renderer, camera,
        static_cast<int>(PrimitiveShape::Cylinder), collar + Vector3 {0.0f, 1.1f, 0.0f},
        {5.8f, 3.5f, 5.8f}, {}, CoreColor);
}

/**
 * @brief 終幕ムービー後に待機する雲海を描画する
 * @param shooter 描画対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawCloudSea(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    constexpr int CloudCount = 240;
    constexpr float StarColor[] = {0.72f, 0.82f, 1.0f, 1.0f};
    constexpr float CloudShadow[] = {0.20f, 0.20f, 0.32f, 1.0f};
    constexpr float CloudTop[] = {0.46f, 0.38f, 0.45f, 1.0f};
    constexpr float CloudLight[] = {0.78f, 0.62f, 0.45f, 1.0f};
    const float travel = CloudSeaTravelOffset(shooter.m_frame);
    const float dawn = TayamaDragonDawnProgress(shooter.m_stage5.phase,
        shooter.m_stage5.tayamaHp, shooter.m_stage5.tayamaMaxHp);

    // 夜空の星を視錐台へ決定的に分散し、朝になるにつれて透明にする
    if (dawn < 0.99f) {
        float starColor[4] = {StarColor[0], StarColor[1], StarColor[2], 1.0f - dawn};
        for (int index = 0; index < CloudSeaStarCount; ++index) {
            const std::uint32_t depthHash = ShooterStages::Stage5::WallWaveHash(index, 719);
            const std::uint32_t lateralHash = ShooterStages::Stage5::WallWaveHash(index, 823);
            const std::uint32_t verticalHash = ShooterStages::Stage5::WallWaveHash(index, 929);
            const float depth = 180.0f + static_cast<float>(depthHash % 71u);
            const float halfHeight = depth * std::tan(camera.FieldOfView() * 0.5f);
            const float halfWidth = halfHeight * renderer.AspectRatio();
            const float lateral = (-0.96f + static_cast<float>(lateralHash & 0xffffu) /
                65535.0f * 1.92f) * halfWidth;
            const float vertical = (-0.18f + static_cast<float>(verticalHash & 0xffffu) /
                65535.0f * 1.12f) * halfHeight;
            const Vector3 position = camera.Position() + camera.Forward() * depth +
                camera.Right() * lateral + camera.Up() * vertical;
            const float size = index % 13 == 0 ? 0.38f : 0.19f;
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box), position,
                {size, size, size * 0.32f}, {}, starColor);
        }
    }

    // 大型Cubeを重なる密度で散らし、循環させながらカメラから奥の+Z方向へ流す
    for (int index = 0; index < CloudCount; ++index) {
        const float x = -110.0f +
            static_cast<float>((index * 101) % 257) / 256.0f * 220.0f;
        const float baseZ = static_cast<float>((index * 97) % 251) / 250.0f *
            CloudSeaDepth;
        const float z = 2.0f + WrapCityDistance(baseZ + travel, CloudSeaDepth);
        const float y = -8.6f + static_cast<float>(index % 7) * 0.34f;
        const float width = 18.0f + static_cast<float>((index * 7) % 11);
        const float depth = 15.0f + static_cast<float>((index * 11) % 9);
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box), {x, y - 0.65f, z},
            {width * 1.16f, 2.8f, depth * 1.16f}, {}, CloudShadow);
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box), {x, y, z},
            {width, 3.2f, depth}, {}, index % 4 == 0 ? CloudLight : CloudTop);
    }

    // 遠景の連続帯で青空との境界を柔らかい雲の地平線として閉じる
    for (int bank = -4; bank <= 4; ++bank) {
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box),
            {static_cast<float>(bank) * 28.0f, 0.5f, 155.0f},
            {32.0f, 8.0f + static_cast<float>((bank + 4) % 3) * 2.0f, 18.0f},
            {}, CloudLight);
    }
}

/**
 * @brief 雲海のTAYAMA龍第2形態を共有座標列から描画する
 * @param shooter 描画対象
 * @param renderer 描画先レンダラー
 * @param camera 現在のカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawTayamaDragon(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float BodyColor[] = {0.10f, 0.12f, 0.16f, 1.0f};
    constexpr float ArmorColor[] = {0.28f, 0.32f, 0.38f, 1.0f};
    constexpr float EdgeColor[] = {0.48f, 0.56f, 0.64f, 1.0f};
    constexpr float CoreColor[] = {0.08f, 0.72f, 1.0f, 1.0f};
    constexpr float HitColor[] = {1.0f, 0.18f, 0.10f, 1.0f};
    const bool collapsing = shooter.m_stage5.phase == Stage5Phase::TayamaDragonCollapse;
    const int destroyed = collapsing ?
        ShooterStages::Stage5::TayamaDragonDestroyedSegmentCount(
            shooter.m_stage5.phaseTimer) : 0;
    const int visibleCount = ShooterStages::Stage5::TayamaDragonSegmentCount - destroyed;
    const bool hitFlash = shooter.m_stage5.tayamaDragonHitFlashFrames > 0 &&
        (shooter.m_stage5.tayamaDragonHitFlashFrames / 2) % 2 != 0;
    const int attackTimeline = ShooterStages::Stage5::TayamaDragonAttackTimeline(
        shooter.m_stage5.tayamaDragonAttack, shooter.m_stage5.tayamaDragonAttackTimer);
    const int romanceFrame = shooter.m_stage5.tayamaDragonAttack ==
        ShooterStages::Stage5::TayamaDragonAttack::RomanceCannon ?
        ShooterStages::Stage5::TayamaDragonRomanceCannonFrame(attackTimeline) : -1;
    const float rushWarning = shooter.m_stage5.tayamaDragonAttack ==
        ShooterStages::Stage5::TayamaDragonAttack::Rush ?
        ShooterStages::Stage5::TayamaDragonRushWarningProgress(attackTimeline) : 0.0f;

    // ステージ3と同じ反射ファンネルモデルを自機周囲へ描画する
    for (const auto& state : shooter.m_stage5.tayamaReflectFunnels) {
        if (!state.active) continue;
        const Vector3 position {ToWorldX(state.x), ToWorldY(state.y), state.z};
        const Vector3 target = shooter.PlayerWorldPosition();
        const Vector3 delta = target - position;
        const float horizontal = (std::max)(0.001f,
            std::sqrt(delta.x * delta.x + delta.z * delta.z));
        const BossModelTransform transform {position, {}, 0.0f, 1.6f};
        auto DrawPart = [&](int shape, const Vector3& partPosition,
            const Vector3& scale, const float color[4], float yaw, float pitch) {
            shooter.DrawModelPrimitive(renderer, camera, shape,
                partPosition.x, partPosition.y, partPosition.z,
                scale.x, scale.y, scale.z, color, yaw, pitch);
        };
        const float spin = state.spinFrames > 0 ? Math::TwoPi * 2.0f *
            static_cast<float>(30 - state.spinFrames) / 29.0f : 0.0f;
        Stage3FunnelModelView::DrawReflectShot(transform,
            std::atan2(delta.z, -delta.x) + spin,
            -std::atan2(delta.y, horizontal), 0.0f, DrawPart);
    }

    // 箱型装甲と球形関節を共有節座標で連結し、視点切替中も龍の形を連続させる
    for (int index = 0; index < visibleCount; ++index) {
        const Vector3 head = TayamaDragonSegmentPosition(shooter, index, railWeight);
        const Vector3 tail = TayamaDragonSegmentPosition(shooter,
            (std::min)(index + 1, ShooterStages::Stage5::TayamaDragonSegmentCount - 1),
            railWeight);
        const Vector3 delta = tail - head;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float yaw = std::atan2(direction.z, -direction.x);
        const float pitch = -std::asin(direction.y);
        const float radius = TayamaDragonSegmentRadius(index, railWeight);
        const Matrix4x4 bodyWorld = Matrix4x4::Translation((head + tail) * 0.5f) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale({length, radius * 1.45f, radius * 1.45f});
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box), bodyWorld,
            hitFlash ? HitColor : BodyColor);
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Sphere), head,
            {radius * TayamaModelView::DragonJointDiameterScale,
                radius * TayamaModelView::DragonJointDiameterScale,
                radius * TayamaModelView::DragonJointDiameterScale}, {},
            hitFlash ? HitColor : ArmorColor);

        // 装甲板と発光コアを節ごとにずらして機械龍の密度を作る
        if (index % 2 == 0) {
            for (int side = -1; side <= 1; side += 2) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    head + Vector3 {static_cast<float>(side) * radius * 0.92f, 0.0f, 0.0f},
                    {radius * 0.72f, radius * 0.34f, radius * 1.28f},
                    {0.0f, static_cast<float>(shooter.m_frame) * 0.004f,
                        static_cast<float>(side) * 0.34f}, EdgeColor);
            }
        }
        if (index % 3 == 0) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Sphere),
                head + Vector3 {0.0f, 0.0f, -radius * 0.64f},
                {radius * 0.34f, radius * 0.34f, radius * 0.16f}, {}, CoreColor);
        }
    }

    // 発射する胴体節を予告中から膨張発光させ、発射後は滑らかに収束させる
    if (shooter.m_stage5.phase == Stage5Phase::TayamaDragonBattle &&
        shooter.m_stage5.tayamaDragonAttack ==
            ShooterStages::Stage5::TayamaDragonAttack::BodyBarrage) {
        constexpr int Sources[] = {4, 10, 16, 22};
        const float glow = SmoothStep(ShooterStages::Stage5::TayamaDragonBarrageGlow(
            attackTimeline));
        if (glow > 0.0f) {
            const int volley = shooter.m_stage5.attackTimer /
                ShooterStages::Stage5::TayamaDragonBarrageIntervalFrames;
            const Vector3 source = TayamaDragonSegmentPosition(shooter,
                Sources[volley % static_cast<int>(std::size(Sources))], railWeight);
            const float radius = 1.2f * glow;
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Sphere), source,
                {radius, radius, radius}, {}, CoreColor);
        }
    }

    // 頭部の分離中だけ露出した首先端へStage4のロマン砲を接続する
    if (!collapsing && romanceFrame >= 0) {
        const Vector3 muzzleBase = TayamaDragonSegmentPosition(shooter, 0, railWeight);
        const Vector3 target = shooter.PlayerWorldPosition();
        const Vector3 direction = (target - muzzleBase).Normalized();
        BossModelTransform cannonTransform;
        cannonTransform.position = muzzleBase;
        cannonTransform.yaw = std::atan2(direction.z, -direction.x);
        cannonTransform.scale = Math::Lerp(0.22f, 0.42f, railWeight);
        Stage4MainWeaponPose pose;
        pose.barrelPitch = std::asin((std::clamp)(direction.y, -1.0f, 1.0f));
        if (romanceFrame >= ShooterStages::Stage5::TayamaDragonRomanceCannonFireFrame &&
            romanceFrame < ShooterStages::Stage5::TayamaDragonRomanceCannonFireFrame + 18) {
            pose.recoil = 1.0f - static_cast<float>(romanceFrame -
                ShooterStages::Stage5::TayamaDragonRomanceCannonFireFrame) / 18.0f;
        }
        Stage4BossModelView::DrawMainWeapon(Stage4MainWeaponType::RomanceCannon,
            cannonTransform, pose,
            [&](int shape, const Vector3& position, const Vector3& scale,
                const float color[4], float yaw, float pitch) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<PrimitiveShape>(shape), position.x, position.y, position.z,
                    scale.x, scale.y, scale.z, color, yaw, pitch);
            });
    }

    // 暗転復帰後に逃げたTAYAMA頭部を首の接続環へ移動して合体させる
    if (!collapsing || shooter.m_stage5.phaseTimer <
        ShooterStages::Stage5::TayamaDragonCollapseHeadExplosionFrame) {
        const Vector3 neck = TayamaDragonSegmentPosition(shooter, 0, railWeight);
        const float assembly = shooter.m_stage5.phase == Stage5Phase::CloudSea ?
            SmoothStep(ShooterStages::Stage5::CloudSeaAssemblyProgress(
                shooter.m_stage5.phaseTimer)) : 1.0f;
        Stage5ModelTransform headTransform = TayamaDragonHeadTransform(shooter, railWeight);
        const Vector3 detachedRoot = neck + Vector3 {-12.0f, 18.0f, -10.0f} -
            Vector3 {0.0f, 12.85f * headTransform.scale, 0.0f};
        headTransform.position = Vector3::Lerp(detachedRoot, headTransform.position, assembly);
        TayamaModelState headState;
        headState.visible.fill(false);
        headState.visible[static_cast<std::size_t>(TayamaPartGroup::Bridge)] = true;
        headState.hitFlash[static_cast<std::size_t>(TayamaPartGroup::Bridge)] = hitFlash;
        headState.headScaleMultiplier = TayamaModelView::DragonHeadScale;
        headState.goldenHeadAdornment = true;
        const int removedHeadParts = collapsing ?
            ShooterStages::Stage5::TayamaDragonDestroyedHeadPartCount(
                shooter.m_stage5.phaseTimer) : 0;
        std::size_t headPartIndex = 0;
        TayamaModelView::VisitParts(headTransform, 1.0f, headState,
            [&](PrimitiveShape shape, const Matrix4x4& world,
                const ColorF& color, TayamaPartGroup) {
                const std::size_t partIndex = headPartIndex++;
                if (!TayamaModelView::IsHeadPartVisibleAfterRemoval(
                    partIndex, removedHeadParts)) return;
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world,
                    hitFlash ? HitColor : &color.r);
            });

        // 突進前は両目を黄金色に発光させ、進行方向へ攻撃範囲を予告する
        if (rushWarning > 0.0f) {
            constexpr float EyeGold[] = {1.0f, 0.72f, 0.08f, 1.0f};
            const float pulse = 0.82f + 0.18f * std::sin(
                static_cast<float>(shooter.m_frame) * 0.32f);
            const float eyeRadius = Math::Lerp(0.16f, 0.52f,
                rushWarning) * pulse * headTransform.scale;
            const auto eyes = TayamaModelView::EyeWorldPositions(
                headTransform, TayamaModelView::DragonHeadScale);
            for (const Vector3& eye : eyes) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Sphere), eye,
                    {eyeRadius, eyeRadius, eyeRadius}, {}, EyeGold);
            }

            const Vector3 warningStart = (eyes[0] + eyes[1]) * 0.5f;
            const Vector3 warningOffset = Vector3::Lerp(
                {-ToWorldX(ShooterStages::Stage5::TayamaDragonRushSideDistance), 0.0f, 0.0f},
                {0.0f, 0.0f, -ShooterStages::Stage5::TayamaDragonRushRailDistance},
                Math::Clamp01(railWeight));
            shooter.DrawRailgunBeamBetween(renderer, camera, warningStart,
                warningStart + warningOffset, Math::Lerp(0.06f, 0.18f, rushWarning),
                pulse, 3);
        }
    }

    // 合体後の頭部レーザーを予告線と照射本体で描き分ける
    if (shooter.m_stage5.phase == Stage5Phase::TayamaDragonBattle &&
        shooter.m_stage5.headLaserArmed) {
        const int laserFrame = shooter.m_stage5.tayamaDragonAttackTimer %
            ShooterStages::Stage5::TayamaHeadLaserCycleFrames;
        const bool active = ShooterStages::Stage5::IsTayamaHeadLaserActive(
            attackTimeline);
        if (laserFrame < ShooterStages::Stage5::TayamaHeadLaserWarningFrames || active) {
            const float progress = static_cast<float>(laserFrame) /
                static_cast<float>(ShooterStages::Stage5::TayamaHeadLaserWarningFrames);
            const auto eyes = TayamaModelView::EyeWorldPositions(
                TayamaDragonHeadTransform(shooter, railWeight),
                TayamaModelView::DragonHeadScale);
            for (const Vector3& eye : eyes) {
                const Vector3 direction =
                    (shooter.m_stage5.headLaserTarget - eye).Normalized();
                const Vector3 end = eye + direction *
                    ShooterStages::Stage5::TayamaHeadLaserLength;
                if (active) {
                    const float pulse = static_cast<float>(shooter.m_frame % 12) / 12.0f;
                    const float widthProgress = SmoothStep(
                        ShooterStages::Stage5::TayamaDragonLaserWidthProgress(
                            attackTimeline));
                    const float width = Math::Lerp(0.16f,
                        ShooterStages::Stage5::TayamaHeadLaserHitRadius, widthProgress);
                    shooter.DrawRailgunBeamBetween(renderer, camera, eye, end,
                        width * 1.7f, pulse,
                        TayamaGoldenLaserGlowEffect);
                    shooter.DrawRailgunBeamBetween(renderer, camera, eye, end,
                        width, pulse,
                        TayamaGoldenLaserCoreEffect);
                } else {
                    shooter.DrawRailgunBeamBetween(renderer, camera, eye, end,
                        0.16f, progress, 3);
                }
            }
        }
    }
}

/**
 * @brief 雲海とTAYAMA龍を2Dと3Dの背景描画経路へ共通描画する
 * @param shooter 描画対象
 * @param renderer 描画先レンダラー
 * @param camera 現在のカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawCloudSeaWorld(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    if (!ShooterStages::Stage5::IsCloudSeaPhase(shooter.m_stage5.phase)) return;
    DrawCloudSea(shooter, renderer, camera);
    if (shooter.m_stage5.phase != Stage5Phase::EndingReady) {
        DrawTayamaDragon(shooter, renderer, camera, railWeight);
    }
}

/**
 * @brief Stage 5の要塞、照明、崩壊演出を3D空間へ描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawStageWorld3D(const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    if (ShooterStages::Stage5::IsCloudSeaPhase(shooter.m_stage5.phase)) {
        return;
    }

    const bool wallFacade = shooter.m_stage5.phase == Stage5Phase::WallClimbTransition;
    if (wallFacade) {
        // 半透明の嵐雲を先に描き、巨大ビルが必ず手前を覆うようにする
        DrawPart2StormSky(shooter, renderer, camera);

        // 外壁上昇用の超巨大ビル躯体を道路へ接地する
        constexpr float GroundTopY = -3.65f;
        constexpr float BuildingZ = 45.0f;
        constexpr float CrossRoadWidth = 180.0f;
        constexpr float CrossRoadDepth = 18.0f;
        constexpr float CrossRoadCenterZ = BuildingZ - 3.0f - CrossRoadDepth * 0.5f;
        constexpr float CrossRoadSurfaceY = GroundTopY + 0.11f;
        constexpr float CrossRoadColor[] = {0.105f, 0.125f, 0.19f, 1.0f};
        constexpr float CrossRoadLaneColor[] = {0.72f, 0.84f, 0.88f, 1.0f};
        constexpr float CrossRoadCenterColor[] = {0.92f, 0.72f, 0.22f, 1.0f};

        // 既存の縦方向道路を進入路にし、ビル前面へ片側2車線の横道路を接続する
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            0.0f, CrossRoadSurfaceY, CrossRoadCenterZ,
            CrossRoadWidth, 0.08f, CrossRoadDepth, CrossRoadColor);

        // 二重中央線と左右の破線で対向2車線ずつを示す
        for (int line = -1; line <= 1; line += 2) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                0.0f, CrossRoadSurfaceY + 0.045f,
                CrossRoadCenterZ + static_cast<float>(line) * 0.18f,
                CrossRoadWidth, 0.025f, 0.12f, CrossRoadCenterColor);
            for (int segment = 0; segment < 18; ++segment) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    -85.0f + static_cast<float>(segment) * 10.0f,
                    CrossRoadSurfaceY + 0.045f,
                    CrossRoadCenterZ + static_cast<float>(line) * 4.5f,
                    5.0f, 0.025f, 0.12f, CrossRoadLaneColor);
            }
        }

        // 手前路肩線は進入路幅だけ空けてT字路の接続口を示す
        constexpr float ApproachRoadWidth = 24.0f;
        constexpr float ShoulderHalfWidth =
            (CrossRoadWidth - ApproachRoadWidth) * 0.25f;
        for (int side = -1; side <= 1; side += 2) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                static_cast<float>(side) *
                    (ApproachRoadWidth * 0.5f + ShoulderHalfWidth),
                CrossRoadSurfaceY + 0.045f,
                CrossRoadCenterZ - CrossRoadDepth * 0.5f + 0.3f,
                ShoulderHalfWidth * 2.0f, 0.025f, 0.16f, CrossRoadLaneColor);
        }

        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            0.0f, GroundTopY + ShooterStages::Stage5::PandDBuildingHeight * 0.5f,
            BuildingZ, ShooterStages::Stage5::PandDBuildingWidth,
            ShooterStages::Stage5::PandDBuildingHeight, 6.0f, TowerFacadeColor);

        // エントランス上部へPANDD-KAIの金色発光サインを固定する
        constexpr float SignCenterY = GroundTopY + 9.0f;
        constexpr float SignPanelZ = BuildingZ - 3.25f;
        constexpr float SignGlyphZ = BuildingZ - 3.50f;
        constexpr float SignCellStep = 0.90f;
        constexpr float SignPanelColor[] = {0.028f, 0.016f, 0.006f, 1.0f};
        constexpr float SignFrameColor[] = {0.78f, 0.42f, 0.055f, 1.0f};
        shooter.DrawModelPrimitive(renderer, camera,
            static_cast<int>(PrimitiveShape::Box),
            0.0f, SignCenterY, SignPanelZ, 56.0f, 9.0f, 0.30f, SignPanelColor);

        // 真鍮枠と左右の菱形飾りで看板へ立体的な輪郭を付ける
        for (int side = -1; side <= 1; side += 2) {
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                0.0f, SignCenterY + static_cast<float>(side) * 4.25f,
                SignGlyphZ + 0.02f, 56.0f, 0.24f, 0.16f, SignFrameColor);
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                static_cast<float>(side) * 27.75f, SignCenterY,
                SignGlyphZ + 0.02f, 0.24f, 8.3f, 0.16f, SignFrameColor);
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box),
                {static_cast<float>(side) * 27.75f, SignCenterY, SignGlyphZ - 0.08f},
                {0.90f, 0.90f, 0.16f},
                {0.0f, 0.0f, Math::Pi * 0.25f}, WallUpperAccentColor);
        }

        // 5x7セルの琥珀色の光彩へ明るい金色の芯を重ねる
        constexpr float FirstCellX =
            -static_cast<float>(PandDBuildingSignCharacterCount * 6 - 2) *
                SignCellStep * 0.5f;
        const float signPulse = 0.92f +
            std::sin(static_cast<float>(shooter.m_frame) * 0.065f) * 0.08f;
        const float signGlowColor[] = {1.0f, 0.46f, 0.025f, 0.38f * signPulse};
        const float signCoreColor[] = {1.0f, 0.76f * signPulse, 0.18f, 1.0f};
        for (int character = 0;
            character < PandDBuildingSignCharacterCount; ++character) {
            for (int row = 0; row < 7; ++row) {
                const unsigned char bits = PandDBuildingSignGlyphs[character][row];
                for (int column = 0; column < 5; ++column) {
                    if ((bits & (1 << (4 - column))) == 0) continue;
                    const float glyphX = FirstCellX +
                        static_cast<float>(character * 6 + column) * SignCellStep;
                    const float glyphY = SignCenterY +
                        static_cast<float>(3 - row) * SignCellStep;
                    shooter.DrawModelPrimitive(renderer, camera,
                        static_cast<int>(PrimitiveShape::Box),
                        glyphX, glyphY, SignGlyphZ + 0.10f,
                        0.82f, 0.82f, 0.10f, signGlowColor);
                    shooter.DrawModelPrimitive(renderer, camera,
                        static_cast<int>(PrimitiveShape::Box),
                        glyphX, glyphY, SignGlyphZ,
                        0.58f, 0.58f, 0.18f, signCoreColor);
                }
            }
        }

        // 接近と壁面上昇の全区間で見える窓を超巨大ビル正面へ規則配置する
        constexpr int WindowRowCount = 50;
        constexpr float WindowRowSpacing = 24.0f;
        for (int row = 0; row < WindowRowCount; ++row) {
            for (int column = 0;
                column < ShooterStages::Stage5::PandDBuildingWindowColumns; ++column) {
                const float columnOffset =
                    (static_cast<float>(column) /
                        static_cast<float>(
                            ShooterStages::Stage5::PandDBuildingWindowColumns - 1) -
                        0.5f) * 0.75f;
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(PrimitiveShape::Box),
                    columnOffset * ShooterStages::Stage5::PandDBuildingWidth,
                    GroundTopY + WindowRowSpacing * (static_cast<float>(row) + 0.5f),
                    BuildingZ - 3.18f,
                    ShooterStages::Stage5::PandDBuildingWidth * 0.08f,
                    0.72f, 0.18f, BuildingWindowColor);
            }
        }

        // PANDD会建造物はスケールを均一に保ち屋上へ載せる
        const Vector3 capSize = TayamaModelView::TowerSize *
            ShooterStages::Stage5::PandDBuildingCapScale;
        const Matrix4x4 capRoot = TayamaModelView::BuildingRoot(
            0.0f, GroundTopY + ShooterStages::Stage5::PandDBuildingHeight,
            BuildingZ, capSize);
        TayamaModelView::VisitParts(capRoot, 0.0f, TayamaModelState {},
            [&](PrimitiveShape shape, const Matrix4x4& world,
                const ColorF& color, TayamaPartGroup) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, partColor);
            });
        return;
    }

    // 第2部道中は背景側で同じ巨大ビルモデルを描画済み
    if (ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase)) return;

    if (shooter.m_stage5.phase <= Stage5Phase::EastsourceFall) {
        // 第1部遠景は奥行きを潰した単色シルエットとして超巨大ビルを配置する
        constexpr float GroundTopY = -3.65f;
        constexpr float MegaBuildingScale = 10.0f;
        constexpr float MegaBuildingDepthScale = 0.5f;
        constexpr float BackgroundSetback = 30.0f;
        const Stage5ModelTransform landmark = TayamaTransform(shooter);
        const Matrix4x4 root = Matrix4x4::Translation(
            {0.0f, GroundTopY, landmark.position.z + BackgroundSetback}) *
            Matrix4x4::Scale(
                {MegaBuildingScale, MegaBuildingScale, MegaBuildingDepthScale});
        Stage5CityModelView::VisitBuilding(Stage5BuildingType::TowerSub, root,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF&) {
                shooter.DrawModelPrimitive(renderer, camera,
                    static_cast<int>(shape), world, MegaBuildingSilhouetteColor);
            });
        return;
    }

    if (ShooterStages::Stage5::IsRooftopPhase(shooter.m_stage5.phase)) {
        constexpr float RoofColor[] = {0.28f, 0.29f, 0.30f, 1.0f};
        constexpr float RoofEdgeColor[] = {0.20f, 0.21f, 0.22f, 1.0f};
        constexpr float RoofJointColor[] = {0.16f, 0.17f, 0.18f, 1.0f};
        constexpr float SearchlightHousingColor[] = {0.12f, 0.13f, 0.14f, 1.0f};
        constexpr float SearchlightGlowColor[] = {1.0f, 0.90f, 0.58f, 0.30f};
        constexpr float RoofWidth = 144.0f;
        constexpr float RoofDepth = 150.0f;
        constexpr float RoofCenterZ = 48.0f;

        // 通常道路を覆う屋上床と外周壁で超巨大ビル上端を示す
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            0.0f, ShooterStages::Stage5::RooftopSurfaceY - 0.35f, RoofCenterZ,
            RoofWidth, 0.7f, RoofDepth, RoofColor);
        for (int side = -1; side <= 1; side += 2) {
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                static_cast<float>(side) * (RoofWidth * 0.5f - 0.6f),
                ShooterStages::Stage5::RooftopSurfaceY + 0.45f, RoofCenterZ,
                1.2f, 1.6f, RoofDepth, RoofEdgeColor);
        }

        // 目地を薄く重ねて道路標示のないコンクリート床として見せる
        for (int joint = -2; joint <= 2; ++joint) {
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                static_cast<float>(joint) * 24.0f,
                ShooterStages::Stage5::RooftopSurfaceY + 0.015f, RoofCenterZ,
                0.10f, 0.025f, RoofDepth - 2.4f, RoofJointColor);
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                0.0f, ShooterStages::Stage5::RooftopSurfaceY + 0.015f,
                RoofCenterZ + static_cast<float>(joint) * 25.0f,
                RoofWidth - 2.4f, 0.025f, 0.10f, RoofJointColor);
        }

        // 屋上の2基を壁面警備ドローン同様のレーザーポインターとして自機へ向ける
        const Vector3 lightTarget = shooter.PlayerWorldPosition();
        for (int xSide = -1; xSide <= 1; xSide += 2) {
            const Vector3 source {
                static_cast<float>(xSide) * (RoofWidth * 0.5f - 6.0f),
                ShooterStages::Stage5::RooftopSurfaceY + 0.9f,
                RoofCenterZ
            };
            const Vector3 delta = lightTarget - source;
            const float length = (std::max)(0.001f, delta.Length());
            const Vector3 direction = delta / length;
            const float yaw = std::atan2(direction.z, -direction.x);
            const float pitch = -std::asin(direction.y);
            const Matrix4x4 beamWorld =
                Matrix4x4::Translation(source + direction * (length * 0.5f)) *
                Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
                Matrix4x4::Scale({length, 0.025f, 0.025f});
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Cylinder), source,
                {1.5f, 0.55f, 1.5f}, {}, SearchlightHousingColor);
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Sphere), source + Vector3 {0.0f, 0.35f, 0.0f},
                {0.75f, 0.55f, 0.75f}, {}, SearchlightGlowColor);
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box), beamWorld, DronePointerColor);
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Sphere), lightTarget,
                {0.16f, 0.16f, 0.05f}, {}, DronePointerColor);
        }
    }

    DrawFinalEscapeDragon(shooter, renderer, camera);

    const Stage5ModelTransform transform = TayamaTransform(shooter);
    TayamaModelState state = TayamaState(shooter);
    const bool lightning = shooter.m_stage5.phase < Stage5Phase::TayamaCommandCore &&
        ((shooter.m_frame % 241) < 3 || ((shooter.m_frame + 73) % 389) < 2);
    if (lightning) {
        for (bool& flash : state.hitFlash) flash = true;
    }

    // 同じ建築モジュールをビル端点から巨大建築ロボ端点まで補間して描画する
    TayamaModelView::VisitParts(transform, shooter.m_stage5.tayamaTransformation, state,
        [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color, TayamaPartGroup) {
            const float brightness = lightning ? 1.55f : 1.0f;
            const float partColor[] = {
                (std::min)(1.0f, color.r * brightness),
                (std::min)(1.0f, color.g * brightness),
                (std::min)(1.0f, color.b * brightness), color.a
            };
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(shape), world, partColor);
        });

    // 正面でロックしたプレイヤー位置へ予告線と両目の太いレーザーを描画する
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore &&
        shooter.m_stage5.headLaserArmed) {
        const int attackTimer = ShooterStages::Stage5::TayamaArmAttackTimer(
            shooter.m_stage5.attackTimer);
        const int laserFrame = attackTimer %
            ShooterStages::Stage5::TayamaHeadLaserCycleFrames;
        const bool active = ShooterStages::Stage5::IsTayamaHeadLaserActive(attackTimer);
        if (laserFrame < ShooterStages::Stage5::TayamaHeadLaserWarningFrames || active) {
            const float progress = static_cast<float>(laserFrame) /
                static_cast<float>(ShooterStages::Stage5::TayamaHeadLaserWarningFrames);
            for (const Vector3& eye : TayamaModelView::EyeWorldPositions(transform)) {
                const Vector3 direction =
                    (shooter.m_stage5.headLaserTarget - eye).Normalized();
                const Vector3 end = eye + direction *
                    ShooterStages::Stage5::TayamaHeadLaserLength;
                if (active) {
                    const float pulse = static_cast<float>(shooter.m_frame % 12) / 12.0f;
                    shooter.DrawRailgunBeamBetween(renderer, camera, eye, end,
                        ShooterStages::Stage5::TayamaHeadLaserHitRadius * 1.7f, pulse, 2);
                    shooter.DrawRailgunBeamBetween(renderer, camera, eye, end,
                        ShooterStages::Stage5::TayamaHeadLaserHitRadius, pulse, 0);
                } else {
                    shooter.DrawRailgunBeamBetween(renderer, camera, eye, end,
                        0.16f, progress, 3);
                }
            }
        }
    }

    // 変形終盤から既存のエンジン炎HLSLを背部と生存中の脚部機関へ付ける
    if (shooter.m_stage5.phase >= Stage5Phase::CarrierTransformation &&
        shooter.m_stage5.phase < Stage5Phase::TayamaCollapse) {
        constexpr TayamaPartGroup EngineGroups[] = {
            TayamaPartGroup::MainThruster,
            TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine
        };
        for (int engine = 0; engine < 3; ++engine) {
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                shooter.m_stage5.tayamaTransformation, state, EngineGroups[engine]);
            if (!bounds.valid) continue;
            const float width = engine == 0 ? 2.2f : 1.25f;
            const Matrix4x4 flameWorld = Matrix4x4::Translation(
                bounds.center + Vector3 {0.0f, -1.0f - static_cast<float>(engine) * 0.12f, 0.0f}) *
                Matrix4x4::Scale({width, 2.8f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * flameWorld,
                static_cast<float>((shooter.m_frame + engine * 7) % 24) / 24.0f, 3});
        }
    }

    if (shooter.m_stage5.phase == Stage5Phase::EastsourceIntro) {
        // 破裂前は赤色警告灯と左右へ押し出される格納庫装甲を段階表示する
        const float warningColor[] = {1.0f, 0.04f, 0.03f,
            (shooter.m_stage5.phaseTimer / 5) % 2 == 0 ? 1.0f : 0.28f};
        for (int light = -2; light <= 2; ++light) {
            shooter.DrawModelPrimitive(renderer, camera, 1,
                static_cast<float>(light) * 1.35f, 1.5f, 61.5f,
                0.34f, 0.18f, 0.16f, warningColor);
        }
        const float deformation = SmoothStep(Math::Clamp01(
            static_cast<float>(shooter.m_stage5.phaseTimer - 18) / 40.0f));
        shooter.DrawModelPrimitive(renderer, camera, 1, -1.5f - deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor,
            0.0f, deformation * 0.16f);
        shooter.DrawModelPrimitive(renderer, camera, 1, 1.5f + deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor, 0.0f, -deformation * 0.16f);
    }

    // 屋上戦の視錐台上部へ決定的な星を分散配置する
    const bool rooftopClouds = ShooterStages::Stage5::IsRooftopPhase(shooter.m_stage5.phase);
    if (rooftopClouds) {
        constexpr float RooftopStarColor[] = {0.68f, 0.80f, 1.0f, 1.0f};
        for (int i = 0; i < RooftopStarCount; ++i) {
            const float depth = 72.0f + static_cast<float>((i * 29) % 61);
            const float halfHeight = depth * std::tan(camera.FieldOfView() * 0.5f);
            const float halfWidth = halfHeight * renderer.AspectRatio();
            const float lateral = (-0.96f +
                static_cast<float>((i * 71) % 193) / 100.0f) * halfWidth;
            Vector3 position = WeatherPosition(camera, lateral, depth);
            position.y = camera.Position().y + camera.Forward().y * depth +
                camera.Up().y * halfHeight *
                    (0.05f + static_cast<float>((i * 43) % 91) / 100.0f);
            const float size = i % 11 == 0 ? 0.34f : 0.18f;
            shooter.DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShape::Box), position,
                {size, size, size * 0.35f}, {}, RooftopStarColor);
        }
    }

    // 嵐の上層をCube帯で表現する
    const bool aboveStorm = shooter.m_stage5.phase >= Stage5Phase::TayamaCommandCore;
    const int cloudCount = rooftopClouds ?
        ShooterStages::Stage5::RooftopStormCloudCount : (aboveStorm ? 64 : 14);
    for (int i = 0; i < cloudCount; ++i) {
        const float x = -34.0f + static_cast<float>((i * 47) % 680) / 10.0f;
        const float z = 24.0f + static_cast<float>((i * 31 + shooter.m_frame / 3) % 760) / 10.0f;
        const float y = aboveStorm ? -5.8f + static_cast<float>(i % 3) * 0.32f :
            12.0f + static_cast<float>(i % 4) * 1.1f;
        const float cloudColor[] = {
            aboveStorm ? 0.32f : StormCloudColor[0],
            aboveStorm ? 0.38f : StormCloudColor[1],
            aboveStorm ? 0.48f : StormCloudColor[2],
            aboveStorm ? 0.82f : 0.72f
        };
        Vector3 cloudPosition {x, y, z};
        float cloudYaw = 0.0f;
        if (rooftopClouds) {
            // カメラ移動から独立した屋上上空の固定座標へ多層の雲を配置する
            cloudPosition = RooftopCloudPosition(i);
        }
        shooter.DrawModelPrimitive(renderer, camera, 1, cloudPosition.x, cloudPosition.y,
            cloudPosition.z,
            rooftopClouds ? 20.0f + static_cast<float>(i % 4) * 3.2f :
                8.0f + static_cast<float>(i % 4) * 2.0f,
            rooftopClouds ? 4.4f + static_cast<float>(i % 3) * 0.8f : 0.75f,
            rooftopClouds ? 12.0f + static_cast<float>(i % 5) * 2.0f : 3.5f,
            cloudColor, cloudYaw);
    }

    // 現フェーズのサーチライト基部と、追尾上限を持つ光軸を同じ座標で描画する
    int activeLights = 0;
    bool tayamaLights = false;
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbLower) activeLights = 1;
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle) activeLights = 2;
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbUpper) activeLights = 3;
    if (ShooterStages::Stage5::IsTayamaBattlePhase(shooter.m_stage5.phase)) {
        activeLights = 2;
        tayamaLights = true;
    }
    for (int index = 0; index < activeLights; ++index) {
        const SearchlightState& light = shooter.m_stage5.searchlights[index];
        if (light.destroyed) continue;
        Vector3 source {
            ToWorldX((static_cast<float>(index) - 1.0f) * 0.72f),
            ToWorldY(0.72f - static_cast<float>(index) * 0.22f),
            tayamaLights ? 57.0f : 46.0f
        };
        if (tayamaLights) {
            const TayamaPartGroup group = index == 0 ?
                TayamaPartGroup::LeftSearchlight : TayamaPartGroup::RightSearchlight;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
                transform, shooter.m_stage5.tayamaTransformation, state, group);
            if (bounds.valid) source = bounds.center;
        }
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector3 target {
            ToWorldX(locked ? light.lockedX : light.beamX),
            ToWorldY(locked ? light.lockedY : light.beamY),
            locked ? light.lockedZ : light.beamZ
        };
        const Vector3 delta = target - source;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float yaw = std::atan2(direction.z, -direction.x);
        const float pitch = -std::asin(direction.y);
        const float* beamColor = tayamaLights ? DronePointerColor :
            (locked ? SearchlightLockedColor : SearchlightColor);
        const float beamWidth = tayamaLights ? 0.025f : (locked ? 0.12f : 0.18f);
        const Matrix4x4 beamWorld = Matrix4x4::Translation(source + direction * (length * 0.5f)) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale({length, beamWidth, beamWidth});
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box), beamWorld, beamColor);
        shooter.DrawModelPrimitive(renderer, camera, 2, source.x, source.y, source.z,
            0.72f, 0.42f, 0.72f, locked ? SearchlightLockedColor : SatelliteLightColor);
    }

    // 有効弱点へ小さな発光リングを重ねて攻略対象を明示する
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaFireControl &&
        shooter.m_stage5.phase <= Stage5Phase::TayamaCommandCore) {
        constexpr TayamaPartGroup Groups[] = {
            TayamaPartGroup::LeftSearchlight, TayamaPartGroup::RightSearchlight,
            TayamaPartGroup::FireControlRadar, TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine, TayamaPartGroup::CommandCore
        };
        for (const TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
            if (!weakpoint.active || weakpoint.destroyed) continue;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                shooter.m_stage5.tayamaTransformation, state, Groups[static_cast<std::size_t>(weakpoint.type)]);
            if (!bounds.valid) continue;
            const float size = (std::max)(0.75f, (std::min)(2.2f, bounds.radius * 0.45f));
            const Matrix4x4 world = Matrix4x4::Translation(bounds.center + Vector3 {0.0f, 0.0f, -0.12f}) *
                Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                static_cast<float>(shooter.m_frame % 30) / 30.0f, 0});
        }
    }

    // 生存中の胸部コアへ射撃予告と同期した収束光を重ねる
    if (ShooterStages::Stage5::IsTayamaBattlePhase(shooter.m_stage5.phase) &&
        !shooter.m_stage5.tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed &&
        shooter.m_stage5.phaseTimer > 75 &&
        shooter.m_stage5.attackTimer % 180 < 42) {
        const Stage5GroupBounds core = TayamaModelView::GroupBounds(transform,
            shooter.m_stage5.tayamaTransformation, state, TayamaPartGroup::CommandCore);
        if (core.valid) {
            const float charge = static_cast<float>(shooter.m_stage5.attackTimer % 180) / 42.0f;
            const Matrix4x4 chargeWorld = Matrix4x4::Translation(
                core.center + Vector3 {0.0f, 0.0f, -1.0f}) *
                Matrix4x4::Scale({1.0f + charge * 1.8f,
                    1.0f + charge * 1.8f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * chargeWorld,
                charge, 0});
        }
    }

    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse && shooter.m_stage5.phaseTimer >= 330 &&
        shooter.m_stage5.phaseTimer < TayamaCollapseFrames) {
        // 最終90フレームは内部白光と二重衝撃波で輪郭ごと消滅させる
        const float finalProgress = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 450) / 90.0f);
        const float glow = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 330) / 120.0f);
        const Matrix4x4 glowWorld = Matrix4x4::Translation(transform.position) *
            Matrix4x4::Scale({3.0f + glow * 8.0f, 2.0f + glow * 5.0f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * glowWorld,
            static_cast<float>(shooter.m_frame % 24) / 24.0f, 0});
        if (finalProgress > 0.0f) {
            const Matrix4x4 shockwave = Matrix4x4::Translation(transform.position) *
                Matrix4x4::Scale({4.0f + finalProgress * 24.0f,
                    4.0f + finalProgress * 24.0f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * shockwave,
                finalProgress, 0});
        }
    }
}

/**
 * @brief Stage 5の雨粒を3D空間へ描画する
 * @param shooter 描画対象
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawRain3D(const SideScrollingShooter& shooter,
    Renderer& renderer, const Camera3D& camera, float railWeight) {
    const float intensity = RainIntensity(shooter.m_stage5.phase, shooter.m_chapterNumber,
        shooter.m_stage5.tayamaTransformation, shooter.m_stage5.phaseTimer);
    const bool wallClimbCinematic =
        shooter.m_stage5.phase == Stage5Phase::WallClimbTransition;
    const int rainCount = intensity > 0.0f ?
        (wallClimbCinematic ? 384 : static_cast<int>(96.0f + intensity * 160.0f)) : 0;
    const int frame = shooter.m_frame;
    const float weight = Math::Clamp01(railWeight);
    const bool transformationCinematic =
        shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation;
    const bool part2Route = ShooterStages::Stage5::IsPart2RoutePhase(shooter.m_stage5.phase);
    const bool tayamaBattle = shooter.IsTayamaBattle();
    const float cinematicAltitude = shooter.m_stage5.phase == Stage5Phase::WallClimbTransition ?
        SmoothStep(ShooterStages::Stage5::WallClimbProgress(shooter.m_stage5.phaseTimer)) *
            ShooterStages::Stage5::WallClimbHeight : 0.0f;

    // 横視点は前景、中景、後景の3層とし、遷移に合わせて3D降雨域へ展開する
    for (int index = 0; index < rainCount; ++index) {
        constexpr float SideGroundY = -6.0f;
        constexpr float RailGroundY = -3.65f;
        const int fallPhase = RainFallPhase(index, frame);
        const float fallRate = static_cast<float>(fallPhase) / static_cast<float>(RainCycle);
        const int sideColumn = (index * 137 + frame * 2) % 340;
        const float streakLength = 0.80f + static_cast<float>(index % 5) * 0.18f;
        const float railStreakLength = streakLength * 1.55f;
        const float sideZ = SidePlaneZ - 3.0f + static_cast<float>(index % 3) * 4.0f;
        const Vector3 sidePosition {
            -17.0f + static_cast<float>(sideColumn) * 0.1f,
            SideGroundY + cinematicAltitude + streakLength * 0.5f + fallRate * 18.0f,
            sideZ
        };

        const float rainWidth = transformationCinematic ?
            TayamaModelView::TowerSize.x * ShooterStages::Stage5::TayamaBossScale * 1.8f : 40.0f;
        const int railColumn = (index * 137 + frame * 2) % 400;
        const float railZ = -2.0f + static_cast<float>((index * 197) % 960) * 0.1f;
        Vector3 railPosition {
            -rainWidth * 0.5f + static_cast<float>(railColumn) / 400.0f * rainWidth,
            RailGroundY + cinematicAltitude + railStreakLength * 0.5f + fallRate *
                (transformationCinematic ?
                    TayamaModelView::TowerSize.y * ShooterStages::Stage5::TayamaBossScale : 24.0f),
            railZ
        };
        if (part2Route) {
            // 壁面上昇中は現在の視錐台上端外から下端外まで雨を通過させる
            const float viewDepth = 18.0f +
                static_cast<float>((index * 197) % 600) * 0.1f;
            const float halfHeight = viewDepth * std::tan(camera.FieldOfView() * 0.5f);
            const float lateral = (-1.0f + static_cast<float>(railColumn) / 200.0f) *
                halfHeight * renderer.AspectRatio() * 1.12f;
            const Vector3 topPosition = camera.Position() +
                camera.Forward() * viewDepth + camera.Right() * lateral +
                camera.Up() * (halfHeight * RainViewMargin + railStreakLength * 0.5f);
            const float worldFallProjection = (std::max)(0.2f,
                camera.Up().y + RainViewMargin *
                    std::tan(camera.FieldOfView() * 0.5f) * camera.Forward().y);
            railPosition = topPosition - Vector3::Up *
                ((1.0f - fallRate) * halfHeight * RainViewTravel / worldFallProjection);
        } else if (tayamaBattle) {
            // 円形アリーナでは原点付近でなく、現在カメラ前方の視錐台を雨で満たす
            const float lateral = -28.0f + static_cast<float>(railColumn) / 400.0f * 56.0f;
            const float depth = 8.0f + static_cast<float>((index * 197) % 640) * 0.1f;
            railPosition = WeatherPosition(camera, lateral, depth);
            railPosition.y = ShooterStages::Stage5::RooftopSurfaceY +
                railStreakLength * 0.5f + fallRate * 24.0f;
        }

        const float thickness = 0.045f + intensity * 0.025f;
        const Vector3 scale {
            thickness,
            Math::Lerp(streakLength, railStreakLength, weight),
            thickness
        };
        const Vector3 rotation {Math::Lerp(0.0f, 0.08f, weight), 0.0f,
            -0.18f - static_cast<float>(index % 3) * 0.025f};
        const float rainColor[] = {0.50f, 0.72f, 0.90f, 0.16f + intensity * 0.34f};
        shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            Vector3::Lerp(sidePosition, railPosition, weight), scale, rotation, rainColor);

        // 落下位相が地面を折り返した直後だけ小さなBox雨粒を跳ね上げる
        const float splashProgress = RainSplashProgress(fallPhase);
        if (splashProgress < 0.0f) continue;
        const int framesSinceImpact =
            ((RainCycle - fallPhase) % RainCycle + RainFallSpeed - 1) / RainFallSpeed;
        const int sideImpactColumn =
            (sideColumn + 340 - framesSinceImpact * 2) % 340;
        const int railImpactColumn =
            (railColumn + 400 - framesSinceImpact * 2) % 400;
        const float arc = std::sin(splashProgress * Math::Pi);
        const float particleSize = Math::Lerp(0.09f, 0.035f, splashProgress);
        const float splashColor[] = {
            0.58f, 0.78f, 0.96f, intensity * (1.0f - splashProgress) * 0.62f
        };
        for (int particle = 0; particle < 4; ++particle) {
            const float direction = static_cast<float>(particle) - 1.5f;
            const Vector3 sideSplash {
                -17.0f + static_cast<float>(sideImpactColumn) * 0.1f +
                    direction * splashProgress * 0.24f,
                SideGroundY + cinematicAltitude + particleSize * 0.5f + arc *
                    (0.28f + static_cast<float>(particle % 2) * 0.12f),
                sideZ + (particle % 2 == 0 ? -1.0f : 1.0f) * splashProgress * 0.16f
            };
            Vector3 railSplash {
                -rainWidth * 0.5f + static_cast<float>(railImpactColumn) / 400.0f * rainWidth +
                    direction * splashProgress * 0.42f,
                RailGroundY + cinematicAltitude + particleSize * 0.5f + arc *
                    (0.42f + static_cast<float>(particle % 2) * 0.18f),
                railZ + direction * splashProgress * 0.28f
            };
            if (tayamaBattle) {
                const float lateral = -28.0f +
                    static_cast<float>(railImpactColumn) / 400.0f * 56.0f +
                    direction * splashProgress * 0.42f;
                const float depth = 8.0f + static_cast<float>((index * 197) % 640) * 0.1f +
                    direction * splashProgress * 0.28f;
                railSplash = WeatherPosition(camera, lateral, depth);
                railSplash.y = ShooterStages::Stage5::RooftopSurfaceY +
                    particleSize * 0.5f + arc *
                    (0.42f + static_cast<float>(particle % 2) * 0.18f);
            }
            shooter.DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
                Vector3::Lerp(sideSplash, railSplash, weight),
                {particleSize, particleSize, particleSize}, {}, splashColor);
        }
    }
}

/**
 * @brief Stage 5の稲光と照準表示を画面空間へ描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @param camera 3D描画時のカメラ、2D描画時はnullptr
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawScreenEffects(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D* camera) {
    const float intensity = RainIntensity(shooter.m_stage5.phase, shooter.m_chapterNumber,
        shooter.m_stage5.tayamaTransformation, shooter.m_stage5.phaseTimer);

    // 第二形態は夜明け進行率に応じた暁色を重ね、2Dと3Dの色調を統一する
    if (ShooterStages::Stage5::IsCloudSeaPhase(shooter.m_stage5.phase)) {
        const float dawn = TayamaDragonDawnProgress(shooter.m_stage5.phase,
            shooter.m_stage5.tayamaHp, shooter.m_stage5.tayamaMaxHp);
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}},
            {0.82f, 0.42f, 0.26f, dawn * 0.10f});
    }

    // 稲光はTAYAMAの輪郭と警告灯を一瞬だけ強調する
    if (intensity > 0.30f && ((shooter.m_frame % 241) < 3 || ((shooter.m_frame + 73) % 389) < 2)) {
        const float alpha = (shooter.m_frame % 2 == 0 ? 0.30f : 0.16f) * intensity;
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.72f, 0.82f, 1.0f, alpha});
    }

    // 撃破地点、壁面ムービー、第2部開始、終幕の雲海を暗転でつなぐ
    const float fadeAlpha = SmoothStep(ShooterStages::Stage5::CinematicFadeAlpha(
        shooter.m_stage5.phase, shooter.m_stage5.phaseTimer));
    if (fadeAlpha > 0.0f) {
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}},
            {0.0f, 0.0f, 0.0f, fadeAlpha});
    }
    if (IsCinematic(shooter)) return;

    // 周回中のロック地点を現在カメラの画面座標へ変換する
    const auto ScreenPosition = [&](float x, float y, float z) {
        Vector2 position {x, y};
        Vector2 pixel;
        if (camera != nullptr && shooter.IsTayamaBattle() &&
            camera->TryWorldToScreen({ToWorldX(x), ToWorldY(y), z}, pixel)) {
            const Viewport& viewport = camera->GetViewport();
            position = {
                (pixel.x - static_cast<float>(viewport.x)) /
                    static_cast<float>(viewport.width) * 2.0f - 1.0f,
                1.0f - (pixel.y - static_cast<float>(viewport.y)) /
                    static_cast<float>(viewport.height) * 2.0f
            };
        }
        return position;
    };
    const Vector2 coreTarget = ScreenPosition(shooter.m_stage5.coreTargetX,
        shooter.m_stage5.coreTargetY, shooter.m_stage5.coreTargetZ);

    // 胸部掃射、脚部斉射、コアレーザーは発射前だけ危険範囲を固定表示する
    if (ShooterStages::Stage5::IsTayamaBattlePhase(shooter.m_stage5.phase) &&
        !shooter.m_stage5.tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed &&
        shooter.m_stage5.phaseTimer > 75 && shooter.m_stage5.attackTimer % 210 < 36) {
        renderer.Draw(Rect {{0.0f, coreTarget.y}, {1.86f, 0.055f}},
            {1.0f, 0.12f, 0.08f, 0.36f});
    }
    const bool liftEngineActive = !shooter.m_stage5.tayamaWeakpoints[
        static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed ||
        !shooter.m_stage5.tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed;
    if (ShooterStages::Stage5::IsTayamaBattlePhase(shooter.m_stage5.phase) &&
        liftEngineActive && shooter.m_stage5.phaseTimer > 75 &&
        shooter.m_stage5.attackTimer % 132 < 32) {
        renderer.Draw(Rect {coreTarget, {0.38f, 0.075f}},
            {1.0f, 0.34f, 0.08f, 0.42f});
        renderer.Draw(Circle {coreTarget, 0.12f},
            {1.0f, 0.58f, 0.12f, 0.58f});
    }
    if (ShooterStages::Stage5::IsTayamaBattlePhase(shooter.m_stage5.phase) &&
        !shooter.m_stage5.tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed &&
        shooter.m_stage5.phaseTimer > 75 && shooter.m_stage5.attackTimer % 180 < 42) {
        renderer.Draw(Circle {coreTarget, 0.11f}, {1.0f, 0.08f, 0.04f, 0.62f});
    }

    // TAYAMA戦はレーザー照準点、壁面区画は検出円と固定ロック地点を表示する
    const bool showSearchlights =
        (shooter.m_stage5.phase >= Stage5Phase::WallClimbLower &&
            shooter.m_stage5.phase <= Stage5Phase::WallClimbUpper) ||
        shooter.m_stage5.phase == Stage5Phase::TayamaFireControl;
    if (!showSearchlights) return;
    for (const SearchlightState& light : shooter.m_stage5.searchlights) {
        if (light.destroyed || light.phase == SearchlightPhase::Cooldown) continue;
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector2 target = ScreenPosition(locked ? light.lockedX : light.beamX,
            locked ? light.lockedY : light.beamY,
            locked ? light.lockedZ : light.beamZ);
        const bool detecting = light.phase == SearchlightPhase::Detecting;
        const bool laserPointer = shooter.m_stage5.phase == Stage5Phase::TayamaFireControl;
        const ColorF color = laserPointer ? ColorF {1.0f, 0.02f, 0.02f, 0.88f} :
            (locked ? ColorF {1.0f, 0.08f, 0.08f, 0.86f} :
            (detecting ? ColorF {1.0f, 0.78f, 0.18f, 0.34f} :
                ColorF {0.92f, 0.82f, 0.42f, 0.16f}));
        renderer.Draw(Circle {target,
            laserPointer ? (detecting ? 0.026f : 0.018f) :
                (locked ? 0.075f : SearchlightDetectionRadius)}, color);
        if (locked) {
            renderer.Draw(Rect {{target.x - 0.10f, target.y}, {0.055f, 0.008f}}, color);
            renderer.Draw(Rect {{target.x + 0.10f, target.y}, {0.055f, 0.008f}}, color);
            renderer.Draw(Rect {{target.x, target.y - 0.10f}, {0.008f, 0.055f}}, color);
            renderer.Draw(Rect {{target.x, target.y + 0.10f}, {0.008f, 0.055f}}, color);
        }
    }
}

/**
 * @brief Stage 5専用HUDを描画する
 * @param shooter 更新対象
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DrawStage5Hud(const SideScrollingShooter& shooter, Renderer& renderer) {
    if (shooter.m_stage5.phase == Stage5Phase::Approach ||
        shooter.m_stage5.phase == Stage5Phase::TayamaCollapse ||
        shooter.m_stage5.phase == Stage5Phase::CloudSea ||
        shooter.m_stage5.phase == Stage5Phase::TayamaDragonCollapse ||
        shooter.m_stage5.phase == Stage5Phase::EndingReady) return;
    constexpr float Back[] = {0.08f, 0.05f, 0.12f, 0.90f};
    constexpr float Fill[] = {0.96f, 0.14f, 0.24f, 1.0f};
    constexpr float Accent[] = {0.16f, 0.82f, 1.0f, 1.0f};
    constexpr float Divider[] = {1.0f, 0.82f, 0.30f, 1.0f};
    constexpr float BarWidth = 0.62f;

    if (shooter.m_stage5.phase == Stage5Phase::EastsourceBattle) {
        const int maxHp = shooter.m_enemies[0].maxHp > 0 ? shooter.m_enemies[0].maxHp : EastsourceMaxHp;
        const float hpRate = Math::Clamp01(shooter.m_displayBossHp / static_cast<float>(maxHp));
        shooter.DrawShape(renderer, 0.0f, 0.76f, BarWidth, 0.025f, Back);
        shooter.DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.76f,
            BarWidth * hpRate, 0.018f, Fill);
        shooter.DrawBossPhaseDividers(
            renderer, 0.755f, BarWidth, maxHp, Divider);
        renderer.DrawText("EASTSOURCE", TextAlign::Center, 0.017f,
            {1.0f, 0.42f, 0.55f, 1.0f}, {0.0f, 0.86f});
        constexpr const char* Labels[] = {"PRECISION", "CROSSFIRE", "PURSUIT", "LAST CONTRACT"};
        const int phase = shooter.m_enemies[0].active ? shooter.m_enemies[0].bossPhase : 0;
        renderer.DrawText(Labels[(std::clamp)(phase, 0, 3)], {-BarWidth, 0.81f}, 0.012f,
            {1.0f, 0.82f, 0.30f, 1.0f});
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceIntro) {
        renderer.DrawText("HOSTILE SIGNAL APPROACHING", TextAlign::Center, 0.018f,
            {1.0f, 0.34f, 0.32f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceFall) {
        renderer.DrawText("SIGNAL LOST", TextAlign::Center, 0.030f,
            {1.0f, 0.18f, 0.18f, 1.0f}, {0.0f, 0.12f});
        return;
    }
    if (shooter.m_stage5.phase >= Stage5Phase::WallClimbTransition &&
        shooter.m_stage5.phase <= Stage5Phase::WallClimbUpper) return;
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival ||
        shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        char status[48];
        std::snprintf(status, sizeof(status), "GIANT MECHA TAYAMA  %03d%%",
            static_cast<int>(shooter.m_stage5.tayamaTransformation * 100.0f));
        renderer.DrawText(status, TextAlign::Center, 0.018f,
            {0.30f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.82f});
        return;
    }

    // TAYAMA戦は第一形態全体で共有する本体HPを表示する
    const int maxHp = shooter.m_stage5.tayamaMaxHp;
    const float hpRate = maxHp > 0 ? Math::Clamp01(shooter.m_displayBossHp / static_cast<float>(maxHp)) : 0.0f;
    shooter.DrawShape(renderer, 0.0f, 0.74f, BarWidth, 0.025f, Back);
    shooter.DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.74f,
        BarWidth * hpRate, 0.018f, Accent);
    renderer.DrawText("TAYAMA", TextAlign::Center, 0.022f,
        {0.20f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.86f});
    char components[96];
    if (shooter.m_stage5.phase == Stage5Phase::TayamaDragonBattle) {
        std::snprintf(components, sizeof(components),
            "ALL BODY SECTIONS VULNERABLE");
    } else {
        std::snprintf(components, sizeof(components),
            "L-LIGHT[%c] R-LIGHT[%c] RADAR[%c] L-LEG[%c] R-LEG[%c] CORE[%c]",
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftSearchlight)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightSearchlight)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed ? 'X' : ' ',
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed ? 'X' : ' ');
    }
    renderer.DrawText(components, TextAlign::Center, 0.010f,
        {0.72f, 0.86f, 0.92f, 1.0f}, {0.0f, 0.79f});
}
