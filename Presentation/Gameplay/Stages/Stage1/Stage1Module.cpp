#include "Stage1Module.h"

#include <cmath>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../SideScrollingShooterShared.h"
#include "../../SideScrollingShooterEnemies.h"
#include "../Common/StageDefinition.h"
#include "Stage1EnemySheetEasy.h"
#include "Stage1EnemySheetHard.h"
#include "Stage1EnemySheetNormal.h"

namespace {
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::SideCameraZ;

constexpr float SideBackgroundColor[4] = { 0.01f, 0.04f, 0.08f, 1.0f };
constexpr float GridColor[4] = { 0.05f, 0.22f, 0.16f, 1.0f };
constexpr float StarColor[4] = { 0.55f, 0.70f, 0.85f, 1.0f };
constexpr float MeteorColor[4] = { 0.30f, 0.22f, 0.18f, 1.0f };
constexpr float MeteorCraterColor[4] = { 0.95f, 0.38f, 0.08f, 1.0f };

constexpr int Stage1BossRushSegmentFrames = 36;
constexpr int Stage1BossRushSegmentCount = 4;
constexpr int Stage1BossRushFrames = Stage1BossRushSegmentFrames * Stage1BossRushSegmentCount;
constexpr int Stage1BossSettleFrames = 96;
constexpr int Stage1BossEntranceFrames = Stage1BossRushFrames + Stage1BossSettleFrames;

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
 * @brief Stage 1ボス出現演出の高速移動区間を取得する
 * @param age 出現演出の経過フレーム
 * @return 0から3までの高速移動区間
 */
constexpr int Stage1BossRushSegment(int age) {
    if (age <= 0) return 0;
    const int segment = age / Stage1BossRushSegmentFrames;
    return segment < Stage1BossRushSegmentCount ? segment : Stage1BossRushSegmentCount - 1;
}

static_assert(Stage1BossRushSegment(0) == 0);
static_assert(Stage1BossRushSegment(Stage1BossRushFrames - 1) == 3);
static_assert(Stage1BossEntranceFrames == 240);

}

const SideScrollingShooter::Stage& SideScrollingShooter::Stage1Module::Definition(
    DifficultyType difficulty) {
    static const Stage1EnemySheetEasy easyStage;
    static const Stage1EnemySheetNormal normalStage;
    static const Stage1EnemySheetHard hardStage;
    switch (difficulty) {
    case Hard: return hardStage;
    case Normal: return normalStage;
    default: return easyStage;
    }
}

void SideScrollingShooter::Stage1Module::Reset(SideScrollingShooter& shooter) {
    constexpr float Travel[] = { 0.0f, 11.5f, 24.8f, 37.0f, 50.6f, 63.4f };
    constexpr float Scale[] = { 1.75f, 1.10f, 2.05f, 1.38f, 1.62f, 0.92f };
    constexpr float Spin[] = { 0.035f, -0.052f, 0.028f, -0.041f, 0.046f, -0.061f };

    // 固定配置と耐久値を既存順序のまま復元する
    for (int i = 0; i < ShooterStages::Stage1::MeteorCount; ++i) {
        shooter.m_stage1.meteors[i] = { Travel[i], Scale[i], static_cast<float>(i) * 0.7f, Spin[i],
            4 + i % 3, false };
    }
}

void SideScrollingShooter::Stage1Module::TickWorld(SideScrollingShooter& shooter) {
    // 大小の異なる隕石をそれぞれ移動・回転させる
    for (auto& meteor : shooter.m_stage1.meteors) {
        if (meteor.destroyed) continue;
        meteor.travel += 0.10f + meteor.scale * 0.06f;
        if (meteor.travel >= 72.0f) meteor.travel -= 72.0f;
        meteor.yaw += meteor.spin;
    }
}

bool SideScrollingShooter::Stage1Module::HitsHazard(const SideScrollingShooter& shooter,
    float x, float y, float z, float radius) {
    return FindMeteor(shooter, x, y, z, radius) >= 0;
}

bool SideScrollingShooter::Stage1Module::TryDamageTarget(
    SideScrollingShooter& shooter, Shot& shot) {
    if (shot.enemy) return false;

    // 共通敵より先に隕石の命中を処理する
    const int meteorIndex = FindMeteor(shooter, shot.x, shot.y, shot.z, shot.hitRadius);
    if (meteorIndex < 0) return false;

    shooter.SpawnExplosion(shot.x, shot.y, shot.z);
    shot.active = false;
    ShooterStages::Stage1::Meteor& meteor = shooter.m_stage1.meteors[meteorIndex];
    meteor.hp -= shot.damage;
    SpawnMeteorDebris(shooter, meteor, meteor.hp <= 0 ? 8 : 2);
    if (meteor.hp <= 0) meteor.destroyed = true;
    shooter.PlayHitSound();
    return true;
}

void SideScrollingShooter::Stage1Module::DrawBackground2D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    // 透視カメラの表示範囲に合わせて背景面を広げ、画面端まで覆う
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();
    const Matrix4x4 backgroundWorld = Matrix4x4::Translation({0.0f, 0.0f, BackgroundZ}) *
        Matrix4x4::Scale({backgroundHalfWidth, backgroundHalfHeight, 1.0f});
    renderer.Draw({
        PrimitiveShape::Sprite2D,
        camera.ProjectionMatrix() * camera.ViewMatrix() * backgroundWorld,
        Vector3::One,
        {SideBackgroundColor[0], SideBackgroundColor[1], SideBackgroundColor[2], SideBackgroundColor[3]}
    });

    // 遷移描画と同じ星・グリッド配置を使い、切り替え完了時の交換を防ぐ
    for (int i = 0; i < 28; ++i) {
        const float x = WrapNdcX(i * 0.137f - shooter.m_scroll * (0.6f + (i % 3) * 0.3f));
        const float y = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        DrawModelPrimitive(renderer, camera, 1,
            x * backgroundHalfWidth, y * backgroundHalfHeight, SidePlaneZ + 12.0f,
            0.06f, 0.06f, 0.06f, StarColor);
    }
    for (int i = 0; i < 8; ++i) {
        const float x = WrapNdcX(i * 0.34f - shooter.m_scroll * 0.55f);
        DrawModelPrimitive(renderer, camera, 1, x * 20.0f, 0.0f, SidePlaneZ + 7.0f,
            0.04f, 18.0f, 0.08f, GridColor);
    }
    for (int i = 0; i < 12; ++i) {
        DrawModelPrimitive(renderer, camera, 1,
            0.0f, (-0.90f + (i % 6) * 0.36f) * 9.0f, SidePlaneZ + 7.0f,
            42.0f, 0.04f, 0.08f, GridColor);
    }
    DrawMeteors(shooter, renderer, camera, 0.0f);
}

void SideScrollingShooter::Stage1Module::DrawBackground3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    // 横視点の背景寸法を使って遷移開始直後の星配置を維持する
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth = sideBackgroundHalfHeight * renderer.AspectRatio();

    // 遷移開始直後は2D背景の配置を保ち、初期ジャンプを避ける
    for (int i = 0; i < 28; ++i) {
        const float sideX = WrapNdcX(i * 0.137f - shooter.m_scroll * (0.6f + (i % 3) * 0.3f));
        const float sideY = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        const float railX = -13.0f + static_cast<float>((i * 37) % 260) / 10.0f;
        const float railY = -6.0f + static_cast<float>((i * 53) % 120) / 10.0f;
        const float railZ = 8.0f + static_cast<float>((i * 29 + shooter.m_frame) % 540) / 9.0f;
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(sideX * sideBackgroundHalfWidth, railX, railWeight),
            Math::Lerp(sideY * sideBackgroundHalfHeight, railY, railWeight),
            Math::Lerp(SidePlaneZ + 12.0f, railZ, railWeight),
            0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f,
            0.06f + railWeight * 0.01f, StarColor);
    }

    // 前半で2D縦線を下へ沈めて潰し、後半で3D床グリッドへ展開する
    const float gridLowerWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f));
    const float gridMoveWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f - 1.0f));
    for (int i = 0; i < 8; ++i) {
        const float x = -35.0f + i * 10.0f;
        const float sideX = WrapNdcX(i * 0.34f - shooter.m_scroll * 0.55f);
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(sideX * 20.0f, x, gridMoveWeight),
            Math::Lerp(0.0f, -3.3f, gridLowerWeight),
            Math::Lerp(SidePlaneZ + 7.0f, 32.0f, gridMoveWeight),
            Math::Lerp(0.04f, 0.025f, gridMoveWeight),
            Math::Lerp(18.0f, 0.025f, gridLowerWeight),
            Math::Lerp(0.08f, 140.0f, gridMoveWeight), GridColor);
    }
    for (int i = 0; i < 12; ++i) {
        const float z = 6.0f + i * 5.5f - std::fmod(shooter.m_scroll * 80.0f, 5.5f);
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp((-0.90f + (i % 6) * 0.36f) * 9.0f, -3.3f, gridLowerWeight),
            Math::Lerp(SidePlaneZ + 7.0f, z, gridMoveWeight),
            Math::Lerp(42.0f, 100.0f, gridMoveWeight),
            Math::Lerp(0.04f, 0.025f, gridMoveWeight),
            Math::Lerp(0.08f, 0.025f, gridMoveWeight), GridColor);
    }
    DrawMeteors(shooter, renderer, camera, railWeight);
}

void SideScrollingShooter::Stage1Module::TickBossIntroduction(SideScrollingShooter& shooter) {
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) return;

    Enemy& boss = shooter.m_enemies[0];
    constexpr float SideX[] = {3.25f, -3.25f, 3.25f, -3.25f, 3.25f};
    constexpr float SideY[] = {1.05f, -1.08f, -0.82f, 1.05f, 0.0f};
    constexpr float RailX[] = {1.45f, -1.45f, 1.45f, -1.45f, 1.45f};
    constexpr float RailY[] = {1.05f, -1.08f, -0.82f, 1.05f, 0.0f};

    // 画面の上下を交互に横断する
    if (shooter.m_bossIntroductionTimer < Stage1BossRushFrames) {
        const int segment = Stage1BossRushSegment(shooter.m_bossIntroductionTimer);
        const float segmentProgress = SmoothStep(static_cast<float>(
            shooter.m_bossIntroductionTimer - segment * Stage1BossRushSegmentFrames) /
            static_cast<float>(Stage1BossRushSegmentFrames));
        if (shooter.IsRailGameplayActive()) {
            boss.x = Math::Lerp(RailX[segment], RailX[segment + 1], segmentProgress);
            boss.y = Math::Lerp(RailY[segment], RailY[segment + 1], segmentProgress);
            boss.z = 32.0f;
        } else {
            boss.x = Math::Lerp(SideX[segment], SideX[segment + 1], segmentProgress);
            boss.y = Math::Lerp(SideY[segment], SideY[segment + 1], segmentProgress);
            boss.z = ToRailZFromSideX(boss.x);
        }
        return;
    }

    // 画面端から低速で定位置へ入る
    const float settleProgress = SmoothStep(static_cast<float>(
        shooter.m_bossIntroductionTimer - Stage1BossRushFrames) /
        static_cast<float>(Stage1BossSettleFrames));
    if (shooter.IsRailGameplayActive()) {
        boss.x = Math::Lerp(RailX[4], 0.0f, settleProgress);
        boss.y = 0.0f;
        boss.z = Math::Lerp(32.0f, 48.0f, settleProgress);
    } else {
        boss.x = Math::Lerp(SideX[4], 1.80f, settleProgress);
        boss.y = 0.0f;
        boss.z = ToRailZFromSideX(boss.x);
    }
}

int SideScrollingShooter::Stage1Module::BossIntroductionFrames() {
    return Stage1BossEntranceFrames;
}

int SideScrollingShooter::Stage1Module::FindMeteor(const SideScrollingShooter& shooter,
    float x, float y, float z, float radius) {
    for (int i = 0; i < ShooterStages::Stage1::MeteorCount; ++i) {
        const ShooterStages::Stage1::Meteor& meteor = shooter.m_stage1.meteors[i];
        if (meteor.destroyed) continue;
        const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
        const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
        if (shooter.IsRailGameplayActive()) {
            const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
            const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                railX, railY, 72.0f - meteor.travel, 2.70f * meteor.scale)) {
                return i;
            }
            continue;
        }
        if (Hit(x, y, radius, sideX, sideY, 0.42f * meteor.scale)) return i;
    }
    return -1;
}

void SideScrollingShooter::Stage1Module::SpawnMeteorDebris(
    SideScrollingShooter& shooter, const ShooterStages::Stage1::Meteor& meteor, int count) {
    const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
    const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
    const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
    const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
    const bool railMode = shooter.IsRailGameplayActive();
    const float x = railMode ? railX : ToWorldX(sideX);
    const float y = railMode ? railY : ToWorldY(sideY);
    const float z = railMode ? 72.0f - meteor.travel : SidePlaneZ + 1.2f;

    // 共通デブリを当たり判定のない小隕石の漂流表現として再利用する
    for (int i = 0; i < count; ++i) {
        const float angle = meteor.yaw + static_cast<float>(i) * Math::TwoPi / static_cast<float>(count);
        const float size = (0.20f + static_cast<float>(i % 3) * 0.08f) * meteor.scale;
        shooter.SpawnDebrisPiece(x, y, z,
            std::cos(angle) * 0.035f, std::sin(angle * 1.4f) * 0.030f,
            railMode ? std::sin(angle) * 0.045f : 0.0f,
            angle, meteor.spin * (1.5f + i * 0.1f),
            5, size, size * 0.85f, size * 0.75f, MeteorColor);
    }
}

void SideScrollingShooter::Stage1Module::DrawMeteors(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    for (int i = 0; i < ShooterStages::Stage1::MeteorCount; ++i) {
        const ShooterStages::Stage1::Meteor& meteor = shooter.m_stage1.meteors[i];
        if (meteor.destroyed) continue;
        const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
        const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
        const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
        const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
        const float x = Math::Lerp(ToWorldX(sideX), railX, railWeight);
        const float y = Math::Lerp(ToWorldY(sideY), railY, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 1.2f, 72.0f - meteor.travel, railWeight);
        const float width = Math::Lerp(2.20f, 5.30f, railWeight) * meteor.scale;
        const float height = Math::Lerp(2.55f, 5.30f, railWeight) * meteor.scale;
        const float depth = Math::Lerp(1.45f, 5.30f, railWeight) * meteor.scale;

        // 本体とクレーターを同じ回転角で動かし、視点遷移中も形状を維持する
        DrawModelPrimitive(renderer, camera, 5,
            x, y, z, width, height, depth, MeteorColor, meteor.yaw);
        for (int crater = 0; crater < 3; ++crater) {
            const float angle = meteor.yaw + static_cast<float>(crater) * Math::TwoPi / 3.0f;
            const float offsetX = std::cos(angle) *
                Math::Lerp(0.52f, 1.28f, railWeight) * meteor.scale;
            const float offsetY = std::sin(angle * 1.7f) *
                Math::Lerp(0.40f, 1.16f, railWeight) * meteor.scale;
            DrawModelPrimitive(renderer, camera, 5, x + offsetX, y + offsetY,
                z - std::cos(angle) * Math::Lerp(0.38f, 1.75f, railWeight) * meteor.scale,
                Math::Lerp(0.38f, 0.92f, railWeight) * meteor.scale,
                Math::Lerp(0.42f, 0.92f, railWeight) * meteor.scale,
                Math::Lerp(0.20f, 0.38f, railWeight) * meteor.scale,
                MeteorCraterColor, meteor.yaw);
        }
    }
}
