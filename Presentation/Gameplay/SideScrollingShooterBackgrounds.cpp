#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../../Engine/Graphics/Renderer.h"
#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::OceanFoamColor;
using SideScrollingShooterShared::SideCameraZ;
using SideScrollingShooterShared::SideCameraFieldOfView;

constexpr float SideBackgroundColor[4] = { 0.01f, 0.04f, 0.08f, 1.0f };
constexpr float GridColor[4] = { 0.05f, 0.22f, 0.16f, 1.0f };
constexpr float StarColor[4] = { 0.55f, 0.70f, 0.85f, 1.0f };
constexpr float DesertDaySkyColor[4] = { 0.30f, 0.68f, 0.92f, 1.0f };
constexpr float DesertNightSkyColor[4] = { 0.015f, 0.03f, 0.12f, 1.0f };
constexpr float DesertSandColor[4] = { 0.84f, 0.58f, 0.25f, 1.0f };
constexpr float DesertCactusColor[4] = { 0.08f, 0.34f, 0.16f, 1.0f };

constexpr float OceanWaterColor[4] = { 0.04f, 0.34f, 0.60f, 1.0f };
constexpr float OceanWaveColor[4] = { 0.20f, 0.74f, 0.86f, 1.0f };

constexpr float OceanCloudColor[4] = { 0.90f, 0.95f, 0.96f, 1.0f };
constexpr float OceanSunColor[4] = { 1.00f, 0.82f, 0.20f, 1.0f };

constexpr float CityStreetColor[4] = { 0.025f, 0.05f, 0.11f, 1.0f };
constexpr float CityBuildingColor[4] = { 0.055f, 0.10f, 0.20f, 1.0f };
constexpr float CityWindowCyanColor[4] = { 0.12f, 0.82f, 0.98f, 1.0f };
constexpr float CityWindowMagentaColor[4] = { 0.90f, 0.18f, 0.76f, 1.0f };
constexpr float CityMoonColor[4] = { 0.86f, 0.90f, 0.72f, 1.0f };
constexpr float CityRoadColor[4] = { 0.075f, 0.09f, 0.16f, 1.0f };
constexpr float CityLaneColor[4] = { 0.72f, 0.84f, 0.88f, 1.0f };
constexpr float CityCarBodyColor[4] = { 0.18f, 0.76f, 0.96f, 1.0f };
constexpr float CityCarAccentColor[4] = { 0.98f, 0.24f, 0.70f, 1.0f };

constexpr int Stage2NightStartFrame = 500;
constexpr int Stage2NightFrame = 750;
constexpr int Stage3DawnStartFrame = 500;
constexpr int Stage3DawnFrame = 750;

/**
 * @brief スクロール座標をNDCの横幅へ循環させる
 * @param value 循環前のX座標
 * @return -1.0f以上1.0f未満のX座標
 */
float WrapNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) {
        wrapped += 2.0f;
    }
    return wrapped - 1.0f;
}

/**
 * @brief 距離を0以上の循環範囲へ収める
 * @param value 循環前の距離
 * @param length 循環範囲の長さ
 * @return 0以上length未満の距離
 */
float WrapDistance(float value, float length) {
    float wrapped = std::fmod(value, length);
    if (wrapped < 0.0f) {
        wrapped += length;
    }
    return wrapped;
}

/**
 * @brief ステージ2の空と夜空の星を画面全体へ描画する
 * @param renderer 描画先
 * @param nightBlend 昼から夜への補間率
 * @return なし
 */
void DrawDesertSky(Renderer& renderer, float nightBlend) {
    const ColorF skyColor {
        Math::Lerp(DesertDaySkyColor[0], DesertNightSkyColor[0], nightBlend),
        Math::Lerp(DesertDaySkyColor[1], DesertNightSkyColor[1], nightBlend),
        Math::Lerp(DesertDaySkyColor[2], DesertNightSkyColor[2], nightBlend), 1.0f
    };
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, skyColor);
}

}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"

void SideScrollingShooter::Render(Renderer& renderer) const {
    // 安定した2D表示では全オブジェクトを同じ奥行きへ固定する
    if (!IsRailRenderActive()) {
        Render2D(renderer);
    } else {
        Render3D(renderer);
    }
    DrawBossNameReveal(renderer);
    DrawMissionBanner(renderer);
}

void SideScrollingShooter::Render2D(Renderer& renderer) const {
    const bool isDesert = m_stageNumber == 2;
    const bool isOcean = m_stageNumber == 3;
    const bool isCity = m_stageNumber == 4 || m_stageNumber == 5;
    const float nightBlend = isDesert ? Math::Clamp01(
        static_cast<float>(m_frame - Stage2NightStartFrame) /
        static_cast<float>(Stage2NightFrame - Stage2NightStartFrame)) :
        (isOcean ? 1.0f - Math::Clamp01(
            static_cast<float>(m_frame - Stage3DawnStartFrame) /
            static_cast<float>(Stage3DawnFrame - Stage3DawnStartFrame)) : (isCity ? 1.0f : 0.0f));
    if (isDesert || isOcean || isCity) DrawDesertSky(renderer, nightBlend);

    Camera3D camera;
    ConfigureSideCamera(camera, renderer);
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    // 透視カメラの表示範囲に合わせて背景面を広げ、画面端まで覆う
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();
    const Matrix4x4 backgroundWorld = Matrix4x4::Translation({0.0f, 0.0f, BackgroundZ}) *
        Matrix4x4::Scale({backgroundHalfWidth, backgroundHalfHeight, 1.0f});
    if (!isDesert && !isOcean && !isCity) {
        renderer.Draw({
            PrimitiveShape::Sprite2D,
            camera.ProjectionMatrix() * camera.ViewMatrix() * backgroundWorld,
            Vector3::One,
            {SideBackgroundColor[0], SideBackgroundColor[1], SideBackgroundColor[2], SideBackgroundColor[3]}
        });
    }

    if (isDesert) {
        // 昼はCubeを組み合わせた雲を横方向へ流す
        if (nightBlend < 0.99f) {
            const float cloudColor[4] = {0.88f, 0.91f, 0.88f, 1.0f - nightBlend};
            for (int i = 0; i < 9; ++i) {
                const float x = WrapNdcX(i * 0.47f - m_scroll * 0.08f) * backgroundHalfWidth;
                const float y = backgroundHalfHeight * (0.18f + static_cast<float>((i * 2) % 5) * 0.17f);
                DrawModelPrimitive(renderer, camera, 1, x, y, SidePlaneZ + 11.5f, 1.25f, 0.18f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - 0.72f, y - 0.08f, SidePlaneZ + 11.5f, 0.72f, 0.14f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + 0.76f, y - 0.05f, SidePlaneZ + 11.5f, 0.64f, 0.12f, 0.16f, cloudColor);
            }
        }
        // 夜は偏りのない決定的な配置でCubeの星を表示する
        if (nightBlend > 0.01f) {
            const float starColor[4] = {0.82f, 0.88f, 0.75f, nightBlend};
            for (int i = 0; i < 36; ++i) {
                const float x = (-0.94f + static_cast<float>((i * 73) % 191) / 100.0f) * backgroundHalfWidth;
                const float y = (0.02f + static_cast<float>((i * 37) % 88) / 100.0f) * backgroundHalfHeight;
                const float size = i % 7 == 0 ? 0.075f : 0.045f;
                DrawModelPrimitive(renderer, camera, 1, x, y, SidePlaneZ + 11.8f, size, size, size, starColor);
            }
        }
        // 砂地と角張ったサボテンを横スクロール背景として配置する
        // 上面Y=-6を維持したまま、画面下端より外まで厚みを伸ばす
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
            60.0f, 10.0f, 0.3f, DesertSandColor);
        for (int i = 0; i < 18; ++i) {
            const float x = WrapNdcX(i * 0.41f - m_scroll * 0.32f) * 17.0f;
            // 高さ1.65の幹の底面を砂地上面Y=-6へ合わせる
            constexpr float y = -5.175f;
            constexpr float z = SidePlaneZ + 14.0f;
            DrawModelPrimitive(renderer, camera, 1, x, y, z, 0.32f, 1.65f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.32f, y, z, 0.48f, 0.18f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.52f, y + 0.35f, z, 0.18f, 0.70f, 0.32f, DesertCactusColor);
        }
        DrawDesertBoneArch(renderer, camera, 0.0f);
    } else if (isOcean) {
        // 海面の上端Y=-6を保ち、ドット絵調の波と泡を横スクロールさせる
        const float dayBlend = 1.0f - nightBlend;
        const float waterColor[4] = {
            Math::Lerp(OceanWaterColor[0], OceanWaterColor[0] * 0.30f, nightBlend),
            Math::Lerp(OceanWaterColor[1], OceanWaterColor[1] * 0.36f, nightBlend),
            Math::Lerp(OceanWaterColor[2], OceanWaterColor[2] * 0.48f, nightBlend), 1.0f
        };
        const float waveColor[4] = {
            Math::Lerp(OceanWaveColor[0], OceanWaveColor[0] * 0.38f, nightBlend),
            Math::Lerp(OceanWaveColor[1], OceanWaveColor[1] * 0.42f, nightBlend),
            Math::Lerp(OceanWaveColor[2], OceanWaveColor[2] * 0.55f, nightBlend), 1.0f
        };
        const float foamColor[4] = {OceanFoamColor[0], OceanFoamColor[1], OceanFoamColor[2], 1.0f - nightBlend * 0.35f};
        const float cloudColor[4] = {OceanCloudColor[0], OceanCloudColor[1], OceanCloudColor[2], dayBlend};
        const float sunColor[4] = {OceanSunColor[0], OceanSunColor[1], OceanSunColor[2], dayBlend};
        // 朝に合わせて太陽を昇らせ、Cubeの雲を空の上部へ流す
        if (dayBlend > 0.01f) {
            const float sunX = backgroundHalfWidth * 0.56f;
            const float sunY = backgroundHalfHeight * (0.26f + dayBlend * 0.30f);
            constexpr float skyZ = SidePlaneZ + 11.6f;
            DrawModelPrimitive(renderer, camera, 1, sunX, sunY, skyZ, 1.05f, 1.05f, 0.14f, sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX - 0.62f, sunY, skyZ, 0.28f, 0.28f, 0.15f, sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX + 0.62f, sunY, skyZ, 0.28f, 0.28f, 0.15f, sunColor);
            for (int i = 0; i < 7; ++i) {
                const float x = WrapNdcX(i * 0.53f - m_scroll * 0.06f) * backgroundHalfWidth;
                const float y = backgroundHalfHeight * (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
                DrawModelPrimitive(renderer, camera, 1, x, y, skyZ, 1.15f, 0.18f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - 0.62f, y - 0.07f, skyZ, 0.62f, 0.13f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + 0.68f, y - 0.05f, skyZ, 0.54f, 0.12f, 0.16f, cloudColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
            60.0f, 10.0f, 0.3f, waterColor);
        for (int i = 0; i < 24; ++i) {
            const float x = WrapNdcX(i * 0.29f - m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
            const float y = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
            const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
            constexpr float z = SidePlaneZ + 13.6f;
            DrawModelPrimitive(renderer, camera, 1, x, y, z, width, 0.10f, 0.18f, waveColor);
            if (i % 3 == 0) {
                DrawModelPrimitive(renderer, camera, 1, x - width * 0.18f, y + 0.16f, z - 0.02f,
                    width * 0.42f, 0.07f, 0.19f, foamColor);
            }
        }
        DrawOceanSeaSerpent(renderer, camera, 0.0f);
    } else if (isCity) {
        // 常夜の空に決定的な星と、ドット絵調の月を配置する
        constexpr float skyZ = SidePlaneZ + 11.8f;
        for (int i = 0; i < 42; ++i) {
            const float x = (-0.96f + static_cast<float>((i * 71) % 193) / 100.0f) * backgroundHalfWidth;
            const float y = (0.08f + static_cast<float>((i * 43) % 82) / 100.0f) * backgroundHalfHeight;
            const float size = i % 9 == 0 ? 0.085f : 0.045f;
            DrawModelPrimitive(renderer, camera, 1, x, y, skyZ, size, size, 0.12f, StarColor);
        }
        const float moonX = backgroundHalfWidth * 0.58f;
        const float moonY = backgroundHalfHeight * 0.58f;
        DrawModelPrimitive(renderer, camera, 1, moonX, moonY, skyZ, 1.05f, 1.05f, 0.14f, CityMoonColor);
        DrawModelPrimitive(renderer, camera, 1, moonX - 0.38f, moonY + 0.30f, skyZ - 0.01f,
            0.52f, 0.52f, 0.15f, DesertNightSkyColor);
        // ビルは道路上面Y=-6へ接地させ、窓のネオンを前面に重ねる
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
            60.0f, 10.0f, 0.3f, CityStreetColor);
        // 2Dでは道路と車を横スクロールさせ、3Dの右手前ビルは描画しない
        constexpr float roadZ = SidePlaneZ + 13.55f;
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -10.0f, roadZ, 60.0f, 8.0f, 0.22f, CityRoadColor);
        for (int i = 0; i < 9; ++i) {
            const float x = WrapNdcX(i * 0.29f - m_scroll * 1.15f) * (backgroundHalfWidth + 2.0f);
            // 車体の底面を道路上面Y=-6へ接地させる
            constexpr float y = -6.0f + 0.42f * 0.5f;
            const float* bodyColor = i % 2 == 0 ? CityCarBodyColor : CityCarAccentColor;
            DrawModelPrimitive(renderer, camera, 1, x, y, roadZ - 0.16f, 1.75f, 0.42f, 0.12f, bodyColor);
            DrawModelPrimitive(renderer, camera, 1, x, y + 0.30f, roadZ - 0.18f, 0.92f, 0.28f, 0.13f, CityBuildingColor);
            DrawModelPrimitive(renderer, camera, 1, x + 0.72f, y, roadZ - 0.20f, 0.18f, 0.11f, 0.14f, CityLaneColor);
        }
        for (int i = 0; i < 30; ++i) {
            if (i % 2 != 0) continue;
            const float x = WrapNdcX(i * 0.035f - m_scroll * 0.18f) * (backgroundHalfWidth + 2.0f);
            const float width = 3.65f + static_cast<float>((i * 11) % 3) * 0.38f;
            const float height = 3.8f + static_cast<float>((i * 17) % 5) * 1.18f;
            const float y = -6.0f + height * 0.5f;
            constexpr float z = SidePlaneZ + 13.7f;
            DrawModelPrimitive(renderer, camera, 1, x, y, z, width, height, 0.42f, CityBuildingColor);
            for (int row = 0; row < 5; ++row) {
                const float windowY = -5.35f + static_cast<float>(row) * 1.18f;
                if (windowY > -6.0f + height - 0.38f) continue;
                const float* windowColor = (i + row) % 3 == 0 ? CityWindowMagentaColor : CityWindowCyanColor;
                DrawModelPrimitive(renderer, camera, 1, x - width * 0.20f, windowY, z - 0.24f,
                    width * 0.24f, 0.18f, 0.08f, windowColor);
                DrawModelPrimitive(renderer, camera, 1, x + width * 0.20f, windowY, z - 0.24f,
                    width * 0.24f, 0.18f, 0.08f, windowColor);
            }
        }
    } else {
        // 遷移描画と同じ星・グリッド配置を使い、切り替え完了時の交換を防ぐ
        for (int i = 0; i < 28; ++i) {
            float x = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
            float y = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
            DrawModelPrimitive(renderer, camera, 1, x * backgroundHalfWidth, y * backgroundHalfHeight, SidePlaneZ + 12.0f,
                0.06f, 0.06f, 0.06f, StarColor);
        }
        for (int i = 0; i < 8; ++i) {
            float x = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
            DrawModelPrimitive(renderer, camera, 1, x * 20.0f, 0.0f, SidePlaneZ + 7.0f,
                0.04f, 18.0f, 0.08f, GridColor);
        }
        for (int i = 0; i < 12; ++i) {
            DrawModelPrimitive(renderer, camera, 1, 0.0f, (-0.90f + (i % 6) * 0.36f) * 9.0f, SidePlaneZ + 7.0f,
                42.0f, 0.04f, 0.08f, GridColor);
        }
        DrawStage1Meteor(renderer, camera, 0.0f);
    }

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        Enemy sideEnemy = enemy;
        sideEnemy.z = SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f);
        if (enemy.type == 2) {
            DrawEnemyModel(renderer, camera, sideEnemy, Math::HalfPi);
        } else {
            DrawEnemyModel(renderer, camera, sideEnemy, Math::HalfPi);
        }
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot sideShot = shot;
        sideShot.z = SidePlaneZ + (shot.enemy ? 1.0f : -0.4f);
        DrawShotModel(renderer, camera, sideShot, Math::HalfPi);
    }
    for (const auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        Explosion sideExplosion = explosion;
        sideExplosion.z = SidePlaneZ - 0.8f;
        DrawExplosion(renderer, camera, sideExplosion);
    }
    for (const auto& debris : m_debris) {
        if (!debris.active) continue;
        Debris sideDebris = debris;
        sideDebris.z = SidePlaneZ - 0.6f;
        DrawDebris(renderer, camera, sideDebris, 0.0f);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        Item sideItem = item;
        sideItem.z = SidePlaneZ - 0.2f;
        DrawItemModel(renderer, camera, sideItem, Math::HalfPi);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        SidePlaneZ, m_playerDestructionTimer == 0 &&
        (m_invincible == 0 || (m_invincible / 5) % 2 == 0), Math::HalfPi);

    renderer.ResetCamera();
    DrawStage5Weather(renderer);
    DrawAttackWarnings2D(renderer);

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / m_stage->BossStartDistance() * 100.0f));
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText(IsStage5ViewLocked() ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  3D MODE LOCKED" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });

    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
}

void SideScrollingShooter::Render3D(Renderer& renderer) const {
    const bool isDesert = m_stageNumber == 2;
    const bool isOcean = m_stageNumber == 3;
    const bool isCity = m_stageNumber == 4 || m_stageNumber == 5;
    const bool isTower = m_stageNumber == 5;
    const float nightBlend = isDesert ? Math::Clamp01(
        static_cast<float>(m_frame - Stage2NightStartFrame) /
        static_cast<float>(Stage2NightFrame - Stage2NightStartFrame)) :
        (isOcean ? 1.0f - Math::Clamp01(
            static_cast<float>(m_frame - Stage3DawnStartFrame) /
            static_cast<float>(Stage3DawnFrame - Stage3DawnStartFrame)) : (isCity ? 1.0f : 0.0f));
    if (isDesert || isOcean || isCity) {
        // カメラ切り替えに影響されない画面背景として空を描画する
        DrawDesertSky(renderer, nightBlend);
    }

    Camera3D camera;
    ConfigureRailCamera(camera, renderer);
    const float railWeight = RailBlend();
    const float playerYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    const float enemyYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth = sideBackgroundHalfHeight * renderer.AspectRatio();
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    if (isDesert) {
        // 横視点とレール視点で同じ3D砂漠を補間して背景の飛びを防ぐ
        const float sandColor[4] = {
            Math::Lerp(DesertSandColor[0], DesertSandColor[0] * 0.32f, nightBlend),
            Math::Lerp(DesertSandColor[1], DesertSandColor[1] * 0.32f, nightBlend),
            Math::Lerp(DesertSandColor[2], DesertSandColor[2] * 0.45f, nightBlend), 1.0f
        };
        // 昼の雲と夜の星はCubeだけで構成し、視点変更に合わせて奥行きへ展開する
        if (nightBlend < 0.99f) {
            const float cloudColor[4] = {0.88f, 0.91f, 0.88f, 1.0f - nightBlend};
            for (int i = 0; i < 9; ++i) {
                const float sideX = WrapNdcX(i * 0.47f - m_scroll * 0.08f) * sideBackgroundHalfWidth;
                const float sideY = sideBackgroundHalfHeight * (0.18f + static_cast<float>((i * 2) % 5) * 0.17f);
                const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
                const float railY = 5.0f + static_cast<float>((i * 7) % 12);
                const float railZ = 24.0f + static_cast<float>((i * 37) % 88);
                const float x = Math::Lerp(sideX, railX, railWeight);
                const float y = Math::Lerp(sideY, railY, railWeight);
                const float z = Math::Lerp(SidePlaneZ + 11.5f, railZ, railWeight);
                DrawModelPrimitive(renderer, camera, 1, x, y, z,
                    Math::Lerp(1.25f, 2.4f, railWeight), Math::Lerp(0.18f, 0.32f, railWeight),
                    Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - Math::Lerp(0.72f, 1.4f, railWeight),
                    y - Math::Lerp(0.08f, 0.15f, railWeight), z,
                    Math::Lerp(0.72f, 1.3f, railWeight), Math::Lerp(0.14f, 0.24f, railWeight),
                    Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + Math::Lerp(0.76f, 1.5f, railWeight),
                    y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                    Math::Lerp(0.64f, 1.1f, railWeight), Math::Lerp(0.12f, 0.20f, railWeight),
                    Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
            }
        }
        if (nightBlend > 0.01f) {
            const float starColor[4] = {0.82f, 0.88f, 0.75f, nightBlend};
            for (int i = 0; i < 36; ++i) {
                const float sideX = (-0.94f + static_cast<float>((i * 73) % 191) / 100.0f) * sideBackgroundHalfWidth;
                const float sideY = (0.02f + static_cast<float>((i * 37) % 88) / 100.0f) * sideBackgroundHalfHeight;
                const float railX = -24.0f + static_cast<float>((i * 73) % 480) / 10.0f;
                const float railY = 1.5f + static_cast<float>((i * 37) % 135) / 10.0f;
                const float railZ = 45.0f + static_cast<float>((i * 29) % 48);
                const float x = Math::Lerp(sideX, railX, railWeight);
                const float y = Math::Lerp(sideY, railY, railWeight);
                const float z = Math::Lerp(SidePlaneZ + 11.8f, railZ, railWeight);
                const float sideSize = i % 7 == 0 ? 0.075f : 0.045f;
                const float size = Math::Lerp(sideSize, i % 7 == 0 ? 0.12f : 0.07f, railWeight);
                DrawModelPrimitive(renderer, camera, 1, x, y, z, size, size, size, starColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(-11.0f, -4.0f, railWeight), Math::Lerp(24.0f, 45.0f, railWeight),
            Math::Lerp(60.0f, 140.0f, railWeight), Math::Lerp(10.0f, 0.7f, railWeight),
            Math::Lerp(0.3f, 140.0f, railWeight), sandColor);
        for (int i = 0; i < 18; ++i) {
            const float sideX = WrapNdcX(i * 0.41f - m_scroll * 0.32f) * 17.0f;
            const float railX = -55.0f + static_cast<float>((i * 73) % 1100) / 10.0f;
            constexpr float sideZ = SidePlaneZ + 14.0f;
            const float railZ = 8.0f +
                WrapDistance(static_cast<float>(i * 43) - m_scroll * 28.0f, 110.0f);
            const float x = Math::Lerp(sideX, railX, railWeight);
            const float y = Math::Lerp(-5.175f, -2.825f, railWeight);
            const float z = Math::Lerp(sideZ, railZ, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z, 0.32f, 1.65f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.32f, y, z, 0.48f, 0.18f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.52f, y + 0.35f, z, 0.18f, 0.70f, 0.32f, DesertCactusColor);
        }
        DrawDesertBoneArch(renderer, camera, railWeight);
    } else if (isOcean) {
        // 横視点の海面と同じ波配置をレール空間へ補間し、遷移開始時の飛びを防ぐ
        const float dayBlend = 1.0f - nightBlend;
        const float waterColor[4] = {
            Math::Lerp(OceanWaterColor[0], OceanWaterColor[0] * 0.30f, nightBlend),
            Math::Lerp(OceanWaterColor[1], OceanWaterColor[1] * 0.36f, nightBlend),
            Math::Lerp(OceanWaterColor[2], OceanWaterColor[2] * 0.48f, nightBlend), 1.0f
        };
        const float waveColor[4] = {
            Math::Lerp(OceanWaveColor[0], OceanWaveColor[0] * 0.38f, nightBlend),
            Math::Lerp(OceanWaveColor[1], OceanWaveColor[1] * 0.42f, nightBlend),
            Math::Lerp(OceanWaveColor[2], OceanWaveColor[2] * 0.55f, nightBlend), 1.0f
        };
        const float foamColor[4] = {OceanFoamColor[0], OceanFoamColor[1], OceanFoamColor[2], 1.0f - nightBlend * 0.35f};
        const float cloudColor[4] = {OceanCloudColor[0], OceanCloudColor[1], OceanCloudColor[2], dayBlend};
        const float sunColor[4] = {OceanSunColor[0], OceanSunColor[1], OceanSunColor[2], dayBlend};
        // 太陽と雲は横視点の配置からレール空間へ補間する
        if (dayBlend > 0.01f) {
            const float sideSunX = sideBackgroundHalfWidth * 0.56f;
            const float sideSunY = sideBackgroundHalfHeight * (0.26f + dayBlend * 0.30f);
            const float sunX = Math::Lerp(sideSunX, 32.0f, railWeight);
            const float sunY = Math::Lerp(sideSunY, 14.0f, railWeight);
            const float sunZ = Math::Lerp(SidePlaneZ + 11.6f, 72.0f, railWeight);
            DrawModelPrimitive(renderer, camera, 1, sunX, sunY, sunZ,
                Math::Lerp(1.05f, 2.8f, railWeight), Math::Lerp(1.05f, 2.8f, railWeight),
                Math::Lerp(0.14f, 0.35f, railWeight), sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX - Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
                Math::Lerp(0.28f, 0.70f, railWeight), Math::Lerp(0.28f, 0.70f, railWeight),
                Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX + Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
                Math::Lerp(0.28f, 0.70f, railWeight), Math::Lerp(0.28f, 0.70f, railWeight),
                Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
            for (int i = 0; i < 7; ++i) {
                const float sideX = WrapNdcX(i * 0.53f - m_scroll * 0.06f) * sideBackgroundHalfWidth;
                const float sideY = sideBackgroundHalfHeight * (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
                const float x = Math::Lerp(sideX, -42.0f + static_cast<float>((i * 73) % 840) / 10.0f, railWeight);
                const float y = Math::Lerp(sideY, 8.0f + static_cast<float>((i * 7) % 10), railWeight);
                const float z = Math::Lerp(SidePlaneZ + 11.6f, 32.0f + static_cast<float>((i * 37) % 70), railWeight);
                DrawModelPrimitive(renderer, camera, 1, x, y, z,
                    Math::Lerp(1.15f, 2.2f, railWeight), Math::Lerp(0.18f, 0.30f, railWeight),
                    Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - Math::Lerp(0.62f, 1.2f, railWeight),
                    y - Math::Lerp(0.07f, 0.12f, railWeight), z,
                    Math::Lerp(0.62f, 1.1f, railWeight), Math::Lerp(0.13f, 0.22f, railWeight),
                    Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + Math::Lerp(0.68f, 1.3f, railWeight),
                    y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                    Math::Lerp(0.54f, 1.0f, railWeight), Math::Lerp(0.12f, 0.20f, railWeight),
                    Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(-11.0f, -4.0f, railWeight), Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
            Math::Lerp(60.0f, 140.0f, railWeight), Math::Lerp(10.0f, 0.7f, railWeight),
            Math::Lerp(0.3f, 140.0f, railWeight), waterColor);
        for (int i = 0; i < 24; ++i) {
            const float sideX = WrapNdcX(i * 0.29f - m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
            const float sideY = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
            const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
            const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
            const float railZ = 8.0f + std::fmod(static_cast<float>(i * 43) - m_scroll * 28.0f + 110.0f, 110.0f);
            const float x = Math::Lerp(sideX, railX, railWeight);
            // 波の底面を海面上面Y=-3.65へ接地させる
            const float y = Math::Lerp(sideY, -3.65f + 0.045f * 0.5f, railWeight);
            const float z = Math::Lerp(SidePlaneZ + 13.6f, railZ, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z,
                Math::Lerp(width, 1.5f, railWeight), Math::Lerp(0.10f, 0.045f, railWeight),
                Math::Lerp(0.18f, 0.70f, railWeight), waveColor);
            if (i % 3 == 0) {
                const float foamY = Math::Lerp(sideY + 0.16f, -3.65f + 0.045f + 0.03f * 0.5f, railWeight);
                DrawModelPrimitive(renderer, camera, 1,
                    x - Math::Lerp(width * 0.18f, 0.25f, railWeight),
                    foamY, z - Math::Lerp(0.02f, 0.08f, railWeight),
                    Math::Lerp(width * 0.42f, 0.65f, railWeight), Math::Lerp(0.07f, 0.03f, railWeight),
                    Math::Lerp(0.19f, 0.72f, railWeight), foamColor);
            }
        }
        DrawOceanSeaSerpent(renderer, camera, railWeight);
    } else if (isCity) {
        // 星・月・ビル群は横視点の配置から都市のレール空間へ補間する
        constexpr float sideSkyZ = SidePlaneZ + 11.8f;
        for (int i = 0; i < 42; ++i) {
            const float sideX = (-0.96f + static_cast<float>((i * 71) % 193) / 100.0f) * sideBackgroundHalfWidth;
            const float sideY = (0.08f + static_cast<float>((i * 43) % 82) / 100.0f) * sideBackgroundHalfHeight;
            const float x = Math::Lerp(sideX, -42.0f + static_cast<float>((i * 71) % 840) / 10.0f, railWeight);
            const float y = Math::Lerp(sideY, 2.0f + static_cast<float>((i * 43) % 150) / 10.0f, railWeight);
            const float z = Math::Lerp(sideSkyZ, 42.0f + static_cast<float>((i * 29) % 60), railWeight);
            const float sideSize = i % 9 == 0 ? 0.085f : 0.045f;
            const float size = Math::Lerp(sideSize, i % 9 == 0 ? 0.15f : 0.08f, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z, size, size, Math::Lerp(0.12f, 0.22f, railWeight), StarColor);
        }
        const float sideMoonX = sideBackgroundHalfWidth * 0.58f;
        const float sideMoonY = sideBackgroundHalfHeight * 0.58f;
        const float moonX = Math::Lerp(sideMoonX, 31.0f, railWeight);
        const float moonY = Math::Lerp(sideMoonY, 15.0f, railWeight);
        const float moonZ = Math::Lerp(sideSkyZ, 88.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 1, moonX, moonY, moonZ,
            Math::Lerp(1.05f, 3.0f, railWeight), Math::Lerp(1.05f, 3.0f, railWeight),
            Math::Lerp(0.14f, 0.40f, railWeight), CityMoonColor);
        DrawModelPrimitive(renderer, camera, 1,
            moonX - Math::Lerp(0.38f, 1.08f, railWeight), moonY + Math::Lerp(0.30f, 0.86f, railWeight),
            moonZ - Math::Lerp(0.01f, 0.05f, railWeight),
            Math::Lerp(0.52f, 1.48f, railWeight), Math::Lerp(0.52f, 1.48f, railWeight),
            Math::Lerp(0.15f, 0.41f, railWeight), DesertNightSkyColor);
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(-11.0f, -4.0f, railWeight), Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
            Math::Lerp(60.0f, 140.0f, railWeight), Math::Lerp(10.0f, 0.7f, railWeight),
            Math::Lerp(0.3f, 140.0f, railWeight), CityStreetColor);
        // レール3D専用の大通りと流れる車列を描画する
        if (railWeight > 0.01f) {
            const float roadWidth = 24.0f * railWeight;
            DrawModelPrimitive(renderer, camera, 1, 0.0f, -3.60f, 45.0f,
                roadWidth, 0.10f, 140.0f, CityRoadColor);
            for (int lane = -1; lane <= 1; ++lane) {
                for (int segment = 0; segment < 16; ++segment) {
                    const float z = 4.0f + WrapDistance(static_cast<float>(segment) * 10.0f - m_scroll * 150.0f, 160.0f);
                    DrawModelPrimitive(renderer, camera, 1, static_cast<float>(lane) * 6.0f, -3.55f + 0.025f * 0.5f, z,
                        0.20f, 0.025f, 4.2f, CityLaneColor);
                }
            }
            for (int i = 0; i < 10; ++i) {
                const float laneX = -8.5f + static_cast<float>(i % 4) * 5.6f;
                const float speed = i % 2 == 0 ? 175.0f : 105.0f;
                const float z = 8.0f + WrapDistance(static_cast<float>(i) * 19.0f - m_scroll * speed, 120.0f);
                const float* bodyColor = i % 2 == 0 ? CityCarBodyColor : CityCarAccentColor;
                DrawModelPrimitive(renderer, camera, 1, laneX, -3.55f + 0.52f * 0.5f, z, 2.25f, 0.52f, 3.8f, bodyColor);
                DrawModelPrimitive(renderer, camera, 1, laneX, -3.55f + 0.52f + 0.38f * 0.5f, z - 0.15f,
                    1.42f, 0.38f, 1.85f, CityBuildingColor);
                DrawModelPrimitive(renderer, camera, 1, laneX - 0.67f, -3.55f + 0.14f * 0.5f, z - 1.94f,
                    0.24f, 0.14f, 0.10f, CityLaneColor);
                DrawModelPrimitive(renderer, camera, 1, laneX + 0.67f, -3.55f + 0.14f * 0.5f, z - 1.94f,
                    0.24f, 0.14f, 0.10f, CityLaneColor);
            }
        }
        for (int i = 0; i < 30; ++i) {
            const bool leftSide = i % 2 == 0;
            // 右手前ビルは3Dへ入ってから現れ、2Dでは遠景ビルだけを残す
            if (!leftSide && railWeight <= 0.01f) continue;
            const float sideX = WrapNdcX(i * 0.035f - m_scroll * 0.18f) * (sideBackgroundHalfWidth + 2.0f);
            const float sideWidth = 3.65f + static_cast<float>((i * 11) % 3) * 0.38f;
            const float sideHeight = 3.8f + static_cast<float>((i * 17) % 5) * 1.18f;
            const float sideY = -6.0f + sideHeight * 0.5f;
            const float railWidth = 4.2f + static_cast<float>(i % 3) * 0.8f;
            const float railHeight = 9.0f + static_cast<float>((i * 17) % 5) * 2.4f;
            const float x = Math::Lerp(sideX, leftSide ? -18.0f : 18.0f, railWeight);
            const float y = Math::Lerp(sideY, -3.65f + railHeight * 0.5f, railWeight);
            const float z = Math::Lerp(SidePlaneZ + 13.7f,
                10.0f + WrapDistance(static_cast<float>(i * 29) - m_scroll * 36.0f, 100.0f), railWeight);
            const float width = Math::Lerp(sideWidth, railWidth, railWeight);
            const float height = Math::Lerp(sideHeight, railHeight, railWeight);
            const float depth = Math::Lerp(0.42f, 7.0f, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z, width, height, depth, CityBuildingColor);
            for (int row = 0; row < 5; ++row) {
                const float sideWindowY = -5.35f + static_cast<float>(row) * 1.18f;
                if (sideWindowY > -6.0f + sideHeight - 0.38f) continue;
                const float* windowColor = (i + row) % 3 == 0 ? CityWindowMagentaColor : CityWindowCyanColor;
                const float windowY = Math::Lerp(sideWindowY, -3.65f + 1.2f + static_cast<float>(row) * 2.1f, railWeight);
                const float windowZ = z - Math::Lerp(0.24f, 3.56f, railWeight);
                const float windowWidth = Math::Lerp(sideWidth * 0.24f, 0.75f, railWeight);
                const float windowHeight = Math::Lerp(0.18f, 0.42f, railWeight);
                const float windowDepth = Math::Lerp(0.08f, 0.10f, railWeight);
                DrawModelPrimitive(renderer, camera, 1,
                    x - Math::Lerp(sideWidth * 0.20f, railWidth * 0.22f, railWeight), windowY, windowZ,
                    windowWidth, windowHeight, windowDepth, windowColor);
                DrawModelPrimitive(renderer, camera, 1,
                    x + Math::Lerp(sideWidth * 0.20f, railWidth * 0.22f, railWeight), windowY, windowZ,
                    windowWidth, windowHeight, windowDepth, windowColor);
            }
        }
    } else {
        // 遷移開始直後は2D背景の配置を保ち、画面全体が反転して見える初期ジャンプを避ける
        for (int i = 0; i < 28; ++i) {
        const float sideX = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
        const float sideY = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        const float railX = -13.0f + static_cast<float>((i * 37) % 260) / 10.0f;
        const float railY = -6.0f + static_cast<float>((i * 53) % 120) / 10.0f;
        const float railZ = 8.0f + static_cast<float>((i * 29 + m_frame) % 540) / 9.0f;
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(sideX * sideBackgroundHalfWidth, railX, railWeight),
            Math::Lerp(sideY * sideBackgroundHalfHeight, railY, railWeight),
            Math::Lerp(SidePlaneZ + 12.0f, railZ, railWeight),
            0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f,
            StarColor);
    }
    // 前半で2D縦線を下へ沈めて潰し、後半で3D床グリッドへ展開する
    const float gridLowerWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f));
    const float gridMoveWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f - 1.0f));
    for (int i = 0; i < 8; ++i) {
        const float x = -35.0f + i * 10.0f;
        const float sideX = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(sideX * 20.0f, x, gridMoveWeight),
            Math::Lerp(0.0f, -3.3f, gridLowerWeight),
            Math::Lerp(SidePlaneZ + 7.0f, 32.0f, gridMoveWeight),
            Math::Lerp(0.04f, 0.025f, gridMoveWeight),
            Math::Lerp(18.0f, 0.025f, gridLowerWeight),
            Math::Lerp(0.08f, 140.0f, gridMoveWeight),
            GridColor);
    }
    for (int i = 0; i < 12; ++i) {
        const float z = 6.0f + i * 5.5f - std::fmod(m_scroll * 80.0f, 5.5f);
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp((-0.90f + (i % 6) * 0.36f) * 9.0f, -3.3f, gridLowerWeight),
            Math::Lerp(SidePlaneZ + 7.0f, z, gridMoveWeight),
            Math::Lerp(42.0f, 100.0f, gridMoveWeight),
            Math::Lerp(0.04f, 0.025f, gridMoveWeight),
            Math::Lerp(0.08f, 0.025f, gridMoveWeight),
            GridColor);
    }
    DrawStage1Meteor(renderer, camera, railWeight);
    }

    if (isTower) RenderStage5(renderer, camera);

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        if (m_stageNumber == 5 && m_stage5Phase == Stage5Phase::EastsourceIntro &&
            m_stage5PhaseTimer < 58 && enemy.type == Stage::BossEnemy) continue;
        Enemy drawEnemy = enemy;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? enemy.transitionSideX :
            (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        drawEnemy.x = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
        drawEnemy.y = Math::Lerp(sideY, exitingRail ? enemy.transitionSideY : enemy.y, railWeight);
        const float railZ = exitingRail ? enemy.transitionRailZ : enemy.z;
        drawEnemy.z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), railZ, railWeight);
        if (enemy.type != 2) {
            const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
            const float minimumRailY = FromWorldY(groundTopY + 0.32f);
            drawEnemy.y = Math::Lerp(drawEnemy.y, (std::max)(drawEnemy.y, minimumRailY), railWeight);
        }

        // レール3Dへ入るほど機体直下の影を表示する
        if (railWeight > 0.01f) {
            const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
            const bool isBoss = enemy.type == 2;
            DrawBlobShadow(renderer, camera, ToWorldX(drawEnemy.x), drawEnemy.z, groundTopY,
                isBoss ? 2.4f : 0.72f, isBoss ? 2.0f : 0.58f,
                railWeight * (isBoss ? 0.34f : 0.26f));
        }
        DrawEnemyModel(renderer, camera, drawEnemy, enemyYaw);
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot drawShot = shot;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? shot.transitionSideX : shot.x;
        const float sideY = enteringRail ? shot.transitionSideY : shot.y;
        drawShot.x = Math::Lerp(sideX, exitingRail ? shot.transitionSideX : shot.x, railWeight);
        drawShot.y = Math::Lerp(sideY, shot.y, railWeight);
        drawShot.z = Math::Lerp(SidePlaneZ + (shot.enemy ? 1.0f : -0.4f), shot.z, railWeight);
        DrawShotModel(renderer, camera, drawShot, shot.enemy ? enemyYaw : playerYaw);
    }
    for (const auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        DrawExplosion(renderer, camera, explosion);
    }
    for (const auto& debris : m_debris) {
        if (!debris.active) continue;
        DrawDebris(renderer, camera, debris, railWeight);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        DrawItemModel(renderer, camera, item, playerYaw);
    }
    const bool playerVisible = m_playerDestructionTimer == 0 &&
        (m_invincible == 0 || (m_invincible / 5) % 2 == 0);
    if (railWeight > 0.01f && playerVisible) {
        const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
        DrawBlobShadow(renderer, camera, ToWorldX(m_playerX),
            Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight), groundTopY,
            1.05f, 0.82f, railWeight * 0.30f);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight),
        playerVisible, playerYaw);
    DrawAttackWarnings3D(renderer, camera, railWeight);

    renderer.ResetCamera();
    DrawStage5Weather(renderer);

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / m_stage->BossStartDistance() * 100.0f));
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText(IsStage5ViewLocked() ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  3D MODE LOCKED" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    if (m_viewTransitionTimer > 0) {
        renderer.DrawText("CAMERA SHIFT", { -0.16f, -0.02f }, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f });
    }
    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
}
