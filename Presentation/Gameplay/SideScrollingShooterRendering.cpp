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
using SideScrollingShooterShared::BossNameRevealFrames;

constexpr float PlayerColor[4] = { 0.80f, 0.80f, 0.85f, 1.0f };
constexpr float PlayerAccent[4] = { 0.10f, 0.90f, 0.90f, 1.0f };
constexpr float EnemyColor[4] = { 0.90f, 0.12f, 0.12f, 1.0f };
constexpr float EnemyAccent[4] = { 1.00f, 0.55f, 0.08f, 1.0f };
constexpr float PlayerShotColor[4] = { 0.15f, 1.00f, 0.25f, 1.0f };
constexpr float EnemyShotColor[4] = { 1.00f, 0.25f, 0.25f, 1.0f };
constexpr float PowerItemColor[4] = { 1.00f, 0.88f, 0.12f, 1.0f };
constexpr float ScoreItemColor[4] = { 0.25f, 0.90f, 1.00f, 1.0f };

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
#include "Stages/Common/StageDefinition.h"
#include "Stages/Common/StageDispatch.h"

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

    StageDispatch::ApplyCameraCorrection(*this, railPosition, railTarget);
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(38.0f + (8.0f * railWeight)));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(StageDispatch::CameraFarClip(*this));
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
        if (StageDispatch::DrawSpecialAttackWarning3D(
            *this, renderer, camera, enemy, size)) continue;
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
    if (enemy.type == 2 &&
        StageDispatch::DrawBossModel(*this, renderer, camera, enemy, yaw)) return;
    if (enemy.type == 2) {
        // 専用モデルを持たないボスは従来の大型戦闘機モデルを維持する
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


void SideScrollingShooter::DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw) const {
    if (StageDispatch::DrawSpecialShot(*this, renderer, camera, shot, yaw)) return;
    if (shot.enemy) {
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
    const Debris& debris, float railWeight) const {
    if (StageDispatch::DrawSpecialDebris(
        *this, renderer, camera, debris, railWeight)) return;
    const float scale = debris.age > debris.shrinkStartAge ?
        1.0f - Math::Clamp01(static_cast<float>(debris.age - debris.shrinkStartAge) /
            static_cast<float>(debris.lifetime - debris.shrinkStartAge)) : 1.0f;
    DrawModelPrimitive(renderer, camera, debris.shape, debris.x, debris.y, debris.z,
        debris.width * scale, debris.height * scale, debris.depth * scale,
        debris.color.data(), debris.yaw);
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
    if (StageDispatch::DrawHud(*this, renderer)) return;
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
    const BossStory story = StageDispatch::Story(m_stageNumber);
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

    if (StageDispatch::SpawnBossDebris(*this, enemy, bossPart)) return;

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

    const BossStory story = StageDispatch::Story(m_stageNumber);
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
    const BossStory story = StageDispatch::Story(m_stageNumber);
    renderer.DrawText(story.bossName, TextAlign::Center, 0.064f,
        {0.94f, 0.92f, 0.84f, nameAlpha}, {0.0f, -0.015f}, 0.008f);
    const float redLine[4] = {0.72f, 0.04f, 0.025f, nameAlpha};
    DrawShape(renderer, 0.0f, -0.33f, 0.84f * inkProgress, 0.018f, redLine);
}
