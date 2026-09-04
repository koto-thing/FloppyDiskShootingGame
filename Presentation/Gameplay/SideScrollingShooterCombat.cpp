#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"
#include "Stages/Common/StageDispatch.h"
#include "Stages/Stage4/Stage4Module.h"

#include "SideScrollingShooterEnemies.h"
#include "Stages/Common/StageDefinition.h"

namespace {
constexpr float MortarExplosionDepthHitRadius = 0.85f;

/**
 * @brief 追尾対象として現在の候補より優先するか判定する
 * @param candidateHp 候補のHP
 * @param candidateDistanceSquared 候補と自機の距離の二乗
 * @param targetHp 現在の対象のHP
 * @param targetDistanceSquared 現在の対象と自機の距離の二乗
 * @return HPが低いか、同じHPで自機に近い場合true
 */
constexpr bool IsPreferredHomingTarget(int candidateHp, float candidateDistanceSquared,
    int targetHp, float targetDistanceSquared) {
    return candidateHp < targetHp ||
        (candidateHp == targetHp && candidateDistanceSquared < targetDistanceSquared);
}

/**
 * @brief ボムの発射位置から画面中央までの座標を求める
 * @param start 発射位置
 * @param age 発射後の経過フレーム
 * @param travelFrames 中央へ到達するフレーム数
 * @return 現在座標
 */
constexpr float BombTravelCoordinate(float start, int age, int travelFrames) {
    if (travelFrames <= 0 || age >= travelFrames) return 0.0f;
    if (age <= 0) return start;
    return start * static_cast<float>(travelFrames - age) / static_cast<float>(travelFrames);
}

/**
 * @brief 透視投影上で同じ位置になる奥行き平面への縮尺を取得する
 * @param cameraZ カメラのZ座標
 * @param sourceZ 投影元のZ座標
 * @param targetZ 投影先平面のZ座標
 * @return カメラを原点とした投影縮尺
 */
constexpr float PerspectiveDepthScale(float cameraZ, float sourceZ, float targetZ) {
    return (targetZ - cameraZ) / (sourceZ - cameraZ);
}

/**
 * @brief 2D縦スクロール用の敵弾を地面側へ向ける
 * @param vx X方向速度
 * @param vy Y方向速度
 * @return なし
 */
void AimShotGroundward(float& vx, float& vy) {
    const float speedSquared = vx * vx + vy * vy;
    vx *= 0.25f;
    vy = -std::sqrt((std::max)(0.0f, speedSquared - vx * vx));
}

static_assert(BombTravelCoordinate(-0.8f, 0, 24) == -0.8f);
static_assert(BombTravelCoordinate(-0.8f, 24, 24) == 0.0f);
static_assert(PerspectiveDepthScale(-13.5f, 35.0f, 10.0f) > 0.0f);
static_assert(PerspectiveDepthScale(-13.5f, 35.0f, 10.0f) < 1.0f);
static_assert(IsPreferredHomingTarget(1, 9.0f, 2, 1.0f));
static_assert(!IsPreferredHomingTarget(2, 1.0f, 1, 9.0f));
static_assert(IsPreferredHomingTarget(1, 1.0f, 1, 9.0f));
}

void SideScrollingShooter::TickPlayer() {
    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    const float speedScale = m_slowMove ? 0.5f : 1.0f;
    if (IsTayamaBattle()) {
        // 3Dはボス中心の周回、2Dは切替時に固定したカメラ面内の横移動として扱う
        if (IsTayamaOrbitViewActive()) {
            m_stage5.tayamaOrbitAngle = std::remainder(
                m_stage5.tayamaOrbitAngle + dx * ShooterStages::Stage5::TayamaOrbitSpeed * speedScale,
                Math::TwoPi);
            m_playerX = 0.0f;
        } else {
            m_playerX = (std::clamp)(m_playerX + dx * 0.023f * speedScale, -1.2f, 1.2f);
        }
        m_playerY = (std::clamp)(m_playerY + dy * 0.058f * speedScale,
            ShooterStages::Stage5::TayamaPlayerMinY,
            ShooterStages::Stage5::TayamaPlayerMaxY);
        return;
    }

    // Stage 5第2部道中の3DだけはHUDを除くプレイ領域全体を移動可能にする
    const bool fullScreenRailMovement = IsRailGameplayActive() &&
        UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    const Vector2 xRange = fullScreenRailMovement ?
        Vector2 {Side2DPlayerMinX, Side2DPlayerMaxX} : StageDispatch::PlayerXRange(*this);
    const Vector2 sideYRange = StageDispatch::SidePlayerYRange(*this);
    const float minY = fullScreenRailMovement ? ShooterStages::Stage5::Part2RailPlayerMinY :
        (IsRailGameplayActive() ? PlayerRailMinY() : sideYRange.x);
    const float maxY = fullScreenRailMovement ? ShooterStages::Stage5::Part2RailPlayerMaxY :
        (IsRailGameplayActive() ? StageDispatch::RailPlayerMaxY(*this) : sideYRange.y);
    m_playerX = (std::clamp)(m_playerX + dx * 0.023f * speedScale, xRange.x, xRange.y);
    m_playerY = (std::clamp)(m_playerY + dy * 0.029f * speedScale, minY, maxY);
}

/**
 * @brief 入力中の通常弾・特殊弾発射を更新する
 * @return なし
 */
void SideScrollingShooter::TickPlayerWeapons() {
    // 通常弾と選択機体の特殊弾をそれぞれのクールダウンで発射する
    bool firedPlayerShot = false;
    if (m_fire && m_shotCooldown == 0) {
        const bool verticalRoute = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
        SpawnShot(m_playerX + (IsRailGameplayActive() || verticalRoute ? 0.0f : 0.12f),
            m_playerY + (verticalRoute ? 0.12f : 0.0f),
            IsRailGameplayActive() || verticalRoute ? 0.0f : 0.045f,
            verticalRoute ? 0.045f : 0.0f, false,
            -1.0f, -1.0f, 1 + PowerLevel());
        m_shotCooldown = (std::max)(3, 7 - PowerLevel());
        PlayShotSound();
    }
    if (m_fire && m_specialShotCooldown == 0) {
        FireSpecialShots();
        const auto& config = PlayerShotConfigs[static_cast<size_t>(m_playerType)];
        m_specialShotCooldown = config.fireIntervalFrames;
        firedPlayerShot = true;
    }
    if (firedPlayerShot) PlayShotSound();
}

/**
 * @brief ボムの移動、発光待機、爆発を更新する
 * @return なし
 */
void SideScrollingShooter::TickBomb() {
    // 同時に存在できるボムは一個だけとする
    if (m_bombRequested && !m_bomb.active && m_bombCount > 0) {
        const Vector3 player = PlayerWorldPosition();
        const float playerX = IsTayamaBattle() ? FromWorldX(player.x) : m_playerX;
        const float playerY = IsTayamaBattle() ? FromWorldY(player.y) : m_playerY;
        m_bomb = {playerX, playerY, playerX, playerY,
            player.z + 6.0f, 0, true};
        --m_bombCount;
    }
    if (!m_bomb.active) return;

    // 発射位置から画面中央へ直線移動し、到達後は短時間発光させる
    ++m_bomb.age;
    m_bomb.x = BombTravelCoordinate(m_bomb.startX, m_bomb.age, BombTravelFrames);
    m_bomb.y = BombTravelCoordinate(m_bomb.startY, m_bomb.age, BombTravelFrames);
    if (m_bomb.age >= BombTravelFrames + BombChargeFrames) DetonateBomb();
}

/**
 * @brief ボムを爆発させ、画面内の通常敵と敵弾を消去する
 * @return なし
 */
void SideScrollingShooter::DetonateBomb() {
    // 画面内の通常敵だけを消し、ボス戦の進行状態は維持する
    const Vector2 sideYRange = StageDispatch::SidePlayerYRange(*this);
    for (auto& enemy : m_enemies) {
        if (!enemy.active || enemy.type == 2) continue;
        const bool visible = IsRailGameplayActive() ?
            enemy.z >= PlayerRailZ - 2.0f && enemy.z <= EnemyRailFarZ &&
                std::abs(enemy.x) <= 1.4f && std::abs(enemy.y) <= 1.4f :
            enemy.x >= Side2DPlayerMinX - Side2DShotCullMargin &&
                enemy.x <= Side2DPlayerMaxX + Side2DShotCullMargin &&
                enemy.y >= sideYRange.x - Side2DShotCullMargin &&
                enemy.y <= sideYRange.y + Side2DShotCullMargin;
        if (visible) {
            enemy.active = false;
            ++m_kills;
            ++m_chapterResult.enemyDefeatCount;
        }
    }

    // 敵弾は固定長プール上の全種類を一括で消去する
    for (auto& shot : m_shots) {
        if (shot.active && shot.enemy) shot.active = false;
    }

    // 中央に青い爆発エフェクトを生成する
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {0.0f, 0.0f, m_bomb.z, 0, false, true,
            BombExplosionEffectType};
        break;
    }
    if (m_audio) m_audio->PlaySE(Audio::SfxrPreset::Explosion);
    ShakeScreen(0.24f, 20);
    m_bomb = {};
}

void SideScrollingShooter::TickEnemies() {
    const Vector2 sideYRange = StageDispatch::SidePlayerYRange(*this);
    const Vector3 playerPosition = PlayerWorldPosition();
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        ++enemy.age;
        for (int& frames : enemy.bossPartHitFlashFrames) {
            if (frames > 0) --frames;
        }
        if (enemy.behavior == nullptr) {
            enemy.behavior = &EnemyBehaviorForType(enemy.type);
        }
        if (enemy.shotInterval <= 0) {
            enemy.shotInterval = enemy.behavior->AimedShotInterval();
        }
        if (enemy.type == 2) {
            m_stage->TickBoss(*this, enemy);
            if (StageDispatch::HandleBossInteractionAfterTick(*this, enemy)) return;
        } else {
            enemy.behavior->Tick(*this, enemy);
        }

        // Stage 5第2部の従来敵は各視点の上端から画面下方へ通過する
        if (enemy.type != Stage::BossEnemy && enemy.entersFromTop) {
            if (IsRailGameplayActive()) {
                enemy.baseY -= ShooterStages::Stage5::Part2RailEnemyFallSpeed;
                enemy.x = enemy.railAnchorX;
                enemy.y = enemy.baseY;
                enemy.z = enemy.baseZ;
            } else {
                enemy.baseY -= 0.014f;
                enemy.x = enemy.railAnchorX;
                enemy.y = enemy.baseY;
                enemy.z = ToRailZFromSideX(enemy.x);
            }
        }

        // 巨大障害物へ接触した通常敵はスコアやアイテムを発生させず、その場で破壊する
        // ボス(type 2)は対象外
        const float boneHitRadius = IsRailGameplayActive() ? enemy.behavior->CollisionRadius3D(enemy) / WorldXScale :
            enemy.behavior->CollisionRadius(enemy);
        if (enemy.type != 2 &&
            StageDispatch::HitsHazard(*this, enemy.x, enemy.y, enemy.z, boneHitRadius)) {
            SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
            SpawnEnemyDebris(enemy);
            enemy.active = false;
            continue;
        }

        if (enemy.attackWarningFrames > 0) --enemy.attackWarningFrames;
        const int aimedShotInterval = enemy.shotInterval;
        const bool canUseAimedShot = !(enemy.type == 2 &&
            StageDispatch::IsBossSpecialAttackActive(*this, enemy));
        if (aimedShotInterval > AttackWarningFrames && canUseAimedShot &&
            enemy.age % aimedShotInterval == aimedShotInterval - AttackWarningFrames) {
            // 発射時の追尾を防ぐため、予告した地点を狙い弾の目標として固定する
            enemy.attackWarningTargetX = m_playerX;
            enemy.attackWarningTargetY = m_playerY;
            enemy.attackWarningFrames = AttackWarningFrames;
        }
        if (aimedShotInterval > 0 && enemy.age % aimedShotInterval == 0 && canUseAimedShot) {
            const float dxToPlayer = enemy.attackWarningTargetX - enemy.x;
            const float dyToPlayer = enemy.attackWarningTargetY - enemy.y;
            const float length = std::sqrt(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);
            if (length > 0.001f) {
                const float shotSpeed = enemy.behavior->AimedShotSpeed();
                SpawnShot(enemy.x - 0.06f, enemy.y, dxToPlayer / length * shotSpeed,
                    dyToPlayer / length * shotSpeed, true, enemy.z, enemy.behavior->RailAimedShotSpeed());
                if (enemy.type == 2) PlayEnemyShotSound();
                else PlayBossMachineGunSound();
            }
        }

        // ボスは通常・特殊フェーズごとの間隔で、未破壊の各部位から弾幕を発射する
        if (enemy.type == 2 &&
            !StageDispatch::IsBossSpecialAttackActive(*this, enemy) &&
            enemy.age % m_stage->BossAttackInterval(static_cast<BossPhase>(enemy.bossPhase)) == 0) {
            StageDispatch::FireBossPartBarrage(*this, enemy);
        }

        if (enemy.type != 2 && !IsRailGameplayActive() && enemy.x < -2.6f) enemy.active = false;
        if (enemy.type != 2 && !IsRailGameplayActive() && enemy.entersFromTop &&
            enemy.y < sideYRange.x - Side2DShotCullMargin) {
            enemy.active = false;
        }
        if (enemy.type != 2 && IsRailGameplayActive() && enemy.entersFromTop &&
            enemy.y < ShooterStages::Stage5::Part2RailEnemyExitY) {
            enemy.active = false;
        }
        if (enemy.type != 2 && IsRailGameplayActive() && enemy.z < 2.0f) enemy.active = false;
        if (enemy.type == 2 && !enemy.collisionEnabled) continue;
        const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
        const bool playerHit = IsRailGameplayActive() ?
            Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 0.42f,
                ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.behavior->CollisionRadius3D(enemy)) :
            Hit(m_playerX, m_playerY, 0.055f, enemy.x, enemy.y, enemyRadius);
        if (enemy.active && m_invincible == 0 && playerHit) {
            if (enemy.type != 2) {
                SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
                SpawnEnemyDebris(enemy);
                enemy.active = false;
            }
            DamagePlayer();
            return;
        }
    }
    TickLinkedEnemyLasers();
}

void SideScrollingShooter::TickLinkedEnemyLasers() {
    if (m_invincible > 0) return;
    const Vector3 playerPosition = PlayerWorldPosition();

    for (const auto& upper : m_enemies) {
        if (!upper.active || upper.type != Stage::LinkedLaserEnemy || upper.laserLinkRole <= 0) continue;
        for (const auto& lower : m_enemies) {
            if (!lower.active || lower.type != Stage::LinkedLaserEnemy ||
                lower.laserLinkId != upper.laserLinkId || lower.laserLinkRole >= 0) continue;
            const bool playerHit = IsRailGameplayActive() ?
                DistancePointToSegment3D(playerPosition,
                    {ToWorldX(upper.x), ToWorldY(upper.y), upper.z},
                    {ToWorldX(lower.x), ToWorldY(lower.y), lower.z}) <=
                        LinkedLaserEnemyBehavior::LaserRadius3D() + 0.38f :
                DistancePointToSegment2D({m_playerX, m_playerY},
                    {upper.x, upper.y}, {lower.x, lower.y}) <=
                        LinkedLaserEnemyBehavior::LaserRadius2D() + 0.050f;
            if (playerHit) {
                DamagePlayer();
                return;
            }
            break;
        }
    }
}

void SideScrollingShooter::TickShots() {
    // Stage3とStage4の遅延点火ミサイルは画面外到達または命中時に爆発へ変換する
    auto DeactivateShot = [this](Shot& shot) {
        if ((m_stageNumber == 3 || m_stageNumber == 4) && shot.enemy &&
            shot.stage2.kind == ShooterStages::Stage2::ShotKind::Funnel &&
            shot.stage2.delayedEngine) {
            SpawnExplosion(shot.x, shot.y, shot.z);
        }
        shot.active = false;
    };

    const Vector3 playerPosition = PlayerWorldPosition();
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        StageDispatch::TickSpecialShotBeforeMove(*this, shot);
        if (!shot.active) continue;
        const float previousX = shot.x;
        const float previousY = shot.y;
        const float previousZ = shot.z;

        /** @brief 追尾弾を最寄りの前方敵へ旋回させる */
        if (!shot.enemy && shot.special && shot.playerType == Homing) {
            UpdateHomingShot(shot);
        }

        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
        StageDispatch::TickSpecialShotAfterMove(*this, shot, previousX, previousY, previousZ);
        if (!shot.active) continue;
        if (!IsRailGameplayActive()) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        // ハッチから出た直後は船体外へ抜けるまで通常弾の画面外カリングを猶予する
        const bool cullProtected = StageDispatch::IsShotCullProtected(*this, shot);
        const Vector2 sideYRange = StageDispatch::SidePlayerYRange(*this);
        if (!cullProtected && !IsRailGameplayActive() &&
            (shot.x < Side2DPlayerMinX - Side2DShotCullMargin ||
                shot.x > Side2DPlayerMaxX + Side2DShotCullMargin ||
                shot.y < sideYRange.x - Side2DShotCullMargin ||
                shot.y > sideYRange.y + Side2DShotCullMargin)) {
            DeactivateShot(shot);
            continue;
        }
        // 端から出る円形弾幕が生成直後に欠けないよう、弾のY消滅範囲だけ少し広げる
        const bool verticalRouteShot = !shot.enemy &&
            UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
        const bool part2RailShot = IsRailGameplayActive() && m_stageNumber == 5 &&
            ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase);
        const float railShotMinY = part2RailShot ?
            ShooterStages::Stage5::Part2RailShotMinY : PlayerRailMinY();
        const float railShotMaxY = part2RailShot ?
            ShooterStages::Stage5::Part2RailShotMaxY : StageDispatch::RailPlayerMaxY(*this);
        const float tayamaDx = ToWorldX(shot.x);
        const float tayamaDz = shot.z - ShooterStages::Stage5::TayamaArenaCenterZ;
        const bool outsideTayamaArena = IsTayamaBattle() &&
            (tayamaDx * tayamaDx + tayamaDz * tayamaDz >
                (ShooterStages::Stage5::TayamaOrbitRadius + 48.0f) *
                (ShooterStages::Stage5::TayamaOrbitRadius + 48.0f) ||
                shot.y < ShooterStages::Stage5::TayamaPlayerMinY - 2.0f ||
                shot.y > ShooterStages::Stage5::TayamaPlayerMaxY + 8.0f);
        const float railShotFarZ = m_stageNumber == 5 &&
            ShooterStages::Stage5::IsTayamaDragonBattlePhase(m_stage5.phase) ?
            ShooterStages::Stage5::TayamaDragonShotFarZ : EnemyRailFarZ;
        const bool outsideRail = !IsTayamaBattle() &&
            (shot.z < 0.0f || shot.z > railShotFarZ ||
                std::abs(shot.x) > 1.2f ||
                shot.y < railShotMinY - Side2DShotCullMargin ||
                shot.y > railShotMaxY + Side2DShotCullMargin);
        if (!cullProtected && IsRailGameplayActive() &&
            (outsideTayamaArena || outsideRail)) {
            DeactivateShot(shot);
            continue;
        }

        if (shot.enemy) {
            const bool playerHit = StageDispatch::CanEnemyShotDamagePlayer(*this, shot) &&
                (IsRailGameplayActive() ?
                Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 0.38f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                    StageDispatch::EnemyShotHitRadius(*this, shot)) :
                Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y,
                    StageDispatch::EnemyShotHitRadius(*this, shot)));
            const bool grazed = IsRailGameplayActive() ?
                Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 1.18f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.28f) :
                Hit(m_playerX, m_playerY, 0.200f, shot.x, shot.y, 0.022f);
            if (!playerHit && grazed && !shot.grazed) {
                shot.grazed = true;
                ++m_chapterResult.grazeCount;
            }
            if (m_invincible == 0 && playerHit) {
                DeactivateShot(shot);
                DamagePlayer();
                return;
            }
            continue;
        }

        if (StageDispatch::TryDamageStageTarget(*this, shot)) continue;

        for (auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            if (m_chapterResultActive && !enemy.collisionEnabled) continue;
            if (enemy.type == 2 && !enemy.collisionEnabled &&
                !StageDispatch::CanHitBossWhileCollisionDisabled(*this)) continue;
            if (enemy.behavior == nullptr) {
                enemy.behavior = &EnemyBehaviorForType(enemy.type);
            }
            BossPart hitPart = BossNose;
            if (enemy.type == 2 && TryHitBossPart(shot, enemy, hitPart)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                shot.RegisterHit();
                enemy.bossPartHitFlashFrames[hitPart] = BossPartHitFlashFrames;
                enemy.bossPartHp[hitPart] -= shot.damage;
                if (enemy.bossPartHp[hitPart] <= 0) {
                    enemy.bossPartHp[hitPart] = 0;
                    SpawnEnemyDebris(enemy, hitPart);
                    const int partDamage = m_stage->BossPartBreakDamage(hitPart);
                    bool bossDefeated = false;
                    if (m_stageNumber == 4 && Stage4Module::IsWeaponSwapActive(*this)) {
                        // 交換中の通常ダメージ保護とは別に副砲破壊ダメージだけ反映する
                        enemy.hp -= partDamage;
                        m_bossHp = (std::max)(0, enemy.hp);
                        bossDefeated = enemy.hp <= 0;
                    } else {
                        bossDefeated = DamageBoss(enemy, partDamage);
                    }
                    PlayHitSound();
                    if (bossDefeated) DefeatBoss(enemy);
                }
                if (enemy.hp <= 0) {
                    DefeatBoss(enemy);
                } else {
                    m_bossHp = enemy.hp;
                }
                break;
            }
            if (enemy.type == 2 && StageDispatch::BlocksPlayerShot(*this, shot, enemy)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                shot.active = false;
                PlayHitSound();
                break;
            }
            if (enemy.type == 2 && StageDispatch::TryHitBossBody(*this, shot, enemy)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                shot.RegisterHit();
                if (DamageBoss(enemy, shot.damage)) DefeatBoss(enemy);
                else m_bossHp = enemy.hp;
                PlayHitSound();
                break;
            }
            // 専用部位判定後に本体接触無効中のボスを共通形状判定から除外する
            if (enemy.type == 2 && !enemy.collisionEnabled) continue;
            const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
            Vector3 railTarget {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z};
            float railTargetRadius = enemy.behavior->ShotHitRadius3D(enemy);
            if (IsRailGameplayActive() && m_stageNumber == 5 &&
                ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase)) {
                railTargetRadius *= ShooterStages::Stage5::Part2EnemyScaleMultiplier(RailBlend());
            }
            if (verticalRouteShot && m_viewTransitionTimer > 0) continue;
            if (verticalRouteShot) {
                // 敵を自機弾の奥行き平面へ透視投影し、画面上で重なった場合だけ命中させる
                const Vector3 cameraPosition {
                    ToWorldX(m_playerX) * 0.18f,
                    ToWorldY(m_playerY) * 0.12f + 1.72f,
                    PlayerRailZ - 21.5f
                };
                const float projectionScale = PerspectiveDepthScale(
                    cameraPosition.z, enemy.z, shot.z);
                railTarget = cameraPosition + (railTarget - cameraPosition) * projectionScale;
                railTargetRadius *= projectionScale;
            }
            const bool enemyHit = IsRailGameplayActive() ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    railTarget.x, railTarget.y, railTarget.z, railTargetRadius) :
                Hit(shot.x, shot.y, shot.hitRadius, enemy.x, enemy.y, enemyRadius);
            if (!enemyHit) continue;
            SpawnExplosion(shot.x, shot.y, shot.z);
            shot.RegisterHit();
            if (enemy.type == 2) DamageBoss(enemy, shot.damage);
            if (enemy.type != 2) enemy.hp -= shot.damage;
            if (enemy.hp <= 0) {
                if (enemy.type == 2) {
                    DefeatBoss(enemy);
                } else {
                    SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
                    SpawnEnemyDebris(enemy);
                    enemy.active = false;
                    ++m_kills;
                    const int score = enemy.behavior->Score(enemy);
                    ++m_chapterResult.enemyDefeatCount;
                    SpawnPowerItem(enemy.x + 0.10f, enemy.y, enemy.z, 0.25f);
                    SpawnScoreItem(enemy.x - 0.10f, enemy.y, enemy.z, score);
                }
                PlayHitSound();
            } else if (enemy.type == 2) {
                m_bossHp = enemy.hp;
            }
            break;
        }
    }
}

/**
 * @brief 取得アイテムを更新して自機との取得判定を行う
 */
void SideScrollingShooter::TickItems() {
    const bool part2Route = m_stageNumber == 5 &&
        ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase);
    const Vector3 playerPosition = PlayerWorldPosition();
    for (auto& item : m_items) {
        if (!item.active) continue;

        // 第2部は両視点で地面側へ落とし、3Dだけ取得可能な手前方向の移動も維持する
        if (IsTayamaBattle()) {
            // 全周回で取得できるようボス中心のドロップを自機へ送る
            const Vector3 itemPosition {ToWorldX(item.x), ToWorldY(item.y), item.z};
            const Vector3 toPlayer = playerPosition - itemPosition;
            const float distance = (std::max)(0.001f, toPlayer.Length());
            const Vector3 velocity = toPlayer / distance * 0.42f;
            item.x += FromWorldX(velocity.x);
            item.y += FromWorldY(velocity.y);
            item.z += velocity.z;
        } else if (IsRailGameplayActive()) {
            item.z = part2Route ? (std::max)(item.z - 0.28f, PlayerRailZ) : item.z - 0.28f;
            if (part2Route) item.y -= ShooterStages::Stage5::Part2RailItemFallSpeed;
        } else {
            if (part2Route) {
                item.y -= ShooterStages::Stage5::Part2SideItemFallSpeed;
            } else {
                item.x -= 0.012f;
            }
            item.z = ToRailZFromSideX(item.x);
        }

        // 画面外へ出たアイテムを破棄する
        const float arenaDx = ToWorldX(item.x);
        const float arenaDz = item.z - ShooterStages::Stage5::TayamaArenaCenterZ;
        const bool outsideTayamaArena = IsTayamaBattle() &&
            arenaDx * arenaDx + arenaDz * arenaDz >
                (ShooterStages::Stage5::TayamaOrbitRadius + 24.0f) *
                (ShooterStages::Stage5::TayamaOrbitRadius + 24.0f);
        if ((!IsRailGameplayActive() &&
                (item.x < Side2DPlayerMinX || item.x > Side2DPlayerMaxX ||
                    item.y < Side2DPlayerMinY || item.y > Side2DPlayerMaxY)) ||
            (IsRailGameplayActive() && !IsTayamaBattle() &&
                (item.z < 0.0f || item.z > 72.0f || std::abs(item.x) > 1.2f ||
                    (part2Route ?
                        item.y < ShooterStages::Stage5::Part2RailEnemyExitY :
                        std::abs(item.y) > 1.24f))) || outsideTayamaArena) {
            item.active = false;
            continue;
        }

        // 自機が近づいたアイテムだけを強く追尾させる
        const float dx = FromWorldX(playerPosition.x) - item.x;
        const float dy = FromWorldY(playerPosition.y) - item.y;
        const bool followsPlayer = IsRailGameplayActive() ?
            Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 3.5f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.0f) :
            Hit(m_playerX, m_playerY, 0.45f, item.x, item.y, 0.0f);
        if (followsPlayer) {
            item.x += dx * 0.45f;
            item.y += dy * 0.45f;
            if (IsRailGameplayActive()) {
                item.z += (playerPosition.z - item.z) * 0.45f;
            }
        }
        const bool collected = IsRailGameplayActive() ?
            Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 0.52f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.38f) :
            Hit(m_playerX, m_playerY, 0.075f, item.x, item.y, 0.045f);
        if (!collected) continue;

        if (item.type == ItemType::Power) {
            const int previousPowerLevel = PowerLevel();
            m_power = (std::min)(MaxPower, m_power + item.power);
            if (PowerLevel() > previousPowerLevel) m_powerUpTimer = 120;
        } else {
            m_chapterResult.score += item.score;
            m_score += item.score;
        }
        item.active = false;
    }
}

void SideScrollingShooter::SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ) {
    constexpr float SideEnemyEntryX = 2.80f;

    if (enemyType == Stage::LinkedLaserEnemy) {
        Enemy* upper = nullptr;
        Enemy* lower = nullptr;
        for (auto& enemy : m_enemies) {
            if (enemy.active) continue;
            if (upper == nullptr) {
                upper = &enemy;
            } else {
                lower = &enemy;
                break;
            }
        }
        if (upper == nullptr || lower == nullptr) return;

        const int linkId = m_frame * EnemyCapacity + m_chapterResult.enemySpawnCount + 1;
        const bool upperRight = (linkId & 1) == 0;
        const float upperRailX = upperRight ?
            LinkedLaserEnemyBehavior::RightRailX() : LinkedLaserEnemyBehavior::LeftRailX();
        const float lowerRailX = upperRight ?
            LinkedLaserEnemyBehavior::LeftRailX() : LinkedLaserEnemyBehavior::RightRailX();
        auto ConfigureLinked = [&](Enemy& enemy, int role, float fixedY, float linkRailX) {
            enemy.active = true;
            m_stage->ConfigureEnemy(*this, enemy, enemyType, m_frame, m_kills, IsRailGameplayActive());
            enemy.entersFromTop = false;
            enemy.entersWallFromTop = false;
            ApplyDifficultyToEnemyHp(enemy);
            enemy.railAnchorX = linkRailX;
            enemy.baseX = IsRailGameplayActive() ? linkRailX : (std::max)(sideX, SideEnemyEntryX);
            enemy.x = enemy.baseX;
            enemy.baseY = fixedY;
            enemy.y = fixedY;
            enemy.z = IsRailGameplayActive() ? (std::max)(railZ, EnemyRailFarZ) : ToRailZFromSideX(enemy.x);
            enemy.laserLinkId = linkId;
            enemy.laserLinkRole = role;
        };
        ConfigureLinked(*upper, 1, LinkedLaserEnemyBehavior::UpperY(), upperRailX);
        ConfigureLinked(*lower, -1, LinkedLaserEnemyBehavior::LowerY(), lowerRailX);
        m_chapterResult.enemySpawnCount += 2;
        return;
    }

    for (auto& enemy : m_enemies) {
        if (enemy.active) continue;
        enemy.active = true;
        m_stage->ConfigureEnemy(*this, enemy, enemyType, m_frame, m_kills, IsRailGameplayActive());
        enemy.entersFromTop = false;
        enemy.entersWallFromTop = enemy.type == Stage::WallSecurityDroneEnemy &&
            IsRailGameplayActive();
        ApplyDifficultyToEnemyHp(enemy);

        // 初めて画面へ出現した通常敵を永続ギャラリーへ登録する
        switch (enemy.type) {
        case 0: UnlockGallery(GalleryEntry::LightEnemy); break;
        case 1: UnlockGallery(GalleryEntry::HeavyEnemy); break;
        case 4: UnlockGallery(GalleryEntry::ArmoredEnemy); break;
        case 10: UnlockGallery(GalleryEntry::WallSecurityDrone); break;
        default: break;
        }
        if (m_stageNumber >= 1 && m_stageNumber <= 4) {
            UnlockGallery(static_cast<GalleryEntry>(
                static_cast<std::uint32_t>(GalleryEntry::Stage1Enemy) + m_stageNumber - 1));
        }
        enemy.railAnchorX = railX;
        // 出現テーブルの座標に関わらず、敵機全体が表示領域外から入る位置に固定する
        enemy.baseX = IsRailGameplayActive() ? railX : (std::max)(sideX, SideEnemyEntryX);
        enemy.x = enemy.baseX;
        enemy.baseY = y;
        enemy.y = y;
        if (enemy.type == 7) {
            enemy.baseY = DiveRusherEnemyBehavior::HighY();
            enemy.y = enemy.baseY;
        }
        if (enemy.type == 8) {
            enemy.baseY = MissileShooterEnemyBehavior::LowY();
            enemy.y = enemy.baseY;
        }
        enemy.z = IsRailGameplayActive() ?
            (enemy.type == Stage::WallSecurityDroneEnemy ?
                WallSecurityDroneEnemyBehavior::WallSurfaceZ() :
                (std::max)(railZ, EnemyRailFarZ)) :
            ToRailZFromSideX(enemy.x);
        ++m_chapterResult.enemySpawnCount;
        return;
    }
}

/**
 * @brief 未解放の展示だけを永続データへ追加する
 * @param entry 解放する展示
 * @return なし
 */
void SideScrollingShooter::UnlockGallery(GalleryEntry entry) {
    const std::uint32_t bit = GalleryEntryBit(entry);
    if ((m_galleryUnlocks & bit) != 0u) return;

    // メモリ上のビットを先に更新して同じプレイ中の重複I/Oを防ぐ
    m_galleryUnlocks |= bit;
    SettingsRepository {}.UnlockGalleryEntry(entry);
}

void SideScrollingShooter::FireBossPartBarrage(const Enemy& boss) {

    constexpr float ModelScale = 0.14f;
    constexpr float PartX[] = { 0.0f, 17.0f, -17.0f, 6.0f, -6.0f };
    constexpr float PartY[] = { 3.0f, 2.0f, 2.0f, -6.0f, -6.0f };
    constexpr float PartZ[] = { -17.5f, 0.0f, 0.0f, 13.0f, 13.0f };
    const bool railMode = IsRailGameplayActive();
    bool fired = false;

    // 未破壊部位ごとに、ステージ定義の通常または特殊弾幕を発射する
    for (int part = 0; part < BossPartCount; ++part) {
        if (boss.bossPartHp[part] <= 0) continue;
        const int bulletCount = m_stage->BossPartBulletCount(
            static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), railMode);
        for (int index = 0; index < bulletCount; ++index) {
            const Stage::BossBullet bullet = m_stage->GetBossPartBullet(
                static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), index, railMode);
            const float x = railMode ? boss.x + PartX[part] * ModelScale / WorldXScale :
                boss.x + PartZ[part] * ModelScale / WorldXScale;
            const float y = boss.y + PartY[part] * ModelScale / WorldYScale;
            const float z = boss.z + PartZ[part] * ModelScale;
            SpawnShot(x + bullet.offsetX, y + bullet.offsetY, bullet.vx, bullet.vy, true,
                z, boss.behavior->RailAimedShotSpeed());
            fired = true;
        }
    }
    if (fired) PlayEnemyShotSound();
}

bool SideScrollingShooter::DamageBoss(Enemy& boss, int damage) {
    // Stage4主砲交換中は、副砲破壊処理から直接与えるダメージ以外を無効化する
    if (m_stageNumber == 4 && Stage4Module::IsWeaponSwapActive(*this)) return false;
    boss.hp -= damage;
    m_bossHp = (std::max)(0, boss.hp);
    if (m_stageNumber == 4) {
        return Stage4Module::HandleBossPhaseAfterDamage(*this, boss);
    }
    const int nextPhase = m_stage->BossPhaseForHp(boss.hp, boss.maxHp);
    if (nextPhase != boss.bossPhase) {
        boss.bossPhase = nextPhase;
        // フェーズ切り替え時は画面上の敵弾を消して次の弾幕を読みやすくする
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
    }
    return boss.hp <= 0;
}

void SideScrollingShooter::DefeatBoss(Enemy& boss) {
    if (!boss.active) return;
    if (StageDispatch::HandleBossDefeat(*this, boss)) return;

    // 共通ボス撃破処理を使うStage 1から4を対応する展示へ登録する
    constexpr GalleryEntry BossEntries[] = {
        GalleryEntry::Stage1Boss,
        GalleryEntry::Stage2Boss,
        GalleryEntry::Stage3Boss,
        GalleryEntry::Stage4Boss
    };
    if (m_stageNumber >= 1 && m_stageNumber <= 4) {
        UnlockGallery(BossEntries[m_stageNumber - 1]);
        if (m_stageNumber == 3) {
            UnlockGallery(GalleryEntry::Stage3BarrierFunnel);
            UnlockGallery(GalleryEntry::Stage3ReflectFunnel);
        }
        if (m_stageNumber == 4) UnlockGallery(GalleryEntry::Stage4WeaponDrone);
    }
    SpawnExplosion(boss.x, boss.y, boss.z, true);
    SpawnEnemyDebris(boss);
    boss.active = false;
    SpawnPowerItem(boss.x, boss.y, boss.z, 1.00f);
    m_bossHp = 0;
    m_score += 5000;
    m_clear = true;
    m_clearTimer = ClearWaitFrames;
}

void SideScrollingShooter::SpawnShot(float x, float y, float vx, float vy, bool enemy,
    float z, float railSpeed, int damage) {
    for (int shotIndex = 0; shotIndex < ActiveShotCapacity(); ++shotIndex) {
        auto& shot = m_shots[shotIndex];
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        const bool verticalRoute = !enemy &&
            UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
        shot.z = IsRailGameplayActive() ? (z >= 0.0f ? z : PlayerRailZ + 2.0f) :
            ToRailZFromSideX(x);
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.vz = 0.0f;
        shot.damage = damage;
        if (!enemy && IsTayamaBattle()) {
            // 機首位置からボス中心へ向け、周回角や2D固定面に依存しない直進弾を生成する
            const Vector3 player = PlayerWorldPosition();
            const float dx = -player.x;
            const float dz = ShooterStages::Stage5::TayamaArenaCenterZ - player.z;
            const float length = (std::max)(0.001f, std::sqrt(dx * dx + dz * dz));
            shot.x = FromWorldX(player.x + dx / length * 2.0f);
            shot.y = FromWorldY(player.y);
            shot.z = player.z + dz / length * 2.0f;
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            shot.vx = FromWorldX(dx / length * 1.45f);
            shot.vy = 0.0f;
            shot.vz = dz / length * 1.45f;
        }
        if (IsRailGameplayActive() && !verticalRoute) {
            if (enemy) {
                const Vector3 player = PlayerWorldPosition();
                const float targetX = IsTayamaBattle() ? FromWorldX(player.x) : m_playerX + vx * 12.0f;
                const float targetY = IsTayamaBattle() ? FromWorldY(player.y) : m_playerY + vy * 12.0f;
                const float targetZ = IsTayamaBattle() ? player.z : PlayerRailZ;
                const float dx = ToWorldX(targetX) - ToWorldX(x);
                const float dy = ToWorldY(targetY) - ToWorldY(y);
                const float dz = targetZ - shot.z;
                const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                const float EnemyShotSpeed = railSpeed >= 0.0f ? railSpeed : 0.62f;
                shot.vx = FromWorldX(dx / length * EnemyShotSpeed);
                shot.vy = FromWorldY(dy / length * EnemyShotSpeed);
                shot.vz = dz / length * EnemyShotSpeed;
            } else if (!IsTayamaBattle()) {
                shot.vx = 0.0f;
                shot.vy = 0.0f;
                shot.vz = 1.45f;
            }
        }
        if (enemy && !IsRailGameplayActive() && m_stageNumber == 5 &&
            ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase)) {
            AimShotGroundward(shot.vx, shot.vy);
        }
        shot.enemy = enemy;
        shot.active = true;
        return;
    }
}

/**
 * @brief 指定座標にPowerアイテムを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @param value 取得時に加算するPower
 */
void SideScrollingShooter::SpawnPowerItem(float x, float y, float z, float value) {
    for (auto& item : m_items) {
        if (item.active) continue;
        item = {};
        item.x = x;
        item.y = y;
        item.z = IsRailGameplayActive() ? z : ToRailZFromSideX(x);
        item.power = value;
        item.type = ItemType::Power;
        item.active = true;
        return;
    }
}

/**
 * @brief 指定座標にScoreアイテムを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @param value 取得時に加算するScore
 */
void SideScrollingShooter::SpawnScoreItem(float x, float y, float z, int value) {
    for (auto& item : m_items) {
        if (item.active) continue;
        item = {};
        item.x = x;
        item.y = y;
        item.z = IsRailGameplayActive() ? z : ToRailZFromSideX(x);
        item.score = value;
        item.type = ItemType::Score;
        item.active = true;
        return;
    }
}

/**
 * @brief XYZ速度を指定して固定長プールへ弾を生成する
 * @param x 発射元ゲーム座標X
 * @param y 発射元ゲーム座標Y
 * @param z 発射元レール座標Z
 * @param vx ゲーム座標X方向速度
 * @param vy ゲーム座標Y方向速度
 * @param vz レール座標Z方向速度
 * @param enemy 敵弾の場合true
 * @param barrageIndex 弾幕内の弾番号
 * @param barrageCount 弾幕の総弾数
 * @param firedByBoss ボスが発射した弾の場合true
 * @return なし
 */
void SideScrollingShooter::SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy,
    int barrageIndex, int barrageCount, bool firedByBoss) {
    Shot* available = nullptr;
    for (auto& shot : m_shots) {
        if (!shot.active) {
            available = &shot;
            break;
        }
    }

    // ステージ側が予約済み攻撃の欠落回避を要求した場合だけ古い自機弾を置換する
    if (!available && StageDispatch::CanReplacePlayerShot(*this, enemy)) {
        for (auto& shot : m_shots) {
            if (!shot.enemy) {
                available = &shot;
                break;
            }
        }
    }
    if (!available) return;

    Shot& shot = *available;
    shot = {};
    shot.x = x;
    shot.y = y;
    shot.z = z;
    shot.transitionSideX = x;
    shot.transitionSideY = y;
    shot.vx = vx;
    shot.vy = vy;
    shot.vz = vz;
    if (enemy && !IsRailGameplayActive() && m_stageNumber == 5 &&
        ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase)) {
        AimShotGroundward(shot.vx, shot.vy);
    }
    shot.barrageIndex = barrageIndex;
    shot.barrageCount = barrageCount;
    shot.enemy = enemy;
    shot.firedByBoss = firedByBoss;
    shot.active = true;
}



/** @brief 選択中の機体タイプに対応する特殊弾を生成する */
void SideScrollingShooter::FireSpecialShots() {
    const auto& config = PlayerShotConfigs[static_cast<size_t>(m_playerType)];
    const int powerLevel = PowerLevel();
    const int projectileCount = m_playerType == Spread ? config.projectileCount + powerLevel : config.projectileCount + powerLevel;
    const int damage = config.damage;
    constexpr float DegreesToRadians = 3.1415926535f / 180.0f;

    /** @brief 弾数に応じて左右対称の角度と発射位置を求める */
    for (int i = 0; i < projectileCount; ++i) {
        const float centeredIndex = static_cast<float>(i) -
            static_cast<float>(projectileCount - 1) * 0.5f;
        const float angleStep = projectileCount > 1
            ? config.spreadAngleDegrees / static_cast<float>(projectileCount - 1)
            : 0.0f;
        const float angle = centeredIndex * angleStep * DegreesToRadians;
        const bool railGameplay = IsRailGameplayActive();
        const bool verticalRoute = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
        const float spawnY = verticalRoute ? m_playerY + config.spawnOffsetX :
            (railGameplay ? m_playerY :
                m_playerY + centeredIndex * config.spawnOffsetY);
        const float railSpawnOffsetX = config.spawnOffsetY > 0.0f ? config.spawnOffsetY : 0.05f;

        /** @brief 空きスロットへ機体タイプ固有の属性を設定する */
        for (int shotIndex = 0; shotIndex < ActiveShotCapacity(); ++shotIndex) {
            auto& shot = m_shots[shotIndex];
            if (shot.active) continue;
            shot = {};
            if (IsTayamaBattle()) {
                // ボス方向とその接線を基準に、周回位置から左右対称の弾道を作る
                const Vector3 player = PlayerWorldPosition();
                const float forwardX = -player.x;
                const float forwardZ = ShooterStages::Stage5::TayamaArenaCenterZ - player.z;
                const float forwardLength = (std::max)(0.001f,
                    std::sqrt(forwardX * forwardX + forwardZ * forwardZ));
                const Vector3 forward {forwardX / forwardLength, 0.0f, forwardZ / forwardLength};
                const Vector3 right {forward.z, 0.0f, -forward.x};
                const Vector3 origin = player + forward * 2.0f +
                    right * (centeredIndex * railSpawnOffsetX * WorldXScale);
                const Vector3 velocity = forward * (std::cos(angle) * 1.45f) +
                    right * (std::sin(angle) * 1.45f);
                shot.x = FromWorldX(origin.x);
                shot.y = FromWorldY(origin.y);
                shot.z = origin.z;
                shot.transitionSideX = shot.x;
                shot.transitionSideY = shot.y;
                shot.vx = FromWorldX(velocity.x);
                shot.vy = 0.0f;
                shot.vz = velocity.z;
            } else {
                // 3Dレールでは翼の左右から、2Dでは従来どおり機首の上下から発射する
                shot.x = verticalRoute ? m_playerX + centeredIndex * config.spawnOffsetY :
                    (railGameplay ? m_playerX + centeredIndex * railSpawnOffsetX :
                        m_playerX + config.spawnOffsetX);
                shot.y = spawnY;
                shot.z = railGameplay ? PlayerRailZ + 2.0f : ToRailZFromSideX(shot.x);
                shot.transitionSideX = shot.x;
                shot.transitionSideY = shot.y;
                if (verticalRoute) {
                    // Stage 5第2部は視点に関わらず画面上方向を基準に拡散する
                    shot.vx = std::sin(angle) * config.speed;
                    shot.vy = std::cos(angle) * config.speed;
                    shot.vz = 0.0f;
                } else if (railGameplay) {
                    /** @brief 3Dレールでは特殊弾を奥行き方向へ進ませ、拡散角を横移動へ適用する */
                    shot.vx = std::sin(angle) * config.speed;
                    shot.vy = 0.0f;
                    shot.vz = 1.45f;
                } else {
                    // 通常の2Dでは画面右方向を基準に拡散する
                    shot.vx = std::cos(angle) * config.speed;
                    shot.vy = std::sin(angle) * config.speed;
                    shot.vz = 0.0f;
                }
            }
            shot.hitRadius = config.hitRadius;
            shot.damage = damage;
            shot.playerType = m_playerType;
            shot.special = true;
            shot.piercing = config.piercing;
            shot.active = true;
            break;
        }
    }
}

/**
 * @brief 追尾弾をHPが最低で同HPなら自機に近い前方敵へ向ける
 * @param shot 更新する追尾弾
 * @return なし
 */
void SideScrollingShooter::UpdateHomingShot(Shot& shot) {
    const bool verticalRoute = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    if (IsRailGameplayActive() && !verticalRoute) {
        const Enemy* target = nullptr;
        float targetDistanceSquared = 0.0f;
        float targetPlayerDistanceSquared = 0.0f;
        const Vector3 player = PlayerWorldPosition();

        // 3Dレールでは奥行き方向の前方からHP最低、同HPなら自機に近い敵を選ぶ
        for (const auto& enemy : m_enemies) {
            if (!enemy.active || enemy.hp <= 0 || enemy.z <= shot.z) continue;
            const float dx = ToWorldX(enemy.x - shot.x);
            const float dy = ToWorldY(enemy.y - shot.y);
            const float dz = enemy.z - shot.z;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared >= 10000.0f) continue;
            const float playerDx = ToWorldX(enemy.x) - player.x;
            const float playerDy = ToWorldY(enemy.y) - player.y;
            const float playerDz = enemy.z - player.z;
            const float playerDistanceSquared =
                playerDx * playerDx + playerDy * playerDy + playerDz * playerDz;
            if (target == nullptr || IsPreferredHomingTarget(
                enemy.hp, playerDistanceSquared, target->hp, targetPlayerDistanceSquared)) {
                targetDistanceSquared = distanceSquared;
                targetPlayerDistanceSquared = playerDistanceSquared;
                target = &enemy;
            }
        }
        if (target == nullptr || targetDistanceSquared <= 0.000001f) return;

        // ワールド空間で旋回量を補間して、レール弾速を維持する
        const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
        const float inverseDistance = 1.0f / std::sqrt(targetDistanceSquared);
        const float speed = 1.45f;
        float vx = ToWorldX(shot.vx);
        float vy = ToWorldY(shot.vy);
        float vz = shot.vz;
        vx += ((ToWorldX(target->x - shot.x) * inverseDistance * speed) - vx) * config.homingStrength;
        vy += ((ToWorldY(target->y - shot.y) * inverseDistance * speed) - vy) * config.homingStrength;
        vz += (((target->z - shot.z) * inverseDistance * speed) - vz) * config.homingStrength;
        const float velocityLength = std::sqrt(vx * vx + vy * vy + vz * vz);
        if (velocityLength > 0.000001f) {
            shot.vx = FromWorldX(vx / velocityLength * speed);
            shot.vy = FromWorldY(vy / velocityLength * speed);
            shot.vz = vz / velocityLength * speed;
        }
        return;
    }

    const Enemy* target = nullptr;
    float targetDistanceSquared = 0.0f;
    float targetPlayerDistanceSquared = 0.0f;

    // 現在の2D進行方向からHP最低、同HPなら自機に近い敵を選ぶ
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.hp <= 0 ||
            (verticalRoute ? enemy.y <= shot.y : enemy.x <= shot.x)) continue;
        const float dx = enemy.x - shot.x;
        const float dy = enemy.y - shot.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared >= 100.0f) continue;
        const float playerDx = enemy.x - m_playerX;
        const float playerDy = enemy.y - m_playerY;
        const float playerDistanceSquared = playerDx * playerDx + playerDy * playerDy;
        if (target == nullptr || IsPreferredHomingTarget(
            enemy.hp, playerDistanceSquared, target->hp, targetPlayerDistanceSquared)) {
            targetDistanceSquared = distanceSquared;
            targetPlayerDistanceSquared = playerDistanceSquared;
            target = &enemy;
        }
    }
    if (target == nullptr || targetDistanceSquared <= 0.000001f) return;

    /** @brief 現在速度と目標方向を補間して速度を一定に保つ */
    const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
    const float inverseDistance = 1.0f / std::sqrt(targetDistanceSquared);
    const float desiredVx = (target->x - shot.x) * inverseDistance * config.speed;
    const float desiredVy = (target->y - shot.y) * inverseDistance * config.speed;
    shot.vx += (desiredVx - shot.vx) * config.homingStrength;
    shot.vy += (desiredVy - shot.vy) * config.homingStrength;
    const float velocityLength = std::sqrt(shot.vx * shot.vx + shot.vy * shot.vy);
    if (velocityLength > 0.000001f) {
        shot.vx = shot.vx / velocityLength * config.speed;
        shot.vy = shot.vy / velocityLength * config.speed;
    }
}

bool SideScrollingShooter::TryHitBossPart(
    const Shot& shot, const Enemy& boss, BossPart& part) const {
    return StageDispatch::TryHitBossPart(*this, shot, boss, part);
}

bool SideScrollingShooter::TryHitDefaultBossPart(
    const Shot& shot, const Enemy& boss, BossPart& part) const {


    // 既存ボスモデルのローカル座標に対応する、破壊可能部位の中心と当たり判定半径
    constexpr float ModelScale = 0.14f;
    constexpr float PartX[] = { 0.0f, 17.0f, -17.0f, 6.0f, -6.0f };
    constexpr float PartY[] = { 3.0f, 2.0f, 2.0f, -6.0f, -6.0f };
    constexpr float PartZ[] = { -17.5f, 0.0f, 0.0f, 13.0f, 13.0f };
    constexpr float PartRadius[] = { 0.50f, 1.20f, 1.20f, 0.58f, 0.58f };

    for (int i = 0; i < BossPartCount; ++i) {
        if (boss.bossPartHp[i] <= 0) continue;
        if (IsRailGameplayActive()) {
            const float partX = ToWorldX(boss.x) + PartX[i] * ModelScale;
            const float partY = ToWorldY(boss.y) + PartY[i] * ModelScale;
            const float partZ = boss.z + PartZ[i] * ModelScale;
            if (!Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                partX, partY, partZ, PartRadius[i])) {
                continue;
            }
        } else {
            // 2D表示ではY軸回転済みモデルの奥行きを画面X座標へ投影する
            const float partX = boss.x + PartZ[i] * ModelScale / WorldXScale;
            const float partY = boss.y + PartY[i] * ModelScale / WorldYScale;
            if (!Hit(shot.x, shot.y, shot.hitRadius, partX, partY,
                PartRadius[i] / WorldXScale)) {
                continue;
            }
        }
        part = static_cast<BossPart>(i);
        return true;
    }
    return false;
}

void SideScrollingShooter::PlayShotSound() {
    // 自機ショットの音量調整 (0.0f ~ 1.0f)
    constexpr float PlayerShotVolume = 0.2f;
    if (m_audio) m_audio->PlayMMLSE("t240 o6 l32 v7 c>c", PlayerShotVolume);
}

void SideScrollingShooter::PlayHitSound() {
    if (m_audio) m_audio->PlayMMLSE("t180 o4 l32 v10 g e c");
}

/**
 * @brief 敵のエネルギー弾発射音を再生する
 */
void SideScrollingShooter::PlayEnemyShotSound() {
    if (!m_audio) return;

    // 敵通常弾・大型弾の発砲音
    static const std::vector<int16_t> pcm = [] {
        // パルス成分の生成
        Audio::SfxrParams pulse;
        pulse.waveType = Audio::SfxrWaveType::Square;
        pulse.squareDuty = 0.30f;
        pulse.attackTime = 0.0f;
        pulse.sustainTime = 0.025f;
        pulse.decayTime = 0.075f;
        pulse.startFrequency = 0.72f;
        pulse.minFrequency = 0.18f;
        pulse.slide = -0.65f;
        pulse.masterVolume = 0.60f;
        const std::vector<int16_t> pcmPulse = Audio::SfxrGenerator::GeneratePCM(pulse, 44100);

        // 鋸波成分の生成
        Audio::SfxrParams beam;
        beam.waveType = Audio::SfxrWaveType::Sawtooth;
        beam.attackTime = 0.002f;
        beam.sustainTime = 0.060f;
        beam.decayTime = 0.160f;
        beam.startFrequency = 0.58f;
        beam.minFrequency = 0.12f;
        beam.slide = -0.50f;
        beam.masterVolume = 0.72f;
        const std::vector<int16_t> pcmBeam = Audio::SfxrGenerator::GeneratePCM(beam, 44100);

        // 2つの波形を加算合成
        const size_t totalSamples = (std::max)(pcmPulse.size(), pcmBeam.size());
        std::vector<int16_t> mixed(totalSamples, 0);
        for (size_t i = 0; i < totalSamples; ++i) {
            int32_t sample = 0;
            if (i < pcmPulse.size()) sample += pcmPulse[i];
            if (i < pcmBeam.size()) sample += pcmBeam[i];
            mixed[i] = static_cast<int16_t>(std::clamp(sample, -32760, 32760));
        }
        return mixed;
    }();
    m_audio->PlaySE(pcm, 1.20f);
}

void SideScrollingShooter::PlayMissileLaunchSound() {
    if (!m_audio) return;

    // 低域を含まない高周波サイン波を緩く下降させて鋭い噴射音を作る
    Audio::SfxrParams sound;
    sound.waveType = Audio::SfxrWaveType::Sine;
    sound.attackTime = 0.015f;
    sound.sustainTime = 0.34f;
    sound.decayTime = 0.26f;
    sound.startFrequency = 2.20f;
    sound.minFrequency = 1.35f;
    sound.slide = -0.08f;
    sound.masterVolume = 0.46f;
    m_audio->PlaySE(sound);
}

void SideScrollingShooter::PlayBossMachineGunSound() {
    if (!m_audio) return;

    // 通常敵の射撃音
    static const std::vector<int16_t> pcm = [] {
        // アタックパルスノイズ成分の生成
        Audio::SfxrParams crack;
        crack.waveType = Audio::SfxrWaveType::Noise;
        crack.attackTime = 0.0f;
        crack.sustainTime = 0.018f;
        crack.decayTime = 0.045f;
        crack.startFrequency = 0.75f;
        crack.minFrequency = 0.25f;
        crack.slide = -0.70f;
        crack.masterVolume = 0.65f;
        const std::vector<int16_t> pcmCrack = Audio::SfxrGenerator::GeneratePCM(crack, 44100);

        // 鋸波成分の生成
        Audio::SfxrParams core;
        core.waveType = Audio::SfxrWaveType::Sawtooth;
        core.attackTime = 0.0f;
        core.sustainTime = 0.045f;
        core.decayTime = 0.120f;
        core.startFrequency = 0.54f;
        core.minFrequency = 0.14f;
        core.slide = -0.58f;
        core.masterVolume = 0.75f;
        const std::vector<int16_t> pcmCore = Audio::SfxrGenerator::GeneratePCM(core, 44100);

        // 2つの波形を加算合成
        const size_t totalSamples = (std::max)(pcmCrack.size(), pcmCore.size());
        std::vector<int16_t> mixed(totalSamples, 0);
        for (size_t i = 0; i < totalSamples; ++i) {
            int32_t sample = 0;
            if (i < pcmCrack.size()) sample += pcmCrack[i];
            if (i < pcmCore.size()) sample += pcmCore[i];
            mixed[i] = static_cast<int16_t>(std::clamp(sample, -32760, 32760));
        }
        return mixed;
    }();
    m_audio->PlaySE(pcm, 1.25f);
}

/** @brief 生存中の爆発エフェクトを更新する @return なし */
void SideScrollingShooter::TickExplosions() {
    for (auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        if (explosion.effectType == 1 && !explosion.damagedPlayer &&
            explosion.age <= AttackWarningFrames && m_invincible == 0) {
            const Vector3 player = PlayerWorldPosition();
            const float depthDistance = std::abs(player.z - explosion.z);
            const bool playerHit = IsRailGameplayActive() ?
                depthDistance <= MortarExplosionDepthHitRadius &&
                    Hit(player.x, player.y, 0.38f,
                        ToWorldX(explosion.x), ToWorldY(explosion.y),
                        explosion.hitRadius * WorldXScale) :
                Hit(m_playerX, m_playerY, 0.050f, explosion.x, explosion.y,
                    explosion.hitRadius);
            if (playerHit) {
                explosion.damagedPlayer = true;
                DamagePlayer();
                return;
            }
        }
        const int lifetime = explosion.effectType == BombExplosionEffectType ?
            BombExplosionLifetimeFrames : explosion.effectType == 1 ? MortarExplosionLifetimeFrames :
            (explosion.destruction ? DestructionExplosionLifetimeFrames : ExplosionLifetimeFrames);
        if (++explosion.age >= lifetime) explosion.active = false;
    }
}

/** @brief 飛散中の機体部品を固定長プール順に更新する @return なし */
void SideScrollingShooter::TickDebris() {
    for (auto& debris : m_debris) {
        if (!debris.active || StageDispatch::TickSpecialDebris(*this, debris)) continue;
        debris.x += debris.vx;
        debris.y += debris.vy;
        debris.z += debris.vz;
        if (debris.gravity || m_stage->HasDebrisGravity()) debris.vy -= 0.006f;
        debris.yaw += debris.spin;
        if (++debris.age >= debris.lifetime) debris.active = false;
    }
}

/**
 * @brief 弾の命中位置へ爆発エフェクトを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @param destruction 敵撃破用の大爆発を生成する場合true
 * @return なし
 */
void SideScrollingShooter::SpawnExplosion(
    float x, float y, float z, bool destruction) {
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {
            x, y, IsRailGameplayActive() ? z : ToRailZFromSideX(x),
            0, destruction, true
        };
        return;
    }
}

/**
 * @brief 迫撃砲着弾用の大爆破エフェクトを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @param hitRadius 爆破当たり判定半径
 * @return なし
 */
void SideScrollingShooter::SpawnMortarExplosion(float x, float y, float z, float hitRadius) {
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {
            x, y, IsRailGameplayActive() ? z : ToRailZFromSideX(x),
            0, false, true, 1, hitRadius
        };
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
 * @return 生成したデブリ、プール満杯の場合nullptr
 */
SideScrollingShooter::Debris* SideScrollingShooter::SpawnDebrisPiece(
    float x, float y, float z, float vx, float vy, float vz,
    float yaw, float spin, int shape, float width, float height, float depth,
    const float color[4], int lifetime, int shrinkStartAge, bool gravity) {
    for (auto& debris : m_debris) {
        if (debris.active) continue;
        debris = {x, y, z, vx, vy, vz, yaw, spin, width, height, depth,
            {color[0], color[1], color[2], color[3]}, shape, 0, lifetime,
            shrinkStartAge, {}, gravity, true};
        return &debris;
    }
    return nullptr;
}
