#include "SideScrollingShooter.h"
#include "../../Application/UseCases/Localization.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string_view>

#include "../../Engine/Graphics/Renderer.h"
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
#include "../../Engine/Graphics/Utf8Text.h"
#endif
#include "../../Engine/Input/Input.h"
#include "Models/AircraftModelView.h"
#include "Models/StageEnemyModelView.h"
#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::SideCameraZ;
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::BossNameRevealFrames;

constexpr float EnemyColor[4] = { 0.90f, 0.12f, 0.12f, 1.0f };
constexpr float EnemyAccent[4] = { 1.00f, 0.55f, 0.08f, 1.0f };
constexpr float PowerItemColor[4] = { 1.00f, 0.88f, 0.12f, 1.0f };
constexpr float ScoreItemColor[4] = { 0.25f, 0.90f, 1.00f, 1.0f };
constexpr float ItemInsetColor[4] = { 0.04f, 0.08f, 0.12f, 1.0f };
constexpr float ItemGlyphColor[4] = { 1.00f, 1.00f, 0.92f, 1.0f };

constexpr int MissionBannerGlyphDelayFrames = 4;
constexpr int MissionBannerGlyphPopFrames = 8;
constexpr int BossWarningFadeFrames = 24;

/**
 * @brief ボス警告帯の表示フレームからフェード率を取得する
 * @param age 登場演出の経過フレーム数
 * @param duration 登場演出の総フレーム数
 * @return 0から1のフェード率
 */
constexpr float BossWarningFade(int age, int duration) {
    if (duration <= 1) return 1.0f;
    const float fadeIn = (std::clamp)(
        static_cast<float>(age) / BossWarningFadeFrames, 0.0f, 1.0f);
    const float fadeOut = (std::clamp)(
        static_cast<float>(duration - 1 - age) / BossWarningFadeFrames, 0.0f, 1.0f);
    return (std::min)(fadeIn, fadeOut);
}

static_assert(BossWarningFade(0, 180) == 0.0f);
static_assert(BossWarningFade(BossWarningFadeFrames, 180) == 1.0f);
static_assert(BossWarningFade(179, 180) == 0.0f);

/**
 * @brief 残HPをHPバー上のX座標へ変換する
 * @param halfWidth HPバーの半幅
 * @param hp フェーズが切り替わる残HP
 * @param maxHp ボスの最大HP
 * @return HPバー上のX座標
 */
constexpr float BossPhaseDividerX(float halfWidth, int hp, int maxHp) {
    return -halfWidth + halfWidth * 2.0f * static_cast<float>(hp) /
        static_cast<float>(maxHp);
}

static_assert(BossPhaseDividerX(0.5f, 75, 100) == 0.25f);

/**
 * @brief 画面揺れの残りフレームから減衰率を取得する
 * @param remainingFrames 残りフレーム数
 * @param durationFrames 総フレーム数
 * @return 0から1の減衰率
 */
constexpr float ScreenShakeDecay(int remainingFrames, int durationFrames) {
    return durationFrames > 0 ?
        static_cast<float>(remainingFrames) / static_cast<float>(durationFrames) : 0.0f;
}

static_assert(ScreenShakeDecay(18, 18) == 1.0f);
static_assert(ScreenShakeDecay(0, 18) == 0.0f);

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
 * @brief 表示切り替えクールダウンの残りフレームから使用可能率を取得する
 * @param remainingFrames クールダウンの残りフレーム数
 * @param durationFrames クールダウンの総フレーム数
 * @return 0から1の使用可能率
 */
constexpr float ViewToggleReadyRate(int remainingFrames, int durationFrames) {
    return durationFrames > 0 ?
        1.0f - static_cast<float>((std::clamp)(remainingFrames, 0, durationFrames)) / durationFrames : 1.0f;
}

static_assert(ViewToggleReadyRate(480, 480) == 0.0f);
static_assert(ViewToggleReadyRate(0, 480) == 1.0f);

/**
 * @brief 表示切り替えメーターのフェード率を取得する
 * @param remainingFrames クールダウンの残りフレーム数
 * @param durationFrames クールダウンの総フレーム数
 * @param fadeFrames フェードに使うフレーム数
 * @return 0から1の不透明度
 */
constexpr float ViewToggleHudOpacity(
    int remainingFrames, int durationFrames, int fadeFrames) {
    if (remainingFrames <= 0 || durationFrames <= 0 || fadeFrames <= 0) return 0.0f;
    const int clampedRemaining = (std::clamp)(remainingFrames, 0, durationFrames);
    const float fadeIn = (std::min)(1.0f,
        static_cast<float>(durationFrames - clampedRemaining) / fadeFrames);
    const float fadeOut = (std::min)(1.0f,
        static_cast<float>(clampedRemaining) / fadeFrames);
    return (std::min)(fadeIn, fadeOut);
}

static_assert(ViewToggleHudOpacity(480, 480, 12) == 0.0f);
static_assert(ViewToggleHudOpacity(468, 480, 12) == 1.0f);
static_assert(ViewToggleHudOpacity(6, 480, 12) == 0.5f);
static_assert(ViewToggleHudOpacity(0, 480, 12) == 0.0f);

/**
 * @brief 表示切り替え可能時の機首を白くするフレームか判定する
 * @param frame 現在フレーム
 * @return 白色で発光する場合true、青色で発光する場合false
 */
constexpr bool ViewToggleNoseIsWhite(int frame) {
    constexpr int HalfPeriodFrames = 18;
    const int phase = frame % (HalfPeriodFrames * 2);
    return phase < HalfPeriodFrames;
}

static_assert(ViewToggleNoseIsWhite(0));
static_assert(!ViewToggleNoseIsWhite(18));
static_assert(ViewToggleNoseIsWhite(36));

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
    const Vector2 shake = ScreenShakeOffset();
    const float cameraY = StageDispatch::SideCameraY(*this);

    // 2DモードはXY移動平面を3Dカメラで正面から見て奥行きを持たせる
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(SideCameraFieldOfView));
    camera.SetNearClip(0.1f);
    // 雲海の遠景は2Dでも描画し、3Dから戻った瞬間の消失を防ぐ
    const bool cloudSea = m_stageNumber == 5 && ShooterStages::Stage5::IsCloudSeaPhase(m_stage5.phase);
    camera.SetFarClip(cloudSea ? StageDispatch::CameraFarClip(*this) : 80.0f);
    camera.SetPosition({shake.x, cameraY + shake.y, SideCameraZ});
    camera.LookAt({shake.x, cameraY + shake.y, SidePlaneZ});
}

void SideScrollingShooter::ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const {
    // 発射時の照準判定にも実際の画面サイズを使う
    const Viewport viewport {0, 0, renderer.Width(), renderer.Height()};
    if (viewport.IsValid()) m_aimViewport = viewport;
    ConfigureRailCamera(camera, viewport);
}

/** @brief 描画と照準に共通のカメラを設定する @param camera 設定先 @param viewport 描画領域 @return なし */
void SideScrollingShooter::ConfigureRailCamera(Camera3D& camera, const Viewport& viewport) const {
    const float railWeight = RailBlend();
    const Vector2 shake = ScreenShakeOffset();
    const float sideCameraY = StageDispatch::SideCameraY(*this);

    const Vector3 playerPosition{ToWorldX(Player().m_playerX), ToWorldY(Player().m_playerY), PlayerRailDepth()};
    // 2Dモードと同じカメラ状態から、3Dレールの追従カメラへ補間する
    Vector3 sidePosition{0.0f, sideCameraY, SideCameraZ};
    Vector3 sideTarget{0.0f, sideCameraY, SidePlaneZ};
    Vector3 railPosition = playerPosition + Vector3 {0.0f, 2.8f, -7.5f};
    Vector3 railTarget = playerPosition + Vector3 {0.0f, 0.5f, 14.0f};

    StageDispatch::ApplyCameraCorrection(*this, railPosition, railTarget);
    if (IsTayamaBattle()) {
        // 2Dは切替角を固定し、3Dは自機の後方かつ上方へカメラを追従させる
        const Vector2 sideOrbit = TayamaOrbitXZ(m_stage5.tayamaSideViewAngle, 0.0f);
        const Vector3 sideRadial {
            std::sin(m_stage5.tayamaSideViewAngle), 0.0f,
            -std::cos(m_stage5.tayamaSideViewAngle)
        };
        const Vector3 tayamaPlayer = PlayerWorldPosition();
        Vector3 orbitRadial {
            tayamaPlayer.x, 0.0f,
            tayamaPlayer.z - ShooterStages::Stage5::TayamaArenaCenterZ
        };
        const float radialLength = (std::max)(0.001f, orbitRadial.Length());
        orbitRadial = orbitRadial / radialLength;
        sidePosition = {
            sideOrbit.x + sideRadial.x * ShooterStages::Stage5::TayamaCameraDistance,
            tayamaPlayer.y + ShooterStages::Stage5::TayamaCameraHeight,
            sideOrbit.y + sideRadial.z * ShooterStages::Stage5::TayamaCameraDistance
        };
        sideTarget = {
            sideOrbit.x - sideRadial.x * ShooterStages::Stage5::TayamaCameraLookAhead,
            tayamaPlayer.y,
            sideOrbit.y - sideRadial.z * ShooterStages::Stage5::TayamaCameraLookAhead
        };
        railPosition = tayamaPlayer +
            orbitRadial * ShooterStages::Stage5::TayamaCameraDistance;
        railPosition.y += ShooterStages::Stage5::TayamaCameraHeight;
        railTarget = tayamaPlayer -
            orbitRadial * ShooterStages::Stage5::TayamaCameraLookAhead;
    }
    camera.SetViewport(viewport);
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(StageDispatch::CameraFieldOfView(
        *this, 38.0f + (8.0f * railWeight))));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(StageDispatch::CameraFarClip(*this));
    const Vector3 shakeOffset{shake.x, shake.y, 0.0f};
    camera.SetPosition(Vector3::Lerp(sidePosition, railPosition, railWeight) + shakeOffset);
    camera.LookAt(Vector3::Lerp(sideTarget, railTarget, railWeight) + shakeOffset);
    // カメラ位置と当たり判定用の投影基準を維持し、2機が入る画角まで広げる
    if (m_playerCount == 2 && !StageDispatch::IsCinematic(*this)) {
        float halfAngleTangent = std::tan(camera.FieldOfView() * 0.5f);
        ForEachPlayer([&] {
            Vector3 position = PlayerWorldPosition();
            if (!IsTayamaBattle()) position.z = Math::Lerp(SidePlaneZ, PlayerRailDepth(), railWeight);
            const Vector3 local = camera.ViewMatrix().TransformPoint(position);
            if (local.z <= 0.1f) return;
            const float required = (std::max)((std::abs(local.x) + 1.0f) / camera.GetViewport().AspectRatio(),
                std::abs(local.y) + 1.0f) / local.z;
            halfAngleTangent = (std::max)(halfAngleTangent, required * 1.25f);
        });
        camera.SetFieldOfView(2.0f * std::atan(halfAngleTangent));
    }

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
    // 旧来の数値指定だけを従来の形状番号からenumへ変換する
    const PrimitiveShape primitiveShape = PrimitiveShapeFromLegacyIndex(shape);
    DrawModelPrimitive(renderer, camera, primitiveShape,
        x, y, z, w, h, d, color, yaw, pitch);
}

void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera,
    PrimitiveShape shape, float x, float y, float z, float w, float h, float d,
    const float color[4], float yaw, float pitch) {
    // 型付き形状を変換せず実3DカメラのViewProjectionへ乗せて描画する
    const Matrix4x4 world = Matrix4x4::Translation({x, y, z}) *
        Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) * Matrix4x4::Scale({w, h, d});
    renderer.Draw({
        shape,
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
    float x, float y, float z, bool visible, float yaw, float pitch, float roll) const {
    if (!visible) return;

    const bool canToggleView = m_activePlayer == 0 && CanToggleView();
    const bool whiteGlow = ViewToggleNoseIsWhite(m_frame);
    constexpr float BlueNose[4] = {0.05f, 0.45f, 2.80f, 1.0f};
    constexpr float WhiteNose[4] = {2.40f, 2.40f, 3.00f, 1.0f};
    constexpr float BlueGlow[4] = {0.05f, 0.35f, 1.00f, 0.24f};
    constexpr float WhiteGlow[4] = {1.00f, 1.00f, 1.00f, 0.34f};
    const float* noseColor = whiteGlow ? WhiteNose : BlueNose;
    const float* glowColor = whiteGlow ? WhiteGlow : BlueGlow;

    // ゲーム本編とギャラリーで同じ機体定義を共有する
    auto drawPart = [&](int shape, const Vector3& partPosition, const Vector3& partScale,
        const float color[4], float partYaw, float partPitch) {
        const bool isNose = shape == 3;
        const float coopColor[4] = {color[0] * 0.35f,
            color[1] * (m_activePlayer == 0 ? 0.65f : 1.6f),
            color[2] * (m_activePlayer == 0 ? 1.7f : 0.40f), color[3]};
        const float* partColor = canToggleView && isNose ? noseColor : (m_playerCount == 2 ? coopColor : color);
        const bool transformed = pitch != 0.0f || roll != 0.0f;
        Vector3 transformedPosition = partPosition;
        if (!transformed) {
            DrawModelPrimitive(renderer, camera, shape,
                partPosition.x, partPosition.y, partPosition.z,
                partScale.x, partScale.y, partScale.z,
                partColor, partYaw, partPitch);
        } else {
            // 機体中心を基準に全部位の位置と姿勢を同じPitchとRollへ乗せる
            const Vector3 center {x, y, z};
            const Vector3 offset = Matrix4x4::RotationZ(roll).TransformVector(
                Matrix4x4::RotationX(pitch).TransformVector(partPosition - center));
            transformedPosition = center + offset;
            const Matrix4x4 world = Matrix4x4::Translation(transformedPosition) *
                Matrix4x4::RotationZ(roll) * Matrix4x4::RotationY(partYaw) *
                Matrix4x4::RotationX(pitch) *
                Matrix4x4::RotationZ(partPitch) * Matrix4x4::Scale(partScale);
            DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShapeFromLegacyIndex(shape)), world, partColor);
        }
        if (!canToggleView || !isNose) return;

        // 青と白の高輝度な機首へ薄い拡大形状を重ね、ブルーム風の点滅にする
        const float glowScale = whiteGlow ? 1.18f : 1.10f;
        if (!transformed) {
            DrawModelPrimitive(renderer, camera, shape,
                partPosition.x, partPosition.y, partPosition.z,
                partScale.x * glowScale, partScale.y * glowScale, partScale.z * glowScale,
                glowColor, partYaw, partPitch);
        } else {
            const Matrix4x4 glowWorld = Matrix4x4::Translation(transformedPosition) *
                Matrix4x4::RotationZ(roll) * Matrix4x4::RotationY(partYaw) *
                Matrix4x4::RotationX(pitch) * Matrix4x4::RotationZ(partPitch) *
                Matrix4x4::Scale(partScale * glowScale);
            DrawModelPrimitive(renderer, camera,
                static_cast<int>(PrimitiveShapeFromLegacyIndex(shape)), glowWorld, glowColor);
        }
    };
    AircraftModelView::DrawPlayer({x, y, z}, yaw, 1.0f, drawPart);
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
    if (StageDispatch::DrawBossModel(*this, renderer, camera, enemy, yaw)) return;
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

    // 第2部は敵種ごとに過去4ステージの通常敵モデルを再登場させる
    const float scale = enemy.behavior != nullptr ? enemy.behavior->RenderScale() : 1.0f;
    const bool stage5Part2 = m_stageNumber == 5 &&
        m_stage5.phase >= Stage5Phase::WallClimbLower &&
        m_stage5.phase <= Stage5Phase::WallClimbUpper;
    const float railWeight = RailBlend();
    const bool groundwardEnemy = stage5Part2 && enemy.entersFromTop;
    const float groundwardRoll = groundwardEnemy ?
        Math::Lerp(Math::HalfPi, 0.0f, railWeight) : 0.0f;
    const float groundwardPitch = groundwardEnemy ?
        Math::Lerp(0.0f, Math::HalfPi, railWeight) : 0.0f;
    const int enemyModelStage = stage5Part2 ? 1 + enemy.type % 4 : m_stageNumber;
    if (enemyModelStage >= 1 && enemyModelStage <= 4) {
        const Matrix4x4 viewProjection = camera.ProjectionMatrix() * camera.ViewMatrix();
        const float modelYaw = yaw + Math::Pi;
        const Matrix4x4 groundwardRotation = Matrix4x4::Translation({x, y, z}) *
            Matrix4x4::RotationZ(groundwardRoll) *
            Matrix4x4::RotationY(modelYaw) * Matrix4x4::RotationX(groundwardPitch) *
            Matrix4x4::RotationY(-modelYaw) *
            Matrix4x4::Translation({-x, -y, -z});
        auto drawPart = [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
            renderer.Draw({shape, viewProjection * groundwardRotation * world,
                Vector3::One, color});
        };
        StageEnemyModelView::Draw(enemyModelStage, {x, y, z}, modelYaw,
            scale * (stage5Part2 ?
                ShooterStages::Stage5::Part2EnemyScaleMultiplier(railWeight) : 1.0f), drawPart);
        return;
    }

    // 専用モデルがない追加ステージは現行の共通機体を維持する
    auto drawPart = [&](int shape, const Vector3& partPosition, const Vector3& partScale,
        const float color[4], float partYaw, float pitch) {
        DrawModelPrimitive(renderer, camera, shape,
            partPosition.x, partPosition.y, partPosition.z,
            partScale.x, partScale.y, partScale.z, color, partYaw, pitch);
    };
    AircraftModelView::DrawEnemy({x, y, z}, yaw + Math::Pi, scale, drawPart);
}

void SideScrollingShooter::DrawLinkedEnemyLasers(
    Renderer& renderer, const Camera3D& camera, float railWeight) const {
    for (const auto& upper : m_enemies) {
        if (!upper.active || upper.type != Stage::LinkedLaserEnemy || upper.laserLinkRole <= 0) continue;
        for (const auto& lower : m_enemies) {
            if (!lower.active || lower.type != Stage::LinkedLaserEnemy ||
                lower.laserLinkId != upper.laserLinkId || lower.laserLinkRole >= 0) continue;
            auto LaserPoint = [&](const Enemy& enemy) {
                if (railWeight <= 0.0f) {
                    return Vector3 {
                        ToWorldX(enemy.x), ToWorldY(enemy.y), SidePlaneZ + 1.48f};
                }
                const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
                const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
                const float sideX = enteringRail ? enemy.transitionSideX :
                    (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
                const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
                const float drawX = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
                float drawY = Math::Lerp(sideY, exitingRail ? enemy.transitionSideY : enemy.y, railWeight);
                const float railZ = exitingRail ? enemy.transitionRailZ : enemy.z;
                const float drawZ = Math::Lerp(SidePlaneZ + 1.48f, railZ, railWeight);
                const float groundTopY = StageDispatch::RailGroundY(*this);
                const float minimumRailY = FromWorldY(groundTopY + 0.32f);
                drawY = Math::Lerp(drawY, (std::max)(drawY, minimumRailY), railWeight);
                return Vector3 {ToWorldX(drawX), ToWorldY(drawY), drawZ};
            };
            const Vector3 start = LaserPoint(upper);
            const Vector3 end = LaserPoint(lower);
            const float pulse = static_cast<float>((m_frame + upper.laserLinkId) % 30) / 30.0f;
            DrawRailgunBeamBetween(renderer, camera, start, end, 0.42f, pulse, 2);
            DrawRailgunBeamBetween(renderer, camera, start, end, 0.20f, pulse, 0);
            break;
        }
    }
}

void SideScrollingShooter::DrawRailgunBeamBetween(Renderer& renderer, const Camera3D& camera,
    const Vector3& start, const Vector3& end, float width, float progress, int effectType) {
    const Vector3 delta = end - start;
    const float length = (std::max)(0.001f, delta.Length());
    const Vector3 direction = delta / length;
    const Vector3 center = start + direction * (length * 0.5f);
    const float yaw = std::atan2(direction.z, -direction.x);
    const float pitch = -std::asin(direction.y);
    const Matrix4x4 world = Matrix4x4::Translation(center) *
        Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
        Matrix4x4::Scale({length * 0.5f, width, 1.0f});
    renderer.DrawRailgun({
        camera.ProjectionMatrix() * camera.ViewMatrix() * world,
        progress, effectType});
}

/**
 * @brief 現在フレームの画面揺れオフセットを取得する
 * @return カメラへ加算するXYオフセット
 */
Vector2 SideScrollingShooter::ScreenShakeOffset() const {
    if (m_screenShakeFrames <= 0 || m_screenShakeDurationFrames <= 0) return Vector2::Zero;

    // 固定パターンへ残り時間の比率を掛け、終端へ向けて自然に減衰させる
    constexpr Vector2 Pattern[] = {
        {1.0f, -0.5f}, {-0.7f, 1.0f}, {0.4f, -0.8f}, {-1.0f, 0.3f},
        {0.8f, 0.6f}, {-0.3f, -1.0f}, {0.6f, 0.2f}, {-0.8f, -0.4f}
    };
    const int elapsedFrames = m_screenShakeDurationFrames - m_screenShakeFrames;
    const float decay = ScreenShakeDecay(m_screenShakeFrames, m_screenShakeDurationFrames);
    constexpr int PatternCount = sizeof(Pattern) / sizeof(Pattern[0]);
    return Pattern[elapsedFrames % PatternCount] * (m_screenShakeIntensity * decay);
}


/** @brief 弾頭と実軌道に沿う追尾弾の残光を描画する @param renderer 描画先 @param camera 投影カメラ @param shot 描画対象 @param yaw 弾モデルの回転角 @return なし */
void SideScrollingShooter::DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw) const {
    if (StageDispatch::DrawSpecialShot(*this, renderer, camera, shot, yaw)) return;
    {
        // 全ショットは画面座標と進行方向を埋め込みHLSLへ渡して描画する
        const Vector3 worldPosition {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};
        Vector2 screenPosition;
        float depth = 0.0f;
        if (!camera.TryWorldToScreen(worldPosition, screenPosition, &depth)) return;

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
        Vector2 size = shot.enemy || !shot.special ?
            Vector2 {0.055f, 0.028f} : Vector2 {0.040f, 0.020f};
        if (!shot.enemy && RailBlend() > 0.0f) {
            // 3Dでは当たり判定球を画面へ投影し、遠方でも見た目と判定範囲を一致させる
            const float worldRadius = shot.hitRadius * WorldXScale;
            Vector2 radiusXScreen;
            Vector2 radiusYScreen;
            if (camera.TryWorldToScreen(worldPosition + Vector3 {worldRadius, 0.0f, 0.0f}, radiusXScreen) &&
                camera.TryWorldToScreen(worldPosition + Vector3 {0.0f, worldRadius, 0.0f}, radiusYScreen)) {
                Vector2 railSize {
                    std::abs(radiusXScreen.x - screenPosition.x) / static_cast<float>(viewport.width) * 2.0f,
                    std::abs(radiusYScreen.y - screenPosition.y) / static_cast<float>(viewport.height)};
                // Stage5第2部だけ2D表示の75%を下限にして、遠方の自機弾も見える大きさを保つ
                if (m_stageNumber == 5 && ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase)) {
                    railSize.x = (std::max)(railSize.x, size.x * 0.75f);
                    railSize.y = (std::max)(railSize.y, size.y * 0.75f);
                }
                size = Vector2::Lerp(size, railSize, RailBlend());
            }
        }
        // 散弾は威力3→2→1の距離を連続補間し、最大減衰後も元の1/3の寸法を保つ
        if (!shot.enemy && shot.special && shot.playerType == Spread) {
            size = size * Math::Lerp(1.0f, 1.0f / 3.0f, std::clamp(shot.travelDistance / 7.0f, 0.0f, 1.0f));
        }
        Vector2 drawPosition = position;
        int type = shot.enemy ? 4 : shot.special ? static_cast<int>(shot.playerType) : 6;

        // 直近8フレームだけを薄く残し、敵弾を隠さない短い残光にする
        if (!shot.enemy && shot.special && shot.playerType == Homing && m_viewTransitionTimer == 0) {
            Vector2 end = position;
            constexpr int TrailFrames = 8;
            const int count = (std::min)(shot.homingTrailCount, TrailFrames);
            for (int back = 1; back < count; back += 2) {
                Vector2 screen;
                Vector3 point = shot.homingTrail[(shot.age - 1 - back) % shot.homingTrail.size()];
                // 弾頭と同じ描画平面へ投影し、座標補間中は残光を出さない
                point.z = Math::Lerp(SidePlaneZ - 0.4f, point.z, RailBlend());
                if (!camera.TryWorldToScreen(point, screen)) break;
                const Vector2 start {
                    (screen.x - viewport.x) / viewport.width * 2.0f - 1.0f,
                    1.0f - (screen.y - viewport.y) / viewport.height * 2.0f};
                const Vector2 segment = end - start;
                const float fade = 1.0f - static_cast<float>(back) / TrailFrames;
                renderer.DrawPlayerShot({(start + end) * 0.5f,
                    {segment.Length() * 0.65f, size.y * 0.48f * fade},
                    std::atan2(segment.y, segment.x), static_cast<float>(m_frame), 0, depth, false, 0.22f * fade * fade});
                end = start;
            }
        }

        // 既存のグレイズ範囲へ入った通常敵弾を反転させ、弾ごとに位相をずらして小刻みに震わせる
        if (shot.enemy) {
            const Vector3 player = PlayerWorldPosition();
            const float dx = IsRailGameplayActive() ? ToWorldX(shot.x) - player.x : shot.x - Player().m_playerX;
            const float dy = IsRailGameplayActive() ? ToWorldY(shot.y) - player.y : shot.y - Player().m_playerY;
            const float dz = IsRailGameplayActive() ? shot.z - player.z : 0.0f;
            const float warningRadius = IsRailGameplayActive() ? 1.46f : 0.222f;
            if (dx * dx + dy * dy + dz * dz <= warningRadius * warningRadius) {
                const float phase = static_cast<float>(m_frame) * 2.73f +
                    static_cast<float>(shot.barrageIndex) * 1.91f;
                drawPosition.x += std::sin(phase) * 0.0045f;
                drawPosition.y += std::cos(phase * 1.37f) * 0.0045f;
                type = 5;
            }
        }
        renderer.DrawPlayerShot({drawPosition, size, std::atan2(direction.y, direction.x),
            static_cast<float>(m_frame), type, depth, shot.enemy});
        return;
    }
}

/**
 * @brief 機体別のミサイル、極太レーザー、シールドを描画する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param bomb 描画対象のボム
 * @return なし
 */
void SideScrollingShooter::DrawBomb(
    Renderer& renderer, const Camera3D& camera, const Bomb& bomb) const {
    // 描画中の自機と同じ位置・姿勢から先端を求め、視点遷移中も機首へ接続する
    const float blend = RailBlend();
    Vector3 center = PlayerWorldPosition();
    if (!IsTayamaBattle()) center.z = Math::Lerp(SidePlaneZ, PlayerRailDepth(), blend);
    const float yaw = IsTayamaBattle() ? std::atan2(-center.x,
        ShooterStages::Stage5::TayamaArenaCenterZ - center.z) : Math::Lerp(Math::HalfPi, 0.0f, blend);
    const float roll = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase) ?
        Math::Lerp(Math::HalfPi, 0.0f, blend) : 0.0f;
    float pitch = 0.0f;
    StageDispatch::ApplyPlayerRenderCorrection(*this, center, pitch);
    const Vector3 forward = (Matrix4x4::RotationZ(roll) * Matrix4x4::RotationX(pitch) *
        Matrix4x4::RotationY(yaw)).TransformVector(Vector3::Forward);
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(bomb.age) * 0.35f);
    if (bomb.type == Piercing) {
        // 円柱状の白熱芯で正面視でも太さを保ち、金色の発光を重ねる
        center += forward * AircraftModelView::NoseTipZ;
        const Vector3 end = center + forward * BombLaserLength;
        const float core[4] = {1.8f, 1.4f, 0.65f, 0.85f};
        const Matrix4x4 beam = Matrix4x4::Translation((center + end) * 0.5f) *
            Matrix4x4::RotationY(std::atan2(-forward.z, forward.x)) *
            Matrix4x4::RotationZ(std::asin(std::clamp(forward.y, -1.0f, 1.0f)) - Math::HalfPi) *
            Matrix4x4::Scale({BombLaserRadius * 1.7f, BombLaserLength, BombLaserRadius * 1.7f});
        DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Cylinder), beam, core);
        DrawRailgunBeamBetween(renderer, camera, center, end, BombLaserRadius * 1.4f, pulse, 4);
        DrawRailgunBeamBetween(renderer, camera, center, end, BombLaserRadius, pulse, 5);
        return;
    }
    if (bomb.type == Spread) {
        // 正二十面体の各面を切頂して六角形を作り、球面へ投影する
        // 球を閉じるための十二か所の五角形状の隙間は透明のまま残す
        constexpr float phi = 1.61803399f;
        constexpr Vector3 vertices[] = {{-1, phi, 0}, {1, phi, 0}, {-1, -phi, 0}, {1, -phi, 0},
            {0, -1, phi}, {0, 1, phi}, {0, -1, -phi}, {0, 1, -phi},
            {phi, 0, -1}, {phi, 0, 1}, {-phi, 0, -1}, {-phi, 0, 1}};
        constexpr unsigned char faces[][3] = {{0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
            {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
            {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
            {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1}};
        const Matrix4x4 rotation = Matrix4x4::RotationY(bomb.age * Math::TwoPi / 360.0f);
        const float rim[4] = {0.25f, 1.45f + 0.15f * std::sin(bomb.age * Math::TwoPi / 180.0f),
            0.85f, 0.70f};
        for (int faceIndex = 0; faceIndex < 20; ++faceIndex) {
            // 被弾ごとに球面全体へ散らした七つのセルを欠損させる
            if ((faceIndex * 7) % 20 < bomb.shieldHits * 7) continue;
            const auto& face = faces[faceIndex];
            Vector3 hex[6];
            const Vector3 inset = (vertices[face[0]] + vertices[face[1]] + vertices[face[2]]) * 0.06f;
            // 枠を少し縮めて隣のセルと離し、透明な内部から自機を見せる
            for (int i = 0; i < 6; ++i) {
                const Vector3 a = vertices[face[i / 2]], b = vertices[face[(i / 2 + 1) % 3]];
                const Vector3 point = a * static_cast<float>(2 - i % 2) + b * static_cast<float>(1 + i % 2);
                hex[i] = rotation.TransformVector((point * 0.94f + inset).Normalized());
            }
            for (int edge = 0; edge < 6; ++edge) {
                // 一辺を二分して球面に沿わせ、細い円柱で切れ目なく結ぶ
                const Vector3 arc[] = {hex[edge], (hex[edge] + hex[(edge + 1) % 6]).Normalized(),
                    hex[(edge + 1) % 6]};
                for (int part = 0; part < 2; ++part) {
                    const Vector3 start = center + arc[part] * BombShieldRadius;
                    const Vector3 end = center + arc[part + 1] * BombShieldRadius;
                    const Vector3 direction = (end - start).Normalized();
                    const Vector3 midpoint = (start + end) * 0.5f;
                    DrawModelPrimitive(renderer, camera, PrimitiveShape::Cylinder,
                        midpoint.x, midpoint.y, midpoint.z, 0.025f, (end - start).Length(), 0.025f, rim,
                        std::atan2(-direction.z, direction.x),
                        std::asin(std::clamp(direction.y, -1.0f, 1.0f)) - Math::HalfPi);
                }
            }
        }
        return;
    }

    // 十発をそれぞれの速度方向へ向け、点火後だけ長い噴射炎を描く
    const float body[4] = {0.60f, 0.72f, 0.82f, 1.0f};
    const float nose[4] = {1.0f, 0.30f, 0.08f, 1.0f};
    const float fins[4] = {0.10f, 0.22f, 0.35f, 1.0f};
    for (const auto& missile : bomb.missiles) {
        if (!missile.active) continue;
        center = {ToWorldX(missile.x), ToWorldY(missile.y), missile.z};
        if (!IsTayamaBattle()) center.z = Math::Lerp(SidePlaneZ - 0.5f, center.z, blend);
        const Vector3 direction = Vector3 {ToWorldX(missile.vx), ToWorldY(missile.vy), missile.vz}.Normalized();
        const float missileYaw = std::atan2(-direction.z, direction.x);
        const float missilePitch = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
        const Matrix4x4 transform = Matrix4x4::Translation(center) *
            Matrix4x4::RotationY(missileYaw) * Matrix4x4::RotationZ(missilePitch) *
            Matrix4x4::Scale(Vector3::One * BombMissileScale);
        // 本体・弾頭・二枚の尾翼を同じ縮尺で組み立てる
        constexpr Vector3 dimensions[] = {{2.6f, 1.1f, 1.1f}, {1.0f, 0.85f, 0.85f},
            {0.65f, 1.7f, 0.15f}, {0.65f, 0.15f, 1.7f}};
        for (int part = 0; part < 4; ++part) {
            const float offset = part == 0 ? 0.0f : part == 1 ? 0.85f : -0.8f;
            DrawModelPrimitive(renderer, camera, static_cast<int>(part < 2 ? PrimitiveShape::Sphere : PrimitiveShape::Box),
                transform * Matrix4x4::Translation({offset, 0.0f, 0.0f}) * Matrix4x4::Scale(dimensions[part]),
                part == 0 ? body : part == 1 ? nose : fins);
        }
        if (bomb.age > BombMissileLaunchFrames)
            DrawRailgunBeamBetween(renderer, camera, center - direction * ((4.0f + pulse) * BombMissileScale),
                center - direction * BombMissileScale, 0.60f * BombMissileScale, 0.25f, 0);
    }
}

/**
 * @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param explosion 描画対象の爆発
 * @return なし
 */
void SideScrollingShooter::DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion) {
    if (explosion.effectType == 1) {
        const float progress = static_cast<float>(explosion.age) / MortarExplosionLifetimeFrames;
        const float fireProgress = Math::Clamp01(progress * 2.20f);
        const float shockProgress = Math::Clamp01(progress * 1.35f);
        const float scale = explosion.hitRadius > 0.0f ? explosion.hitRadius / 0.55f : 1.0f;
        const Vector3 center {ToWorldX(explosion.x), ToWorldY(explosion.y), explosion.z};

        // 迫撃砲の着弾点から大火球、横方向の衝撃波、遅れて残る黒煙を重ねる
        const float fireSize = (1.45f + fireProgress * 2.85f) * scale;
        const Matrix4x4 fireWorld = Matrix4x4::Translation(center) *
            Matrix4x4::Scale({fireSize, fireSize * 1.10f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * fireWorld, fireProgress});

        constexpr Vector3 FireOffsets[] = {
            {-0.42f, 0.20f, 0.0f}, {0.38f, 0.34f, 0.0f}, {0.05f, 0.58f, 0.0f}
        };
        for (int i = 0; i < 3; ++i) {
            const float lobeProgress = Math::Clamp01(fireProgress * 1.12f -
                static_cast<float>(i) * 0.08f);
            const Matrix4x4 lobeWorld = Matrix4x4::Translation(center + FireOffsets[i] * fireSize) *
                Matrix4x4::Scale({fireSize * 0.62f, fireSize * 0.68f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * lobeWorld, lobeProgress});
        }

        const float shockSize = (1.10f + shockProgress * 4.40f) * scale;
        const Matrix4x4 shockWorld = Matrix4x4::Translation(center + Vector3 {0.0f, -0.10f, 0.0f}) *
            Matrix4x4::Scale({shockSize * 1.70f, shockSize * 0.42f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * shockWorld, shockProgress, 4});

        const float smokeSize = (1.55f + progress * 3.10f) * scale;
        const Matrix4x4 smokeWorld = Matrix4x4::Translation(center + Vector3 {0.0f, progress * 0.80f, 0.0f}) *
            Matrix4x4::Scale({smokeSize, smokeSize * 1.45f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld, progress, 2});
        return;
    }

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
 * @return なし
 */
void SideScrollingShooter::DrawItemModel(Renderer& renderer, const Camera3D& camera,
    const Item& item, float yaw) {
    const Matrix4x4 itemWorld = Matrix4x4::Translation(
        {ToWorldX(item.x), ToWorldY(item.y), item.z}) * Matrix4x4::RotationY(yaw);
    const auto drawBox = [&](const Vector3& position, const Vector3& scale, const float color[4]) {
        DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box),
            itemWorld * Matrix4x4::Translation(position) * Matrix4x4::Scale(scale), color);
    };

    if (item.type == ItemType::Power) {
        // 角形ケースの前面へ暗い銘板とPを重ねる
        drawBox({}, {0.38f, 0.38f, 0.22f}, PowerItemColor);
        drawBox({0.0f, 0.0f, -0.121f}, {0.29f, 0.29f, 0.025f}, ItemInsetColor);
        drawBox({-0.070f, 0.0f, -0.139f}, {0.040f, 0.220f, 0.020f}, ItemGlyphColor);
        drawBox({0.010f, 0.090f, -0.139f}, {0.120f, 0.040f, 0.020f}, ItemGlyphColor);
        drawBox({0.010f, 0.000f, -0.139f}, {0.120f, 0.040f, 0.020f}, ItemGlyphColor);
        drawBox({0.070f, 0.045f, -0.139f}, {0.040f, 0.130f, 0.020f}, ItemGlyphColor);
        return;
    }

    // 小型球体の手前へ5本のバーでSを表示する
    DrawModelPrimitive(renderer, camera, 5, ToWorldX(item.x), ToWorldY(item.y), item.z,
        0.23f, 0.23f, 0.23f, ScoreItemColor, yaw);
    drawBox({0.0f, 0.080f, -0.122f}, {0.150f, 0.035f, 0.018f}, ItemGlyphColor);
    drawBox({-0.058f, 0.040f, -0.122f}, {0.035f, 0.080f, 0.018f}, ItemGlyphColor);
    drawBox({0.0f, 0.000f, -0.122f}, {0.150f, 0.035f, 0.018f}, ItemGlyphColor);
    drawBox({0.058f, -0.040f, -0.122f}, {0.035f, 0.080f, 0.018f}, ItemGlyphColor);
    drawBox({0.0f, -0.080f, -0.122f}, {0.150f, 0.035f, 0.018f}, ItemGlyphColor);
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
    char line[Localization::BufferSize(64)];

    renderer.DrawText("CHAPTER RESULT", TextAlign::Center, 0.028f, { 1.0f, 0.88f, 0.25f, alpha }, { 0.0f, 0.40f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), Localization::Text("GRAZE        %d"), graze);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, 0.20f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), Localization::Text("ANNIHILATION  %d / %d  %d%%"), defeat, m_chapterResult.enemySpawnCount, displayedRate);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, 0.08f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), Localization::Text("RETRY        %d"), retry);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, -0.04f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), Localization::Text("CHAPTER SCORE  %06d"), score);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, -0.16f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), Localization::Text("TOTAL SCORE    %06d"), total);
    renderer.DrawText(line, TextAlign::Center, 0.020f, { 1.0f, 0.88f, 0.25f, alpha }, { 0.0f, -0.31f }, CharacterSpacing);
    if (m_chapterResult.bombAwarded && (m_chapterResultTimer / 8) % 2 != 0) {
        renderer.DrawText("BOMB GET", TextAlign::Center, 0.022f,
            {0.35f, 0.85f, 1.0f, alpha}, {0.0f, -0.47f}, CharacterSpacing);
    }
}

/**
 * @brief リスタート中のカウントダウンを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawRestart(Renderer& renderer) const {
    if (m_restartTimer <= 0) return;

    const int countdown = (m_restartTimer + 59) / 60;
    char text[Localization::BufferSize(16)];
    std::snprintf(text, sizeof(text), Localization::Text("RESTART %d"), countdown);
    renderer.DrawText(text, TextAlign::Center, 0.038f, { 1.0f, 0.88f, 0.25f, 1.0f }, { 0.0f, 0.12f });
}

/**
 * @brief 武装強化時の点滅メッセージを自機上へ描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param playerZ 描画中の自機のワールド座標Z
 * @return なし
 */
void SideScrollingShooter::DrawPowerUp(
    Renderer& renderer, const Camera3D& camera, float playerZ) const {
    if (Player().m_powerUpTimer <= 0 || (Player().m_powerUpTimer / 8) % 2 == 0) return;

    // 自機上方のワールド座標を画面座標へ投影する
    Vector2 screenPosition;
    Vector3 player = PlayerWorldPosition();
    if (!IsTayamaBattle()) player.z = playerZ;
    if (!camera.TryWorldToScreen(
            {player.x, player.y + 0.85f, player.z}, screenPosition)) return;
    const Viewport& viewport = camera.GetViewport();
    const Vector2 position {
        (screenPosition.x - static_cast<float>(viewport.x)) /
                static_cast<float>(viewport.width) * 2.0f - 1.0f,
        1.0f - (screenPosition.y - static_cast<float>(viewport.y)) /
                static_cast<float>(viewport.height) * 2.0f};
    renderer.DrawText("POWER UP", TextAlign::Center, 0.022f,
        {1.0f, 0.88f, 0.12f, 1.0f}, position);
}

/**
 * @brief ミッション開始または終了の文字アニメーションを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawMissionBanner(Renderer& renderer) const {
    if ((!m_clear && m_missionStartTimer <= 0) || (m_clear && m_clearTimer <= 0)) return;

    char startText[Localization::BufferSize(24)];
    std::snprintf(startText, sizeof(startText), Localization::Text("MISSION %d START"), m_stageNumber);
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
    const std::string text = Localization::Translate(m_tutorialMode ?
        (m_clear ? "TUTORIAL COMPLETE" : "TUTORIAL START") :
        (m_clear ? "ARRESTED" : startText));
#else
    const std::string_view text = m_tutorialMode ?
        (m_clear ? "TUTORIAL COMPLETE" : "TUTORIAL START") :
        (m_clear ? "ARRESTED" : startText);
#endif
    const int remainingFrames = m_clear ? (std::max)(0, m_clearTimer) : m_missionStartTimer;
    const int elapsedFrames = (m_clear ? ClearWaitFrames : MissionBannerDisplayFrames) - remainingFrames;
    const float fade = remainingFrames < 20 ? static_cast<float>(remainingFrames) / 20.0f : 1.0f;
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
    // 文字単位でアニメーションを進め、UTF-8の途中で字形を分割しない
    float baseSize = 0.050f;
    float spacing = 0.003f;
    auto metrics = Utf8Text::Measure(text, baseSize, spacing, renderer.AspectRatio());
    if (metrics.width > 1.8f) {
        const float fit = 1.8f / metrics.width;
        baseSize *= fit;
        spacing *= fit;
        metrics = Utf8Text::Measure(text, baseSize, spacing, renderer.AspectRatio());
    }
    float x = -metrics.width * 0.5f + metrics.firstGlyphOffset;
    std::size_t offset = 0;
    int glyphIndex = 0;
    while (offset < text.size()) {
        const std::size_t start = offset;
        const auto codepoint = Utf8Text::Next(text, offset);
        const float scale = MissionBannerGlyphScale(elapsedFrames - glyphIndex++ * MissionBannerGlyphDelayFrames);
        if (scale > 0.0f && codepoint != ' ') {
            const std::string_view glyph(text.data() + start, offset - start);
            const Vector2 position {x, 0.10f};
            renderer.DrawText(glyph, position + Vector2 {0.012f, -0.014f}, baseSize * scale,
                {0.05f, 0.02f, 0.01f, fade * 0.85f});
            renderer.DrawText(glyph, position, baseSize * scale, {1.0f, 0.78f, 0.12f, fade});
        }
        x += Utf8Text::Advance(codepoint, baseSize, spacing);
    }
#else
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
#endif
}

/**
 * @brief 実際のHPフェーズ境界へボスHPバーの区切りを描画する
 * @param renderer 描画先レンダラー
 * @param y HPバーの中心Y座標
 * @param halfWidth HPバーの半幅
 * @param maxHp ボスの最大HP
 * @param color 区切り線のRGBA色
 * @return なし
 */
void SideScrollingShooter::DrawBossPhaseDividers(Renderer& renderer, float y,
    float halfWidth, int maxHp, const float color[4]) const {
    if (maxHp <= 1) return;

    // ponytail: HPが数千程度の間は定義済みフェーズ判定の線形走査で十分、上限増加時はStageへ境界列を持たせる
    int previousPhase = m_stage->BossPhaseForHp(maxHp, maxHp);
    for (int hp = maxHp - 1; hp > 0; --hp) {
        const int phase = m_stage->BossPhaseForHp(hp, maxHp);
        if (phase == previousPhase) continue;
        DrawShape(renderer, BossPhaseDividerX(halfWidth, hp, maxHp),
            y, 0.008f, 0.035f, color);
        previousPhase = phase;
    }
}

/** @brief チュートリアル案内を描画する @param renderer 描画先 */
void SideScrollingShooter::DrawTutorialHud(Renderer& renderer) const {
    constexpr float CharacterSpacing = 0.0025f;
    if (m_tutorialStep >= TutorialStepCount) return;
    constexpr const char* Titles[] = {
        "MOVE", "FOCUS MOVE", "SHOT", "BOMB", "2D <-> 3D"
    };
    constexpr const char* Instructions[] = {
        "DODGE THE METEORS",
        "PASS THROUGH THE NARROW GAPS",
        "DESTROY ALL TARGETS",
        "USE YOUR BOMB TO SURVIVE THE BARRAGE",
        "ESCAPE THE 2D BARRAGE"
    };
    char progress[Localization::BufferSize(24)];
    std::snprintf(progress, sizeof(progress), Localization::Text("LESSON %d / %d"), m_tutorialStep + 1, TutorialStepCount);
    renderer.DrawText(progress, TextAlign::TopCenter, 0.014f,
        {0.65f, 0.90f, 0.95f, 1.0f}, {0.0f, -0.04f}, CharacterSpacing);
    renderer.DrawText(Titles[m_tutorialStep], TextAlign::Center, 0.030f,
        {1.0f, 0.78f, 0.12f, 1.0f}, {0.0f, 0.74f}, CharacterSpacing);
    renderer.DrawText(Instructions[m_tutorialStep], TextAlign::Center, 0.015f,
        {0.85f, 0.95f, 0.90f, 1.0f}, {0.0f, 0.60f}, CharacterSpacing);
}

/** @brief 文字表示領域へ共通の黒いHUD背景を描画する @param renderer 描画先 @return なし */
void SideScrollingShooter::DrawHudBackground(Renderer& renderer) const {
    constexpr ColorF HudBackground {0.0f, 0.0f, 0.0f, 0.82f};
    // ステータスと操作文字の背面だけを覆い、残りはプレイ領域として開放する
    renderer.Draw(Rect {{0.0f, 0.92f}, {2.0f, 0.08f}}, HudBackground);
    renderer.Draw(Rect {{0.0f, -0.92f}, {2.0f, 0.08f}}, HudBackground);
}

/**
 * @brief チュートリアルの操作キーを自機上部へ表示する
 * @param renderer 描画先レンダラー
 * @param camera 自機位置の投影に使うカメラ
 * @param playerZ 描画中の自機Z座標
 * @return なし
 */
void SideScrollingShooter::DrawTutorialControlHint(
    Renderer& renderer, const Camera3D& camera, float playerZ) const {
    if (!m_tutorialMode || m_missionStartTimer > 0 ||
        m_tutorialStep >= TutorialStepCount) return;

    constexpr const char* KeyboardHints[] = {
        "MOVE WASD", "SLOW SHIFT", "Z ATTACK", "C BOMB", "X SHIFT"
    };
    constexpr const char* GamepadHints[] = {
        "MOVE L STICK", "SLOW LB/LT", "A/RB/RT ATTACK", "Y BOMB", "X SHIFT"
    };
    constexpr const char* Switch2Hints[] = {
        "MOVE L STICK", "SLOW L/ZL", "A/R/ZR ATTACK", "Y BOMB", "X SHIFT"
    };
    Vector2 screenPosition;
    Vector3 player = PlayerWorldPosition();
    if (!IsTayamaBattle()) player.z = playerZ;
    if (!camera.TryWorldToScreen(
            {player.x, player.y + 0.95f, player.z}, screenPosition)) return;
    const Viewport& viewport = camera.GetViewport();
    const Vector2 position {
        (screenPosition.x - static_cast<float>(viewport.x)) /
                static_cast<float>(viewport.width) * 2.0f - 1.0f,
        1.0f - (screenPosition.y - static_cast<float>(viewport.y)) /
                static_cast<float>(viewport.height) * 2.0f};
    const char* hint = (Input::IsSwitch2ProConnected() ? Switch2Hints :
        Input::IsGamepadConnected() ? GamepadHints : KeyboardHints)[m_tutorialStep];
    renderer.DrawText(hint, TextAlign::Center, 0.015f,
        {0.35f, 1.0f, 0.85f, 1.0f}, position, 0.0025f);
}

/**
 * @brief 3D視点の十字照準とスナップ対象の四角い照準を描画する
 * @param renderer 描画先レンダラー
 * @param camera 射撃方向の投影に使うカメラ
 * @return なし
 */
void SideScrollingShooter::DrawReticle(Renderer& renderer, const Camera3D& camera) const {
    // 2D、視点遷移、演出中、撃墜中は照準を表示しない
    if (m_viewMode != ViewMode::Rail3D || m_viewTransitionTimer > 0 ||
        m_clear || Player().m_playerDestructionTimer > 0 || StageDispatch::IsCinematic(*this)) return;
    const Viewport& viewport = camera.GetViewport();
    if (viewport.width <= 0 || viewport.height <= 0) return;

    // 通常弾の進行方向を投影し、周回戦と壁面の縦スクロール弾道へ追従する
    Vector2 screen;
    if (!camera.TryWorldToScreen(PlayerAimPoint(), screen)) return;
    const Vector2 center = Player().m_crosshairInitialized ? Player().m_crosshairPosition : Vector2 {
        (screen.x - viewport.x) / viewport.width * 2.0f - 1.0f,
        1.0f - (screen.y - viewport.y) / viewport.height * 2.0f};
    const float aspect = static_cast<float>(viewport.height) / viewport.width;

    // 中央の小さな十字と上下左右の短い目盛りを描画する
    for (int layer = 0; layer < 2; ++layer) {
        const float thickness = layer == 0 ? 0.00525f : 0.00225f;
        const ColorF color = layer == 0 ? ColorF {0.08f, 0.07f, 0.02f, 0.22f} :
            ColorF {1.0f, 0.9f, 0.3f, 0.50f};
        renderer.Draw(Rect {center, {0.0135f * aspect, thickness}}, color);
        renderer.Draw(Rect {center, {thickness * aspect, 0.0135f}}, color);
        for (float sign : {-1.0f, 1.0f}) {
            renderer.Draw(Rect {{center.x + sign * 0.0525f * aspect, center.y},
                {0.0135f * aspect, thickness}}, color);
            renderer.Draw(Rect {{center.x, center.y + sign * 0.0525f},
                {thickness * aspect, 0.0135f}}, color);
        }
    }

    // 四角い枠を常時表示し、捕捉・対象変更・解除のすべてを連続した移動にする
    camera.TryWorldToScreen(PlayerAimPoint() + Player().m_aimSnapOffset, screen);
    const Vector2 lockedCenter {
        (screen.x - viewport.x) / viewport.width * 2.0f - 1.0f,
        1.0f - (screen.y - viewport.y) / viewport.height * 2.0f};
    for (int layer = 0; layer < 2; ++layer) {
        const float thickness = layer == 0 ? 0.0075f : 0.003f;
        const ColorF color = layer == 0 ? ColorF {0.08f, 0.07f, 0.02f, 0.22f} :
            ColorF {1.0f, 0.85f, 0.20f, 0.50f};
        for (float x : {-1.0f, 1.0f}) {
            for (float y : {-1.0f, 1.0f}) {
                renderer.Draw(Rect {{lockedCenter.x + x * (0.09f - 0.016875f) * aspect,
                    lockedCenter.y + y * 0.09f}, {(0.03375f + thickness) * aspect, thickness}}, color);
                renderer.Draw(Rect {{lockedCenter.x + x * 0.09f * aspect,
                    lockedCenter.y + y * (0.09f - 0.016875f)}, {thickness * aspect, 0.03375f + thickness}}, color);
            }
        }
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
    const int bossMaxHp = m_enemies[0].maxHp > 0 ? m_enemies[0].maxHp : m_stage->BossMaxHp();
    const float hpRate = bossMaxHp > 0 ? Math::Clamp01(m_displayBossHp / bossMaxHp) : 0.0f;
    DrawShape(renderer, 0.0f, 0.76f, BossBarWidth, 0.025f, BossBarBack);
    DrawShape(renderer, BossBarWidth * (1.0f - hpRate), 0.76f,
        BossBarWidth * hpRate, 0.018f, BossBarFill);
    // 実際のフェーズ判定が切り替わるHPへ黄色い区切り線を重ねる
    DrawBossPhaseDividers(renderer, 0.755f, BossBarWidth, bossMaxHp, BossBarDivider);
    const BossStory story = StageDispatch::Story(*this);
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
 * @brief ボス登場中に上下の警告帯を描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawBossWarning(Renderer& renderer) const {
    // 共通登場フェーズと独自進行のStage 5登場フェーズを同じ警告帯へまとめる
    const bool standardEntrance =
        m_bossIntroductionPhase == BossIntroductionPhase::Entrance;
    const bool stage5Entrance = m_stageNumber == 5 &&
        m_stage5.phase == ShooterStages::Stage5::Phase::EastsourceIntro;
    if (!standardEntrance && !stage5Entrance) return;

    constexpr std::string_view WarningText =
        "/// WARNING /// WARNING /// WARNING /// WARNING /// WARNING /// WARNING /// WARNING ///";
    constexpr float TextSize = 0.035f;
    constexpr float CharacterSpacing = 0.006f;
    constexpr float PhraseAdvance = 0.72f;
    constexpr float ScrollPerFrame = 0.008f;
    const int age = stage5Entrance ? m_stage5.phaseTimer : m_bossIntroductionTimer;
    const int duration = stage5Entrance ? ShooterStages::Stage5::EastsourceIntroFrames :
        StageDispatch::BossIntroductionFrames(*this);
    const float alpha = SmoothStep(BossWarningFade(age, duration));
    const float scroll = std::fmod(
        static_cast<float>(age) * ScrollPerFrame, PhraseAdvance);
    const float firstGlyphX = -1.0f + TextSize * renderer.AspectRatio();

    // 既存HUDを半透明の黒帯で上書きして内側の赤線でプレイ領域と分ける
    renderer.Draw(Rect {{0.0f, 0.90f}, {1.0f, 0.10f}},
        {0.015f, 0.005f, 0.008f, 0.92f * alpha});
    renderer.Draw(Rect {{0.0f, -0.90f}, {1.0f, 0.10f}},
        {0.015f, 0.005f, 0.008f, 0.92f * alpha});
    renderer.Draw(Rect {{0.0f, 0.80f}, {1.0f, 0.004f}},
        {1.0f, 0.04f, 0.04f, alpha});
    renderer.Draw(Rect {{0.0f, -0.80f}, {1.0f, 0.004f}},
        {1.0f, 0.04f, 0.04f, alpha});

    // 上段は左、下段は右へ同じ速度で流して警告文字を途切れさせない
    const ColorF warningColor {1.0f, 0.045f, 0.035f, alpha};
    renderer.DrawText(WarningText, {firstGlyphX - scroll, 0.90f},
        TextSize, warningColor, CharacterSpacing);
    renderer.DrawText(WarningText,
        {firstGlyphX - PhraseAdvance + scroll, -0.90f},
        TextSize, warningColor, CharacterSpacing);
}

/**
 * @brief 2Dと3Dの表示切り替えクールダウンを描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param playerZ 描画中の自機のワールド座標Z
 * @return なし
 */
void SideScrollingShooter::DrawViewToggleCooldownHud(
    Renderer& renderer, const Camera3D& camera, float playerZ) const {
    constexpr int FadeFrames = 12;
    constexpr float BarWidth = 0.11f;
    const float readyRate = ViewToggleReadyRate(m_viewToggleCooldown, ViewToggleCooldownFrames);
    const float opacity = ViewToggleHudOpacity(
        m_viewToggleCooldown, ViewToggleCooldownFrames, FadeFrames);
    if (opacity <= 0.0f) return;
    const float trackColor[4] = {m_grazing ? 0.30f : 0.10f,
        m_grazing ? 0.04f : 0.18f, m_grazing ? 0.04f : 0.20f, 0.92f * opacity};
    const float fillColor[4] = {m_grazing ? 1.00f : 0.20f,
        m_grazing ? 0.12f : 0.82f, m_grazing ? 0.08f : 1.00f, opacity};

    // 自機下方のワールド座標を画面座標へ投影してメーターを追従させる
    Vector2 screenPosition;
    Vector3 player = PlayerWorldPosition();
    if (!IsTayamaBattle()) player.z = playerZ;
    if (!camera.TryWorldToScreen(
            {player.x, player.y - 0.70f, player.z}, screenPosition)) return;
    const Viewport& viewport = camera.GetViewport();
    Vector2 position {
        (screenPosition.x - static_cast<float>(viewport.x)) /
                static_cast<float>(viewport.width) * 2.0f - 1.0f,
        1.0f - (screenPosition.y - static_cast<float>(viewport.y)) /
                static_cast<float>(viewport.height) * 2.0f};

    // グレイズ中はトラックとゲージを一緒に揺らし、回復量の読みやすさを保つ
    if (m_grazing) {
        const float age = static_cast<float>(m_tutorialMode ? m_tutorialStepFrame : m_frame);
        position.x += std::sin(age * 1.7f) * 0.006f;
        position.y += std::sin(age * 2.3f) * 0.004f;
    }

    // 通常は水色、グレイズによる回復加速中は赤色で表示する
    DrawShape(renderer, position.x, position.y, BarWidth, 0.013f, trackColor);
    DrawShape(renderer, position.x, position.y, BarWidth * readyRate, 0.008f, fillColor);
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

    const BossStory story = StageDispatch::Story(*this);
    if (m_bossStoryLine >= story.lineCount) return;

    // 戦闘画面を少し暗くして会話を前面へ表示する
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.0f, 0.0f, 0.0f, 0.38f});
    const BossStoryLine& line = story.lines[m_bossStoryLine];
    const ColorF nameColor = line.isBoss ? ColorF {1.0f, 0.45f, 0.65f, 1.0f} :
        ColorF {0.35f, 0.90f, 1.0f, 1.0f};
    constexpr float CharacterSpacing = 0.0015f;
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
    // 会話全体を翻訳してから文字幅で二行に折り返す
    const std::string translated = Localization::Translate(line.text);
    const std::string_view text = translated;
    float textSize = 0.016f;
    std::size_t firstLineEnd = 0;
    std::size_t secondLineStart = 0;
    for (;;) {
        firstLineEnd = Utf8Text::WrapLineEnd(text, 1.55f, textSize, CharacterSpacing, renderer.AspectRatio());
        secondLineStart = firstLineEnd;
        while (secondLineStart < text.size() && text[secondLineStart] == ' ') ++secondLineStart;
        if (Utf8Text::Measure(text.substr(secondLineStart), textSize, CharacterSpacing,
                renderer.AspectRatio()).width <= 1.55f || textSize <= 0.006f) break;
        textSize *= 0.95f;
    }
    renderer.Draw(Rect {{0.0f, -0.48f}, {1.72f, 0.34f}}, {0.03f, 0.08f, 0.14f, 0.92f});
    renderer.DrawText(line.speaker, {-0.78f, -0.37f}, 0.022f, nameColor, CharacterSpacing);
    renderer.DrawText(text.substr(0, firstLineEnd), {-0.78f, -0.51f}, textSize,
        ColorF::White(), CharacterSpacing);
    if (secondLineStart < text.size()) renderer.DrawText(text.substr(secondLineStart),
        {-0.78f, -0.57f}, textSize, ColorF::White(), CharacterSpacing);
    renderer.DrawText(Input::IsGamepadConnected() ? "A: NEXT   X: SKIP" : "Z: NEXT   X: SKIP",
        TextAlign::CenterRight, 0.012f, {0.65f, 0.75f, 0.82f, 1.0f}, {-0.17f, -0.65f}, CharacterSpacing);
#else
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
        renderer.DrawText(text.substr(secondLineStart), {-0.78f, -0.57f}, 0.016f,
            ColorF::White(), CharacterSpacing);
    }
    renderer.DrawText(Input::IsGamepadConnected() ? "A: NEXT   X: SKIP" : "Z: NEXT   X: SKIP",
        {0.50f, -0.60f}, 0.012f,
        {0.65f, 0.75f, 0.82f, 1.0f}, CharacterSpacing);
#endif
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
    const BossStory story = StageDispatch::Story(*this);
    renderer.DrawText(story.bossName, TextAlign::Center, 0.064f,
        {0.94f, 0.92f, 0.84f, nameAlpha}, {0.0f, -0.015f}, 0.008f);
    const float redLine[4] = {0.72f, 0.04f, 0.025f, nameAlpha};
    DrawShape(renderer, 0.0f, -0.33f, 0.84f * inkProgress, 0.018f, redLine);
}
