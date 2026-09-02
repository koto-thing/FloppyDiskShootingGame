#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string_view>

#include "../../Engine/Graphics/Renderer.h"
#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::SideCameraZ;
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::Stage2BossApproachFrames;
using SideScrollingShooterShared::Stage2BossAssemblyFrames;
using SideScrollingShooterShared::BossNameRevealFrames;

constexpr float ModeTextColor[4] = { 0.55f, 0.85f, 1.0f, 1.0f };
constexpr float PlayerColor[4] = { 0.80f, 0.80f, 0.85f, 1.0f };
constexpr float PlayerAccent[4] = { 0.10f, 0.90f, 0.90f, 1.0f };
constexpr float EnemyColor[4] = { 0.90f, 0.12f, 0.12f, 1.0f };
constexpr float EnemyAccent[4] = { 1.00f, 0.55f, 0.08f, 1.0f };
constexpr float PlayerShotColor[4] = { 0.15f, 1.00f, 0.25f, 1.0f };
constexpr float HomingShotColor[4] = { 0.15f, 0.85f, 1.00f, 1.0f };
constexpr float PiercingShotColor[4] = { 0.72f, 0.20f, 1.00f, 1.0f };
constexpr float SpreadShotColor[4] = { 1.00f, 0.20f, 0.52f, 1.0f };
constexpr float EnemyShotColor[4] = { 1.00f, 0.25f, 0.25f, 1.0f };
constexpr float PowerItemColor[4] = { 1.00f, 0.88f, 0.12f, 1.0f };
constexpr float ScoreItemColor[4] = { 0.25f, 0.90f, 1.00f, 1.0f };

constexpr float TowerFacadeColor[4] = { 0.10f, 0.13f, 0.24f, 1.0f };
constexpr float TowerNeonColor[4] = { 0.90f, 0.08f, 0.42f, 1.0f };
constexpr float TowerRoofColor[4] = { 0.12f, 0.14f, 0.20f, 1.0f };
constexpr float SatelliteBodyColor[4] = { 0.58f, 0.68f, 0.78f, 1.0f };
constexpr float SatellitePanelColor[4] = { 0.16f, 0.48f, 0.88f, 1.0f };
constexpr float SatelliteLightColor[4] = { 0.82f, 0.94f, 1.0f, 1.0f };
constexpr float SearchlightColor[4] = { 1.00f, 0.82f, 0.20f, 0.24f };
constexpr float SearchlightLockedColor[4] = { 1.00f, 0.08f, 0.08f, 0.50f };
constexpr float StormCloudColor[4] = { 0.05f, 0.07f, 0.13f, 1.0f };

constexpr int MissionBannerGlyphDelayFrames = 4;
constexpr int MissionBannerGlyphPopFrames = 8;

/**
 * @brief 文字の登場経過に対応する拡大率を取得する
 * @param glyphAge 文字が登場してからのフレーム数
 * @return 登場前は0、登場中は縮小する拡大率、登場後は1
 */
constexpr float MissionBannerGlyphScale(int glyphAge) {
    if (glyphAge < 0) return 0.0f;
    if (glyphAge >= MissionBannerGlyphPopFrames) return 1.0f;
    return 1.0f + static_cast<float>(MissionBannerGlyphPopFrames - glyphAge) * 0.16f;
}

static_assert(MissionBannerGlyphScale(-1) == 0.0f);
static_assert(MissionBannerGlyphScale(MissionBannerGlyphPopFrames) == 1.0f);

/**
 * @brief 機体のローカル配置をY軸回転してワールド配置へ変換する
 */
Vector3 RotateYawOffset(float x, float y, float z, float yaw) {
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);
    return {x * c + z * s, y, -x * s + z * c};
}

}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"
#include "BossModelView.h"
#include "Stage5ModelView.h"
#include "Stage1Story.h"

void SideScrollingShooter::ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const {
    // 2DモードはXY移動平面を3Dカメラで正面から見て奥行きを持たせる
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(SideCameraFieldOfView));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(80.0f);
    camera.SetPosition({0.0f, 0.0f, SideCameraZ});
    camera.LookAt({0.0f, 0.0f, SidePlaneZ});
}

void SideScrollingShooter::ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const {
    const float railWeight = RailBlend();

    const Vector3 playerPosition{ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ};
    // 2Dモードと同じカメラ状態から、3Dレールの追従カメラへ補間する
    const Vector3 sidePosition{0.0f, 0.0f, SideCameraZ};
    const Vector3 sideTarget{0.0f, 0.0f, SidePlaneZ};
    Vector3 railPosition{playerPosition.x * 0.18f, playerPosition.y * 0.12f + 1.0f, PlayerRailZ - 15.5f};
    Vector3 railTarget{playerPosition.x * 0.28f, playerPosition.y * 0.18f, PlayerRailZ + 22.0f};

    // 壁面上昇だけ視線を上へ向け、屋上で水平へ戻す
    if (m_stageNumber == 5 && m_stage5Phase >= Stage5Phase::WallClimbTransition) {
        float climbWeight = 0.0f;
        if (m_stage5Phase == Stage5Phase::WallClimbTransition) {
            climbWeight = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer) /
                static_cast<float>(WallClimbTransitionFrames));
        } else if (m_stage5Phase == Stage5Phase::WallClimbLower) {
            climbWeight = 0.42f;
        } else if (m_stage5Phase == Stage5Phase::WallClimbMiddle) {
            climbWeight = 0.68f;
        } else if (m_stage5Phase == Stage5Phase::WallClimbUpper) {
            climbWeight = 0.92f;
        } else if (m_stage5Phase == Stage5Phase::RooftopArrival) {
            climbWeight = 1.0f - SmoothStep(Math::Clamp01(
                static_cast<float>(m_stage5PhaseTimer) / RooftopArrivalFrames));
        }
        railPosition.y -= climbWeight * 1.8f;
        railTarget.y += climbWeight * 13.0f;
        railTarget.z += climbWeight * 8.0f;
    }
    if (m_stageNumber == 5 && m_stage5Phase == Stage5Phase::TayamaCollapse) {
        const float pullBack = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 60) / 390.0f);
        railPosition.z -= pullBack * 8.0f;
        railPosition.y += pullBack * 2.0f;
        railTarget.z += pullBack * 7.0f;
    }
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(38.0f + (8.0f * railWeight)));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(m_stageNumber == 5 ? 220.0f : 120.0f);
    camera.SetPosition(Vector3::Lerp(sidePosition, railPosition, railWeight));
    camera.LookAt(Vector3::Lerp(sideTarget, railTarget, railWeight));
}

void SideScrollingShooter::DrawShape(Renderer& renderer,
    float x, float y, float w, float h, const float color[4]) {
    // 描画ファサードへ矩形コマンドとして記録する
    renderer.Draw(Rect { { x, y }, { w, h } }, { color[0], color[1], color[2], color[3] });
}

/**
 * @brief 2D画面上の敵攻撃予告を十字フラッシュとして描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawAttackWarnings2D(Renderer& renderer) const {
    constexpr float FlashColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.attackWarningFrames <= 0) continue;

        // 残り時間に合わせて拡大する十字を、攻撃を行う敵機へ重ねる
        const float progress = Math::Clamp01(
            1.0f - static_cast<float>(enemy.attackWarningFrames) / AttackWarningFrames);
        const float armLength = 0.035f + progress * 0.055f;
        DrawShape(renderer, enemy.x, enemy.y, armLength, 0.008f, FlashColor);
        DrawShape(renderer, enemy.x, enemy.y, 0.008f, armLength, FlashColor);
    }
}

/**
 * @brief 3D空間内の敵攻撃予告を発光マーカーとして描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawAttackWarnings3D(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    constexpr float FlashColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.attackWarningFrames <= 0) continue;

        // 2Dと3Dのカメラ遷移中も、予告を敵機の発射位置へ追従させる
        const float progress = Math::Clamp01(
            1.0f - static_cast<float>(enemy.attackWarningFrames) / AttackWarningFrames);
        const float size = 0.20f + progress * 0.35f;
        if (m_stageNumber == 5 && enemy.type == Stage::BossEnemy &&
            m_stage5Phase >= Stage5Phase::EastsourceIntro &&
            m_stage5Phase <= Stage5Phase::EastsourceBattle) {
            // 固定した照準地点をプレイヤー面へ表示して発射後の追尾と誤認させない
            DrawModelPrimitive(renderer, camera, 1,
                ToWorldX(enemy.attackWarningTargetX), ToWorldY(enemy.attackWarningTargetY),
                PlayerRailZ + 0.3f, size, size, size, FlashColor);
            continue;
        }
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? enemy.transitionSideX :
            (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        const float x = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
        const float y = Math::Lerp(sideY, exitingRail ? enemy.transitionSideY : enemy.y, railWeight);
        const float railZ = exitingRail ? enemy.transitionRailZ : enemy.z;
        const float z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 1, ToWorldX(x), ToWorldY(y), z - 0.3f,
            size, size, size, FlashColor);
    }
}

void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    float x, float y, float z, float w, float h, float d, const float color[4], float yaw, float pitch) {
    // プリミティブ形状を実3DカメラのViewProjectionへ乗せて描画する
    const Matrix4x4 world = Matrix4x4::Translation({x, y, z}) *
        Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) * Matrix4x4::Scale({w, h, d});
    const PrimitiveShape primitiveShape = shape == 0 ? PrimitiveShape::Plate :
        shape == 1 ? PrimitiveShape::Box :
        shape == 2 ? PrimitiveShape::Cylinder :
        shape == 3 ? PrimitiveShape::Cone :
        shape == 4 ? PrimitiveShape::Prism : PrimitiveShape::Sphere;
    renderer.Draw({
        primitiveShape,
        camera.ProjectionMatrix() * camera.ViewMatrix() * world,
        Vector3::One,
        {color[0], color[1], color[2], color[3]},
        yaw
    });
}

/**
 * @brief XYZ回転を維持して3Dプリミティブを描画する
 * @param renderer 描画先
 * @param camera 使用するカメラ
 * @param shape PrimitiveShapeの列挙値
 * @param position ワールド座標
 * @param scale 寸法
 * @param rotation XYZ回転
 * @param color 色
 * @return なし
 */
void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    const Vector3& position, const Vector3& scale, const Vector3& rotation, const float color[4]) {
    const Matrix4x4 world = Matrix4x4::Translation(position) *
        Matrix4x4::RotationY(rotation.y) * Matrix4x4::RotationX(rotation.x) *
        Matrix4x4::RotationZ(rotation.z) * Matrix4x4::Scale(scale);
    DrawModelPrimitive(renderer, camera, shape, world, color);
}

/**
 * @brief 合成済みワールド行列で3Dプリミティブを描画する
 * @param renderer 描画先
 * @param camera 使用するカメラ
 * @param shape PrimitiveShapeの列挙値
 * @param world 合成済みワールド行列
 * @param color 色
 * @return なし
 */
void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    const Matrix4x4& world, const float color[4]) {
    renderer.Draw({
        static_cast<PrimitiveShape>(shape),
        camera.ProjectionMatrix() * camera.ViewMatrix() * world,
        Vector3::One,
        {color[0], color[1], color[2], color[3]},
        0.0f
    });
}

/**
 * @brief 機体直下の地面へ軽量なBlob Shadowを描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param x 影の中心X座標
 * @param z 影の中心Z座標
 * @param groundTopY 地面上面Y座標
 * @param width 影の半幅
 * @param depth 影の半奥行き
 * @param opacity 影の不透明度
 * @return なし
 */
void SideScrollingShooter::DrawBlobShadow(Renderer& renderer, const Camera3D& camera,
    float x, float z, float groundTopY, float width, float depth, float opacity) {
    constexpr float ShadowHeight = 0.025f;
    const float shadowColor[4] = {0.015f, 0.020f, 0.025f, opacity};

    // 既存の円柱を薄く潰して八角形のBlobとし、Z-fightingを避ける
    DrawModelPrimitive(renderer, camera, 2, x, groundTopY + ShadowHeight * 0.5f, z,
        width * 2.0f, ShadowHeight, depth * 2.0f, shadowColor);
}

void SideScrollingShooter::DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
    float x, float y, float z, bool visible, float yaw) {
    if (!visible) return;

    // 自機は円錐の機首、箱の胴体、三角柱の翼で構成する(仮)
    Vector3 offset = RotateYawOffset(0.0f, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 1, x + offset.x, y + offset.y, z + offset.z,
        0.58f, 0.32f, 1.35f, PlayerColor, yaw);
    offset = RotateYawOffset(0.0f, 0.0f, 0.82f, yaw);
    DrawModelPrimitive(renderer, camera, 3, x + offset.x, y + offset.y, z + offset.z,
        0.42f, 0.42f, 0.78f, PlayerAccent, yaw);
    offset = RotateYawOffset(-0.75f, 0.0f, -0.08f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        1.15f, 0.12f, 0.62f, PlayerAccent, yaw);
    offset = RotateYawOffset(0.75f, 0.0f, -0.08f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        1.15f, 0.12f, 0.62f, PlayerAccent, yaw);
}

void SideScrollingShooter::DrawEnemyModel(Renderer& renderer, const Camera3D& camera,
    const Enemy& enemy, float yaw) const {
    const float x = ToWorldX(enemy.x);
    const float y = ToWorldY(enemy.y);
    const float z = enemy.z;
    auto DrawDamageSmoke = [&](BossPart part, const Vector3& position, float size) {
        const int maxHp = enemy.bossPartMaxHp[part];
        if (maxHp <= 0 || enemy.bossPartHp[part] <= 0 || enemy.bossPartHp[part] * 100 > maxHp * 35) return;
        const Matrix4x4 world = Matrix4x4::Translation(position) * Matrix4x4::Scale({size, size * 1.7f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
            static_cast<float>(enemy.age) / 30.0f + static_cast<float>(part) * 0.37f, 1});
    };
    if (enemy.type == 2 && m_stageNumber == 5) {
        const Stage5ModelTransform transform = EastsourceTransform(enemy);
        const EastsourceModelState state = EastsourceState(enemy);

        // 参照ブランチdrawBoss1の26パーツとXYZ回転を変更せず描画する
        EastsourceModelView::VisitParts(transform, state,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color, EastsourcePartGroup) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                DrawModelPrimitive(renderer, camera, static_cast<int>(shape), world, partColor);
            });

        constexpr EastsourcePartGroup Groups[] = {
            EastsourcePartGroup::Nose,
            EastsourcePartGroup::LeftWing,
            EastsourcePartGroup::RightWing,
            EastsourcePartGroup::LeftEngine,
            EastsourcePartGroup::RightEngine
        };
        for (int part = BossNose; part <= BossRightEngine; ++part) {
            const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
                transform, state, Groups[part]);
            if (bounds.valid) DrawDamageSmoke(static_cast<BossPart>(part), bounds.center, 0.82f);
        }
        return;
    }
    if (enemy.type == 2 && m_stageNumber == 2) {
        // 上下ユニットへ別Transformを渡し、合体状態を描画する
        constexpr float BossScale = 1.92f;
        const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
        const int railgunCycle = enemy.stage2BossActionAge % Stage2RailgunCycleFrames;
        const bool railgunLocked = railgunCycle < Stage2RailgunFireFrame + Stage2RailgunVisualFrames;
        const Vector3 aimTarget {
            ToWorldX(railgunLocked ? enemy.actionX : enemy.turretAimX),
            ToWorldY(railgunLocked ? enemy.actionY : enemy.turretAimY),
            railgunLocked ? Math::Lerp(SidePlaneZ, enemy.actionZ, railWeight) :
                Math::Lerp(SidePlaneZ, enemy.turretAimZ, railWeight)
        };
        bool weaponsDestroyed = true;
        for (int i = 0; i < BossPartCount; ++i) {
            if (i != BossRightEngine && enemy.bossPartHp[i] > 0) weaponsDestroyed = false;
        }
        const bool separated = enemy.phase >= 2.0f;
        const float battleshipPatrolX = enemy.phase >= 2.0f ?
            std::sin(static_cast<float>(enemy.age) * 0.018f) * 2.4f * railWeight : 0.0f;
        const BossModelTransform submarine {{x + ToWorldX(enemy.sandSubmarineOffsetX),
            Stage2SubmarineWorldY(enemy),
            z + enemy.sandSubmarineOffsetZ}, aimTarget, yaw + (separated ? Math::HalfPi : 0.0f), BossScale};
        BossModelTransform battleship {{x + ToWorldX(enemy.landBattleshipOffsetX) + battleshipPatrolX,
            Stage2BattleshipWorldY(enemy),
            z + enemy.landBattleshipOffsetZ}, aimTarget, yaw, BossScale,
            enemy.phase >= 3.0f && enemy.stage2BossAction != Stage2BossAction::Separating, true};
        battleship.secondaryAimTarget = {ToWorldX(enemy.turretAimX), ToWorldY(enemy.turretAimY),
            Math::Lerp(SidePlaneZ, enemy.turretAimZ, railWeight)};
        BossModelDamageState damage {
            enemy.bossPartHp[BossNose] > 0,
            {enemy.bossPartHp[BossLeftWing] > 0, enemy.bossPartHp[BossRightWing] > 0,
                enemy.bossPartHp[BossLeftEngine] > 0},
            enemy.bossPartHp[BossRightEngine] > 0,
            weaponsDestroyed,
            enemy.bossPartHitFlashFrames[BossNose] > 0 && (enemy.bossPartHitFlashFrames[BossNose] / 2) % 2 != 0,
            {enemy.bossPartHitFlashFrames[BossLeftWing] > 0 && (enemy.bossPartHitFlashFrames[BossLeftWing] / 2) % 2 != 0,
                enemy.bossPartHitFlashFrames[BossRightWing] > 0 && (enemy.bossPartHitFlashFrames[BossRightWing] / 2) % 2 != 0,
                enemy.bossPartHitFlashFrames[BossLeftEngine] > 0 && (enemy.bossPartHitFlashFrames[BossLeftEngine] / 2) % 2 != 0},
            enemy.bossPartHitFlashFrames[BossRightEngine] > 0 && (enemy.bossPartHitFlashFrames[BossRightEngine] / 2) % 2 != 0
        };
        for (int i = 0; i < BossFunnelHatchCount; ++i) {
            damage.funnelHatches[i] = enemy.bossPartHp[BossFunnelHatch0 + i] > 0;
            const int frames = enemy.bossPartHitFlashFrames[BossFunnelHatch0 + i];
            damage.funnelHatchesHit[i] = frames > 0 && (frames / 2) % 2 != 0;
        }
        auto DrawBossPart = [&](int shape, const Vector3& position, const Vector3& scale,
            const float color[4], float partYaw, float partPitch) {
            DrawModelPrimitive(renderer, camera, shape, position.x, position.y, position.z,
                scale.x, scale.y, scale.z, color, partYaw, partPitch);
        };
        SandSubmarineView::Draw(submarine, DrawBossPart, damage);
        LandBattleshipView::Draw(battleship, DrawBossPart, damage);

        const bool introducing = m_bossIntroductionPhase == BossIntroductionPhase::Entrance;

        // 登場中とPhase2以降は潜砂艦の移動位置へファンネル出現時と同じ砂埃を連続発生させる
        if (separated || introducing) {
            constexpr int DustLifetimeFrames = 28;
            constexpr float SideGroundTopY = -6.0f;
            constexpr float RailGroundTopY = -3.65f;
            constexpr float EdgeLocalX[] = {-4.2f, 0.0f, 4.2f};
            constexpr float HullHalfWidth = 1.58f;
            const float groundTopY = Math::Lerp(SideGroundTopY, RailGroundTopY, railWeight);
            const float cosine = std::cos(submarine.yaw);
            const float sine = std::sin(submarine.yaw);
            const int effectAge = introducing ? m_bossIntroductionTimer : enemy.age;
            for (int edge = 0; edge < 6; ++edge) {
                const float localX = EdgeLocalX[edge / 2];
                const float localZ = (edge % 2 == 0 ? -1.0f : 1.0f) * HullHalfWidth * railWeight;
                const int dustAge = (effectAge + edge * DustLifetimeFrames / 6) % DustLifetimeFrames;
                DrawSandDust(renderer, camera, {
                    submarine.position.x + (localX * cosine + localZ * sine) * submarine.scale,
                    groundTopY,
                    submarine.position.z + (-localX * sine + localZ * cosine) * submarine.scale
                }, dustAge, railWeight);
            }
        }

        // 合体接近の後半は補助エンジンを止め、炎が潜砂艦を貫通しないようにする
        const bool introductionEngineVisible = introducing &&
            m_bossIntroductionTimer < Stage2BossApproachFrames + Stage2BossAssemblyFrames / 2;
        if (separated || introductionEngineVisible) {
            constexpr float EngineLocalX[] = {-1.80f, -1.80f, 1.70f, 1.70f};
            constexpr float EngineLocalZ[] = {-0.78f, 0.78f, -0.78f, 0.78f};
            constexpr int EngineFlameEffectType = 3;
            const float cosine = std::cos(battleship.yaw);
            const float sine = std::sin(battleship.yaw);
            for (int engine = 0; engine < 4; ++engine) {
                const Vector3 nozzle {
                    battleship.position.x +
                        (EngineLocalX[engine] * cosine + EngineLocalZ[engine] * sine) * battleship.scale,
                    battleship.position.y + 0.02f * battleship.scale,
                    battleship.position.z +
                        (-EngineLocalX[engine] * sine + EngineLocalZ[engine] * cosine) * battleship.scale
                };
                const float flameHalfLength = 0.82f + static_cast<float>(engine % 2) * 0.10f;
                const Matrix4x4 world = Matrix4x4::Translation({
                    nozzle.x, nozzle.y - flameHalfLength, nozzle.z
                }) * Matrix4x4::Scale({0.34f, flameHalfLength, 1.0f});
                renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                    static_cast<float>(introducing ? m_bossIntroductionTimer : enemy.age) / 12.0f +
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
                battleship.position.x + (offset.x * cosine + offset.z * sine) * battleship.scale,
                battleship.position.y + offset.y * battleship.scale,
                battleship.position.z + (-offset.x * sine + offset.z * cosine) * battleship.scale
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
                submarine.position.x + (localX * cosine + side * 1.8f * sine) * submarine.scale,
                submarine.position.y + 0.10f * submarine.scale,
                submarine.position.z + (-localX * sine + side * 1.8f * cosine) * submarine.scale
            }, 0.65f);
        }

        // Phase3では発射直後だけ、専用HLSLで固定照準の軌跡を急速に消す
        const int beamCycle = enemy.stage2BossActionAge % Stage2RailgunCycleFrames;
        const int beamAge = beamCycle - Stage2RailgunFireFrame;
        const bool pointerVisible = beamCycle < Stage2RailgunFireFrame &&
            enemy.stage2BossAction != Stage2BossAction::Separating;
        const bool beamVisible = beamAge >= 0 && beamAge < Stage2RailgunVisualFrames;
        const bool mirageVisible = beamAge >= 0 && beamAge < Stage2RailgunMirageFrames;
        if (enemy.phase >= 3.0f && damage.mainGun && (pointerVisible || beamVisible || mirageVisible)) {
            const float cosine = std::cos(battleship.yaw);
            const float sine = std::sin(battleship.yaw);
            const Vector3 pivot {
                battleship.position.x + (-1.55f * cosine) * battleship.scale,
                battleship.position.y + 2.08f * battleship.scale,
                battleship.position.z + (1.55f * sine) * battleship.scale
            };
            const Vector3 lockedTarget {ToWorldX(enemy.actionX), ToWorldY(enemy.actionY),
                railWeight > 0.5f ? enemy.actionZ : SidePlaneZ};
            const Vector3 delta = lockedTarget - pivot;
            const float horizontal = (std::max)(0.001f, std::sqrt(delta.x * delta.x + delta.z * delta.z));
            const float targetLength = (std::max)(0.001f, std::sqrt(horizontal * horizontal + delta.y * delta.y));
            const Vector3 direction = delta / targetLength;
            const float beamLength = targetLength + 18.0f;
            const Vector3 beamCenter = pivot + direction * (beamLength * 0.5f);
            const float beamYaw = std::atan2(direction.z, -direction.x);
            const float beamPitch = -std::asin(direction.y);
            auto DrawRailgunLayer = [&](float width, float progress, int effectType) {
                const Matrix4x4 world = Matrix4x4::Translation(beamCenter) *
                    Matrix4x4::RotationY(beamYaw) * Matrix4x4::RotationZ(beamPitch) *
                    Matrix4x4::Scale({beamLength * 0.5f, width, 1.0f});
                renderer.DrawRailgun({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                    progress, effectType});
            };
            if (mirageVisible) {
                DrawRailgunLayer(1.75f,
                    static_cast<float>(beamAge) / static_cast<float>(Stage2RailgunMirageFrames), 2);
            }
            if (pointerVisible) {
                DrawRailgunLayer(0.10f,
                    static_cast<float>(beamCycle) / static_cast<float>(Stage2RailgunFireFrame), 1);
            } else if (beamVisible) {
                DrawRailgunLayer(0.85f,
                    static_cast<float>(beamAge) / static_cast<float>(Stage2RailgunVisualFrames), 0);
            }
        }
        return;
    }

    if (enemy.type == 2) {
        // Stage2以外は従来の大型戦闘機モデルを維持する
        constexpr float ModelScale = 0.14f;
        constexpr float Gray[] = {0.50f, 0.50f, 0.50f, 1.0f};
        constexpr float White[] = {0.60f, 0.60f, 0.60f, 1.0f};
        constexpr float Black[] = {0.20f, 0.20f, 0.20f, 1.0f};
        constexpr float HitColor[] = {1.0f, 0.03f, 0.03f, 1.0f};
        auto PartColor = [&](BossPart part, const float color[4]) {
            const int frames = enemy.bossPartHitFlashFrames[part];
            return frames > 0 && (frames / 2) % 2 != 0 ? HitColor : color;
        };
        auto DrawBossPart = [&](int shape, float localX, float localY, float localZ,
            float width, float height, float depth, const float color[4]) {
            const Vector3 offset = RotateYawOffset(localX * ModelScale, localY * ModelScale,
                localZ * ModelScale, yaw);
            DrawModelPrimitive(renderer, camera, shape, x + offset.x, y + offset.y, z + offset.z,
                width * ModelScale, height * ModelScale, depth * ModelScale, color, yaw);
        };

        // 機首と上部メインボディを描画する
        if (enemy.bossPartHp[BossNose] > 0) {
            DrawBossPart(2, 0.0f, 3.0f, -14.0f, 6.0f, 6.0f, 4.0f, PartColor(BossNose, Gray));
            DrawBossPart(2, 0.0f, 2.0f, -17.5f, 2.0f, 2.0f, 3.0f, PartColor(BossNose, Gray));
            DrawBossPart(2, 0.0f, 4.5f, -20.0f, 1.0f, 1.0f, 8.0f, PartColor(BossNose, Black));
        }
        DrawBossPart(2, 0.0f, 2.0f, 0.0f, 18.0f, 18.0f, 16.0f, Gray);
        DrawBossPart(2, 0.0f, 2.0f, -10.0f, 14.0f, 14.0f, 4.0f, Gray);
        DrawBossPart(2, 0.0f, 2.0f, 10.0f, 14.0f, 14.0f, 4.0f, Gray);
        DrawBossPart(1, 0.0f, 12.0f, 2.0f, 4.0f, 4.0f, 4.0f, Gray);
        DrawBossPart(2, 0.0f, 13.0f, -2.0f, 1.0f, 1.0f, 4.0f, Black);

        // 下部ボディと左右主翼を描画する
        DrawBossPart(2, 0.0f, -12.0f, 0.0f, 4.0f, 4.0f, 10.0f, Gray);
        DrawBossPart(2, 0.0f, -15.0f, 1.0f, 2.0f, 2.0f, 8.0f, Gray);
        DrawBossPart(2, 0.0f, -12.0f, -7.0f, 1.0f, 1.0f, 6.0f, Black);
        DrawBossPart(1, 2.0f, -8.0f, 0.0f, 1.0f, 5.0f, 1.0f, Black);
        DrawBossPart(1, -2.0f, -8.0f, 0.0f, 1.0f, 5.0f, 1.0f, Black);
        if (enemy.bossPartHp[BossLeftWing] > 0) {
            DrawBossPart(1, 13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, PartColor(BossLeftWing, White));
            DrawBossPart(1, 21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, PartColor(BossLeftWing, White));
        }
        if (enemy.bossPartHp[BossRightWing] > 0) {
            DrawBossPart(1, -13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, PartColor(BossRightWing, White));
            DrawBossPart(1, -21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, PartColor(BossRightWing, White));
        }

        // 主エンジンと左右エンジンを描画する
        DrawBossPart(2, 0.0f, 3.0f, 15.0f, 10.0f, 10.0f, 6.0f, Gray);
        DrawBossPart(2, 7.0f, 3.0f, 18.0f, 4.0f, 4.0f, 6.0f, Black);
        DrawBossPart(2, -7.0f, 3.0f, 18.0f, 4.0f, 4.0f, 6.0f, Black);
        DrawBossPart(1, 0.0f, -6.0f, 16.5f, 2.0f, 8.0f, 3.0f, White);
        DrawBossPart(1, 0.0f, 12.0f, 16.5f, 2.0f, 8.0f, 3.0f, White);
        if (enemy.bossPartHp[BossLeftEngine] > 0) {
            DrawBossPart(2, 6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, PartColor(BossLeftEngine, Black));
            DrawBossPart(2, 6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, PartColor(BossLeftEngine, Black));
        }
        if (enemy.bossPartHp[BossRightEngine] > 0) {
            DrawBossPart(2, -6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, PartColor(BossRightEngine, Black));
            DrawBossPart(2, -6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, PartColor(BossRightEngine, Black));
        }
        // 従来ボスの主要部位位置から煙を上げる
        constexpr float SmokeX[] = {0.0f, 17.0f, -17.0f, 6.0f, -6.0f};
        constexpr float SmokeY[] = {4.5f, 3.0f, 3.0f, -4.0f, -4.0f};
        constexpr float SmokeZ[] = {-17.0f, 0.0f, 0.0f, 13.0f, 13.0f};
        for (int part = BossNose; part <= BossRightEngine; ++part) {
            const Vector3 offset = RotateYawOffset(SmokeX[part] * ModelScale,
                SmokeY[part] * ModelScale, SmokeZ[part] * ModelScale, yaw);
            DrawDamageSmoke(static_cast<BossPart>(part), {x + offset.x, y + offset.y, z + offset.z}, 0.75f);
        }
        return;
    }

    // 通常敵は奥から来る小型機として描画する
    const float scale = enemy.behavior != nullptr ? enemy.behavior->RenderScale() : 1.0f;
    Vector3 offset = RotateYawOffset(0.0f, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 1, x + offset.x, y + offset.y, z + offset.z,
        0.65f * scale, 0.42f * scale, 1.0f * scale, EnemyColor, yaw);
    offset = RotateYawOffset(0.0f, 0.0f, -0.68f * scale, yaw);
    DrawModelPrimitive(renderer, camera, 3, x + offset.x, y + offset.y, z + offset.z,
        0.45f * scale, 0.45f * scale, 0.68f * scale, EnemyAccent, yaw);
    offset = RotateYawOffset(-0.75f * scale, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        0.9f * scale, 0.10f, 0.5f * scale, EnemyAccent, yaw);
    offset = RotateYawOffset(0.75f * scale, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        0.9f * scale, 0.10f, 0.5f * scale, EnemyAccent, yaw);
}

/**
 * @brief 砂面から放物線状に舞う砂埃を描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param position 発生地点のワールド座標
 * @param age 発生からの経過フレーム
 * @param railWeight 3D表示の補間率
 * @return なし
 */
void SideScrollingShooter::DrawSandDust(Renderer& renderer, const Camera3D& camera,
    const Vector3& position, int age, float railWeight) {
    constexpr int DustLifetimeFrames = 28;
    constexpr int DustParticleCount = 13;
    constexpr float DustColor[] = {0.62f, 0.43f, 0.20f, 1.0f};
    static_assert(DustLifetimeFrames > 0 && DustParticleCount > 0);
    if (age < 0 || age >= DustLifetimeFrames) return;

    // ウミヘビの水しぶきと同じ放物線で、2Dでは横方向、3Dでは円形へ砂粒を広げる
    const float progress = static_cast<float>(age) / static_cast<float>(DustLifetimeFrames);
    const float fade = 1.0f - progress;
    for (int i = 0; i < DustParticleCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) /
            static_cast<float>(DustParticleCount);
        const float launchVelocity = 1.10f + static_cast<float>((i * 5) % 4) * 0.18f;
        const float height = launchVelocity * progress - launchVelocity * progress * progress;
        const float radius = (0.28f + static_cast<float>(i % 3) * 0.10f) * progress;
        const float sideOffsetX = static_cast<float>(i - 6) * 0.12f * progress;
        const float offsetX = Math::Lerp(sideOffsetX, std::cos(angle) * radius, railWeight);
        const float offsetZ = Math::Lerp(0.0f, std::sin(angle) * radius, railWeight);
        const float size = (0.14f + static_cast<float>(i % 4) * 0.035f) * fade;
        const float color[4] = {DustColor[0], DustColor[1], DustColor[2], fade * 0.82f};
        DrawModelPrimitive(renderer, camera, 5, position.x + offsetX,
            position.y + 0.06f + height, position.z + offsetZ,
            size, size * 1.35f, size, color);
    }
}

void SideScrollingShooter::DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw) const {
    if (shot.enemy) {
        if (shot.funnel || shot.missile) {
            // 砂地を抜けた地点へ砂埃を発生させる
            constexpr int DustLifetimeFrames = 28;
            if (shot.funnel && shot.funnelDustAge >= 0 && shot.funnelDustAge < DustLifetimeFrames) {
                const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
                DrawSandDust(renderer, camera, {ToWorldX(shot.funnelDustX),
                    ToWorldY(shot.funnelDustY), shot.funnelDustZ}, shot.funnelDustAge, railWeight);
            }
            constexpr float FunnelBody[] = {0.18f, 0.16f, 0.14f, 1.0f};
            constexpr float FunnelEdge[] = {0.72f, 0.20f, 0.08f, 1.0f};
            const float dx = ToWorldX(shot.vx);
            const float dy = ToWorldY(shot.vy);
            const float dz = shot.vz;
            const float horizontal = (std::max)(0.001f, std::sqrt(dx * dx + dz * dz));
            const float length = (std::max)(0.001f, std::sqrt(horizontal * horizontal + dy * dy));
            const float funnelYaw = std::atan2(dz, -dx);
            const float funnelPitch = -std::asin(dy / length);
            DrawModelPrimitive(renderer, camera, 3, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                0.72f, 0.30f, 0.30f, FunnelBody, funnelYaw, funnelPitch);
            DrawModelPrimitive(renderer, camera, 4, ToWorldX(shot.x) - dx / length * 0.28f,
                ToWorldY(shot.y) - dy / length * 0.28f, shot.z - dz / length * 0.28f,
                0.44f, 0.42f, 0.42f, FunnelEdge, funnelYaw, funnelPitch);
            if (shot.funnel) {
                // 推進方向の後方へ既存の煙シェーダーを置き、ロケット噴出煙として流用する
                const Vector3 smokeCenter {ToWorldX(shot.x) - dx / length * 0.72f,
                    ToWorldY(shot.y) - dy / length * 0.72f, shot.z - dz / length * 0.72f};
                const Matrix4x4 smokeWorld = Matrix4x4::Translation(smokeCenter) *
                    Matrix4x4::Scale({0.42f, 0.62f, 1.0f});
                renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld,
                    static_cast<float>(shot.age) / 9.0f, 1});
            }
            return;
        }
        DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
            0.16f, 0.16f, 0.65f, EnemyShotColor, yaw);
        return;
    }

    // 特殊弾は専用HLSLへ画面座標と進行方向を渡して描画する
    if (shot.special) {
        const Vector3 worldPosition {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};
        Vector2 screenPosition;
        if (!camera.TryWorldToScreen(worldPosition, screenPosition)) return;

        // 弾速ベクトルを画面へ投影して、専用シェーダーの弾頭方向へ反映する
        const Vector3 worldNextPosition {
            ToWorldX(shot.x + shot.vx), ToWorldY(shot.y + shot.vy), shot.z + shot.vz};
        Vector2 nextScreenPosition;
        if (!camera.TryWorldToScreen(worldNextPosition, nextScreenPosition)) nextScreenPosition = screenPosition;
        const Viewport& viewport = camera.GetViewport();
        const Vector2 position {
            (screenPosition.x - static_cast<float>(viewport.x)) / static_cast<float>(viewport.width) * 2.0f - 1.0f,
            1.0f - (screenPosition.y - static_cast<float>(viewport.y)) / static_cast<float>(viewport.height) * 2.0f};
        const Vector2 direction {
            (nextScreenPosition.x - screenPosition.x) / static_cast<float>(viewport.width) * 2.0f,
            (screenPosition.y - nextScreenPosition.y) / static_cast<float>(viewport.height) * 2.0f};
        renderer.DrawPlayerShot({position, {0.040f, 0.020f}, std::atan2(direction.y, direction.x),
            static_cast<float>(m_frame), static_cast<int>(shot.playerType)});
        return;
    }
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.16f, 0.16f, 1.15f, PlayerShotColor, yaw);
}

/**
 * @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param explosion 描画対象の爆発
 * @return なし
 */
void SideScrollingShooter::DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion) {
    if (explosion.destruction) {
        const float progress = static_cast<float>(explosion.age) / DestructionExplosionLifetimeFrames;
        const float fireProgress = Math::Clamp01(progress * 2.55f);
        const float fireSize = 0.72f + fireProgress * 1.55f;
        const Vector3 center {ToWorldX(explosion.x), ToWorldY(explosion.y), explosion.z};

        // 中心火球とずらした火球を重ね、撃破直後の爆発炎を厚くする
        const Matrix4x4 fireWorld = Matrix4x4::Translation(center) *
            Matrix4x4::Scale({fireSize, fireSize, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * fireWorld, fireProgress});
        constexpr Vector3 FireOffsets[] = {{-0.52f, 0.18f, 0.0f}, {0.46f, 0.34f, 0.0f}};
        for (int i = 0; i < 2; ++i) {
            const float lobeProgress = Math::Clamp01(fireProgress * 1.18f - static_cast<float>(i) * 0.10f);
            const Matrix4x4 lobeWorld = Matrix4x4::Translation(center + FireOffsets[i] * fireSize) *
                Matrix4x4::Scale({fireSize * 0.68f, fireSize * 0.68f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * lobeWorld, lobeProgress});
        }

        // 炎が消える頃から大きな黒煙を残す
        const float smokeSize = 1.15f + progress * 1.85f;
        const Matrix4x4 smokeWorld = Matrix4x4::Translation(center) *
            Matrix4x4::Scale({smokeSize, smokeSize * 1.28f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld, progress, 2});
        return;
    }

    const float progress = static_cast<float>(explosion.age) / ExplosionLifetimeFrames;
    const float size = 0.36f + progress * 0.72f;
    const Matrix4x4 world = Matrix4x4::Translation({ToWorldX(explosion.x), ToWorldY(explosion.y), explosion.z}) *
        Matrix4x4::Scale({size, size, 1.0f});
    renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world, progress});
}

/**
 * @brief 飛散中の機体部品を描画する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param debris 描画対象の飛散部品
 * @return なし
 */
void SideScrollingShooter::DrawDebris(Renderer& renderer, const Camera3D& camera,
    const Debris& debris, float railWeight) {
    const float scale = debris.age > debris.shrinkStartAge ?
        1.0f - Math::Clamp01(static_cast<float>(debris.age - debris.shrinkStartAge) /
            static_cast<float>(debris.lifetime - debris.shrinkStartAge)) : 1.0f;
    DrawModelPrimitive(renderer, camera, debris.shape, debris.x, debris.y, debris.z,
        debris.width * scale, debris.height * scale, debris.depth * scale, debris.color.data(), debris.yaw);

    // 沈下と再浮上の各区間で砂埃を絶えず重ね、船体幅全体から大量に噴き上げる
    if (debris.effect == Debris::Effect::Stage2Sink) {
        constexpr int FirstSinkEndFrame = 108;
        constexpr int ResurfaceStartFrame = 148;
        const bool movingThroughSand = debris.age < FirstSinkEndFrame ||
            debris.age >= ResurfaceStartFrame || debris.effectAge >= 0;
        const float groundTopY = Math::Lerp(-6.0f, -3.65f, railWeight);
        if (movingThroughSand) {
            for (int wave = 0; wave < 4; ++wave) {
                const int dustAge = (debris.age + wave * 7) % 28;
                for (int side = -2; side <= 2; ++side) {
                    DrawSandDust(renderer, camera,
                        {debris.x + side * debris.width * 0.16f, groundTopY, debris.z}, dustAge, railWeight);
                }
            }
        }
    }

    // 降下中は連続爆発と煙を引き、下部船体との衝突後に大爆発へ切り替える
    if (debris.effect == Debris::Effect::Stage2Impact) {
        constexpr int EffectFrames = 72;
        const Matrix4x4 viewProjection = camera.ProjectionMatrix() * camera.ViewMatrix();
        if (debris.effectAge < 0) {
            for (int i = 0; i < 3; ++i) {
                const int cycleAge = (debris.age + i * 17) % 54;
                const float cycle = static_cast<float>(cycleAge) / 54.0f;
                const float offsetX = static_cast<float>(i - 1) * debris.width * 0.22f;
                const float size = debris.width * (0.18f + cycle * 0.24f);
                const Matrix4x4 fireWorld = Matrix4x4::Translation(
                    {debris.x + offsetX, debris.y + debris.height * 0.18f, debris.z}) *
                    Matrix4x4::Scale({size, size, 1.0f});
                renderer.DrawExplosion({viewProjection * fireWorld, cycle});
            }
            const float smokeCycle = static_cast<float>(debris.age % 72) / 72.0f;
            const float smokeSize = debris.width * (0.34f + smokeCycle * 0.38f);
            const Matrix4x4 smokeWorld = Matrix4x4::Translation(
                {debris.x, debris.y + debris.height * (0.45f + smokeCycle * 0.75f), debris.z}) *
                Matrix4x4::Scale({smokeSize, smokeSize * 1.45f, 1.0f});
            renderer.DrawExplosion({viewProjection * smokeWorld, smokeCycle, 2});
            return;
        }

        const float progress = Math::Clamp01(static_cast<float>(debris.effectAge) / EffectFrames);
        for (int i = 0; i < 5; ++i) {
            const float delayed = Math::Clamp01(progress * 1.55f - static_cast<float>(i) * 0.10f);
            const float offsetX = static_cast<float>(i - 2) * debris.width * 0.16f;
            const float size = debris.width * (0.34f + delayed * 0.42f);
            const Matrix4x4 fireWorld = Matrix4x4::Translation(
                {debris.x + offsetX, debris.y + debris.height * 0.22f, debris.z}) *
                Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({viewProjection * fireWorld, delayed});
        }
        const float smokeSize = debris.width * (0.58f + progress * 0.72f);
        const Matrix4x4 smokeWorld = Matrix4x4::Translation(
            {debris.x, debris.y + debris.height * (0.35f + progress * 0.85f), debris.z}) *
            Matrix4x4::Scale({smokeSize, smokeSize * 1.35f, 1.0f});
        renderer.DrawExplosion({viewProjection * smokeWorld, progress, 2});
        for (int side = -2; side <= 2; ++side) {
            DrawSandDust(renderer, camera, {debris.x + side * debris.width * 0.18f,
                debris.y - debris.height * 0.5f, debris.z}, debris.effectAge, railWeight);
        }
    }
}

/**
 * @brief 取得アイテムを描画する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param item 描画対象の取得アイテム
 * @param yaw モデルのY軸回転角度
 */
void SideScrollingShooter::DrawItemModel(Renderer& renderer, const Camera3D& camera,
    const Item& item, float yaw) {
    DrawModelPrimitive(renderer, camera, 5, ToWorldX(item.x), ToWorldY(item.y), item.z,
        0.28f, 0.28f, 0.28f, item.type == ItemType::Power ? PowerItemColor : ScoreItemColor, yaw);
}

/**
 * @brief チャプター終了時の戦績を描画する
 * @param renderer 描画先レンダラー
 */
void SideScrollingShooter::DrawChapterResult(Renderer& renderer) const {
    if (!m_chapterResultActive) return;

    constexpr float CharacterSpacing = 0.003f;
    constexpr int FadeFrames = 30;
    const float progress = (std::min)(1.0f,
        static_cast<float>(m_chapterResultTimer) / static_cast<float>(ChapterResultCountUpFrames));
    // 表示開始と終了の両方で文字を滑らかにフェードさせる
    const float alpha = (std::min)(
        SmoothStep(static_cast<float>(m_chapterResultTimer) / FadeFrames),
        SmoothStep(static_cast<float>(ChapterResultDisplayFrames - m_chapterResultTimer) / FadeFrames));
    const int annihilationRate = m_chapterResult.enemySpawnCount == 0 ? 0 :
        m_chapterResult.enemyDefeatCount * 100 / m_chapterResult.enemySpawnCount;
    const int graze = static_cast<int>(m_chapterResult.grazeCount * progress);
    const int defeat = static_cast<int>(m_chapterResult.enemyDefeatCount * progress);
    const int retry = static_cast<int>(m_chapterResult.retryCount * progress);
    const int score = static_cast<int>(m_chapterResult.score * progress);
    const int total = static_cast<int>(m_chapterResult.totalScore * progress);
    const int displayedRate = static_cast<int>(annihilationRate * progress);
    char line[64];

    renderer.DrawText("CHAPTER RESULT", TextAlign::Center, 0.028f, { 1.0f, 0.88f, 0.25f, alpha }, { 0.0f, 0.40f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "GRAZE        %d", graze);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, 0.20f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "ANNIHILATION  %d / %d  %d%%", defeat, m_chapterResult.enemySpawnCount, displayedRate);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, 0.08f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "RETRY        %d", retry);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, -0.04f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "CHAPTER SCORE  %06d", score);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, -0.16f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "TOTAL SCORE    %06d", total);
    renderer.DrawText(line, TextAlign::Center, 0.020f, { 1.0f, 0.88f, 0.25f, alpha }, { 0.0f, -0.31f }, CharacterSpacing);
}

/**
 * @brief リスタート中のカウントダウンを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawRestart(Renderer& renderer) const {
    if (m_restartTimer <= 0) return;

    const int countdown = (m_restartTimer + 59) / 60;
    char text[16];
    std::snprintf(text, sizeof(text), "RESTART %d", countdown);
    renderer.DrawText(text, TextAlign::Center, 0.038f, { 1.0f, 0.88f, 0.25f, 1.0f }, { 0.0f, 0.12f });
}

/**
 * @brief ミッション開始または終了の文字アニメーションを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawMissionBanner(Renderer& renderer) const {
    if ((!m_clear && m_missionStartTimer <= 0) || (m_clear && m_clearTimer <= 0)) return;

    char startText[24];
    std::snprintf(startText, sizeof(startText), "MISSION %d START", m_stageNumber);
    const std::string_view text = m_clear ? "ARRESTED" : startText;
    const int remainingFrames = m_clear ? (std::max)(0, m_clearTimer) : m_missionStartTimer;
    const int elapsedFrames = (m_clear ? ClearWaitFrames : MissionBannerDisplayFrames) - remainingFrames;
    const float fade = remainingFrames < 20 ? static_cast<float>(remainingFrames) / 20.0f : 1.0f;
    constexpr float BaseSize = 0.050f;
    constexpr float Advance = 0.078f;
    const float firstX = -static_cast<float>(text.size() - 1) * Advance * 0.5f;

    // 各文字を時間差で大きく出し、中央の定位置へ収束させる
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == ' ') continue;
        const float scale = MissionBannerGlyphScale(elapsedFrames - static_cast<int>(i) * MissionBannerGlyphDelayFrames);
        if (scale <= 0.0f) continue;
        const char glyph[] = {text[i], '\0'};
        const float size = BaseSize * scale;
        const Vector2 position {firstX + static_cast<float>(i) * Advance, 0.10f};
        renderer.DrawText(glyph, position + Vector2 {0.012f, -0.014f}, size,
            {0.05f, 0.02f, 0.01f, fade * 0.85f});
        renderer.DrawText(glyph, position, size, {1.0f, 0.78f, 0.12f, fade});
    }
}

void SideScrollingShooter::DrawBossHud(Renderer& renderer) const {
    if (m_stageNumber == 5 && m_stage5Phase != Stage5Phase::Approach) {
        DrawStage5Hud(renderer);
        return;
    }
    if (!m_bossBattle || m_clear || m_bossIntroductionPhase != BossIntroductionPhase::None) {
        return;
    }

    // 2D/3D共通のボスHP表示をカメラリセット後のUI座標へ描画する
    constexpr float BossBarBack[4] = { 0.20f, 0.08f, 0.22f, 1.0f };
    constexpr float BossBarFill[4] = { 0.95f, 0.15f, 0.45f, 1.0f };
    constexpr float BossBarDivider[4] = { 1.00f, 0.82f, 0.30f, 0.95f };
    constexpr float BossBarWidth = 0.62f;
    const int bossMaxHp = m_stage->BossMaxHp();
    const float hpRate = bossMaxHp > 0 ? Math::Clamp01(m_displayBossHp / bossMaxHp) : 0.0f;
    DrawShape(renderer, 0.0f, 0.76f, BossBarWidth, 0.025f, BossBarBack);
    DrawShape(renderer, BossBarWidth * (1.0f - hpRate), 0.76f,
        BossBarWidth * hpRate, 0.018f, BossBarFill);
    // 4フェーズの境界をHPバー上に常時表示する
    for (int phase = 1; phase < BossPhaseCount; ++phase) {
        DrawShape(renderer, -BossBarWidth + BossBarWidth * 2.0f * static_cast<float>(phase) / static_cast<float>(BossPhaseCount),
            0.755f, 0.008f, 0.035f, BossBarDivider);
    }
    const BossStory story = BossStories::ForStage(m_stageNumber);
    renderer.DrawText(story.bossName, TextAlign::Center, 0.014f,
        { 1.0f, 0.45f, 0.65f, 1.0f }, { 0.0f, 0.86f });
    const char* phaseLabel = "NORMAL 1";
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.type != 2) continue;
        constexpr const char* PhaseLabels[] = {
            "NORMAL 1", "SPECIAL 1", "NORMAL 2", "SPECIAL 2"
        };
        phaseLabel = PhaseLabels[enemy.bossPhase];
        break;
    }
    renderer.DrawText(phaseLabel, { -BossBarWidth, 0.81f }, 0.012f,
        { 1.0f, 0.82f, 0.30f, 1.0f });
}

/**
 * @brief 機体モデルを構成する部品を飛散エフェクトとして生成する
 * @param enemy 分解する敵機
 * @param bossPart 分解するボス部位、-1は撃破時の残存ボディ
 * @return なし
 */
void SideScrollingShooter::SpawnEnemyDebris(const Enemy& enemy, int bossPart) {
    constexpr float Gray[4] = { 0.50f, 0.50f, 0.50f, 1.0f };
    constexpr float White[4] = { 0.60f, 0.60f, 0.60f, 1.0f };
    constexpr float Black[4] = { 0.20f, 0.20f, 0.20f, 1.0f };
    const float yaw = IsRailGameplayActive() ? 0.0f : Math::HalfPi;
    const float x = ToWorldX(enemy.x);
    const float y = ToWorldY(enemy.y);
    int pieceNumber = 0;
    auto AddPiece = [&](int shape, float localX, float localY, float localZ,
        float width, float height, float depth, const float color[4], float scale = 1.0f) {
        const Vector3 offset = RotateYawOffset(localX * scale, localY * scale, localZ * scale, yaw);
        static constexpr Vector3 SpreadDirections[] = {
            {-0.85f, 0.55f, -0.65f}, {0.90f, -0.40f, -0.75f},
            {-0.70f, -0.80f, 0.85f}, {0.65f, 0.90f, 0.70f},
            {-0.45f, 0.25f, 1.00f}, {0.50f, -0.95f, -0.35f},
            {-1.00f, 0.10f, 0.35f}, {0.95f, 0.35f, -0.15f}
        };
        const Vector3 direction = SpreadDirections[pieceNumber++ % 8];
        const Vector3 velocity = RotateYawOffset(direction.x * 0.040f, direction.y * 0.040f,
            direction.z * 0.040f, yaw);
        SpawnDebrisPiece(x + offset.x, y + offset.y, enemy.z + offset.z,
            velocity.x, velocity.y, velocity.z, yaw, 0.08f + direction.x * 0.050f,
            shape, width * scale, height * scale, depth * scale, color);
    };

    if (enemy.type != 2) {
        const float scale = enemy.behavior != nullptr ? enemy.behavior->RenderScale() : 1.0f;
        AddPiece(1, 0.0f, 0.0f, 0.0f, 0.65f, 0.42f, 1.0f, EnemyColor, scale);
        AddPiece(3, 0.0f, 0.0f, -0.68f, 0.45f, 0.45f, 0.68f, EnemyAccent, scale);
        AddPiece(4, -0.75f, 0.0f, 0.0f, 0.9f, 0.10f, 0.5f, EnemyAccent, scale);
        AddPiece(4, 0.75f, 0.0f, 0.0f, 0.9f, 0.10f, 0.5f, EnemyAccent, scale);
        return;
    }

    if (m_stageNumber == 5) {
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
        const Stage5ModelTransform transform = EastsourceTransform(enemy);

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
                SpawnDebrisPiece(center.x, center.y, center.z,
                    direction * (0.035f + static_cast<float>(pieceNumber % 3) * 0.008f),
                    0.018f + static_cast<float>(pieceNumber % 2) * 0.012f,
                    -0.025f + static_cast<float>(pieceNumber % 3) * 0.018f,
                    0.0f, direction * 0.10f, debrisShape,
                    radius, radius * 0.65f, radius, pieceColor, 90, 64, false);
                ++pieceNumber;
            });
        return;
    }

    if (m_stageNumber == 2) {
        constexpr float ModelScale = 1.92f;
        constexpr float Hull[] = {0.28f, 0.24f, 0.17f, 1.0f};
        constexpr float Armor[] = {0.36f, 0.31f, 0.22f, 1.0f};
        constexpr float Metal[] = {0.48f, 0.45f, 0.36f, 1.0f};
        constexpr float Dark[] = {0.07f, 0.065f, 0.055f, 1.0f};
        constexpr float Core[] = {0.95f, 0.28f, 0.055f, 1.0f};
        auto AddStage2Piece = [&](int shape, const Vector3& local, const Vector3& size,
            const float color[4], float unitOffsetY) {
            AddPiece(shape, local.x, local.y + unitOffsetY / ModelScale, local.z,
                size.x, size.y, size.z, color, ModelScale);
        };

        // 個別部位は描画と同じ構成の主要プリミティブを飛散させる
        if (bossPart == BossNose) {
            AddStage2Piece(2, {-1.55f, 2.03f, 0.0f}, {1.75f, 1.10f, 1.75f}, Dark, 0.78f);
            AddStage2Piece(4, {-2.30f, 2.08f, 0.0f}, {1.85f, 1.08f, 1.55f}, Armor, 0.78f);
            AddStage2Piece(2, {-4.25f, 2.08f, 0.0f}, {2.95f, 0.54f, 0.54f}, Metal, 0.78f);
            return;
        }
        if (bossPart == BossLeftWing || bossPart == BossRightWing || bossPart == BossLeftEngine) {
            constexpr Vector3 Positions[] = {
                {-0.55f, 2.34f, -0.92f}, {0.65f, 2.34f, 0.0f}, {2.85f, 1.92f, 0.92f}
            };
            const int index = bossPart == BossLeftWing ? 0 :
                (bossPart == BossRightWing ? 1 : 2);
            const Vector3& position = Positions[index];
            AddStage2Piece(2, position, {0.76f, 0.46f, 0.76f}, Dark, 0.78f);
            AddStage2Piece(1, {position.x - 0.20f, position.y + 0.26f, position.z},
                {0.82f, 0.46f, 0.64f}, Armor, 0.78f);
            AddStage2Piece(2, {position.x - 1.10f, position.y + 0.26f, position.z},
                {1.35f, 0.20f, 0.20f}, Metal, 0.78f);
            return;
        }
        if (bossPart == BossRightEngine) {
            AddStage2Piece(2, {0.25f, 1.42f, 0.0f}, {1.35f, 0.22f, 1.35f}, Core, -0.45f);
            return;
        }
        if (bossPart >= BossFunnelHatch0) {
            const int hatch = bossPart - BossFunnelHatch0;
            const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
            const float localZ = (hatch < 6 ? -1.0f : 1.0f) * 1.80f;
            AddStage2Piece(1, {localX, -0.22f, localZ}, {0.38f, 0.34f, 0.08f}, Core, -0.45f);
            return;
        }

        // 撃破時は下部の沈下と再浮上、上部の緩やかな降下を同じ時間軸で開始する
        const float submarineY = Stage2SubmarineWorldY(enemy);
        const float battleshipY = Stage2BattleshipWorldY(enemy);
        const Vector3 lowerOffset = RotateYawOffset(0.0f, -0.45f, 0.0f, yaw);
        SpawnDebrisPiece(x + lowerOffset.x, submarineY + lowerOffset.y, enemy.z + lowerOffset.z,
            0.0f, 0.0f, 0.0f, yaw, 0.0f, 5, 8.8f * ModelScale, 2.05f * ModelScale,
            3.15f * ModelScale, Hull, 420, 420, false, Debris::Effect::Stage2Sink);
        auto AddFallingUpper = [&](int shape, const Vector3& local, const Vector3& size,
            const float color[4], Debris::Effect effect) {
            const Vector3 offset = RotateYawOffset(local.x * ModelScale, local.y * ModelScale,
                local.z * ModelScale, yaw);
            SpawnDebrisPiece(x + offset.x, battleshipY + offset.y + 3.25f, enemy.z + offset.z,
                0.0f, 0.0f, 0.0f, yaw,
                0.018f + local.x * 0.004f, shape, size.x * ModelScale, size.y * ModelScale,
                size.z * ModelScale, color, 420, 420, true, effect);
        };
        AddFallingUpper(1, {0.20f, 0.95f, 0.0f}, {2.65f, 1.30f, 2.70f}, Hull,
            Debris::Effect::Stage2Impact);
        AddFallingUpper(4, {-1.65f, 0.70f, 0.0f}, {2.70f, 1.08f, 2.55f}, Hull,
            Debris::Effect::Stage2ImpactPiece);
        AddFallingUpper(4, {2.65f, 1.62f, 0.0f}, {1.45f, 0.76f, 1.78f}, Armor,
            Debris::Effect::Stage2ImpactPiece);
        for (int part = 0; part < BossPartCount; ++part) {
            if (enemy.bossPartHp[part] > 0) SpawnEnemyDebris(enemy, part);
        }
        return;
    }

    constexpr float ModelScale = 0.14f;
    auto AddBossPiece = [&](int shape, float localX, float localY, float localZ,
        float width, float height, float depth, const float color[4]) {
        AddPiece(shape, localX, localY, localZ, width, height, depth, color, ModelScale);
    };
    if (bossPart == BossNose) {
        AddBossPiece(2, 0.0f, 3.0f, -14.0f, 6.0f, 6.0f, 4.0f, Gray);
        AddBossPiece(2, 0.0f, 2.0f, -17.5f, 2.0f, 2.0f, 3.0f, Gray);
        AddBossPiece(2, 0.0f, 4.5f, -20.0f, 1.0f, 1.0f, 8.0f, Black);
        return;
    }
    if (bossPart == BossLeftWing || bossPart == BossRightWing) {
        const float side = bossPart == BossLeftWing ? 1.0f : -1.0f;
        AddBossPiece(1, side * 13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, White);
        AddBossPiece(1, side * 21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, White);
        return;
    }
    if (bossPart == BossLeftEngine || bossPart == BossRightEngine) {
        const float side = bossPart == BossLeftEngine ? 1.0f : -1.0f;
        AddBossPiece(2, side * 6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, Black);
        AddBossPiece(2, side * 6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, Black);
        return;
    }

    // 撃破時は、部位破壊で既に離脱した箇所を除くボディ全体を分解する
    AddBossPiece(2, 0.0f, 2.0f, 0.0f, 18.0f, 18.0f, 16.0f, Gray);
    AddBossPiece(2, 0.0f, 2.0f, -10.0f, 14.0f, 14.0f, 4.0f, Gray);
    AddBossPiece(2, 0.0f, 2.0f, 10.0f, 14.0f, 14.0f, 4.0f, Gray);
    AddBossPiece(2, 0.0f, -12.0f, 0.0f, 4.0f, 4.0f, 10.0f, Gray);
    AddBossPiece(2, 0.0f, -15.0f, 1.0f, 2.0f, 2.0f, 8.0f, Gray);
    AddBossPiece(2, 0.0f, 3.0f, 15.0f, 10.0f, 10.0f, 6.0f, Gray);
    if (enemy.bossPartHp[BossNose] > 0) SpawnEnemyDebris(enemy, BossNose);
    if (enemy.bossPartHp[BossLeftWing] > 0) SpawnEnemyDebris(enemy, BossLeftWing);
    if (enemy.bossPartHp[BossRightWing] > 0) SpawnEnemyDebris(enemy, BossRightWing);
    if (enemy.bossPartHp[BossLeftEngine] > 0) SpawnEnemyDebris(enemy, BossLeftEngine);
    if (enemy.bossPartHp[BossRightEngine] > 0) SpawnEnemyDebris(enemy, BossRightEngine);
}

/**
 * @brief ボス戦前会話を画面へ描画する
 * @param renderer 描画先レンダラー
 */
void SideScrollingShooter::DrawBossStory(Renderer& renderer) const {
    if (!m_bossStoryActive) return;

    const BossStory story = BossStories::ForStage(m_stageNumber);
    if (m_bossStoryLine >= story.lineCount) return;

    // 戦闘画面を少し暗くして会話を前面へ表示する
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.0f, 0.0f, 0.0f, 0.38f});
    const BossStoryLine& line = story.lines[m_bossStoryLine];
    const ColorF nameColor = line.isBoss ? ColorF {1.0f, 0.45f, 0.65f, 1.0f} :
        ColorF {0.35f, 0.90f, 1.0f, 1.0f};
    constexpr float CharacterSpacing = 0.0015f;
    constexpr std::size_t MaxDialogueLineLength = 61;
    const std::string_view text = line.text;
    std::size_t firstLineEnd = text.size();
    if (text.size() > MaxDialogueLineLength) {
        firstLineEnd = text.rfind(' ', MaxDialogueLineLength);
        if (firstLineEnd == std::string_view::npos || firstLineEnd == 0) {
            firstLineEnd = MaxDialogueLineLength;
        }
    }

    // 台詞はボックス幅に合わせて最大二行に折り返す
    renderer.Draw(Rect {{0.0f, -0.48f}, {1.72f, 0.34f}}, {0.03f, 0.08f, 0.14f, 0.92f});
    renderer.DrawText(line.speaker, {-0.78f, -0.37f}, 0.022f, nameColor, CharacterSpacing);
    renderer.DrawText(text.substr(0, firstLineEnd), {-0.78f, -0.51f}, 0.016f,
        ColorF::White(), CharacterSpacing);
    if (firstLineEnd < text.size()) {
        const std::size_t secondLineStart = text[firstLineEnd] == ' ' ? firstLineEnd + 1 : firstLineEnd;
        renderer.DrawText(text.substr(secondLineStart), {-0.78f, -0.56f}, 0.016f,
            ColorF::White(), CharacterSpacing);
    }
    renderer.DrawText("Z: NEXT", {0.61f, -0.60f}, 0.012f,
        {0.65f, 0.75f, 0.82f, 1.0f}, CharacterSpacing);
}

/**
 * @brief 墨の筆跡を模したボス名演出を画面へ描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawBossNameReveal(Renderer& renderer) const {
    if (m_bossIntroductionPhase != BossIntroductionPhase::NameReveal) return;

    const float age = static_cast<float>(m_bossIntroductionTimer);
    const float inkProgress = SmoothStep(Math::Clamp01(age / 34.0f));
    const float fadeOut = SmoothStep(Math::Clamp01(
        static_cast<float>(BossNameRevealFrames - m_bossIntroductionTimer) / 24.0f));
    const float alpha = inkProgress * fadeOut;
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.0f, 0.0f, 0.0f, 0.52f * alpha});

    // 高さと濃さの異なる筆跡を横へ走らせて、乾いた墨のかすれを作る
    constexpr float StrokeY[] = {-0.22f, -0.14f, -0.07f, 0.01f, 0.09f, 0.16f, 0.22f};
    constexpr float StrokeHeight[] = {0.08f, 0.10f, 0.12f, 0.13f, 0.11f, 0.09f, 0.06f};
    for (int i = 0; i < 7; ++i) {
        const float reveal = SmoothStep(Math::Clamp01(
            inkProgress * 1.30f - static_cast<float>(i) * 0.035f));
        const float width = (1.35f + static_cast<float>((i * 7) % 4) * 0.11f) * reveal;
        const float strokeColor[4] = {
            0.01f, 0.008f, 0.006f, alpha * (0.78f + static_cast<float>(i % 2) * 0.16f)
        };
        DrawShape(renderer, 0.0f, StrokeY[i], width, StrokeHeight[i], strokeColor);
    }

    // 筆の始点と終点へ不揃いな飛沫を置き、矩形だけの帯に見えないよう崩す
    constexpr Vector2 SplatterPositions[] = {
        {-0.82f, 0.27f}, {-0.72f, -0.31f}, {-0.60f, 0.34f},
        {0.66f, 0.31f}, {0.78f, -0.27f}, {0.88f, 0.13f}, {0.57f, -0.36f}
    };
    for (int i = 0; i < 7; ++i) {
        const float radius = (0.018f + static_cast<float>((i * 5) % 4) * 0.009f) * inkProgress;
        renderer.Draw(Circle {SplatterPositions[i], radius},
            {0.01f, 0.008f, 0.006f, alpha * 0.90f});
    }

    // 墨が広がった後にボス名を打ち込み、短い朱色の見得線を添える
    const float nameAlpha = SmoothStep(Math::Clamp01((age - 24.0f) / 18.0f)) * fadeOut;
    const BossStory story = BossStories::ForStage(m_stageNumber);
    renderer.DrawText(story.bossName, TextAlign::Center, 0.064f,
        {0.94f, 0.92f, 0.84f, nameAlpha}, {0.0f, -0.015f}, 0.008f);
    const float redLine[4] = {0.72f, 0.04f, 0.025f, nameAlpha};
    DrawShape(renderer, 0.0f, -0.33f, 0.84f * inkProgress, 0.018f, redLine);
}

/**
 * @brief Stage 5の要塞、照明、崩壊演出を3D空間へ描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::RenderStage5(Renderer& renderer, const Camera3D& camera) const {
    const Stage5ModelTransform transform = TayamaTransform();
    TayamaModelState state = TayamaState();
    const bool lightning = m_stage5Phase < Stage5Phase::TayamaCommandCore &&
        ((m_frame % 241) < 3 || ((m_frame + 73) % 389) < 2);
    if (lightning) {
        for (bool& flash : state.hitFlash) flash = true;
    }

    // 同じ46パーツをビル端点から空母端点まで補間して描画する
    TayamaModelView::VisitParts(transform, m_tayamaTransformation, state,
        [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color, TayamaPartGroup) {
            const float brightness = lightning ? 1.55f : 1.0f;
            const float partColor[] = {
                (std::min)(1.0f, color.r * brightness),
                (std::min)(1.0f, color.g * brightness),
                (std::min)(1.0f, color.b * brightness), color.a
            };
            DrawModelPrimitive(renderer, camera, static_cast<int>(shape), world, partColor);
        });

    // 変形終盤から既存のエンジン炎HLSLを主推進機と生存中の揚力機関へ付ける
    if (m_stage5Phase >= Stage5Phase::CarrierTransformation &&
        m_stage5Phase < Stage5Phase::TayamaCollapse) {
        constexpr TayamaPartGroup EngineGroups[] = {
            TayamaPartGroup::MainThruster,
            TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine
        };
        for (int engine = 0; engine < 3; ++engine) {
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                m_tayamaTransformation, state, EngineGroups[engine]);
            if (!bounds.valid) continue;
            const float width = engine == 0 ? 2.2f : 1.25f;
            const Matrix4x4 flameWorld = Matrix4x4::Translation(
                bounds.center + Vector3 {0.0f, -1.0f - static_cast<float>(engine) * 0.12f, 0.0f}) *
                Matrix4x4::Scale({width, 2.8f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * flameWorld,
                static_cast<float>((m_frame + engine * 7) % 24) / 24.0f, 3});
        }
    }

    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        // 破裂前は赤色警告灯と左右へ押し出される格納庫装甲を段階表示する
        const float warningColor[] = {1.0f, 0.04f, 0.03f,
            (m_stage5PhaseTimer / 5) % 2 == 0 ? 1.0f : 0.28f};
        for (int light = -2; light <= 2; ++light) {
            DrawModelPrimitive(renderer, camera, 1,
                static_cast<float>(light) * 1.35f, 1.5f, 61.5f,
                0.34f, 0.18f, 0.16f, warningColor);
        }
        const float deformation = SmoothStep(Math::Clamp01(
            static_cast<float>(m_stage5PhaseTimer - 18) / 40.0f));
        DrawModelPrimitive(renderer, camera, 1, -1.5f - deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor,
            0.0f, deformation * 0.16f);
        DrawModelPrimitive(renderer, camera, 1, 1.5f + deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor, 0.0f, -deformation * 0.16f);
    }

    // 嵐の上層とCOMMAND CORE以降の雲海を少数のCube帯で表現する
    const bool aboveStorm = m_stage5Phase >= Stage5Phase::TayamaCommandCore;
    const int cloudCount = aboveStorm ? 22 : 14;
    for (int i = 0; i < cloudCount; ++i) {
        const float x = -34.0f + static_cast<float>((i * 47) % 680) / 10.0f;
        const float z = 24.0f + static_cast<float>((i * 31 + m_frame / 3) % 760) / 10.0f;
        const float y = aboveStorm ? -5.8f + static_cast<float>(i % 3) * 0.32f :
            12.0f + static_cast<float>(i % 4) * 1.1f;
        const float cloudColor[] = {
            aboveStorm ? 0.32f : StormCloudColor[0],
            aboveStorm ? 0.38f : StormCloudColor[1],
            aboveStorm ? 0.48f : StormCloudColor[2],
            aboveStorm ? 0.82f : 0.72f
        };
        DrawModelPrimitive(renderer, camera, 1, x, y, z,
            8.0f + static_cast<float>(i % 4) * 2.0f, 0.75f, 3.5f, cloudColor);
    }

    // 現フェーズのサーチライト基部と、追尾上限を持つ光軸を同じ座標で描画する
    int activeLights = 0;
    bool tayamaLights = false;
    if (m_stage5Phase == Stage5Phase::WallClimbLower) activeLights = 1;
    if (m_stage5Phase == Stage5Phase::WallClimbMiddle) activeLights = 2;
    if (m_stage5Phase == Stage5Phase::WallClimbUpper) activeLights = 3;
    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        activeLights = 2;
        tayamaLights = true;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceBattle && m_enemies[0].active &&
        m_enemies[0].bossPhase >= BossNormalPhase2 && m_enemies[0].age % 180 < 90) {
        activeLights = 1;
    }
    for (int index = 0; index < activeLights; ++index) {
        const SearchlightState& light = m_searchlights[index];
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
                transform, m_tayamaTransformation, state, group);
            if (bounds.valid) source = bounds.center;
        }
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector3 target {
            ToWorldX(locked ? light.lockedX : light.beamX),
            ToWorldY(locked ? light.lockedY : light.beamY), PlayerRailZ
        };
        const Vector3 delta = target - source;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float yaw = std::atan2(direction.z, -direction.x);
        const float pitch = -std::asin(direction.y);
        const float* beamColor = locked ? SearchlightLockedColor : SearchlightColor;
        const Matrix4x4 beamWorld = Matrix4x4::Translation(source + direction * (length * 0.5f)) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale({length, locked ? 0.12f : 0.18f, locked ? 0.12f : 0.18f});
        DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box), beamWorld, beamColor);
        DrawModelPrimitive(renderer, camera, 2, source.x, source.y, source.z,
            0.72f, 0.42f, 0.72f, locked ? SearchlightLockedColor : SatelliteLightColor);
    }

    // 有効弱点へ小さな発光リングを重ねて攻略対象を明示する
    if (m_stage5Phase >= Stage5Phase::TayamaFireControl &&
        m_stage5Phase <= Stage5Phase::TayamaCommandCore) {
        constexpr TayamaPartGroup Groups[] = {
            TayamaPartGroup::LeftSearchlight, TayamaPartGroup::RightSearchlight,
            TayamaPartGroup::FireControlRadar, TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine, TayamaPartGroup::CommandCore
        };
        for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
            if (!weakpoint.active || weakpoint.destroyed) continue;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                m_tayamaTransformation, state, Groups[static_cast<std::size_t>(weakpoint.type)]);
            if (!bounds.valid) continue;
            const float size = (std::max)(0.75f, (std::min)(2.2f, bounds.radius * 0.45f));
            const Matrix4x4 world = Matrix4x4::Translation(bounds.center + Vector3 {0.0f, 0.0f, -0.12f}) *
                Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                static_cast<float>(m_frame % 30) / 30.0f, 0});
        }
    }

    if (m_stage5Phase == Stage5Phase::TayamaCollapse && m_stage5PhaseTimer >= 330 &&
        m_stage5PhaseTimer < TayamaCollapseFrames) {
        // 最終90フレームは内部白光と二重衝撃波で輪郭ごと消滅させる
        const float finalProgress = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 450) / 90.0f);
        const float glow = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 330) / 120.0f);
        const Matrix4x4 glowWorld = Matrix4x4::Translation(transform.position) *
            Matrix4x4::Scale({3.0f + glow * 8.0f, 2.0f + glow * 5.0f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * glowWorld,
            static_cast<float>(m_frame % 24) / 24.0f, 0});
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
 * @brief Stage 5の雨、稲光、照準表示を画面空間へ描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawStage5Weather(Renderer& renderer) const {
    if (m_stageNumber != 5) return;
    float intensity = 0.0f;
    if (m_stage5Phase == Stage5Phase::Approach) {
        intensity = 0.22f + static_cast<float>(m_chapterNumber - 1) * 0.27f;
    } else if (m_stage5Phase <= Stage5Phase::EastsourceFall) {
        intensity = 1.0f;
    } else if (m_stage5Phase <= Stage5Phase::WallClimbUpper) {
        intensity = 0.88f - m_tayamaTransformation * 0.28f;
    } else if (m_stage5Phase <= Stage5Phase::TayamaFireControl) {
        intensity = 0.42f;
    } else if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
        intensity = 0.22f;
    } else if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        intensity = 0.22f * (1.0f - Math::Clamp01(static_cast<float>(m_stage5PhaseTimer) / 180.0f));
    }

    // 最大96本の決定的な横殴り雨を画面空間へ流す
    const int rainCount = intensity > 0.0f ? static_cast<int>(32.0f + intensity * 64.0f) : 0;
    for (int index = 0; index < rainCount; ++index) {
        const int phase = (index * 73 + m_frame * (3 + index % 3)) % 220;
        const int row = (index * 47 + m_frame * (5 + index % 2)) % 210;
        const float x = -1.10f + static_cast<float>(phase) * 0.010f;
        const float y = 1.05f - static_cast<float>(row) * 0.010f;
        const float rainColor[] = {0.50f, 0.72f, 0.90f, 0.16f + intensity * 0.34f};
        DrawShape(renderer, x, y, 0.004f + intensity * 0.002f,
            0.035f + intensity * 0.055f, rainColor);
    }

    // 稲光はTAYAMAの輪郭と警告灯を一瞬だけ強調する
    if (intensity > 0.30f && ((m_frame % 241) < 3 || ((m_frame + 73) % 389) < 2)) {
        const float alpha = (m_frame % 2 == 0 ? 0.30f : 0.16f) * intensity;
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.72f, 0.82f, 1.0f, alpha});
    }

    // 甲板掃射、排気レーン、コアレーザーは発射前だけ危険範囲を固定表示する
    if (m_stage5Phase == Stage5Phase::TayamaFireControl && m_stage5AttackTimer % 210 < 36) {
        renderer.Draw(Rect {{0.0f, m_stage5CoreTargetY}, {1.86f, 0.055f}},
            {1.0f, 0.12f, 0.08f, 0.36f});
    }
    if (m_stage5Phase == Stage5Phase::TayamaLiftEngines && m_stage5AttackTimer % 132 < 32) {
        renderer.Draw(Rect {{m_stage5CoreTargetX, m_stage5CoreTargetY}, {0.38f, 0.075f}},
            {1.0f, 0.34f, 0.08f, 0.42f});
        renderer.Draw(Circle {{m_stage5CoreTargetX, m_stage5CoreTargetY}, 0.12f},
            {1.0f, 0.58f, 0.12f, 0.58f});
    }
    if (m_stage5Phase == Stage5Phase::TayamaCommandCore && m_stage5AttackTimer % 180 < 42) {
        const Vector2 target {m_stage5CoreTargetX, m_stage5CoreTargetY};
        renderer.Draw(Circle {target, 0.11f}, {1.0f, 0.08f, 0.04f, 0.62f});
    }

    // 検出円と固定ロック地点を表示し、光軸がロック後に追尾しないことを示す
    const bool eastsourceSearchlight = m_stage5Phase == Stage5Phase::EastsourceBattle &&
        m_enemies[0].active && m_enemies[0].bossPhase >= BossNormalPhase2 &&
        m_enemies[0].age % 180 < 90;
    const bool showSearchlights = eastsourceSearchlight ||
        (m_stage5Phase >= Stage5Phase::WallClimbLower &&
            m_stage5Phase <= Stage5Phase::WallClimbUpper) ||
        m_stage5Phase == Stage5Phase::TayamaFireControl;
    if (!showSearchlights) return;
    for (const SearchlightState& light : m_searchlights) {
        if (light.destroyed || light.phase == SearchlightPhase::Cooldown) continue;
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector2 target {locked ? light.lockedX : light.beamX,
            locked ? light.lockedY : light.beamY};
        const bool detecting = light.phase == SearchlightPhase::Detecting;
        const ColorF color = locked ? ColorF {1.0f, 0.08f, 0.08f, 0.86f} :
            (detecting ? ColorF {1.0f, 0.78f, 0.18f, 0.34f} :
                ColorF {0.92f, 0.82f, 0.42f, 0.16f});
        renderer.Draw(Circle {target, locked ? 0.075f : SearchlightDetectionRadius}, color);
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
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawStage5Hud(Renderer& renderer) const {
    if (m_stage5Phase == Stage5Phase::Approach ||
        m_stage5Phase == Stage5Phase::TayamaCollapse ||
        m_stage5Phase == Stage5Phase::EndingReady) return;
    constexpr float Back[] = {0.08f, 0.05f, 0.12f, 0.90f};
    constexpr float Fill[] = {0.96f, 0.14f, 0.24f, 1.0f};
    constexpr float Accent[] = {0.16f, 0.82f, 1.0f, 1.0f};
    constexpr float BarWidth = 0.62f;

    if (m_stage5Phase == Stage5Phase::EastsourceBattle) {
        const float hpRate = Math::Clamp01(m_displayBossHp / static_cast<float>(EastsourceMaxHp));
        DrawShape(renderer, 0.0f, 0.76f, BarWidth, 0.025f, Back);
        DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.76f,
            BarWidth * hpRate, 0.018f, Fill);
        for (int phaseDivider = 1; phaseDivider < BossPhaseCount; ++phaseDivider) {
            const float x = -BarWidth + BarWidth * 2.0f *
                static_cast<float>(phaseDivider) / static_cast<float>(BossPhaseCount);
            DrawShape(renderer, x, 0.755f, 0.008f, 0.035f, Accent);
        }
        renderer.DrawText("EASTSOURCE", TextAlign::Center, 0.017f,
            {1.0f, 0.42f, 0.55f, 1.0f}, {0.0f, 0.86f});
        constexpr const char* Labels[] = {"PRECISION", "CROSSFIRE", "PURSUIT", "LAST CONTRACT"};
        const int phase = m_enemies[0].active ? m_enemies[0].bossPhase : 0;
        renderer.DrawText(Labels[(std::clamp)(phase, 0, 3)], {-BarWidth, 0.81f}, 0.012f,
            {1.0f, 0.82f, 0.30f, 1.0f});
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        renderer.DrawText("HOSTILE SIGNAL APPROACHING", TextAlign::Center, 0.018f,
            {1.0f, 0.34f, 0.32f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceFall) {
        renderer.DrawText("SIGNAL LOST", TextAlign::Center, 0.030f,
            {1.0f, 0.18f, 0.18f, 1.0f}, {0.0f, 0.12f});
        return;
    }
    if (m_stage5Phase >= Stage5Phase::WallClimbTransition &&
        m_stage5Phase <= Stage5Phase::WallClimbUpper) {
        const char* section = m_stage5Phase <= Stage5Phase::WallClimbLower ?
            "WALL CLIMB: LOWER" : (m_stage5Phase == Stage5Phase::WallClimbMiddle ?
                "WALL CLIMB: MIDDLE" : "WALL CLIMB: UPPER");
        renderer.DrawText(section, TextAlign::Center, 0.017f,
            {0.85f, 0.94f, 1.0f, 1.0f}, {0.0f, 0.84f});
        char status[40];
        int remaining = 0;
        for (const SearchlightState& light : m_searchlights) if (!light.destroyed) ++remaining;
        std::snprintf(status, sizeof(status), "SEARCHLIGHTS ACTIVE %d", remaining);
        renderer.DrawText(status, TextAlign::Center, 0.012f,
            {1.0f, 0.74f, 0.20f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (m_stage5Phase == Stage5Phase::RooftopArrival ||
        m_stage5Phase == Stage5Phase::CarrierTransformation) {
        char status[48];
        std::snprintf(status, sizeof(status), "MOBILE FORTRESS TAYAMA  %03d%%",
            static_cast<int>(m_tayamaTransformation * 100.0f));
        renderer.DrawText(status, TextAlign::Center, 0.018f,
            {0.30f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.82f});
        return;
    }

    // TAYAMA戦は現在フェーズの有効弱点HP合計だけを表示する
    int maxHp = 0;
    for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (IsTayamaWeakpointActiveForPhase(weakpoint.type, m_stage5Phase)) {
            maxHp += weakpoint.maxHp;
        }
    }
    const float hpRate = maxHp > 0 ? Math::Clamp01(m_displayBossHp / static_cast<float>(maxHp)) : 0.0f;
    DrawShape(renderer, 0.0f, 0.74f, BarWidth, 0.025f, Back);
    DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.74f,
        BarWidth * hpRate, 0.018f, Accent);
    renderer.DrawText("MOBILE FORTRESS", TextAlign::Center, 0.011f,
        {0.65f, 0.82f, 0.90f, 1.0f}, {0.0f, 0.88f});
    renderer.DrawText("TAYAMA", TextAlign::Center, 0.022f,
        {0.20f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.83f});
    const char* phase = m_stage5Phase == Stage5Phase::TayamaFireControl ? "PHASE: FIRE CONTROL" :
        (m_stage5Phase == Stage5Phase::TayamaLiftEngines ?
            "PHASE: LIFT ENGINES" : "PHASE: COMMAND CORE");
    renderer.DrawText(phase, {-BarWidth, 0.79f}, 0.012f,
        {1.0f, 0.82f, 0.30f, 1.0f});
    char components[96];
    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        std::snprintf(components, sizeof(components), "L-LIGHT[%c]  R-LIGHT[%c]  RADAR[%c]",
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftSearchlight)].destroyed ? 'X' : ' ',
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightSearchlight)].destroyed ? 'X' : ' ',
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed ? 'X' : ' ');
    } else if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
        std::snprintf(components, sizeof(components), "L-ENGINE[%c]  R-ENGINE[%c]",
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed ? 'X' : ' ',
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed ? 'X' : ' ');
    } else {
        std::snprintf(components, sizeof(components), "COMMAND CORE[%c]",
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed ? 'X' : ' ');
    }
    renderer.DrawText(components, TextAlign::Center, 0.010f,
        {0.72f, 0.86f, 0.92f, 1.0f}, {0.0f, 0.68f});
}
