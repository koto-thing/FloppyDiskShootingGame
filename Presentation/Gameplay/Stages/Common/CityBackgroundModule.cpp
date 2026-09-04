#include "CityBackgroundModule.h"

#include <cmath>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::SideCameraZ;

constexpr float DaySkyColor[4] = {0.30f, 0.68f, 0.92f, 1.0f};
constexpr float NightSkyColor[4] = {0.015f, 0.03f, 0.12f, 1.0f};
constexpr float StarColor[4] = {0.55f, 0.70f, 0.85f, 1.0f};
constexpr float StreetColor[4] = {0.025f, 0.05f, 0.11f, 1.0f};
constexpr float BuildingColor[4] = {0.055f, 0.10f, 0.20f, 1.0f};
constexpr float WindowCyanColor[4] = {0.12f, 0.82f, 0.98f, 1.0f};
constexpr float WindowMagentaColor[4] = {0.90f, 0.18f, 0.76f, 1.0f};
constexpr float MoonColor[4] = {0.86f, 0.90f, 0.72f, 1.0f};
constexpr float CityRoadColor[4] = {0.105f, 0.125f, 0.19f, 1.0f};
constexpr float LaneColor[4] = {0.72f, 0.84f, 0.88f, 1.0f};
constexpr float CarBodyColor[4] = {0.18f, 0.76f, 0.96f, 1.0f};
constexpr float CarAccentColor[4] = {0.98f, 0.24f, 0.70f, 1.0f};
constexpr float TruckBodyColor[4] = {0.11f, 0.04f, 0.24f, 1.0f};
constexpr float TruckNeonColor[4] = {0.10f, 0.96f, 1.0f, 1.0f};
constexpr float TruckAccentColor[4] = {1.0f, 0.12f, 0.72f, 1.0f};
constexpr float TruckSideWidth = 7.4f;
constexpr float TruckSideHeight = 3.8f;
constexpr float TruckRailWidth = 4.8f;
constexpr float TruckRailHeight = 6.0f;
constexpr float TruckRailDepth = 13.5f;
constexpr float TruckRailCycleLength = 220.0f;
constexpr int TruckNeonFlashFrames = 10;
constexpr float SideRoadTopY = -6.0f;
constexpr float SideRoadCenterY = -8.65f;
constexpr float SideRoadBottomY = -11.3f;
constexpr float UpperTrafficY = -7.15f;
constexpr float LaneDividerY = -8.65f;
constexpr float LowerTrafficY = -9.65f;
constexpr float Stage4SideRoadTopY = -6.0f;
constexpr float Stage4SideRoadCenterY = -8.65f;
constexpr float Stage4SideRoadBottomY = -11.3f;
constexpr float Stage4UpperTrafficY = -7.15f;
constexpr float Stage4LaneDividerY = -8.65f;
constexpr float Stage4LowerTrafficY = -9.65f;
constexpr int CityBuildingCount = 30;
constexpr float CityBuildingNdcSpacing = 2.0f / CityBuildingCount;
static_assert(TruckRailCycleLength > 120.0f);
static_assert(CityBuildingCount * CityBuildingNdcSpacing == 2.0f);
static_assert(SideRoadTopY > UpperTrafficY &&
    UpperTrafficY > LaneDividerY &&
    LaneDividerY > LowerTrafficY &&
    LowerTrafficY > SideRoadBottomY);
static_assert(Stage4SideRoadTopY > Stage4UpperTrafficY &&
    Stage4UpperTrafficY > Stage4LaneDividerY &&
    Stage4LaneDividerY > Stage4LowerTrafficY &&
    Stage4LowerTrafficY > Stage4SideRoadBottomY);

/**
 * @brief 都市背景の交通車両を描画するか判定する
 * @param stageNumber 現在のステージ番号
 * @param bossBattle ボス戦中か
 * @return 交通車両を描画する場合true
 */
constexpr bool ShouldDrawTraffic(int stageNumber, bool bossBattle, bool bossEntering = false) {
    return stageNumber != 4 || !bossBattle || bossEntering;
}

/**
 * @brief トラックのネオンを一定間隔で明滅させる輝度を返す
 * @param frame 現在のフレーム番号
 * @return ネオンの輝度
 */
constexpr float TruckNeonIntensity(int frame) {
    return (frame / TruckNeonFlashFrames) % 2 == 0 ? 1.0f : 0.16f;
}

static_assert(ShouldDrawTraffic(4, false));
static_assert(!ShouldDrawTraffic(4, true));
static_assert(ShouldDrawTraffic(4, true, true));
static_assert(ShouldDrawTraffic(5, true));
static_assert(TruckNeonIntensity(0) == 1.0f);
static_assert(TruckNeonIntensity(TruckNeonFlashFrames) == 0.16f);
static_assert(TruckNeonIntensity(TruckNeonFlashFrames * 2) == 1.0f);

}

float SideScrollingShooter::CityBackgroundModule::WrapNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) {
        wrapped += 2.0f;
    }
    return wrapped - 1.0f;
}

float SideScrollingShooter::CityBackgroundModule::WrapDistance(float value, float length) {
    float wrapped = std::fmod(value, length);
    if (wrapped < 0.0f) {
        wrapped += length;
    }
    return wrapped;
}

void SideScrollingShooter::CityBackgroundModule::DrawRoad(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float sideHalfWidth, float railWeight) {
    constexpr float SideRoadZ = SidePlaneZ + 13.55f;
    constexpr float RailRoadTopY = -3.55f;

    // 画面下部を覆う道路面を横視点からレール空間へ連続して展開する
    DrawModelPrimitive(renderer, camera, 1,
        0.0f,
        Math::Lerp(SideRoadCenterY, -3.60f, railWeight),
        Math::Lerp(SideRoadZ, 45.0f, railWeight),
        Math::Lerp(60.0f, 24.0f, railWeight),
        Math::Lerp(SideRoadTopY - SideRoadBottomY, 0.10f, railWeight),
        Math::Lerp(0.22f, 140.0f, railWeight), CityRoadColor);

    // 遠い路肩を細く遅く、手前を太く速く流して2Dのまま路面へ奥行きを付ける
    for (int lane = -1; lane <= 1; ++lane) {
        const float sideY = lane < 0 ? SideRoadTopY + 0.08f :
            (lane > 0 ? SideRoadBottomY + 0.18f : LaneDividerY);
        const float sideSegmentWidth = lane < 0 ? 3.8f : (lane > 0 ? 4.4f : 2.2f);
        const float sideSegmentHeight = lane < 0 ? 0.09f : (lane > 0 ? 0.18f : 0.10f);
        const float sideScrollRate = lane < 0 ? 0.24f : (lane > 0 ? 0.60f : 0.42f);
        for (int segment = 0; segment < 16; ++segment) {
            const float sideX = WrapNdcX(
                static_cast<float>(segment) * 0.125f - shooter.m_scroll * sideScrollRate) *
                (sideHalfWidth + 2.0f);
            const float railZ = 4.0f + WrapDistance(
                static_cast<float>(segment) * 10.0f - shooter.m_scroll * 150.0f,
                160.0f);
            DrawModelPrimitive(renderer, camera, 1,
                Math::Lerp(sideX, static_cast<float>(lane) * 6.0f, railWeight),
                Math::Lerp(sideY, RailRoadTopY + 0.025f * 0.5f, railWeight),
                Math::Lerp(SideRoadZ - 0.16f, railZ, railWeight),
                Math::Lerp(sideSegmentWidth, 0.20f, railWeight),
                Math::Lerp(sideSegmentHeight, 0.025f, railWeight),
                Math::Lerp(0.08f, 4.2f, railWeight), LaneColor);
        }
    }

    const bool bossEntering = shooter.m_stageNumber == 4 && shooter.m_bossBattle &&
        shooter.m_bossIntroductionPhase == BossIntroductionPhase::Entrance;
    if (!ShouldDrawTraffic(shooter.m_stageNumber, shooter.m_bossBattle, bossEntering)) return;

    // 横向きの遠景車を小さく、手前車を大きくして上下車線を逆方向へ流す
    for (int i = 0; i < 10; ++i) {
        const bool lowerLane = i % 2 != 0;
        const float sideDirection = lowerLane ? 1.0f : -1.0f;
        const float sideSpeed = lowerLane ? 1.12f : 0.72f;
        const float sideBodyWidth = lowerLane ? 2.65f : 1.90f;
        const float sideBodyHeight = lowerLane ? 0.68f : 0.46f;
        const float sideX = WrapNdcX(
            static_cast<float>(i) * 0.31f +
            sideDirection * shooter.m_scroll * sideSpeed) * (sideHalfWidth + 2.0f);
        const float sideY = lowerLane ? LowerTrafficY : UpperTrafficY;
        const float railX = -8.5f + static_cast<float>(i % 4) * 5.6f;
        const float railDirection = i % 4 < 2 ? -1.0f : 1.0f;
        const float railSpeed = i % 2 == 0 ? 175.0f : 105.0f;
        const float railZ = 8.0f + WrapDistance(
            static_cast<float>(i) * 19.0f +
            railDirection * shooter.m_scroll * railSpeed, 120.0f);
        float x = Math::Lerp(sideX, railX, railWeight);
        float y = Math::Lerp(sideY, RailRoadTopY + 0.52f * 0.5f, railWeight);
        float z = Math::Lerp(SideRoadZ - 0.24f, railZ, railWeight);
        float tumble = 0.0f;
        float alpha = 1.0f;

        // 先頭六台は超重戦車の進路から左右へ弾き、残りは混雑を避けて消す
        if (bossEntering) {
            if (i < 6) {
                const float kick = SmoothStep(ShooterStages::Stage4::TrafficKickRate(
                    shooter.m_bossIntroductionTimer, i));
                const float direction = i % 2 == 0 ? -1.0f : 1.0f;
                x += direction * kick * Math::Lerp(3.8f, 10.0f, railWeight);
                y += std::sin(kick * Math::Pi) * Math::Lerp(2.2f, 4.5f, railWeight) + kick;
                z -= kick * railWeight * (2.0f + static_cast<float>(i % 3));
                tumble = direction * kick * Math::Pi * 1.35f;
                alpha = 1.0f - SmoothStep(ShooterStages::Stage4::EntranceRate(
                    shooter.m_bossIntroductionTimer - 82 - i * 3, 42));
            } else {
                alpha = SmoothStep(ShooterStages::Stage4::TrafficFadeAlpha(
                    shooter.m_bossIntroductionTimer));
            }
        }
        if (alpha <= 0.0f) continue;

        // 全部品を車体中心の同じ回転へ乗せて飛散中も一台の車として保つ
        const float cosine = std::cos(tumble);
        const float sine = std::sin(tumble);
        const auto TumblePoint = [&](float partX, float partY, float partZ) {
            const float dx = partX - Math::Lerp(sideX, railX, railWeight);
            const float dy = partY - Math::Lerp(
                sideY, RailRoadTopY + 0.52f * 0.5f, railWeight);
            return Vector3 {x + dx * cosine - dy * sine,
                y + dx * sine + dy * cosine, z + partZ - Math::Lerp(SideRoadZ - 0.24f, railZ, railWeight)};
        };
        const float* bodyColor = i % 2 == 0 ? CarBodyColor : CarAccentColor;
        const float fadedBodyColor[4] = {bodyColor[0], bodyColor[1], bodyColor[2], alpha};
        DrawModelPrimitive(renderer, camera, 1,
            x, y, z,
            Math::Lerp(sideBodyWidth, 2.25f, railWeight),
            Math::Lerp(sideBodyHeight, 0.52f, railWeight),
            Math::Lerp(0.12f, 3.8f, railWeight), fadedBodyColor, 0.0f, tumble);

        // 車体上へキャビンを積み、横向きのシルエットを作る
        const Vector3 cabin = TumblePoint(
            Math::Lerp(sideX + sideDirection * sideBodyWidth * 0.10f, railX, railWeight),
            Math::Lerp(sideY + sideBodyHeight * 0.58f,
                RailRoadTopY + 0.52f + 0.38f * 0.5f, railWeight),
            Math::Lerp(SideRoadZ - 0.34f, railZ - 0.15f, railWeight));
        const float cabinColor[4] = {BuildingColor[0], BuildingColor[1], BuildingColor[2], alpha};
        DrawModelPrimitive(renderer, camera, 1, cabin.x, cabin.y, cabin.z,
            Math::Lerp(sideBodyWidth * 0.48f, 1.42f, railWeight),
            Math::Lerp(sideBodyHeight * 0.72f, 0.38f, railWeight),
            Math::Lerp(0.08f, 1.85f, railWeight), cabinColor, 0.0f, tumble);

        // 2Dでは角張ったタイヤ、3Dへ入ると後部灯火になる同数の部品を描画する
        const float wheelColor[4] = {
            Math::Lerp(BuildingColor[0], LaneColor[0], railWeight),
            Math::Lerp(BuildingColor[1], LaneColor[1], railWeight),
            Math::Lerp(BuildingColor[2], LaneColor[2], railWeight),
            alpha
        };
        for (int side = -1; side <= 1; side += 2) {
            const Vector3 wheel = TumblePoint(
                Math::Lerp(sideX + static_cast<float>(side) * sideBodyWidth * 0.32f,
                    railX + static_cast<float>(side) * 0.67f, railWeight),
                Math::Lerp(sideY - sideBodyHeight * 0.48f,
                    RailRoadTopY + 0.14f * 0.5f, railWeight),
                Math::Lerp(SideRoadZ - 0.38f, railZ - 1.94f, railWeight));
            DrawModelPrimitive(renderer, camera, 1, wheel.x, wheel.y, wheel.z,
                Math::Lerp(sideBodyHeight * 0.56f, 0.24f, railWeight),
                Math::Lerp(sideBodyHeight * 0.56f, 0.14f, railWeight),
                Math::Lerp(0.06f, 0.10f, railWeight), wheelColor, 0.0f, tumble);
        }
    }
}

void SideScrollingShooter::CityBackgroundModule::DrawSky(Renderer& renderer) {
    constexpr float NightBlend = 1.0f;
    const ColorF skyColor {
        Math::Lerp(DaySkyColor[0], NightSkyColor[0], NightBlend),
        Math::Lerp(DaySkyColor[1], NightSkyColor[1], NightBlend),
        Math::Lerp(DaySkyColor[2], NightSkyColor[2], NightBlend),
        1.0f
    };
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, skyColor);
}

void SideScrollingShooter::CityBackgroundModule::DrawBackground2D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();

    // 常夜の空に決定的な星とドット絵調の月を配置する
    constexpr float SkyZ = SidePlaneZ + 11.8f;
    for (int i = 0; i < 42; ++i) {
        const float x = (-0.96f + static_cast<float>((i * 71) % 193) / 100.0f) *
            backgroundHalfWidth;
        const float y = (0.08f + static_cast<float>((i * 43) % 82) / 100.0f) *
            backgroundHalfHeight;
        const float size = i % 9 == 0 ? 0.085f : 0.045f;
        DrawModelPrimitive(renderer, camera, 1,
            x, y, SkyZ, size, size, 0.12f, StarColor);
    }
    const float moonX = backgroundHalfWidth * 0.58f;
    const float moonY = backgroundHalfHeight * 0.58f;
    DrawModelPrimitive(renderer, camera, 1,
        moonX, moonY, SkyZ, 1.05f, 1.05f, 0.14f, MoonColor);
    DrawModelPrimitive(renderer, camera, 1,
        moonX - 0.38f, moonY + 0.30f, SkyZ - 0.01f,
        0.52f, 0.52f, 0.15f, NightSkyColor);

    // ビルを道路上面Y=-6へ接地させ、窓のネオンを前面に重ねる
    DrawModelPrimitive(renderer, camera, 1,
        0.0f, -11.0f, SidePlaneZ + 14.0f,
        60.0f, 10.0f, 0.3f, StreetColor);

    // Stage 4とStage 5で同じ横向きの2.5D道路を描画する
    DrawRoad(shooter, renderer, camera, backgroundHalfWidth, 0.0f);
    if (shooter.m_stageNumber == 4 && (!shooter.m_bossBattle ||
        shooter.m_bossIntroductionPhase == BossIntroductionPhase::Entrance)) {
        DrawTruck(shooter, renderer, camera, 0.0f);
    }

    // Stage5のビル群は専用モデルで同じ配置枠へ描画する
    if (shooter.m_stageNumber == 5) return;
    for (int i = 0; i < CityBuildingCount; ++i) {
        const float x = WrapNdcX(i * CityBuildingNdcSpacing - shooter.m_scroll * 0.18f) *
            (backgroundHalfWidth + 2.0f);
        const float width = 3.65f + static_cast<float>((i * 11) % 3) * 0.38f;
        const float height = 3.8f + static_cast<float>((i * 17) % 5) * 1.18f;
        const float y = -6.0f + height * 0.5f;
        constexpr float Z = SidePlaneZ + 13.7f;
        DrawModelPrimitive(renderer, camera, 1,
            x, y, Z, width, height, 0.42f, BuildingColor);
        for (int row = 0; row < 5; ++row) {
            const float windowY = -5.35f + static_cast<float>(row) * 1.18f;
            if (windowY > -6.0f + height - 0.38f) {
                continue;
            }
            const float* windowColor = (i + row) % 3 == 0 ?
                WindowMagentaColor : WindowCyanColor;
            DrawModelPrimitive(renderer, camera, 1,
                x - width * 0.20f, windowY, Z - 0.24f,
                width * 0.24f, 0.18f, 0.08f, windowColor);
            DrawModelPrimitive(renderer, camera, 1,
                x + width * 0.20f, windowY, Z - 0.24f,
                width * 0.24f, 0.18f, 0.08f, windowColor);
        }
    }
}

void SideScrollingShooter::CityBackgroundModule::DrawBackground3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth = sideBackgroundHalfHeight * renderer.AspectRatio();

    // 星と月を横視点の配置から都市のレール空間へ補間する
    constexpr float SideSkyZ = SidePlaneZ + 11.8f;
    for (int i = 0; i < 42; ++i) {
        const float sideX = (-0.96f + static_cast<float>((i * 71) % 193) / 100.0f) *
            sideBackgroundHalfWidth;
        const float sideY = (0.08f + static_cast<float>((i * 43) % 82) / 100.0f) *
            sideBackgroundHalfHeight;
        const float x = Math::Lerp(sideX,
            -42.0f + static_cast<float>((i * 71) % 840) / 10.0f, railWeight);
        const float y = Math::Lerp(sideY,
            2.0f + static_cast<float>((i * 43) % 150) / 10.0f, railWeight);
        const float z = Math::Lerp(SideSkyZ,
            42.0f + static_cast<float>((i * 29) % 60), railWeight);
        const float sideSize = i % 9 == 0 ? 0.085f : 0.045f;
        const float size = Math::Lerp(
            sideSize, i % 9 == 0 ? 0.15f : 0.08f, railWeight);
        DrawModelPrimitive(renderer, camera, 1,
            x, y, z, size, size, Math::Lerp(0.12f, 0.22f, railWeight), StarColor);
    }
    const float sideMoonX = sideBackgroundHalfWidth * 0.58f;
    const float sideMoonY = sideBackgroundHalfHeight * 0.58f;
    const float moonX = Math::Lerp(sideMoonX, 31.0f, railWeight);
    const float moonY = Math::Lerp(sideMoonY, 15.0f, railWeight);
    const float moonZ = Math::Lerp(SideSkyZ, 88.0f, railWeight);
    DrawModelPrimitive(renderer, camera, 1, moonX, moonY, moonZ,
        Math::Lerp(1.05f, 3.0f, railWeight),
        Math::Lerp(1.05f, 3.0f, railWeight),
        Math::Lerp(0.14f, 0.40f, railWeight), MoonColor);
    DrawModelPrimitive(renderer, camera, 1,
        moonX - Math::Lerp(0.38f, 1.08f, railWeight),
        moonY + Math::Lerp(0.30f, 0.86f, railWeight),
        moonZ - Math::Lerp(0.01f, 0.05f, railWeight),
        Math::Lerp(0.52f, 1.48f, railWeight),
        Math::Lerp(0.52f, 1.48f, railWeight),
        Math::Lerp(0.15f, 0.41f, railWeight), NightSkyColor);

    DrawModelPrimitive(renderer, camera, 1, 0.0f,
        Math::Lerp(-11.0f, -4.0f, railWeight),
        Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
        Math::Lerp(60.0f, 140.0f, railWeight),
        Math::Lerp(10.0f, 0.7f, railWeight),
        Math::Lerp(0.3f, 140.0f, railWeight), StreetColor);

    // 両都市ステージを同じ2D終点からレール道路へ連続変形する
    DrawRoad(shooter, renderer, camera, sideBackgroundHalfWidth, railWeight);
    if (shooter.m_stageNumber == 4 && (!shooter.m_bossBattle ||
        shooter.m_bossIntroductionPhase == BossIntroductionPhase::Entrance)) {
        DrawTruck(shooter, renderer, camera, railWeight);
    }

    // Stage5のビル群は専用モデルで同じ配置枠へ描画する
    if (shooter.m_stageNumber == 5) return;
    for (int i = 0; i < CityBuildingCount; ++i) {
        const bool leftSide = i % 2 == 0;
        const float sideX = WrapNdcX(i * CityBuildingNdcSpacing - shooter.m_scroll * 0.18f) *
            (sideBackgroundHalfWidth + 2.0f);
        const float sideWidth = 3.65f + static_cast<float>((i * 11) % 3) * 0.38f;
        const float sideHeight = 3.8f + static_cast<float>((i * 17) % 5) * 1.18f;
        const float sideY = -6.0f + sideHeight * 0.5f;
        const float railWidth = 4.2f + static_cast<float>(i % 3) * 0.8f;
        const float railHeight = 9.0f + static_cast<float>((i * 17) % 5) * 2.4f;
        const float x = Math::Lerp(sideX, leftSide ? -18.0f : 18.0f, railWeight);
        const float y = Math::Lerp(
            sideY, -3.65f + railHeight * 0.5f, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.7f,
            10.0f + WrapDistance(
                static_cast<float>(i * 29) - shooter.m_scroll * 36.0f, 100.0f),
            railWeight);
        const float width = Math::Lerp(sideWidth, railWidth, railWeight);
        const float height = Math::Lerp(sideHeight, railHeight, railWeight);
        const float depth = Math::Lerp(0.42f, 7.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 1,
            x, y, z, width, height, depth, BuildingColor);
        for (int row = 0; row < 5; ++row) {
            const float sideWindowY = -5.35f + static_cast<float>(row) * 1.18f;
            if (sideWindowY > -6.0f + sideHeight - 0.38f) {
                continue;
            }
            const float* windowColor = (i + row) % 3 == 0 ?
                WindowMagentaColor : WindowCyanColor;
            const float windowY = Math::Lerp(sideWindowY,
                -3.65f + 1.2f + static_cast<float>(row) * 2.1f, railWeight);
            const float windowZ = z - Math::Lerp(0.24f, 3.56f, railWeight);
            const float windowWidth = Math::Lerp(
                sideWidth * 0.24f, 0.75f, railWeight);
            const float windowHeight = Math::Lerp(0.18f, 0.42f, railWeight);
            const float windowDepth = Math::Lerp(0.08f, 0.10f, railWeight);
            DrawModelPrimitive(renderer, camera, 1,
                x - Math::Lerp(sideWidth * 0.20f, railWidth * 0.22f, railWeight),
                windowY, windowZ, windowWidth, windowHeight, windowDepth, windowColor);
            DrawModelPrimitive(renderer, camera, 1,
                x + Math::Lerp(sideWidth * 0.20f, railWidth * 0.22f, railWeight),
                windowY, windowZ, windowWidth, windowHeight, windowDepth, windowColor);
        }
    }
}

void SideScrollingShooter::CityBackgroundModule::DrawTruck(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    const float alpha = shooter.m_bossBattle ?
        SmoothStep(ShooterStages::Stage4::TrafficFadeAlpha(
            shooter.m_bossIntroductionTimer)) : 1.0f;
    if (alpha <= 0.0f) return;
    const float neonIntensity = TruckNeonIntensity(shooter.m_frame);
    const float bodyColor[4] = {TruckBodyColor[0], TruckBodyColor[1], TruckBodyColor[2], alpha};
    const float neonColor[4] = {
        TruckNeonColor[0] * neonIntensity,
        TruckNeonColor[1] * neonIntensity,
        TruckNeonColor[2] * neonIntensity,
        alpha};
    const float accentColor[4] = {
        TruckAccentColor[0] * neonIntensity,
        TruckAccentColor[1] * neonIntensity,
        TruckAccentColor[2] * neonIntensity,
        alpha};
    const float cabinColor[4] = {BuildingColor[0], BuildingColor[1], BuildingColor[2], alpha};
    constexpr float SideRoadZ = SidePlaneZ + 13.55f;
    const float sideX = WrapNdcX(0.47f - shooter.m_scroll * 0.72f) * 18.0f;
    constexpr float SideY = -6.0f + TruckSideHeight * 0.5f;
    constexpr float RailX = 2.8f;
    constexpr float RailY = -3.55f + TruckRailHeight * 0.5f;
    const float railZ = 12.0f + WrapDistance(
        37.0f - shooter.m_scroll * 92.0f, TruckRailCycleLength);
    const float x = Math::Lerp(sideX, RailX, railWeight);
    const float y = Math::Lerp(SideY, RailY, railWeight);
    const float z = Math::Lerp(SideRoadZ - 0.18f, railZ, railWeight);
    const float width = Math::Lerp(TruckSideWidth, TruckRailWidth, railWeight);
    const float height = Math::Lerp(TruckSideHeight, TruckRailHeight, railWeight);
    const float depth = Math::Lerp(0.18f, TruckRailDepth, railWeight);

    // 荷台を道路から高く立ち上げ、大型トラックの箱型シルエットを作る
    DrawModelPrimitive(renderer, camera, 1,
        x, y, z, width, height, depth, bodyColor);

    // 車体後面を囲う発光フレームで暗い市街地でも外形を見失わないようにする
    const float frameZ = z - depth * 0.505f;
    const float frameThickness = Math::Lerp(0.12f, 0.18f, railWeight);
    DrawModelPrimitive(renderer, camera, 1,
        x, y + height * 0.47f, frameZ,
        width, frameThickness, Math::Lerp(0.08f, 0.14f, railWeight), accentColor);
    for (int side = -1; side <= 1; side += 2) {
        DrawModelPrimitive(renderer, camera, 1,
            x + static_cast<float>(side) * width * 0.47f, y, frameZ,
            frameThickness, height, Math::Lerp(0.08f, 0.14f, railWeight),
            accentColor);
    }

    // 発光帯を荷台正面から離し、同一深度面によるちらつきを防ぐ
    const float stripeZ = frameZ - Math::Lerp(0.05f, 0.08f, railWeight);
    const float stripeDepth = Math::Lerp(0.04f, 0.08f, railWeight);
    DrawModelPrimitive(renderer, camera, 1,
        x, y + height * 0.30f, stripeZ,
        width * 0.96f, height * 0.12f, stripeDepth, neonColor);
    DrawModelPrimitive(renderer, camera, 1,
        x, y - height * 0.28f, stripeZ,
        width * 0.98f, height * 0.10f, stripeDepth, accentColor);

    // 前部キャブと灯火を荷台手前の低い位置へ配置する
    const float cabZ = z - Math::Lerp(0.02f, depth * 0.34f, railWeight);
    DrawModelPrimitive(renderer, camera, 1,
        x, Math::Lerp(y, -3.55f + 1.15f, railWeight), cabZ,
        width * 0.88f, Math::Lerp(height * 0.48f, 2.3f, railWeight),
        Math::Lerp(0.20f, depth * 0.30f, railWeight),
        cabinColor);
    for (int side = -1; side <= 1; side += 2) {
        DrawModelPrimitive(renderer, camera, 1,
            x + static_cast<float>(side) * width * 0.34f,
            y - height * 0.23f, z - depth * 0.51f,
            width * 0.12f, height * 0.13f, Math::Lerp(0.10f, 0.16f, railWeight),
            neonColor);
    }
}

bool SideScrollingShooter::CityBackgroundModule::HitsTruck(
    const SideScrollingShooter& shooter,
    float x, float y, float z, float radius) {
    if (shooter.m_stageNumber != 4 || shooter.m_bossBattle) return false;

    // 描画と同じ循環座標と車体寸法で横視点とレール視点を判定する
    if (shooter.IsRailGameplayActive()) {
        constexpr float RailX = 2.8f;
        constexpr float RailY = -3.55f + TruckRailHeight * 0.5f;
        const float railZ = 12.0f + WrapDistance(
            37.0f - shooter.m_scroll * 92.0f, TruckRailCycleLength);
        return SideScrollingShooterShared::HitsEllipsoid(
            ToWorldX(x) - RailX, ToWorldY(y) - RailY, z - railZ,
            TruckRailWidth * 0.5f + radius * WorldXScale,
            TruckRailHeight * 0.5f + radius * WorldYScale,
            TruckRailDepth * 0.5f + radius * WorldXScale);
    }

    const float sideX = WrapNdcX(0.47f - shooter.m_scroll * 0.72f) * 18.0f;
    constexpr float SideY = -6.0f + TruckSideHeight * 0.5f;
    const float dx = (ToWorldX(x) - sideX) /
        (TruckSideWidth * 0.5f + radius * WorldXScale);
    const float dy = (ToWorldY(y) - SideY) /
        (TruckSideHeight * 0.5f + radius * WorldYScale);
    return dx * dx + dy * dy <= 1.0f;
}
