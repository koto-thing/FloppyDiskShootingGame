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
constexpr float RoadColor[4] = {0.075f, 0.09f, 0.16f, 1.0f};
constexpr float LaneColor[4] = {0.72f, 0.84f, 0.88f, 1.0f};
constexpr float CarBodyColor[4] = {0.18f, 0.76f, 0.96f, 1.0f};
constexpr float CarAccentColor[4] = {0.98f, 0.24f, 0.70f, 1.0f};

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

    // 2Dでは道路と車を横スクロールさせる
    constexpr float RoadZ = SidePlaneZ + 13.55f;
    DrawModelPrimitive(renderer, camera, 1,
        0.0f, -10.0f, RoadZ, 60.0f, 8.0f, 0.22f, RoadColor);
    for (int i = 0; i < 9; ++i) {
        const float x = WrapNdcX(i * 0.29f - shooter.m_scroll * 1.15f) *
            (backgroundHalfWidth + 2.0f);
        constexpr float Y = -6.0f + 0.42f * 0.5f;
        const float* bodyColor = i % 2 == 0 ? CarBodyColor : CarAccentColor;
        DrawModelPrimitive(renderer, camera, 1,
            x, Y, RoadZ - 0.16f, 1.75f, 0.42f, 0.12f, bodyColor);
        DrawModelPrimitive(renderer, camera, 1,
            x, Y + 0.30f, RoadZ - 0.18f, 0.92f, 0.28f, 0.13f, BuildingColor);
        DrawModelPrimitive(renderer, camera, 1,
            x + 0.72f, Y, RoadZ - 0.20f, 0.18f, 0.11f, 0.14f, LaneColor);
    }

    for (int i = 0; i < 30; ++i) {
        if (i % 2 != 0) {
            continue;
        }
        const float x = WrapNdcX(i * 0.035f - shooter.m_scroll * 0.18f) *
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

    // レール3D専用の大通りと流れる車列を描画する
    if (railWeight > 0.01f) {
        const float roadWidth = 24.0f * railWeight;
        DrawModelPrimitive(renderer, camera, 1,
            0.0f, -3.60f, 45.0f, roadWidth, 0.10f, 140.0f, RoadColor);
        for (int lane = -1; lane <= 1; ++lane) {
            for (int segment = 0; segment < 16; ++segment) {
                const float z = 4.0f + WrapDistance(
                    static_cast<float>(segment) * 10.0f - shooter.m_scroll * 150.0f, 160.0f);
                DrawModelPrimitive(renderer, camera, 1,
                    static_cast<float>(lane) * 6.0f, -3.55f + 0.025f * 0.5f, z,
                    0.20f, 0.025f, 4.2f, LaneColor);
            }
        }
        for (int i = 0; i < 10; ++i) {
            const float laneX = -8.5f + static_cast<float>(i % 4) * 5.6f;
            const float speed = i % 2 == 0 ? 175.0f : 105.0f;
            const float z = 8.0f + WrapDistance(
                static_cast<float>(i) * 19.0f - shooter.m_scroll * speed, 120.0f);
            const float* bodyColor = i % 2 == 0 ? CarBodyColor : CarAccentColor;
            DrawModelPrimitive(renderer, camera, 1,
                laneX, -3.55f + 0.52f * 0.5f, z,
                2.25f, 0.52f, 3.8f, bodyColor);
            DrawModelPrimitive(renderer, camera, 1,
                laneX, -3.55f + 0.52f + 0.38f * 0.5f, z - 0.15f,
                1.42f, 0.38f, 1.85f, BuildingColor);
            DrawModelPrimitive(renderer, camera, 1,
                laneX - 0.67f, -3.55f + 0.14f * 0.5f, z - 1.94f,
                0.24f, 0.14f, 0.10f, LaneColor);
            DrawModelPrimitive(renderer, camera, 1,
                laneX + 0.67f, -3.55f + 0.14f * 0.5f, z - 1.94f,
                0.24f, 0.14f, 0.10f, LaneColor);
        }
    }

    for (int i = 0; i < 30; ++i) {
        const bool leftSide = i % 2 == 0;
        // 右手前ビルは3Dへ入ってから現れ、2Dでは遠景ビルだけを残す
        if (!leftSide && railWeight <= 0.01f) {
            continue;
        }
        const float sideX = WrapNdcX(i * 0.035f - shooter.m_scroll * 0.18f) *
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
