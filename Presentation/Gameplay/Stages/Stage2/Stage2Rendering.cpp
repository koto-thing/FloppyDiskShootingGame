#include "Stage2Module.h"

#include <algorithm>
#include <cmath>

#include "../../../../Engine/Graphics/Renderer.h"
#include "Stage2BossModelView.h"
#include "../../SideScrollingShooterShared.h"

namespace {
using ShooterStages::Stage2::BossAction;
using ShooterStages::Stage2::DebrisKind;
using ShooterStages::Stage2::ShotKind;
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::SideCameraZ;

constexpr float DesertDaySkyColor[4] = {0.30f, 0.68f, 0.92f, 1.0f};
constexpr float DesertNightSkyColor[4] = {0.015f, 0.03f, 0.12f, 1.0f};
constexpr float DesertSandColor[4] = {0.84f, 0.58f, 0.25f, 1.0f};
constexpr float DesertCactusColor[4] = {0.08f, 0.34f, 0.16f, 1.0f};
constexpr float DesertBoneColor[4] = {0.88f, 0.78f, 0.56f, 1.0f};
constexpr int NightStartFrame = 500;
constexpr int NightFrame = 750;

/**
 * @brief Y軸回転したローカル座標を取得する
 * @param x ローカルX座標
 * @param y ローカルY座標
 * @param z ローカルZ座標
 * @param yaw Y軸回転
 * @return 回転後のオフセット
 */
Vector3 RotateYawOffset(float x, float y, float z, float yaw) {
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    return {x * cosine + z * sine, y, -x * sine + z * cosine};
}

}

float SideScrollingShooter::Stage2Module::NightBlend(
    const SideScrollingShooter& shooter) {
    return Math::Clamp01(static_cast<float>(shooter.m_frame - NightStartFrame) /
        static_cast<float>(NightFrame - NightStartFrame));
}

float SideScrollingShooter::Stage2Module::WrapNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) wrapped += 2.0f;
    return wrapped - 1.0f;
}

float SideScrollingShooter::Stage2Module::WrapDistance(float value, float length) {
    float wrapped = std::fmod(value, length);
    if (wrapped < 0.0f) wrapped += length;
    return wrapped;
}

void SideScrollingShooter::Stage2Module::DrawSky(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    const float nightBlend = NightBlend(shooter);
    const ColorF skyColor {
        Math::Lerp(DesertDaySkyColor[0], DesertNightSkyColor[0], nightBlend),
        Math::Lerp(DesertDaySkyColor[1], DesertNightSkyColor[1], nightBlend),
        Math::Lerp(DesertDaySkyColor[2], DesertNightSkyColor[2], nightBlend), 1.0f
    };
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, skyColor);
}

void SideScrollingShooter::Stage2Module::DrawBackground2D(
    const SideScrollingShooter& shooter,
    Renderer& renderer, const Camera3D& camera) {
    const float nightBlend = NightBlend(shooter);

    // 透視カメラの表示範囲に合わせて背景面を広げ、画面端まで覆う
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();

    // 昼はCubeを組み合わせた雲を横方向へ流す
    if (nightBlend < 0.99f) {
        const float cloudColor[4] = {0.88f, 0.91f, 0.88f, 1.0f - nightBlend};
        for (int i = 0; i < 9; ++i) {
            const float x = WrapNdcX(i * 0.47f - shooter.m_scroll * 0.08f) *
                backgroundHalfWidth;
            const float y = backgroundHalfHeight *
                (0.18f + static_cast<float>((i * 2) % 5) * 0.17f);
            DrawModelPrimitive(renderer, camera, 1,
                x, y, SidePlaneZ + 11.5f, 1.25f, 0.18f, 0.16f, cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x - 0.72f, y - 0.08f, SidePlaneZ + 11.5f,
                0.72f, 0.14f, 0.16f, cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x + 0.76f, y - 0.05f, SidePlaneZ + 11.5f,
                0.64f, 0.12f, 0.16f, cloudColor);
        }
    }

    // 夜は偏りのない決定的な配置でCubeの星を表示する
    if (nightBlend > 0.01f) {
        const float starColor[4] = {0.82f, 0.88f, 0.75f, nightBlend};
        for (int i = 0; i < 36; ++i) {
            const float x = (-0.94f + static_cast<float>((i * 73) % 191) / 100.0f) *
                backgroundHalfWidth;
            const float y = (0.02f + static_cast<float>((i * 37) % 88) / 100.0f) *
                backgroundHalfHeight;
            const float size = i % 7 == 0 ? 0.075f : 0.045f;
            DrawModelPrimitive(renderer, camera, 1,
                x, y, SidePlaneZ + 11.8f, size, size, size, starColor);
        }
    }

    // 砂地と角張ったサボテンを横スクロール背景として配置する
    DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
        60.0f, 10.0f, 0.3f, DesertSandColor);
    for (int i = 0; i < 18; ++i) {
        const float x = WrapNdcX(i * 0.41f - shooter.m_scroll * 0.32f) * 17.0f;
        constexpr float y = -5.175f;
        constexpr float z = SidePlaneZ + 14.0f;
        DrawModelPrimitive(renderer, camera, 1,
            x, y, z, 0.32f, 1.65f, 0.32f, DesertCactusColor);
        DrawModelPrimitive(renderer, camera, 1,
            x - 0.32f, y, z, 0.48f, 0.18f, 0.32f, DesertCactusColor);
        DrawModelPrimitive(renderer, camera, 1,
            x - 0.52f, y + 0.35f, z, 0.18f, 0.70f, 0.32f, DesertCactusColor);
    }
    DrawBoneArch(shooter, renderer, camera, 0.0f);
}

void SideScrollingShooter::Stage2Module::DrawBackground3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    const float nightBlend = NightBlend(shooter);
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth =
        sideBackgroundHalfHeight * renderer.AspectRatio();
    const float sandColor[4] = {
        Math::Lerp(DesertSandColor[0], DesertSandColor[0] * 0.32f, nightBlend),
        Math::Lerp(DesertSandColor[1], DesertSandColor[1] * 0.32f, nightBlend),
        Math::Lerp(DesertSandColor[2], DesertSandColor[2] * 0.45f, nightBlend), 1.0f
    };

    // 昼の雲は横視点の座標からレール空間へ展開する
    if (nightBlend < 0.99f) {
        const float cloudColor[4] = {0.88f, 0.91f, 0.88f, 1.0f - nightBlend};
        for (int i = 0; i < 9; ++i) {
            const float sideX = WrapNdcX(i * 0.47f - shooter.m_scroll * 0.08f) *
                sideBackgroundHalfWidth;
            const float sideY = sideBackgroundHalfHeight *
                (0.18f + static_cast<float>((i * 2) % 5) * 0.17f);
            const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
            const float railY = 5.0f + static_cast<float>((i * 7) % 12);
            const float railZ = 24.0f + static_cast<float>((i * 37) % 88);
            const float x = Math::Lerp(sideX, railX, railWeight);
            const float y = Math::Lerp(sideY, railY, railWeight);
            const float z = Math::Lerp(SidePlaneZ + 11.5f, railZ, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z,
                Math::Lerp(1.25f, 2.4f, railWeight),
                Math::Lerp(0.18f, 0.32f, railWeight),
                Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x - Math::Lerp(0.72f, 1.4f, railWeight),
                y - Math::Lerp(0.08f, 0.15f, railWeight), z,
                Math::Lerp(0.72f, 1.3f, railWeight),
                Math::Lerp(0.14f, 0.24f, railWeight),
                Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x + Math::Lerp(0.76f, 1.5f, railWeight),
                y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                Math::Lerp(0.64f, 1.1f, railWeight),
                Math::Lerp(0.12f, 0.20f, railWeight),
                Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
        }
    }

    // 夜の星は横視点の決定配置からレール奥行きへ補間する
    if (nightBlend > 0.01f) {
        const float starColor[4] = {0.82f, 0.88f, 0.75f, nightBlend};
        for (int i = 0; i < 36; ++i) {
            const float sideX =
                (-0.94f + static_cast<float>((i * 73) % 191) / 100.0f) *
                sideBackgroundHalfWidth;
            const float sideY =
                (0.02f + static_cast<float>((i * 37) % 88) / 100.0f) *
                sideBackgroundHalfHeight;
            const float railX = -24.0f + static_cast<float>((i * 73) % 480) / 10.0f;
            const float railY = 1.5f + static_cast<float>((i * 37) % 135) / 10.0f;
            const float railZ = 45.0f + static_cast<float>((i * 29) % 48);
            const float x = Math::Lerp(sideX, railX, railWeight);
            const float y = Math::Lerp(sideY, railY, railWeight);
            const float z = Math::Lerp(SidePlaneZ + 11.8f, railZ, railWeight);
            const float sideSize = i % 7 == 0 ? 0.075f : 0.045f;
            const float size = Math::Lerp(
                sideSize, i % 7 == 0 ? 0.12f : 0.07f, railWeight);
            DrawModelPrimitive(renderer, camera, 1,
                x, y, z, size, size, size, starColor);
        }
    }

    // 砂面とサボテンを横視点からレール空間へ連続補間する
    DrawModelPrimitive(renderer, camera, 1, 0.0f,
        Math::Lerp(-11.0f, -4.0f, railWeight),
        Math::Lerp(24.0f, 45.0f, railWeight),
        Math::Lerp(60.0f, 140.0f, railWeight),
        Math::Lerp(10.0f, 0.7f, railWeight),
        Math::Lerp(0.3f, 140.0f, railWeight), sandColor);
    for (int i = 0; i < 18; ++i) {
        const float sideX = WrapNdcX(i * 0.41f - shooter.m_scroll * 0.32f) * 17.0f;
        const float railX = -55.0f + static_cast<float>((i * 73) % 1100) / 10.0f;
        constexpr float sideZ = SidePlaneZ + 14.0f;
        const float railZ = 8.0f + WrapDistance(
            static_cast<float>(i * 43) - shooter.m_scroll * 28.0f, 110.0f);
        const float x = Math::Lerp(sideX, railX, railWeight);
        const float y = Math::Lerp(-5.175f, -2.825f, railWeight);
        const float z = Math::Lerp(sideZ, railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 1,
            x, y, z, 0.32f, 1.65f, 0.32f, DesertCactusColor);
        DrawModelPrimitive(renderer, camera, 1,
            x - 0.32f, y, z, 0.48f, 0.18f, 0.32f, DesertCactusColor);
        DrawModelPrimitive(renderer, camera, 1,
            x - 0.52f, y + 0.35f, z, 0.18f, 0.70f, 0.32f, DesertCactusColor);
    }
    DrawBoneArch(shooter, renderer, camera, railWeight);
}

void SideScrollingShooter::Stage2Module::DrawBoneArch(
    const SideScrollingShooter& shooter,
    Renderer& renderer, const Camera3D& camera, float railWeight) {
    if (shooter.m_stage2.boneArchDestroyed) return;

    constexpr int BoneCount = 13;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    constexpr float SideZ = SidePlaneZ + 1.2f;
    const float phase = std::fmod(shooter.m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(shooter.m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 横視点では骨を縦に並べ、レール視点へ移ると同じ頂点数のアーチへ補間する
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) /
            static_cast<float>(BoneCount - 1);
        const float x = Math::Lerp(
            ToWorldX(sideCenterX), std::cos(angle) * RailRadius, railWeight);
        const float y = Math::Lerp(
            ToWorldY(-1.30f + static_cast<float>(i) * 0.24f),
            RailCenterY + std::sin(angle) * RailRadius, railWeight);
        const float z = Math::Lerp(SideZ, railZ, railWeight);
        const float jointScale = i % 3 == 0 ? 1.18f : 1.0f;
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(1.25f, 2.55f, railWeight) * jointScale,
            Math::Lerp(1.40f, 2.55f, railWeight) * jointScale,
            Math::Lerp(0.85f, 2.55f, railWeight) * jointScale,
            DesertBoneColor);
    }
}

void SideScrollingShooter::Stage2Module::DrawSandDust(
    Renderer& renderer, const Camera3D& camera,
    const Vector3& position, int age, float railWeight) {
    constexpr int DustParticleCount = 13;
    constexpr float DustColor[] = {0.62f, 0.43f, 0.20f, 1.0f};
    static_assert(DustLifetimeFrames > 0 && DustParticleCount > 0);
    if (age < 0 || age >= DustLifetimeFrames) return;

    // 横視点では横方向、レール視点では円形へ砂粒を放物線状に広げる
    const float progress = static_cast<float>(age) /
        static_cast<float>(DustLifetimeFrames);
    const float fade = 1.0f - progress;
    for (int i = 0; i < DustParticleCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) /
            static_cast<float>(DustParticleCount);
        const float launchVelocity =
            1.10f + static_cast<float>((i * 5) % 4) * 0.18f;
        const float height =
            launchVelocity * progress - launchVelocity * progress * progress;
        const float radius =
            (0.28f + static_cast<float>(i % 3) * 0.10f) * progress;
        const float sideOffsetX = static_cast<float>(i - 6) * 0.12f * progress;
        const float offsetX = Math::Lerp(
            sideOffsetX, std::cos(angle) * radius, railWeight);
        const float offsetZ = Math::Lerp(0.0f, std::sin(angle) * radius, railWeight);
        const float size = (0.14f + static_cast<float>(i % 4) * 0.035f) * fade;
        const float color[4] = {
            DustColor[0], DustColor[1], DustColor[2], fade * 0.82f};
        DrawModelPrimitive(renderer, camera, 5,
            position.x + offsetX, position.y + 0.06f + height, position.z + offsetZ,
            size, size * 1.35f, size, color);
    }
}

bool SideScrollingShooter::Stage2Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& boss, float yaw) {
    if (boss.type != 2) return false;

    const float x = ToWorldX(boss.x);
    const float z = boss.z;
    auto DrawDamageSmoke = [&](BossPart part, const Vector3& position, float size) {
        const int maxHp = boss.bossPartMaxHp[part];
        if (maxHp <= 0 || boss.bossPartHp[part] <= 0 ||
            boss.bossPartHp[part] * 100 > maxHp * 35) return;
        const Matrix4x4 world = Matrix4x4::Translation(position) *
            Matrix4x4::Scale({size, size * 1.7f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
            static_cast<float>(boss.age) / 30.0f + static_cast<float>(part) * 0.37f, 1});
    };

    // 上下ユニットへ別Transformを渡し、合体状態を描画する
    constexpr float BossScale = 1.92f;
    const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
    const int railgunCycle = shooter.m_stage2.boss.actionAge % RailgunCycleFrames;
    const bool railgunLocked = railgunCycle < RailgunFireFrame + RailgunVisualFrames;
    const Vector3 aimTarget {
        ToWorldX(railgunLocked ? boss.actionX : boss.turretAimX),
        ToWorldY(railgunLocked ? boss.actionY : boss.turretAimY),
        railgunLocked ? Math::Lerp(SidePlaneZ, boss.actionZ, railWeight) :
            Math::Lerp(SidePlaneZ, boss.turretAimZ, railWeight)
    };
    bool weaponsDestroyed = true;
    for (int i = 0; i < BossPartCount; ++i) {
        if (i != BossRightEngine && boss.bossPartHp[i] > 0) weaponsDestroyed = false;
    }
    const bool separated = boss.phase >= 2.0f;
    const float battleshipPatrolX = boss.phase >= 2.0f ?
        std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * railWeight : 0.0f;
    const BossModelTransform submarine {
        {x + ToWorldX(shooter.m_stage2.boss.sandSubmarineOffsetX),
            SubmarineWorldY(shooter, boss),
            z + shooter.m_stage2.boss.sandSubmarineOffsetZ},
        aimTarget, yaw + (separated ? Math::HalfPi : 0.0f), BossScale
    };
    BossModelTransform battleship {
        {x + ToWorldX(shooter.m_stage2.boss.landBattleshipOffsetX) + battleshipPatrolX,
            BattleshipWorldY(shooter, boss),
            z + shooter.m_stage2.boss.landBattleshipOffsetZ},
        aimTarget, yaw, BossScale,
        boss.phase >= 3.0f && shooter.m_stage2.boss.action != BossAction::Separating, true
    };
    battleship.secondaryAimTarget = {
        ToWorldX(boss.turretAimX), ToWorldY(boss.turretAimY),
        Math::Lerp(SidePlaneZ, boss.turretAimZ, railWeight)
    };
    BossModelDamageState damage {
        boss.bossPartHp[BossNose] > 0,
        {boss.bossPartHp[BossLeftWing] > 0,
            boss.bossPartHp[BossRightWing] > 0,
            boss.bossPartHp[BossLeftEngine] > 0},
        boss.bossPartHp[BossRightEngine] > 0,
        weaponsDestroyed,
        boss.bossPartHitFlashFrames[BossNose] > 0 &&
            (boss.bossPartHitFlashFrames[BossNose] / 2) % 2 != 0,
        {boss.bossPartHitFlashFrames[BossLeftWing] > 0 &&
                (boss.bossPartHitFlashFrames[BossLeftWing] / 2) % 2 != 0,
            boss.bossPartHitFlashFrames[BossRightWing] > 0 &&
                (boss.bossPartHitFlashFrames[BossRightWing] / 2) % 2 != 0,
            boss.bossPartHitFlashFrames[BossLeftEngine] > 0 &&
                (boss.bossPartHitFlashFrames[BossLeftEngine] / 2) % 2 != 0},
        boss.bossPartHitFlashFrames[BossRightEngine] > 0 &&
            (boss.bossPartHitFlashFrames[BossRightEngine] / 2) % 2 != 0
    };
    for (int i = 0; i < BossFunnelHatchCount; ++i) {
        damage.funnelHatches[i] = boss.bossPartHp[BossFunnelHatch0 + i] > 0;
        const int frames = boss.bossPartHitFlashFrames[BossFunnelHatch0 + i];
        damage.funnelHatchesHit[i] = frames > 0 && (frames / 2) % 2 != 0;
    }
    auto DrawBossPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float partYaw, float partPitch) {
        DrawModelPrimitive(renderer, camera, shape,
            position.x, position.y, position.z,
            scale.x, scale.y, scale.z, color, partYaw, partPitch);
    };
    SandSubmarineView::Draw(submarine, DrawBossPart, damage);
    LandBattleshipView::Draw(battleship, DrawBossPart, damage);

    const bool introducing =
        shooter.m_bossIntroductionPhase == BossIntroductionPhase::Entrance;

    // 登場中とPhase 2以降は潜砂艦の移動位置へ砂埃を連続発生させる
    if (separated || introducing) {
        constexpr float SideGroundTopY = -6.0f;
        constexpr float RailGroundTopY = -3.65f;
        constexpr float EdgeLocalX[] = {-4.2f, 0.0f, 4.2f};
        constexpr float HullHalfWidth = 1.58f;
        const float groundTopY = Math::Lerp(
            SideGroundTopY, RailGroundTopY, railWeight);
        const float cosine = std::cos(submarine.yaw);
        const float sine = std::sin(submarine.yaw);
        const int effectAge = introducing ? shooter.m_bossIntroductionTimer : boss.age;
        for (int edge = 0; edge < 6; ++edge) {
            const float localX = EdgeLocalX[edge / 2];
            const float localZ =
                (edge % 2 == 0 ? -1.0f : 1.0f) * HullHalfWidth * railWeight;
            const int dustAge =
                (effectAge + edge * DustLifetimeFrames / 6) % DustLifetimeFrames;
            DrawSandDust(renderer, camera, {
                submarine.position.x +
                    (localX * cosine + localZ * sine) * submarine.scale,
                groundTopY,
                submarine.position.z +
                    (-localX * sine + localZ * cosine) * submarine.scale
            }, dustAge, railWeight);
        }
    }

    // 合体接近の後半は補助エンジンを止め、炎が潜砂艦を貫通しないようにする
    const bool introductionEngineVisible = introducing &&
        shooter.m_bossIntroductionTimer < BossApproachFrames + BossAssemblyFrames / 2;
    if (separated || introductionEngineVisible) {
        constexpr float EngineLocalX[] = {-1.80f, -1.80f, 1.70f, 1.70f};
        constexpr float EngineLocalZ[] = {-0.78f, 0.78f, -0.78f, 0.78f};
        constexpr int EngineFlameEffectType = 3;
        const float cosine = std::cos(battleship.yaw);
        const float sine = std::sin(battleship.yaw);
        for (int engine = 0; engine < 4; ++engine) {
            const Vector3 nozzle {
                battleship.position.x +
                    (EngineLocalX[engine] * cosine + EngineLocalZ[engine] * sine) *
                        battleship.scale,
                battleship.position.y + 0.02f * battleship.scale,
                battleship.position.z +
                    (-EngineLocalX[engine] * sine + EngineLocalZ[engine] * cosine) *
                        battleship.scale
            };
            const float flameHalfLength =
                0.82f + static_cast<float>(engine % 2) * 0.10f;
            const Matrix4x4 world = Matrix4x4::Translation({
                nozzle.x, nozzle.y - flameHalfLength, nozzle.z
            }) * Matrix4x4::Scale({0.34f, flameHalfLength, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                static_cast<float>(introducing ?
                    shooter.m_bossIntroductionTimer : boss.age) / 12.0f +
                    static_cast<float>(engine) * 0.71f,
                EngineFlameEffectType});
        }
    }

    // 各武装の損傷位置から煙を上げる
    constexpr Vector3 SmokeOffsets[] = {
        {-1.55f, 2.55f, 0.0f}, {-0.55f, 2.70f, -0.92f},
        {0.65f, 2.70f, 0.0f}, {2.85f, 2.28f, 0.92f}
    };
    for (int part = BossNose; part <= BossLeftEngine; ++part) {
        const Vector3& offset = SmokeOffsets[part];
        const float cosine = std::cos(battleship.yaw);
        const float sine = std::sin(battleship.yaw);
        DrawDamageSmoke(static_cast<BossPart>(part), {
            battleship.position.x +
                (offset.x * cosine + offset.z * sine) * battleship.scale,
            battleship.position.y + offset.y * battleship.scale,
            battleship.position.z +
                (-offset.x * sine + offset.z * cosine) * battleship.scale
        }, 1.25f);
    }
    const float submarineCosine = std::cos(submarine.yaw);
    const float submarineSine = std::sin(submarine.yaw);
    DrawDamageSmoke(BossRightEngine, {
        submarine.position.x + 0.25f * submarineCosine * submarine.scale,
        submarine.position.y + 1.75f * submarine.scale,
        submarine.position.z - 0.25f * submarineSine * submarine.scale
    }, 1.35f);
    for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
        const float side = hatch < 6 ? -1.0f : 1.0f;
        const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
        const float cosine = std::cos(submarine.yaw);
        const float sine = std::sin(submarine.yaw);
        DrawDamageSmoke(static_cast<BossPart>(BossFunnelHatch0 + hatch), {
            submarine.position.x +
                (localX * cosine + side * 1.8f * sine) * submarine.scale,
            submarine.position.y + 0.10f * submarine.scale,
            submarine.position.z +
                (-localX * sine + side * 1.8f * cosine) * submarine.scale
        }, 0.65f);
    }

    // Phase 3では発射直後だけ専用HLSLで固定照準の軌跡を急速に消す
    const int beamCycle = shooter.m_stage2.boss.actionAge % RailgunCycleFrames;
    const int beamAge = beamCycle - RailgunFireFrame;
    const bool pointerVisible = beamCycle < RailgunFireFrame &&
        shooter.m_stage2.boss.action != BossAction::Separating;
    const bool beamVisible = beamAge >= 0 && beamAge < RailgunVisualFrames;
    const bool mirageVisible = beamAge >= 0 && beamAge < RailgunMirageFrames;
    if (boss.phase >= 3.0f && damage.mainGun &&
        (pointerVisible || beamVisible || mirageVisible)) {
        const float cosine = std::cos(battleship.yaw);
        const float sine = std::sin(battleship.yaw);
        const Vector3 pivot {
            battleship.position.x + (-1.55f * cosine) * battleship.scale,
            battleship.position.y + 2.08f * battleship.scale,
            battleship.position.z + (1.55f * sine) * battleship.scale
        };
        const Vector3 lockedTarget {
            ToWorldX(boss.actionX), ToWorldY(boss.actionY),
            railWeight > 0.5f ? boss.actionZ : SidePlaneZ
        };
        const Vector3 delta = lockedTarget - pivot;
        const float horizontal = (std::max)(
            0.001f, std::sqrt(delta.x * delta.x + delta.z * delta.z));
        const float targetLength = (std::max)(
            0.001f, std::sqrt(horizontal * horizontal + delta.y * delta.y));
        const Vector3 direction = delta / targetLength;
        const float beamLength = targetLength + 18.0f;
        const Vector3 beamCenter = pivot + direction * (beamLength * 0.5f);
        const float beamYaw = std::atan2(direction.z, -direction.x);
        const float beamPitch = -std::asin(direction.y);
        auto DrawRailgunLayer = [&](float width, float progress, int effectType) {
            const Matrix4x4 world = Matrix4x4::Translation(beamCenter) *
                Matrix4x4::RotationY(beamYaw) * Matrix4x4::RotationZ(beamPitch) *
                Matrix4x4::Scale({beamLength * 0.5f, width, 1.0f});
            renderer.DrawRailgun({
                camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                progress, effectType});
        };
        if (mirageVisible) {
            DrawRailgunLayer(1.75f,
                static_cast<float>(beamAge) /
                    static_cast<float>(RailgunMirageFrames), 2);
        }
        if (pointerVisible) {
            DrawRailgunLayer(0.10f,
                static_cast<float>(beamCycle) /
                    static_cast<float>(RailgunFireFrame), 1);
        } else if (beamVisible) {
            DrawRailgunLayer(0.85f,
                static_cast<float>(beamAge) /
                    static_cast<float>(RailgunVisualFrames), 0);
        }
    }
    return true;
}

bool SideScrollingShooter::Stage2Module::DrawSpecialShot(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Shot& shot, float yaw) {
    if (!shot.enemy || shot.stage2.kind == ShotKind::None) return false;

    // 砂地を抜けた地点へ砂埃を発生させる
    if (shot.stage2.kind == ShotKind::Funnel &&
        shot.stage2.dustAge >= 0 && shot.stage2.dustAge < DustLifetimeFrames) {
        const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
        DrawSandDust(renderer, camera, {
            ToWorldX(shot.stage2.dustX),
            ToWorldY(shot.stage2.dustY), shot.stage2.dustZ
        }, shot.stage2.dustAge, railWeight);
    }

    // 進行方向へ向けた本体と後部リングを描画する
    constexpr float FunnelBody[] = {0.18f, 0.16f, 0.14f, 1.0f};
    constexpr float FunnelEdge[] = {0.72f, 0.20f, 0.08f, 1.0f};
    const float dx = ToWorldX(shot.vx);
    const float dy = ToWorldY(shot.vy);
    const float dz = shot.vz;
    const float horizontal = (std::max)(0.001f, std::sqrt(dx * dx + dz * dz));
    const float length = (std::max)(
        0.001f, std::sqrt(horizontal * horizontal + dy * dy));
    const float funnelYaw = std::atan2(dz, -dx);
    const float funnelPitch = -std::asin(dy / length);
    DrawModelPrimitive(renderer, camera, 3,
        ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.72f, 0.30f, 0.30f, FunnelBody, funnelYaw, funnelPitch);
    DrawModelPrimitive(renderer, camera, 4,
        ToWorldX(shot.x) - dx / length * 0.28f,
        ToWorldY(shot.y) - dy / length * 0.28f,
        shot.z - dz / length * 0.28f,
        0.44f, 0.42f, 0.42f, FunnelEdge, funnelYaw, funnelPitch);
    if (shot.stage2.kind == ShotKind::Funnel) {
        // 推進方向の後方へ既存の煙シェーダーを置きロケット噴出煙として流用する
        const Vector3 smokeCenter {
            ToWorldX(shot.x) - dx / length * 0.72f,
            ToWorldY(shot.y) - dy / length * 0.72f,
            shot.z - dz / length * 0.72f
        };
        const Matrix4x4 smokeWorld = Matrix4x4::Translation(smokeCenter) *
            Matrix4x4::Scale({0.42f, 0.62f, 1.0f});
        renderer.DrawExplosion({
            camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld,
            static_cast<float>(shot.age) / 9.0f, 1});
    }
    (void)shooter;
    return true;
}

bool SideScrollingShooter::Stage2Module::DrawSpecialDebris(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Debris& debris, float railWeight) {
    if (debris.stage2.kind == DebrisKind::None) return false;

    // 共通描画へ戻さず専用更新と同じ所有境界で基礎モデルも一度だけ描画する
    const float scale = debris.age > debris.shrinkStartAge ?
        1.0f - Math::Clamp01(
            static_cast<float>(debris.age - debris.shrinkStartAge) /
            static_cast<float>(debris.lifetime - debris.shrinkStartAge)) : 1.0f;
    DrawModelPrimitive(renderer, camera, debris.shape,
        debris.x, debris.y, debris.z,
        debris.width * scale, debris.height * scale, debris.depth * scale,
        debris.color.data(), debris.yaw);

    // 沈下と再浮上の各区間で船体幅全体から砂埃を噴き上げる
    if (debris.stage2.kind == DebrisKind::Sink) {
        const bool movingThroughSand = debris.age < FirstSinkEndFrame ||
            debris.age >= ResurfaceStartFrame || debris.stage2.effectAge >= 0;
        const float groundTopY = Math::Lerp(-6.0f, -3.65f, railWeight);
        if (movingThroughSand) {
            for (int wave = 0; wave < 4; ++wave) {
                const int dustAge = (debris.age + wave * 7) % DustLifetimeFrames;
                for (int side = -2; side <= 2; ++side) {
                    DrawSandDust(renderer, camera, {
                        debris.x + side * debris.width * 0.16f,
                        groundTopY, debris.z
                    }, dustAge, railWeight);
                }
            }
        }
    }

    // 降下中は連続爆発と煙を引き、下部船体との衝突後に大爆発へ切り替える
    if (debris.stage2.kind == DebrisKind::Impact) {
        constexpr int EffectFrames = 72;
        const Matrix4x4 viewProjection = camera.ProjectionMatrix() * camera.ViewMatrix();
        if (debris.stage2.effectAge < 0) {
            for (int i = 0; i < 3; ++i) {
                const int cycleAge = (debris.age + i * 17) % 54;
                const float cycle = static_cast<float>(cycleAge) / 54.0f;
                const float offsetX = static_cast<float>(i - 1) * debris.width * 0.22f;
                const float size = debris.width * (0.18f + cycle * 0.24f);
                const Matrix4x4 fireWorld = Matrix4x4::Translation({
                    debris.x + offsetX,
                    debris.y + debris.height * 0.18f, debris.z
                }) * Matrix4x4::Scale({size, size, 1.0f});
                renderer.DrawExplosion({viewProjection * fireWorld, cycle});
            }
            const float smokeCycle = static_cast<float>(debris.age % 72) / 72.0f;
            const float smokeSize = debris.width * (0.34f + smokeCycle * 0.38f);
            const Matrix4x4 smokeWorld = Matrix4x4::Translation({
                debris.x,
                debris.y + debris.height * (0.45f + smokeCycle * 0.75f), debris.z
            }) * Matrix4x4::Scale({smokeSize, smokeSize * 1.45f, 1.0f});
            renderer.DrawExplosion({viewProjection * smokeWorld, smokeCycle, 2});
            return true;
        }

        const float progress = Math::Clamp01(
            static_cast<float>(debris.stage2.effectAge) / EffectFrames);
        for (int i = 0; i < 5; ++i) {
            const float delayed = Math::Clamp01(
                progress * 1.55f - static_cast<float>(i) * 0.10f);
            const float offsetX = static_cast<float>(i - 2) * debris.width * 0.16f;
            const float size = debris.width * (0.34f + delayed * 0.42f);
            const Matrix4x4 fireWorld = Matrix4x4::Translation({
                debris.x + offsetX,
                debris.y + debris.height * 0.22f, debris.z
            }) * Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({viewProjection * fireWorld, delayed});
        }
        const float smokeSize = debris.width * (0.58f + progress * 0.72f);
        const Matrix4x4 smokeWorld = Matrix4x4::Translation({
            debris.x,
            debris.y + debris.height * (0.35f + progress * 0.85f), debris.z
        }) * Matrix4x4::Scale({smokeSize, smokeSize * 1.35f, 1.0f});
        renderer.DrawExplosion({viewProjection * smokeWorld, progress, 2});
        for (int side = -2; side <= 2; ++side) {
            DrawSandDust(renderer, camera, {
                debris.x + side * debris.width * 0.18f,
                debris.y - debris.height * 0.5f, debris.z
            }, debris.stage2.effectAge, railWeight);
        }
    }
    (void)shooter;
    return true;
}

bool SideScrollingShooter::Stage2Module::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& boss, int bossPart) {
    if (boss.type != 2) return false;

    constexpr float ModelScale = 1.92f;
    constexpr float Hull[] = {0.28f, 0.24f, 0.17f, 1.0f};
    constexpr float Armor[] = {0.36f, 0.31f, 0.22f, 1.0f};
    constexpr float Metal[] = {0.48f, 0.45f, 0.36f, 1.0f};
    constexpr float Dark[] = {0.07f, 0.065f, 0.055f, 1.0f};
    constexpr float Core[] = {0.95f, 0.28f, 0.055f, 1.0f};
    const float yaw = shooter.IsRailGameplayActive() ? 0.0f : Math::HalfPi;
    const float x = ToWorldX(boss.x);
    const float y = ToWorldY(boss.y);
    int pieceNumber = 0;
    auto AddPiece = [&](int shape, float localX, float localY, float localZ,
        float width, float height, float depth, const float color[4], float scale) {
        const Vector3 offset = RotateYawOffset(
            localX * scale, localY * scale, localZ * scale, yaw);
        static constexpr Vector3 SpreadDirections[] = {
            {-0.85f, 0.55f, -0.65f}, {0.90f, -0.40f, -0.75f},
            {-0.70f, -0.80f, 0.85f}, {0.65f, 0.90f, 0.70f},
            {-0.45f, 0.25f, 1.00f}, {0.50f, -0.95f, -0.35f},
            {-1.00f, 0.10f, 0.35f}, {0.95f, 0.35f, -0.15f}
        };
        const Vector3 direction = SpreadDirections[pieceNumber++ % 8];
        const Vector3 velocity = RotateYawOffset(
            direction.x * 0.040f, direction.y * 0.040f,
            direction.z * 0.040f, yaw);
        shooter.SpawnDebrisPiece(
            x + offset.x, y + offset.y, boss.z + offset.z,
            velocity.x, velocity.y, velocity.z,
            yaw, 0.08f + direction.x * 0.050f,
            shape, width * scale, height * scale, depth * scale, color);
    };
    auto AddStage2Piece = [&](int shape, const Vector3& local, const Vector3& size,
        const float color[4], float unitOffsetY) {
        AddPiece(shape, local.x, local.y + unitOffsetY / ModelScale, local.z,
            size.x, size.y, size.z, color, ModelScale);
    };

    // 個別部位は描画と同じ構成の主要プリミティブを飛散させる
    if (bossPart == BossNose) {
        AddStage2Piece(2, {-1.55f, 2.03f, 0.0f},
            {1.75f, 1.10f, 1.75f}, Dark, 0.78f);
        AddStage2Piece(4, {-2.30f, 2.08f, 0.0f},
            {1.85f, 1.08f, 1.55f}, Armor, 0.78f);
        AddStage2Piece(2, {-4.25f, 2.08f, 0.0f},
            {2.95f, 0.54f, 0.54f}, Metal, 0.78f);
        return true;
    }
    if (bossPart == BossLeftWing ||
        bossPart == BossRightWing || bossPart == BossLeftEngine) {
        constexpr Vector3 Positions[] = {
            {-0.55f, 2.34f, -0.92f},
            {0.65f, 2.34f, 0.0f},
            {2.85f, 1.92f, 0.92f}
        };
        const int index = bossPart == BossLeftWing ? 0 :
            (bossPart == BossRightWing ? 1 : 2);
        const Vector3& position = Positions[index];
        AddStage2Piece(2, position, {0.76f, 0.46f, 0.76f}, Dark, 0.78f);
        AddStage2Piece(1,
            {position.x - 0.20f, position.y + 0.26f, position.z},
            {0.82f, 0.46f, 0.64f}, Armor, 0.78f);
        AddStage2Piece(2,
            {position.x - 1.10f, position.y + 0.26f, position.z},
            {1.35f, 0.20f, 0.20f}, Metal, 0.78f);
        return true;
    }
    if (bossPart == BossRightEngine) {
        AddStage2Piece(2, {0.25f, 1.42f, 0.0f},
            {1.35f, 0.22f, 1.35f}, Core, -0.45f);
        return true;
    }
    if (bossPart >= BossFunnelHatch0) {
        const int hatch = bossPart - BossFunnelHatch0;
        const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
        const float localZ = (hatch < 6 ? -1.0f : 1.0f) * 1.80f;
        AddStage2Piece(1, {localX, -0.22f, localZ},
            {0.38f, 0.34f, 0.08f}, Core, -0.45f);
        return true;
    }

    // 撃破時は下部の沈下と再浮上、上部の緩やかな降下を同じ時間軸で開始する
    const float submarineY = SubmarineWorldY(shooter, boss);
    const float battleshipY = BattleshipWorldY(shooter, boss);
    const Vector3 lowerOffset = RotateYawOffset(0.0f, -0.45f, 0.0f, yaw);
    Debris* lowerHull = shooter.SpawnDebrisPiece(
        x + lowerOffset.x, submarineY + lowerOffset.y, boss.z + lowerOffset.z,
        0.0f, 0.0f, 0.0f, yaw, 0.0f,
        5, 8.8f * ModelScale, 2.05f * ModelScale, 3.15f * ModelScale,
        Hull, 420, 420, false);
    if (lowerHull != nullptr) lowerHull->stage2 = {DebrisKind::Sink, -1};
    auto AddFallingUpper = [&](int shape, const Vector3& local,
        const Vector3& size, const float color[4], DebrisKind effect) {
        const Vector3 offset = RotateYawOffset(
            local.x * ModelScale, local.y * ModelScale, local.z * ModelScale, yaw);
        Debris* debris = shooter.SpawnDebrisPiece(
            x + offset.x, battleshipY + offset.y + 3.25f, boss.z + offset.z,
            0.0f, 0.0f, 0.0f, yaw,
            0.018f + local.x * 0.004f,
            shape, size.x * ModelScale, size.y * ModelScale, size.z * ModelScale,
            color, 420, 420, true);
        if (debris != nullptr) debris->stage2 = {effect, -1};
    };
    AddFallingUpper(1, {0.20f, 0.95f, 0.0f},
        {2.65f, 1.30f, 2.70f}, Hull, DebrisKind::Impact);
    AddFallingUpper(4, {-1.65f, 0.70f, 0.0f},
        {2.70f, 1.08f, 2.55f}, Hull, DebrisKind::ImpactPiece);
    AddFallingUpper(4, {2.65f, 1.62f, 0.0f},
        {1.45f, 0.76f, 1.78f}, Armor, DebrisKind::ImpactPiece);
    for (int part = 0; part < BossPartCount; ++part) {
        if (boss.bossPartHp[part] > 0) SpawnBossDebris(shooter, boss, part);
    }
    return true;
}
