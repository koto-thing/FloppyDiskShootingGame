#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>

#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::OceanFoamColor;

constexpr float DesertBoneColor[4] = { 0.88f, 0.78f, 0.56f, 1.0f };
constexpr float MeteorColor[4] = { 0.30f, 0.22f, 0.18f, 1.0f };
constexpr float MeteorCraterColor[4] = { 0.95f, 0.38f, 0.08f, 1.0f };

constexpr float SeaSerpentColor[4] = { 0.05f, 0.24f, 0.20f, 1.0f };
constexpr float SeaSerpentBellyColor[4] = { 0.28f, 0.62f, 0.48f, 1.0f };
constexpr float SeaSerpentEyeColor[4] = { 1.00f, 0.84f, 0.16f, 1.0f };
constexpr float SeaSerpentSideEyeSurfaceOffset = 0.90f;
constexpr float SeaSerpentRailEyeSurfaceOffset = 1.70f;
static_assert(SeaSerpentSideEyeSurfaceOffset > 1.35f * 1.25f * 0.5f);
static_assert(SeaSerpentRailEyeSurfaceOffset > 2.50f * 1.25f * 0.5f);

struct SeaSerpentMotion {
    int segmentCount;
    float progress;
    float direction;
    float sideOriginX;
    float railOriginX;
    float railOriginZ;
    float railDirection;
    float railTravel;
    float elevation;
    float travel;
    float segmentSpacing;
    float segmentDelay;
    float scale;
};

/**
 * @brief 現フレームのウミヘビ行動と胴体配置を取得する
 * @param frame ステージ開始からのフレーム数
 * @param motion 行動情報の格納先
 * @return ウミヘビが海面上にいる場合true
 */
bool GetSeaSerpentMotion(int frame, SeaSerpentMotion& motion) {
    constexpr int CycleFrames = 420;
    constexpr int Duration[] = {112, 138, 172};
    constexpr int Segments[] = {14, 16, 23};
    constexpr float Elevation[] = {8.2f, 6.4f, 12.0f};
    constexpr float Travel[] = {12.0f, 17.0f, 14.0f};
    constexpr float Spacing[] = {0.90f, 0.78f, 1.12f};
    constexpr float Delay[] = {0.055f, 0.042f, 0.050f};
    constexpr float Scale[] = {1.70f, 2.00f, 3.20f};
    static_assert(Segments[2] > Segments[0] && Elevation[2] > Elevation[0] && Scale[2] > Scale[0]);
    const int cycle = frame / CycleFrames;
    const int action = cycle % 3;
    const int cycleFrame = frame % CycleFrames;
    const int startFrame = 48 + (cycle * 97) % 170;
    if (cycleFrame < startFrame || cycleFrame >= startFrame + Duration[action]) return false;

    motion.segmentCount = Segments[action];
    motion.progress = static_cast<float>(cycleFrame - startFrame) / static_cast<float>(Duration[action] - 1);
    motion.direction = cycle % 2 == 0 ? 1.0f : -1.0f;
    motion.sideOriginX = -motion.direction * Travel[action] * 0.5f;
    motion.railOriginX = -8.0f + static_cast<float>((cycle * 29) % 17);
    motion.railDirection = cycle % 2 == 0 ? -1.0f : 1.0f;
    motion.railOriginZ = motion.railDirection < 0.0f ? 68.0f : 4.0f;
    motion.railTravel = 64.0f;
    motion.elevation = Elevation[action];
    motion.travel = Travel[action];
    motion.segmentSpacing = Spacing[action];
    motion.segmentDelay = Delay[action];
    motion.scale = Scale[action];
    return true;
}

/**
 * @brief ウミヘビの各胴体節が頭から遅れて一周する進行率を取得する
 * @param progress 行動全体の進行率
 * @param segmentCount 胴体節数
 * @param segmentDelay 隣接する胴体節間の遅延
 * @param segmentIndex 頭を0とする胴体節番号
 * @return 水中を0、再入水完了を1とする進行率
 */
constexpr float GetSeaSerpentSegmentProgress(float progress, int segmentCount,
    float segmentDelay, int segmentIndex) {
    const float tailDelay = static_cast<float>(segmentCount - 1) * segmentDelay;
    return Math::Clamp01(progress * (1.0f + tailDelay) - static_cast<float>(segmentIndex) * segmentDelay);
}

static_assert(GetSeaSerpentSegmentProgress(1.0f, 23, 0.05f, 22) >= 1.0f - Math::Epsilon);

}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"

/** @brief ステージ固有の破壊可能ギミックを更新する */
void SideScrollingShooter::TickStageGimmicks() {
    if (m_stageNumber != 1) return;

    // 大小の異なる隕石をそれぞれ移動・回転させる
    for (auto& meteor : m_meteors) {
        if (meteor.destroyed) continue;
        meteor.travel += 0.10f + meteor.scale * 0.06f;
        if (meteor.travel >= 72.0f) meteor.travel -= 72.0f;
        meteor.yaw += meteor.spin;
    }
}

/** @brief ステージ固有の破壊可能ギミックを初期状態へ戻す */
void SideScrollingShooter::ResetStageGimmicks() {
    constexpr float Travel[] = { 0.0f, 11.5f, 24.8f, 37.0f, 50.6f, 63.4f };
    constexpr float Scale[] = { 1.75f, 1.10f, 2.05f, 1.38f, 1.62f, 0.92f };
    constexpr float Spin[] = { 0.035f, -0.052f, 0.028f, -0.041f, 0.046f, -0.061f };
    for (int i = 0; i < MeteorCount; ++i) {
        m_meteors[i] = { Travel[i], Scale[i], static_cast<float>(i) * 0.7f, Spin[i],
            4 + i % 3, false };
    }
    m_boneArchHp = BoneArchMaxHp;
    m_boneArchDestroyed = false;
}

/**
 * @brief 自機弾がステージ固有ギミックへ命中した場合にダメージを適用する
 * @param shot 命中判定対象の自機弾
 * @return ギミックへ命中した場合true
 */
bool SideScrollingShooter::TryDamageStageGimmick(Shot& shot) {
    if (shot.enemy) return false;
    const int meteorIndex = m_stageNumber == 1 ?
        FindStage1Meteor(shot.x, shot.y, shot.z, shot.hitRadius) : -1;
    if (meteorIndex >= 0) {
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        Meteor& meteor = m_meteors[meteorIndex];
        meteor.hp -= shot.damage;
        SpawnMeteorDebris(meteor, meteor.hp <= 0 ? 8 : 2);
        if (meteor.hp <= 0) meteor.destroyed = true;
        PlayHitSound();
        return true;
    }
    if (m_stageNumber == 2 && !m_boneArchDestroyed &&
        HitsDesertBoneArch(shot.x, shot.y, shot.z, shot.hitRadius)) {
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        m_boneArchHp -= shot.damage;
        if (m_boneArchHp <= 0) DestroyDesertBoneArch();
        PlayHitSound();
        return true;
    }
    return false;
}

/**
 * @brief 砂漠を横切る骨アーチへ指定球が接触したか判定する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return 骨アーチに接触している場合true
 */
bool SideScrollingShooter::HitsDesertBoneArch(float x, float y, float z, float radius) const {
    constexpr int BoneCount = 13;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    if (m_boneArchDestroyed) return false;
    const float phase = std::fmod(m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) / static_cast<float>(BoneCount - 1);
        if (IsRailGameplayActive()) {
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                std::cos(angle) * RailRadius, RailCenterY + std::sin(angle) * RailRadius, railZ, 1.35f)) {
                return true;
            }
        } else if (Hit(x, y, radius, sideCenterX,
            -1.30f + static_cast<float>(i) * 0.24f, 0.32f)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief ステージ1を横切る隕石へ指定球が接触したか判定する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return 隕石に接触している場合true
 */
bool SideScrollingShooter::HitsStage1Meteor(float x, float y, float z, float radius) const {
    return FindStage1Meteor(x, y, z, radius) >= 0;
}

/**
 * @brief 指定球が接触しているステージ1隕石の番号を取得する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return 接触した隕石の番号、接触していない場合-1
 */
int SideScrollingShooter::FindStage1Meteor(float x, float y, float z, float radius) const {
    for (int i = 0; i < MeteorCount; ++i) {
        const Meteor& meteor = m_meteors[i];
        if (meteor.destroyed) continue;
        const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
        const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
        if (IsRailGameplayActive()) {
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

/**
 * @brief 海面からアーチ状に飛び出すウミヘビへ指定球が接触したか判定する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return ウミヘビに接触している場合true
 */
bool SideScrollingShooter::HitsOceanSeaSerpent(float x, float y, float z, float radius) const {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(m_frame, motion)) return false;

    // 描画と同じ胴体球を使い、横視点とレール視点の双方で即死障害物として判定する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const float bodyProgress = GetSeaSerpentSegmentProgress(
            motion.progress, motion.segmentCount, motion.segmentDelay, i);
        const float elevation = std::sin(Math::Pi * bodyProgress) *
            (motion.elevation - static_cast<float>(i) * 0.16f);
        const float sideX = motion.sideOriginX + motion.direction *
            (bodyProgress * motion.travel - static_cast<float>(i) * motion.segmentSpacing);
        const float railX = motion.railOriginX + std::sin(bodyProgress * 4.0f + static_cast<float>(i) * 0.6f) * 6.0f;
        const float railZ = motion.railOriginZ + motion.railDirection *
            (bodyProgress * motion.railTravel - static_cast<float>(i) * 2.6f);
        const float segmentScale = motion.scale * (i == 0 ? 1.25f : 1.0f);
        if (IsRailGameplayActive()) {
            const float railHeight = 2.50f * segmentScale;
            const float visibleHeight = Math::Clamp01(elevation / railHeight) * railHeight;
            if (visibleHeight <= 0.0f) continue;
            const float visibleScale = segmentScale * std::sqrt(visibleHeight / railHeight);
            const float railY = -3.65f + (elevation < railHeight ? visibleHeight * 0.5f :
                elevation - railHeight * 0.5f);
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                railX, railY, railZ,
                1.25f * visibleScale)) {
                return true;
            }
        } else {
            // 2D描画は自機をワールド座標へ拡大しているため、同じ座標系の胴体楕円で判定する
            const float sideHeight = 1.35f * segmentScale;
            const float visibleHeight = Math::Clamp01(elevation / sideHeight) * sideHeight;
            if (visibleHeight <= 0.0f) continue;
            const float visibleWidth = 1.18f * segmentScale * std::sqrt(visibleHeight / sideHeight);
            const float hitWidth = visibleWidth * 0.5f + radius * WorldXScale;
            const float hitHeight = visibleHeight * 0.5f + radius * WorldYScale;
            const float dx = (ToWorldX(x) - sideX) / hitWidth;
            const float sideY = -6.0f + (elevation < sideHeight ? visibleHeight * 0.5f :
                elevation - sideHeight * 0.5f);
            const float dy = (ToWorldY(y) - sideY) / hitHeight;
            if (dx * dx + dy * dy <= 1.0f) return true;
        }
    }
    return false;
}

/** @brief 生存中の爆発エフェクトを更新する */
void SideScrollingShooter::TickExplosions() {
    for (auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        const int lifetime = explosion.destruction ?
            DestructionExplosionLifetimeFrames : ExplosionLifetimeFrames;
        if (++explosion.age >= lifetime) explosion.active = false;
    }
}

/** @brief 飛散中の機体部品を更新する */
void SideScrollingShooter::TickDebris() {
    constexpr int Stage2FirstSinkEndFrame = 108;
    constexpr int Stage2ResurfaceStartFrame = 148;
    static_assert(Stage2FirstSinkEndFrame < Stage2ResurfaceStartFrame);
    Debris* stage2LowerHull = nullptr;
    for (auto& debris : m_debris) {
        if (debris.active && debris.effect == Debris::Effect::Stage2Sink) {
            stage2LowerHull = &debris;
            break;
        }
    }
    for (auto& debris : m_debris) {
        if (!debris.active) continue;

        // Stage2下部船体は一旦沈み、最後の再浮上後に上部船体へ押し戻される
        if (debris.effect == Debris::Effect::Stage2Sink) {
            if (debris.effectAge >= 0) {
                debris.y -= 0.018f;
                ++debris.effectAge;
            } else if (debris.age < Stage2FirstSinkEndFrame) {
                debris.y -= 0.0075f;
            } else if (debris.age >= Stage2ResurfaceStartFrame) {
                const float groundTopY = Math::Lerp(-6.0f, -3.65f, RailBlend());
                const float visibleTargetY = groundTopY - debris.height * 0.12f;
                debris.y = (std::min)(visibleTargetY, debris.y + 0.026f);
            }
            if (++debris.age >= debris.lifetime) debris.active = false;
            continue;
        }

        // 船底が再浮上した下部船体の上面へ届くまで、上部船体をゆっくり降下させる
        if ((debris.effect == Debris::Effect::Stage2Impact ||
            debris.effect == Debris::Effect::Stage2ImpactPiece) && debris.effectAge < 0) {
            const bool lowerHullHit = stage2LowerHull != nullptr &&
                debris.effect == Debris::Effect::Stage2Impact &&
                stage2LowerHull->age >= Stage2ResurfaceStartFrame &&
                debris.y - debris.height * 0.5f <=
                    stage2LowerHull->y + stage2LowerHull->height * 0.5f;
            const bool collisionStarted = stage2LowerHull != nullptr && stage2LowerHull->effectAge >= 0;
            if (lowerHullHit) {
                // 船体同士が衝突した瞬間に最後の大爆発音を重ねる
                stage2LowerHull->effectAge = 0;
                PlayStage2DefeatSound(true);
            }
            if (!lowerHullHit && !collisionStarted) {
                debris.y -= 0.012f;
                debris.yaw += debris.spin * 0.18f;
                if (++debris.age >= debris.lifetime) debris.active = false;
                continue;
            }
        }

        debris.x += debris.vx;
        debris.y += debris.vy;
        debris.z += debris.vz;
        if (debris.gravity || m_stage->HasDebrisGravity()) debris.vy -= 0.006f;
        debris.yaw += debris.spin;

        // 衝突後は上部船体を大きく飛散させ、主船体の衝突時刻を爆発演出へ渡す
        if (debris.effect == Debris::Effect::Stage2Impact ||
            debris.effect == Debris::Effect::Stage2ImpactPiece) {
            if (debris.effectAge < 0) {
                debris.effectAge = 0;
                debris.vx += debris.effect == Debris::Effect::Stage2Impact ? 0.025f :
                    (debris.x < 0.0f ? -0.055f : 0.055f);
                debris.vy = 0.075f;
                debris.vz += debris.effect == Debris::Effect::Stage2Impact ? 0.0f :
                    (debris.z < 0.0f ? -0.040f : 0.040f);
                debris.spin *= 3.5f;
            }
            const float groundTopY = Math::Lerp(-6.0f, -3.65f, RailBlend());
            const float minimumY = groundTopY + debris.height * 0.5f;
            if (debris.y <= minimumY) {
                debris.y = minimumY;
                debris.vx *= 0.86f;
                debris.vy = 0.0f;
                debris.vz *= 0.86f;
                debris.spin *= 0.78f;
            }
            ++debris.effectAge;
        }
        if (++debris.age >= debris.lifetime) debris.active = false;
    }
}

/**
 * @brief プリミティブ球だけで構成した砂漠の巨大骨アーチを描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawDesertBoneArch(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    if (m_boneArchDestroyed) return;

    constexpr int BoneCount = 13;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    constexpr float SideZ = SidePlaneZ + 1.2f;
    const float phase = std::fmod(m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 横視点では骨を縦に並べ、レール視点へ移ると同じ頂点数のアーチへ補間する
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) / static_cast<float>(BoneCount - 1);
        const float x = Math::Lerp(ToWorldX(sideCenterX),
            std::cos(angle) * RailRadius, railWeight);
        const float y = Math::Lerp(ToWorldY(-1.30f + static_cast<float>(i) * 0.24f),
            RailCenterY + std::sin(angle) * RailRadius, railWeight);
        const float z = Math::Lerp(SideZ, railZ, railWeight);
        const float jointScale = i % 3 == 0 ? 1.18f : 1.0f;
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(1.25f, 2.55f, railWeight) * jointScale,
            Math::Lerp(1.40f, 2.55f, railWeight) * jointScale,
            Math::Lerp(0.85f, 2.55f, railWeight) * jointScale, DesertBoneColor);
    }
}

/**
 * @brief プリミティブ球だけで構成したステージ1の巨大隕石を描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawStage1Meteor(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    for (int i = 0; i < MeteorCount; ++i) {
        const Meteor& meteor = m_meteors[i];
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
        DrawModelPrimitive(renderer, camera, 5, x, y, z, width, height, depth, MeteorColor, meteor.yaw);
        for (int crater = 0; crater < 3; ++crater) {
            const float angle = meteor.yaw + static_cast<float>(crater) * Math::TwoPi / 3.0f;
            const float offsetX = std::cos(angle) * Math::Lerp(0.52f, 1.28f, railWeight) * meteor.scale;
            const float offsetY = std::sin(angle * 1.7f) * Math::Lerp(0.40f, 1.16f, railWeight) * meteor.scale;
            DrawModelPrimitive(renderer, camera, 5, x + offsetX, y + offsetY,
                z - std::cos(angle) * Math::Lerp(0.38f, 1.75f, railWeight) * meteor.scale,
                Math::Lerp(0.38f, 0.92f, railWeight) * meteor.scale,
                Math::Lerp(0.42f, 0.92f, railWeight) * meteor.scale,
                Math::Lerp(0.20f, 0.38f, railWeight) * meteor.scale, MeteorCraterColor, meteor.yaw);
        }
    }
}

/**
 * @brief 海面から飛び出す巨大ウミヘビを描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawOceanSeaSerpent(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(m_frame, motion)) return;

    // 通常跳躍、低空横断、超巨大ジャンプを同じ判定用の胴体配置で描画する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const float bodyProgress = GetSeaSerpentSegmentProgress(
            motion.progress, motion.segmentCount, motion.segmentDelay, i);
        const float elevation = std::sin(Math::Pi * bodyProgress) *
            (motion.elevation - static_cast<float>(i) * 0.16f);
        const float sideX = motion.sideOriginX + motion.direction *
            (bodyProgress * motion.travel - static_cast<float>(i) * motion.segmentSpacing);
        const float railX = motion.railOriginX + std::sin(bodyProgress * 4.0f + static_cast<float>(i) * 0.6f) * 6.0f;
        const float railZ = motion.railOriginZ + motion.railDirection *
            (bodyProgress * motion.railTravel - static_cast<float>(i) * 2.6f);
        const float scale = motion.scale * (i == 0 ? 1.25f : 1.0f);
        const float sideWidth = 1.18f * scale;
        const float sideHeight = 1.35f * scale;
        const float sideDepth = 0.72f * scale;
        const float railSize = 2.50f * scale;
        const float sideVisibleHeight = Math::Clamp01(elevation / sideHeight) * sideHeight;
        const float railVisibleHeight = Math::Clamp01(elevation / railSize) * railSize;
        if (sideVisibleHeight <= 0.0f && railVisibleHeight <= 0.0f) continue;
        const float sideVisibleScale = std::sqrt(sideVisibleHeight / sideHeight);
        const float railVisibleScale = std::sqrt(railVisibleHeight / railSize);
        const float x = Math::Lerp(sideX, railX, railWeight);
        const float sideY = -6.0f + (elevation < sideHeight ? sideVisibleHeight * 0.5f :
            elevation - sideHeight * 0.5f);
        const float railY = -3.65f + (elevation < railSize ? railVisibleHeight * 0.5f :
            elevation - railSize * 0.5f);
        const float y = Math::Lerp(sideY, railY, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.1f, railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(sideWidth * sideVisibleScale, railSize * railVisibleScale, railWeight),
            Math::Lerp(sideVisibleHeight, railVisibleHeight, railWeight),
            Math::Lerp(sideDepth * sideVisibleScale, railSize * railVisibleScale, railWeight), SeaSerpentColor);
        if (i % 2 == 1 && (sideVisibleHeight >= sideHeight || railVisibleHeight >= railSize)) {
            DrawModelPrimitive(renderer, camera, 5, x, y - Math::Lerp(0.48f, 0.82f, railWeight), z - 0.05f,
                Math::Lerp(0.52f, 1.15f, railWeight), Math::Lerp(0.28f, 0.48f, railWeight),
                Math::Lerp(0.18f, 0.42f, railWeight), SeaSerpentBellyColor);
        }
    }
    const float headScale = motion.scale * 1.25f;
    const float headProgress = GetSeaSerpentSegmentProgress(
        motion.progress, motion.segmentCount, motion.segmentDelay, 0);
    const float headElevation = std::sin(Math::Pi * headProgress) * motion.elevation;
    const float headSideX = motion.sideOriginX + motion.direction * headProgress * motion.travel;
    const float headRailX = motion.railOriginX + std::sin(headProgress * 4.0f) * 6.0f;
    const float headRailZ = motion.railOriginZ + motion.railDirection * headProgress * motion.railTravel;
    const float headX = Math::Lerp(headSideX, headRailX, railWeight);
    const float headSideVisibleHeight = Math::Clamp01(headElevation / (1.35f * headScale)) * 1.35f * headScale;
    const float headRailVisibleHeight = Math::Clamp01(headElevation / (2.50f * headScale)) * 2.50f * headScale;
    const float headSideY = -6.0f + (headElevation < 1.35f * headScale ? headSideVisibleHeight * 0.5f :
        headElevation - 1.35f * headScale * 0.5f);
    const float headRailY = -3.65f + (headElevation < 2.50f * headScale ? headRailVisibleHeight * 0.5f :
        headElevation - 2.50f * headScale * 0.5f);
    const float headY = Math::Lerp(headSideY, headRailY, railWeight);
    const float headZ = Math::Lerp(SidePlaneZ + 13.1f, headRailZ, railWeight);

    // 頭が海面を出入りする短い時間だけ、水滴を初速と重力による放物線で飛ばす
    const float emergeProgress = std::asin((std::min)(1.0f, 1.35f * headScale / motion.elevation)) /
        Math::HalfPi;
    const float reentryProgress = 1.0f - emergeProgress;
    const float splashCenter = headProgress < 0.5f ? emergeProgress : reentryProgress;
    const float splashTime = Math::Clamp01((headProgress - (splashCenter - emergeProgress)) /
        (emergeProgress * 2.0f));
    const bool splashActive = std::abs(headProgress - splashCenter) <= emergeProgress;
    if (splashActive) {
        for (int i = 0; i < 17; ++i) {
            const float spread = static_cast<float>(i - 8) * 0.22f * motion.scale * splashTime;
            const float launchVelocity = (1.05f + static_cast<float>((i * 5) % 4) * 0.22f) * motion.scale;
            const float gravity = launchVelocity * 2.0f;
            const float dropletHeight = launchVelocity * splashTime - gravity * splashTime * splashTime * 0.5f;
            const float sideSplashX = headSideX - motion.direction * spread;
            const float railSplashX = headRailX - motion.direction * spread;
            DrawModelPrimitive(renderer, camera, 5,
                Math::Lerp(sideSplashX, railSplashX, railWeight),
                Math::Lerp(-5.75f + dropletHeight, -3.45f + dropletHeight, railWeight),
                Math::Lerp(SidePlaneZ + 13.0f, headRailZ - motion.railDirection * spread, railWeight),
                Math::Lerp(0.16f, 0.42f, railWeight) * motion.scale,
                Math::Lerp(0.28f, 0.65f, railWeight) * motion.scale,
                Math::Lerp(0.10f, 0.32f, railWeight) * motion.scale, OceanFoamColor);
        }
    }
    if (headSideVisibleHeight >= 1.35f * headScale || headRailVisibleHeight >= 2.50f * headScale) {
        // 目をカメラ側の頭部表面より前へ置き、移動で視線が斜めになっても胴体へ埋まらないようにする
        const Vector3 headPosition {headX, headY, headZ};
        const Vector3 eyePosition = headPosition +
            camera.Right() * (Math::Lerp(0.36f, 0.82f, railWeight) * motion.scale) +
            camera.Up() * (Math::Lerp(0.30f, 0.72f, railWeight) * motion.scale) +
            (camera.Position() - headPosition).Normalized() *
                (Math::Lerp(SeaSerpentSideEyeSurfaceOffset, SeaSerpentRailEyeSurfaceOffset, railWeight) * motion.scale);
        DrawModelPrimitive(renderer, camera, 5, eyePosition.x, eyePosition.y, eyePosition.z,
            Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale, Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale,
            Math::Lerp(0.10f, 0.20f, railWeight) * motion.scale, SeaSerpentEyeColor);
    }
}

/**
 * @brief 弾の命中位置へ爆発エフェクトを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @return なし
 */
void SideScrollingShooter::SpawnExplosion(float x, float y, float z, bool destruction) {
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {x, y, IsRailGameplayActive() ? z : ToRailZFromSideX(x), 0, destruction, true};
        return;
    }
}

/**
 * @brief 飛散するモデル部品を固定長プールへ追加する
 * @param x 部品のワールドX座標
 * @param y 部品のワールドY座標
 * @param z 部品のワールドZ座標
 * @param vx 部品のX速度
 * @param vy 部品のY速度
 * @param vz 部品のZ速度
 * @param yaw 部品の初期Y軸回転
 * @param spin 部品のY軸回転速度
 * @param shape 描画するプリミティブ形状
 * @param width 部品の幅
 * @param height 部品の高さ
 * @param depth 部品の奥行き
 * @param color 部品の色
 * @param lifetime 部品が消滅するまでのフレーム数
 * @param shrinkStartAge 縮小を開始するフレーム
 * @param gravity 重力を適用する場合true
 * @return なし
 */
void SideScrollingShooter::SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
    float yaw, float spin, int shape, float width, float height, float depth, const float color[4],
    int lifetime, int shrinkStartAge, bool gravity, Debris::Effect effect) {
    for (auto& debris : m_debris) {
        if (debris.active) continue;
        debris = {x, y, z, vx, vy, vz, yaw, spin, width, height, depth,
            {color[0], color[1], color[2], color[3]}, shape, 0, lifetime, shrinkStartAge,
            -1, effect, gravity, true};
        return;
    }
}

/**
 * @brief 砂漠の骨アーチを破壊して小さな骨を飛散させる
 * @return なし
 */
void SideScrollingShooter::DestroyDesertBoneArch() {
    constexpr int BoneCount = 13;
    constexpr int Lifetime = 150;
    constexpr int ShrinkStartAge = 90;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    constexpr float SideZ = SidePlaneZ + 1.2f;
    const float railWeight = RailBlend();
    const float phase = std::fmod(m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 描画中の各関節位置から小さな骨を外向きに飛散させる
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) / static_cast<float>(BoneCount - 1);
        const float x = Math::Lerp(ToWorldX(sideCenterX), std::cos(angle) * RailRadius, railWeight);
        const float y = Math::Lerp(ToWorldY(-1.30f + static_cast<float>(i) * 0.24f),
            RailCenterY + std::sin(angle) * RailRadius, railWeight);
        const float z = Math::Lerp(SideZ, railZ, railWeight);
        const float scatterAngle = angle + static_cast<float>(i % 3 - 1) * 0.35f;
        const float size = 0.30f + static_cast<float>(i % 3) * 0.08f;
        SpawnDebrisPiece(x, y, z, std::cos(scatterAngle) * 0.055f,
            0.055f + static_cast<float>(i % 4) * 0.012f,
            railWeight > 0.5f ? std::sin(scatterAngle) * 0.055f : 0.0f,
            angle, (i % 2 == 0 ? 0.08f : -0.08f) * (1.0f + i * 0.04f),
            5, size, size * 1.35f, size * 0.75f, DesertBoneColor,
            Lifetime, ShrinkStartAge, true);
    }

    m_boneArchHp = 0;
    m_boneArchDestroyed = true;
}

/**
 * @brief 被弾または破壊された隕石から当たり判定を持たない小隕石を飛散させる
 * @param meteor 破片の発生元となる隕石
 * @param count 発生させる小隕石の数
 * @return なし
 */
void SideScrollingShooter::SpawnMeteorDebris(const Meteor& meteor, int count) {
    const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
    const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
    const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
    const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
    const bool railMode = IsRailGameplayActive();
    const float x = railMode ? railX : ToWorldX(sideX);
    const float y = railMode ? railY : ToWorldY(sideY);
    const float z = railMode ? 72.0f - meteor.travel : SidePlaneZ + 1.2f;

    // 既存のデブリは判定に使われないため、そのまま小隕石の漂流表現として再利用する
    for (int i = 0; i < count; ++i) {
        const float angle = meteor.yaw + static_cast<float>(i) * Math::TwoPi / static_cast<float>(count);
        const float size = (0.20f + static_cast<float>(i % 3) * 0.08f) * meteor.scale;
        SpawnDebrisPiece(x, y, z, std::cos(angle) * 0.035f, std::sin(angle * 1.4f) * 0.030f,
            railMode ? std::sin(angle) * 0.045f : 0.0f, angle, meteor.spin * (1.5f + i * 0.1f),
            5, size, size * 0.85f, size * 0.75f, MeteorColor);
    }
}
