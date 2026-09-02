#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::Stage2BossApproachFrames;
using SideScrollingShooterShared::Stage2BossAssemblyFrames;

constexpr int Phase3FunnelEngineStartFrame = 26;
constexpr float Phase3FunnelLaunchVelocity = 0.09f;
constexpr float Phase3FunnelGravity = 0.0035f;
constexpr float Phase3FunnelRise = Phase3FunnelEngineStartFrame * Phase3FunnelLaunchVelocity -
    Phase3FunnelGravity * Phase3FunnelEngineStartFrame * (Phase3FunnelEngineStartFrame + 1) * 0.5f;
static_assert(Phase3FunnelRise > 0.63f);
static_assert(Phase3FunnelLaunchVelocity -
    Phase3FunnelGravity * Phase3FunnelEngineStartFrame < 0.0f);

}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"
#include "Stage5ModelView.h"

void SideScrollingShooter::TickPlayer() {
    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    /** @brief 2D画面ではHUDを除くプレイ領域全体を移動可能にする */
    const float minX = IsRailGameplayActive() ? -1.2f : Side2DPlayerMinX;
    const float maxX = IsRailGameplayActive() ? 1.2f : Side2DPlayerMaxX;
    const float minY = IsRailGameplayActive() ? PlayerRailMinY() : Side2DPlayerMinY;
    const float maxY = IsRailGameplayActive() ? 0.9f : Side2DPlayerMaxY;
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f, minX, maxX);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f, minY, maxY);
}

void SideScrollingShooter::TickEnemies() {
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
        } else {
            enemy.behavior->Tick(*this, enemy);
        }

        // Phase3レールガンは発射フレームの一瞬だけ固定照準線へ当たり判定を通す
        const int beamCycle = enemy.stage2BossActionAge % Stage2RailgunCycleFrames;
        if (enemy.type == 2 && m_stageNumber == 2 && enemy.phase >= 3.0f &&
            enemy.bossPartHp[BossNose] > 0 && beamCycle == Stage2RailgunFireFrame && m_invincible == 0) {
            constexpr float BossScale = 1.92f;
            const float yaw = IsRailGameplayActive() ? 0.0f : Math::HalfPi;
            const float cosine = std::cos(yaw);
            const float sine = std::sin(yaw);
            const float patrolX = std::sin(static_cast<float>(enemy.age) * 0.018f) * 2.4f * RailBlend();
            const float battleshipX = ToWorldX(enemy.x + enemy.landBattleshipOffsetX) + patrolX;
            const float battleshipY = Stage2BattleshipWorldY(enemy);
            const float battleshipZ = enemy.z + enemy.landBattleshipOffsetZ;
            const float startX = battleshipX - 1.55f * cosine * BossScale;
            const float startY = battleshipY + 2.08f * BossScale;
            const float startZ = battleshipZ + 1.55f * sine * BossScale;
            const float targetX = ToWorldX(enemy.actionX);
            const float targetY = ToWorldY(enemy.actionY);
            const float targetZ = IsRailGameplayActive() ? enemy.actionZ : SidePlaneZ;
            const Vector3 direction = Vector3 {targetX - startX, targetY - startY, targetZ - startZ}.Normalized();
            if (Hit3DSegment(startX, startY, startZ,
                targetX + direction.x * 18.0f, targetY + direction.y * 18.0f,
                targetZ + direction.z * 18.0f, 0.52f,
                ToWorldX(m_playerX), ToWorldY(m_playerY), IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ, 0.38f)) {
                DamagePlayer();
                return;
            }
        }

        // Phase3は上部戦艦の船体だけで骨アーチとの接触を判定する
        if (enemy.type == 2 && m_stageNumber == 2 && enemy.phase >= 3.0f && !m_boneArchDestroyed) {
            constexpr float BodyLocalX = 0.55f;
            constexpr float BodyLocalY = 0.95f;
            constexpr float ModelScale = 1.92f;
            constexpr float BodyRadius = 4.25f;
            const float yaw = IsRailGameplayActive() ? 0.0f : Math::HalfPi;
            const float patrolWorldX = std::sin(static_cast<float>(enemy.age) * 0.018f) *
                2.4f * RailBlend();
            const Vector3 bodyCenter {
                ToWorldX(enemy.x + enemy.landBattleshipOffsetX) + patrolWorldX +
                    std::cos(yaw) * BodyLocalX * ModelScale,
                Stage2BattleshipWorldY(enemy) + BodyLocalY * ModelScale,
                enemy.z + enemy.landBattleshipOffsetZ - std::sin(yaw) * BodyLocalX * ModelScale
            };
            if (HitsDesertBoneArch(FromWorldX(bodyCenter.x), FromWorldY(bodyCenter.y),
                bodyCenter.z, BodyRadius / WorldXScale)) {
                DestroyDesertBoneArch();
                SpawnExplosion(FromWorldX(bodyCenter.x), FromWorldY(bodyCenter.y), bodyCenter.z);
                PlayHitSound();
            }
        }

        // 巨大障害物へ接触した通常敵はスコアやアイテムを発生させず、その場で破壊する
        // ボス(type 2)は対象外
        const float boneHitRadius = IsRailGameplayActive() ? enemy.behavior->CollisionRadius3D(enemy) / WorldXScale :
            enemy.behavior->CollisionRadius(enemy);
        if (enemy.type != 2 && ((m_stageNumber == 1 && HitsStage1Meteor(enemy.x, enemy.y, enemy.z, boneHitRadius)) ||
            (m_stageNumber == 2 && HitsDesertBoneArch(enemy.x, enemy.y, enemy.z, boneHitRadius)) ||
            (m_stageNumber == 3 && HitsOceanSeaSerpent(enemy.x, enemy.y, enemy.z, boneHitRadius)))) {
            SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
            SpawnEnemyDebris(enemy);
            enemy.active = false;
            continue;
        }

        if (enemy.attackWarningFrames > 0) --enemy.attackWarningFrames;
        const int aimedShotInterval = enemy.shotInterval;
        const bool canUseAimedShot = !(enemy.type == 2 && m_stage->IsBossSpecialAttackActive(enemy));
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
            }
        }

        // ボスは通常・特殊フェーズごとの間隔で、未破壊の各部位から弾幕を発射する
        if (enemy.type == 2 &&
            !m_stage->IsBossSpecialAttackActive(enemy) &&
            enemy.age % m_stage->BossAttackInterval(static_cast<BossPhase>(enemy.bossPhase)) == 0) {
            FireBossPartBarrage(enemy);
        }

        if (enemy.type != 2 && !IsRailGameplayActive() && enemy.x < -2.6f) enemy.active = false;
        if (enemy.type != 2 && IsRailGameplayActive() && enemy.z < 2.0f) enemy.active = false;
        if (enemy.type == 2 && !enemy.collisionEnabled) continue;
        const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
        const bool playerHit = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.42f,
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
}

void SideScrollingShooter::TickShots() {
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        if (shot.funnelDustAge >= 0) ++shot.funnelDustAge;
        const float previousY = shot.y;

        // Phase3ファンネルは短く自由落下してから補助エンジンで自機へ向かう
        if (shot.funnel) ++shot.age;
        if (shot.enemy && shot.funnelDelayedEngine) {
            constexpr int FunnelEngineStartFrame = Phase3FunnelEngineStartFrame;
            constexpr int FunnelLaunchCullGraceFrames = 45;
            constexpr float FunnelGravity = Phase3FunnelGravity;
            static_assert(FunnelEngineStartFrame < FunnelLaunchCullGraceFrames);
            if (shot.age < FunnelEngineStartFrame) {
                shot.vy -= FunnelGravity;
            } else {
                constexpr float FunnelSpeed = 0.72f;
                constexpr float EngineAcceleration = 0.085f;
                if (shot.age == FunnelEngineStartFrame) {
                    // 点火時の自機位置から進行方向を一度だけ固定する
                    const float dx = ToWorldX(m_playerX - shot.x);
                    const float dy = ToWorldY(m_playerY - shot.y);
                    const float dz = IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
                    const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                    shot.funnelEngineVx = FromWorldX(dx / length * FunnelSpeed);
                    shot.funnelEngineVy = FromWorldY(dy / length * FunnelSpeed);
                    shot.funnelEngineVz = dz / length * FunnelSpeed;
                }
                shot.vx += (shot.funnelEngineVx - shot.vx) * EngineAcceleration;
                shot.vy += (shot.funnelEngineVy - shot.vy) * EngineAcceleration;
                shot.vz += (shot.funnelEngineVz - shot.vz) * EngineAcceleration;
            }
        }
        // Phase2ファンネルは進路を変えず、上方向へ徐々に加速する
        if (shot.enemy && shot.funnel && !shot.funnelDelayedEngine) {
            shot.vy = (std::min)(0.115f, shot.vy + 0.0018f);
        }

        /** @brief 追尾弾を最寄りの前方敵へ旋回させる */
        if (!shot.enemy && shot.special && shot.playerType == Homing) {
            UpdateHomingShot(shot);
        }

        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
        if (shot.funnel && shot.funnelDustAge < 0) {
            const float groundY = FromWorldY(Math::Lerp(-6.0f, -3.65f, RailBlend()));
            if (previousY < groundY && shot.y >= groundY) {
                shot.funnelDustAge = 0;
                shot.funnelDustX = shot.x;
                shot.funnelDustY = groundY;
                shot.funnelDustZ = shot.z;
            }
        }
        if (!IsRailGameplayActive()) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        // ハッチから出た直後は船体外へ抜けるまで通常弾の画面外カリングを猶予する
        constexpr int FunnelLaunchCullGraceFrames = 45;
        const bool funnelLaunching = shot.funnel && shot.age <= FunnelLaunchCullGraceFrames;
        if (!funnelLaunching && !IsRailGameplayActive() &&
            (shot.x < Side2DPlayerMinX - Side2DShotCullMargin ||
                shot.x > Side2DPlayerMaxX + Side2DShotCullMargin ||
                shot.y < Side2DPlayerMinY - Side2DShotCullMargin ||
                shot.y > Side2DPlayerMaxY + Side2DShotCullMargin)) {
            shot.active = false;
            continue;
        }
        // 端から出る円形弾幕が生成直後に欠けないよう、弾のY消滅範囲だけ少し広げる
        if (!funnelLaunching && IsRailGameplayActive() && (shot.z < 0.0f || shot.z > 72.0f ||
            std::abs(shot.x) > 1.2f || std::abs(shot.y) > 1.24f)) {
            shot.active = false;
            continue;
        }

        if (shot.enemy) {
            const bool playerHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.38f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.funnel ? 0.42f : 0.28f) :
                Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y, shot.funnel ? 0.055f : 0.022f);
            const bool grazed = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.80f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.28f) :
                Hit(m_playerX, m_playerY, 0.140f, shot.x, shot.y, 0.022f);
            if (!playerHit && grazed && !shot.grazed) {
                shot.grazed = true;
                ++m_chapterResult.grazeCount;
            }
            if (m_invincible == 0 && playerHit) {
                shot.active = false;
                DamagePlayer();
                return;
            }
            continue;
        }

        if (m_stageNumber == 5 &&
            (TryDamageTayama(shot) || TryDamageWallSearchlight(shot))) continue;
        if (TryDamageStageGimmick(shot)) continue;

        for (auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            if (m_chapterResultActive && !enemy.collisionEnabled) continue;
            if (enemy.type == 2 && !enemy.collisionEnabled && m_stageNumber != 2) continue;
            if (enemy.behavior == nullptr) {
                enemy.behavior = &EnemyBehaviorForType(enemy.type);
            }
            BossPart hitPart = BossNose;
            if (enemy.type == 2 && TryHitBossPart(shot, enemy, hitPart)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                if (!shot.piercing) shot.active = false;
                enemy.bossPartHitFlashFrames[hitPart] = BossPartHitFlashFrames;
                enemy.bossPartHp[hitPart] -= shot.damage;
                if (enemy.bossPartHp[hitPart] <= 0) {
                    enemy.bossPartHp[hitPart] = 0;
                    SpawnEnemyDebris(enemy, hitPart);
                    const int partDamage = m_stage->BossPartBreakDamage(hitPart);
                    const bool bossDefeated = DamageBoss(enemy, partDamage);
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
            if (enemy.type == 2 && m_stageNumber == 2 && TryHitStage2BossBody(shot, enemy)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                if (!shot.piercing) shot.active = false;
                if (DamageBoss(enemy, shot.damage)) DefeatBoss(enemy);
                else m_bossHp = enemy.hp;
                PlayHitSound();
                break;
            }
            // Stage2のcollisionEnabledは潜航中の本体接触だけを無効化し、露出中の上部戦艦は部位判定を維持する
            if (enemy.type == 2 && !enemy.collisionEnabled) continue;
            const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
            const bool enemyHit = IsRailGameplayActive() ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.behavior->ShotHitRadius3D(enemy)) :
                Hit(shot.x, shot.y, shot.hitRadius, enemy.x, enemy.y, enemyRadius);
            if (!enemyHit) continue;
            SpawnExplosion(shot.x, shot.y, shot.z);
            if (!shot.piercing) shot.active = false;
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
    for (auto& item : m_items) {
        if (!item.active) continue;

        // 2Dでは左、3Dでは手前へスクロールする
        if (IsRailGameplayActive()) {
            item.z -= 0.28f;
        } else {
            item.x -= 0.012f;
            item.z = ToRailZFromSideX(item.x);
        }

        // 画面外へ出たアイテムを破棄する
        if ((!IsRailGameplayActive() &&
                (item.x < Side2DPlayerMinX || item.x > Side2DPlayerMaxX ||
                    item.y < Side2DPlayerMinY || item.y > Side2DPlayerMaxY)) ||
            (IsRailGameplayActive() &&
                (item.z < 0.0f || item.z > 72.0f || std::abs(item.x) > 1.2f || std::abs(item.y) > 1.24f))) {
            item.active = false;
            continue;
        }

        // 自機が近づいたアイテムだけを強く追尾させる
        const float dx = m_playerX - item.x;
        const float dy = m_playerY - item.y;
        const bool followsPlayer = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 3.5f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.0f) :
            Hit(m_playerX, m_playerY, 0.45f, item.x, item.y, 0.0f);
        if (followsPlayer) {
            item.x += dx * 0.45f;
            item.y += dy * 0.45f;
            if (IsRailGameplayActive()) {
                item.z += (PlayerRailZ - item.z) * 0.45f;
            }
        }
        const bool collected = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.52f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.38f) :
            Hit(m_playerX, m_playerY, 0.075f, item.x, item.y, 0.045f);
        if (!collected) continue;

        if (item.type == ItemType::Power) {
            m_power = (std::min)(MaxPower, m_power + item.power);
        } else {
            m_chapterResult.score += item.score;
            m_score += item.score;
        }
        item.active = false;
    }
}

void SideScrollingShooter::SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ) {
    constexpr float SideEnemyEntryX = 2.80f;

    for (auto& enemy : m_enemies) {
        if (enemy.active) continue;
        enemy.active = true;
        m_stage->ConfigureEnemy(*this, enemy, enemyType, m_frame, m_kills, IsRailGameplayActive());
        enemy.railAnchorX = railX;
        // 出現テーブルの座標に関わらず、敵機全体が表示領域外から入る位置に固定する
        enemy.baseX = IsRailGameplayActive() ? railX : (std::max)(sideX, SideEnemyEntryX);
        enemy.x = enemy.baseX;
        enemy.baseY = y;
        enemy.y = y;
        enemy.z = IsRailGameplayActive() ? (std::max)(railZ, EnemyRailFarZ) : ToRailZFromSideX(enemy.x);
        ++m_chapterResult.enemySpawnCount;
        return;
    }
}

void SideScrollingShooter::FireBossPartBarrage(const Enemy& boss) {
    if (m_stageNumber == 2) {
        constexpr float ModelScale = 1.92f;
        constexpr Vector3 PartPosition[] = {
            {-2.30f, 2.08f, 0.0f}, {-0.55f, 2.60f, -0.92f},
            {0.65f, 2.60f, 0.0f}, {2.85f, 2.18f, 0.92f}, {0.25f, 1.42f, 0.0f}
        };
        const bool railMode = IsRailGameplayActive();
        const float yaw = railMode ? 0.0f : Math::HalfPi;
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        const float patrolX = boss.phase >= 2.0f ?
            std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * Math::Clamp01(1.0f - yaw / Math::HalfPi) : 0.0f;
        const Vector3 battleshipPosition {
            ToWorldX(boss.x + boss.landBattleshipOffsetX) + patrolX,
            Stage2BattleshipWorldY(boss),
            boss.z + boss.landBattleshipOffsetZ
        };

        // 毎フレーム移動する上部戦艦の描画位置へローカル砲塔座標を合成する
        for (int part = 0; part < BossRightEngine; ++part) {
            if (boss.bossPartHp[part] <= 0) continue;
            const Vector3& local = PartPosition[part];
            const Vector3 world {
                battleshipPosition.x + (local.x * cosine + local.z * sine) * ModelScale,
                battleshipPosition.y + local.y * ModelScale,
                battleshipPosition.z + (-local.x * sine + local.z * cosine) * ModelScale
            };
            const int bulletCount = m_stage->BossPartBulletCount(
                static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), railMode);
            for (int index = 0; index < bulletCount; ++index) {
                const Stage::BossBullet bullet = m_stage->GetBossPartBullet(
                    static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), index, railMode);
                SpawnShot(FromWorldX(world.x) + bullet.offsetX, FromWorldY(world.y) + bullet.offsetY,
                    bullet.vx, bullet.vy, true, world.z, boss.behavior->RailAimedShotSpeed());
            }
        }
        return;
    }

    constexpr float ModelScale = 0.14f;
    constexpr float PartX[] = { 0.0f, 17.0f, -17.0f, 6.0f, -6.0f };
    constexpr float PartY[] = { 3.0f, 2.0f, 2.0f, -6.0f, -6.0f };
    constexpr float PartZ[] = { -17.5f, 0.0f, 0.0f, 13.0f, 13.0f };
    const bool railMode = IsRailGameplayActive();

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
        }
    }
}

bool SideScrollingShooter::DamageBoss(Enemy& boss, int damage) {
    boss.hp -= damage;
    m_bossHp = (std::max)(0, boss.hp);
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
    if (m_stageNumber == 5 && m_stage5Phase == Stage5Phase::EastsourceBattle) {
        DefeatEastsource(boss);
        return;
    }
    SpawnExplosion(boss.x, boss.y, boss.z, true);
    SpawnEnemyDebris(boss);
    boss.active = false;
    SpawnPowerItem(boss.x, boss.y, boss.z, 1.00f);
    m_bossHp = 0;
    m_score += 5000;
    m_clear = true;
    // Stage2は上下船体の沈下、再浮上、衝突、大破まで見届けてから次ステージへ進む
    m_clearTimer = m_stageNumber == 2 ? 440 : ClearWaitFrames;
    if (m_stageNumber == 2) PlayStage2DefeatSound(false);
}

/**
 * @brief 指定地点へ向かう敵弾を生成する
 * @param sourceX 発射元ゲーム座標X
 * @param sourceY 発射元ゲーム座標Y
 * @param sourceZ 発射元レール座標Z
 * @param targetX 固定目標ゲーム座標X
 * @param targetY 固定目標ゲーム座標Y
 * @param targetZ 固定目標レール座標Z
 * @param speed ワールド空間の弾速
 * @return なし
 */
void SideScrollingShooter::SpawnEnemyShotAt(float sourceX, float sourceY, float sourceZ,
    float targetX, float targetY, float targetZ, float speed) {
    const float dx = ToWorldX(targetX - sourceX);
    const float dy = ToWorldY(targetY - sourceY);
    const float dz = targetZ - sourceZ;
    const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
    SpawnShotDirect(sourceX, sourceY, sourceZ,
        FromWorldX(dx / length * speed), FromWorldY(dy / length * speed),
        dz / length * speed, true);
}

/**
 * @brief Stage2ボス撃破演出の振動音または最終爆発音を再生する
 * @param finalExplosion 最終爆発音を再生する場合true
 * @return なし
 */
void SideScrollingShooter::PlayStage2DefeatSound(bool finalExplosion) {
    if (!m_audio) return;

    // 低域ノイズの長さと落下量を切り替えてゴゴゴ音とドカーン音を作る
    Audio::SfxrParams sound;
    sound.waveType = Audio::SfxrWaveType::Noise;
    sound.startFrequency = finalExplosion ? 0.24f : 0.10f;
    sound.minFrequency = finalExplosion ? 0.01f : 0.06f;
    sound.slide = finalExplosion ? -0.32f : -0.015f;
    sound.attackTime = finalExplosion ? 0.0f : 0.08f;
    sound.sustainTime = finalExplosion ? 0.42f : 1.10f;
    sound.decayTime = finalExplosion ? 0.90f : 0.72f;
    sound.masterVolume = finalExplosion ? 0.95f : 0.42f;
    m_audio->PlaySE(sound);

    // 最終爆発だけ低い衝撃音を足して爆発の芯を強める
    if (finalExplosion) m_audio->PlayMMLSE("t72 o1 l2 v15 c g c");
}

void SideScrollingShooter::SpawnShot(float x, float y, float vx, float vy, bool enemy,
    float z, float railSpeed, int damage) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = IsRailGameplayActive() ? (z >= 0.0f ? z : PlayerRailZ + 2.0f) :
            ToRailZFromSideX(x);
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.vz = 0.0f;
        shot.damage = damage;
        if (IsRailGameplayActive()) {
            if (enemy) {
                const float targetX = m_playerX + vx * 12.0f;
                const float targetY = m_playerY + vy * 12.0f;
                const float dx = ToWorldX(targetX) - ToWorldX(x);
                const float dy = ToWorldY(targetY) - ToWorldY(y);
                const float dz = PlayerRailZ - shot.z;
                const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                const float EnemyShotSpeed = railSpeed >= 0.0f ? railSpeed : 0.62f;
                shot.vx = FromWorldX(dx / length * EnemyShotSpeed);
                shot.vy = FromWorldY(dy / length * EnemyShotSpeed);
                shot.vz = dz / length * EnemyShotSpeed;
            } else {
                shot.vx = 0.0f;
                shot.vy = 0.0f;
                shot.vz = 1.45f;
            }
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
 * @return なし
 */
void SideScrollingShooter::SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy,
    int barrageIndex, int barrageCount) {
    Shot* available = nullptr;
    for (auto& shot : m_shots) {
        if (!shot.active) {
            available = &shot;
            break;
        }
    }

    // Stage 5の予告済み攻撃は満杯時に古い自機弾を置換して欠落を防ぐ
    if (!available && enemy && m_stageNumber == 5) {
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
    shot.barrageIndex = barrageIndex;
    shot.barrageCount = barrageCount;
    shot.enemy = enemy;
    shot.active = true;
}

void SideScrollingShooter::SpawnStage2Funnel(float x, float y, float z, bool delayedEngine) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = 0.0f;
        // Phase3は砂面を越えてから短く落下する高さまで、点火前の射出初速を与える
        shot.vy = delayedEngine ? Phase3FunnelLaunchVelocity : 0.018f;
        shot.vz = 0.0f;
        shot.hitRadius = 0.055f;
        shot.damage = 2;
        shot.enemy = true;
        shot.funnel = true;
        shot.funnelDelayedEngine = delayedEngine;
        shot.active = true;
        return;
    }
}

void SideScrollingShooter::SpawnStage2Missile(float x, float y, float z, float side) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        const float dx = ToWorldX(m_playerX - x);
        const float dy = ToWorldY(m_playerY - y);
        const float dz = IsRailGameplayActive() ? PlayerRailZ - z : 0.0f;
        const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
        constexpr float MissileSpeed = 0.68f;
        shot.vx = FromWorldX(dx / length * MissileSpeed) + side * 0.006f;
        shot.vy = FromWorldY(dy / length * MissileSpeed);
        shot.vz = dz / length * MissileSpeed;
        shot.hitRadius = 0.045f;
        shot.damage = 2;
        shot.enemy = true;
        shot.missile = true;
        shot.active = true;
        return;
    }
}

/** @brief 選択中の機体タイプに対応する特殊弾を生成する */
void SideScrollingShooter::FireSpecialShots() {
    const auto& config = PlayerShotConfigs[static_cast<size_t>(m_playerType)];
    const int powerLevel = PowerLevel();
    const int projectileCount = m_playerType == Spread ? config.projectileCount + powerLevel * 2 :
        config.projectileCount + powerLevel;
    const int damage = m_playerType == Piercing ? config.damage + powerLevel : config.damage;
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
        const float spawnY = railGameplay ? m_playerY : m_playerY + centeredIndex * config.spawnOffsetY;
        const float railSpawnOffsetX = config.spawnOffsetY > 0.0f ? config.spawnOffsetY : 0.05f;

        /** @brief 空きスロットへ機体タイプ固有の属性を設定する */
        for (auto& shot : m_shots) {
            if (shot.active) continue;
            shot = {};
            // 3Dレールでは翼の左右から、2Dでは従来どおり機首の上下から発射する
            shot.x = railGameplay ? m_playerX + centeredIndex * railSpawnOffsetX :
                m_playerX + config.spawnOffsetX;
            shot.y = spawnY;
            shot.z = railGameplay ? PlayerRailZ + 2.0f : ToRailZFromSideX(shot.x);
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            if (railGameplay) {
                /** @brief 3Dレールでは特殊弾を奥行き方向へ進ませ、拡散角を横移動へ適用する */
                shot.vx = std::sin(angle) * config.speed;
                shot.vy = 0.0f;
                shot.vz = 1.45f;
            } else {
                /** @brief 2Dでは従来どおり画面右方向を基準に拡散する */
                shot.vx = std::cos(angle) * config.speed;
                shot.vy = std::sin(angle) * config.speed;
                shot.vz = 0.0f;
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

/** @brief 追尾弾の進行方向を最寄りの前方敵へ近づける */
void SideScrollingShooter::UpdateHomingShot(Shot& shot) {
    if (IsRailGameplayActive()) {
        const Enemy* target = nullptr;
        float nearestDistanceSquared = 10000.0f;

        // 3Dレールでは奥行き方向の前方にいる最寄りの敵を追尾する
        for (const auto& enemy : m_enemies) {
            if (!enemy.active || enemy.z <= shot.z) continue;
            const float dx = ToWorldX(enemy.x - shot.x);
            const float dy = ToWorldY(enemy.y - shot.y);
            const float dz = enemy.z - shot.z;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared < nearestDistanceSquared) {
                nearestDistanceSquared = distanceSquared;
                target = &enemy;
            }
        }
        if (target == nullptr || nearestDistanceSquared <= 0.000001f) return;

        // ワールド空間で旋回量を補間して、レール弾速を維持する
        const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
        const float inverseDistance = 1.0f / std::sqrt(nearestDistanceSquared);
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
    float nearestDistanceSquared = 100.0f;

    /** @brief 前方にいる最寄りの敵を追尾対象として検索する */
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.x <= shot.x) continue;
        const float dx = enemy.x - shot.x;
        const float dy = enemy.y - shot.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            target = &enemy;
        }
    }
    if (target == nullptr || nearestDistanceSquared <= 0.000001f) return;

    /** @brief 現在速度と目標方向を補間して速度を一定に保つ */
    const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
    const float inverseDistance = 1.0f / std::sqrt(nearestDistanceSquared);
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

/**
 * @brief Stage2潜砂艦のPhase1接地基準Y座標を取得する
 * @param boss 座標を求めるStage2ボス
 * @return 切削爪が地中へ収まる潜砂艦のワールドY座標
 */
float SideScrollingShooter::Stage2Phase1SubmarineWorldY(const Enemy& boss) const {
    constexpr float SideGroundTopY = -6.0f;
    constexpr float RailGroundTopY = -3.65f;
    constexpr float CutterTopLocalY = -0.99f;
    constexpr float CutterEmbedDepth = 0.55f;
    constexpr float ModelScale = 1.92f;
    constexpr float Phase1BobWorldAmplitude = 0.10f * WorldYScale;
    static_assert(CutterEmbedDepth > Phase1BobWorldAmplitude);

    // 2Dと3Dの地面上面を補間し、上下動の最高点でも切削爪を地中へ残す
    const float groundTopY = Math::Lerp(SideGroundTopY, RailGroundTopY, RailBlend());
    return groundTopY - CutterTopLocalY * ModelScale - CutterEmbedDepth + ToWorldY(boss.y);
}

/**
 * @brief Stage2潜砂艦の現在の親Y座標を取得する
 * @param boss 座標を求めるStage2ボス
 * @return 描画と当たり判定で共有するワールドY座標
 */
float SideScrollingShooter::Stage2SubmarineWorldY(const Enemy& boss) const {
    const float assembledY = boss.phase < 2.0f ? Stage2Phase1SubmarineWorldY(boss) :
        ToWorldY(boss.y) - 1.45f + boss.sandSubmarineOffsetY;
    if (m_stageNumber != 2 || m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return assembledY;
    }

    // 前半で地中から砂面直下まで掘り進み、後半で上部戦艦との合体位置まで浮上する
    const float approach = SmoothStep(static_cast<float>(m_bossIntroductionTimer) /
        static_cast<float>(Stage2BossApproachFrames));
    const float assembly = SmoothStep(static_cast<float>(m_bossIntroductionTimer - Stage2BossApproachFrames) /
        static_cast<float>(Stage2BossAssemblyFrames));
    return assembledY + Math::Lerp(-7.0f, -2.4f, approach) * (1.0f - assembly);
}

/**
 * @brief Stage2上部戦艦の親Y座標を取得する
 * @param boss 座標を求めるStage2ボス
 * @return 描画と当たり判定で共有するワールドY座標
 */
float SideScrollingShooter::Stage2BattleshipWorldY(const Enemy& boss) const {
    constexpr float SideGroundTopY = -6.0f;
    constexpr float RailGroundTopY = -3.65f;
    constexpr float HullBottomLocalY = 0.02f;
    constexpr float HoverHeight = 0.18f;
    constexpr float ModelScale = 1.92f;
    constexpr float AssembledUnitOffsetY = 0.77f;
    if (boss.phase >= 2.0f && boss.phase < 3.0f) {
        // Phase2開始時の高度から地表付近まで滑らかに下降する
        constexpr float DescentFrames = 120.0f;
        const float t = Math::Clamp01(static_cast<float>(boss.stage2BossActionAge) / DescentFrames);
        const float smooth = t * t * (3.0f - 2.0f * t);
        const float startY = Stage2Phase1SubmarineWorldY(boss) + AssembledUnitOffsetY;
        const float targetY = Math::Lerp(SideGroundTopY, RailGroundTopY, RailBlend()) +
            HoverHeight - HullBottomLocalY * ModelScale;
        return Math::Lerp(startY, targetY, smooth);
    }
    const float assembledY = boss.phase < 2.0f ?
        Stage2Phase1SubmarineWorldY(boss) + AssembledUnitOffsetY :
        ToWorldY(boss.y) + 1.32f + boss.landBattleshipOffsetY;
    if (m_stageNumber != 2 || m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return assembledY;
    }

    // 前半で上空から降下し、後半で潜砂艦との合体位置へ接近する
    const float approach = SmoothStep(static_cast<float>(m_bossIntroductionTimer) /
        static_cast<float>(Stage2BossApproachFrames));
    const float assembly = SmoothStep(static_cast<float>(m_bossIntroductionTimer - Stage2BossApproachFrames) /
        static_cast<float>(Stage2BossAssemblyFrames));
    return assembledY + Math::Lerp(13.0f, 4.2f, approach) * (1.0f - assembly);
}

/**
 * @brief Stage2上部戦艦の船体へ弾が命中したか判定する
 * @param shot 判定する自機弾
 * @param boss 判定するStage2ボス
 * @return 船体へ命中した場合true
 */
bool SideScrollingShooter::TryHitStage2BossBody(const Shot& shot, const Enemy& boss) const {
    constexpr float BodyLocalX = 0.55f;
    constexpr float BodyLocalY = 0.95f;
    constexpr float ModelScale = 1.92f;
    constexpr float BodyRadius = 4.25f;
    const bool railMode = IsRailGameplayActive();
    const float yaw = railMode ? 0.0f : Math::HalfPi;
    const float patrolX = boss.phase >= 2.0f ?
        std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * RailBlend() : 0.0f;
    const Vector3 center {
        ToWorldX(boss.x + boss.landBattleshipOffsetX) + patrolX + std::cos(yaw) * BodyLocalX * ModelScale,
        Stage2BattleshipWorldY(boss) + BodyLocalY * ModelScale,
        boss.z + boss.landBattleshipOffsetZ - std::sin(yaw) * BodyLocalX * ModelScale
    };
    return railMode ?
        Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            center.x, center.y, center.z, BodyRadius) :
        Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(center.x), FromWorldY(center.y),
            BodyRadius / WorldXScale);
}

bool SideScrollingShooter::TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const {
    if (m_stageNumber == 5) {
        constexpr EastsourcePartGroup Groups[] = {
            EastsourcePartGroup::Nose,
            EastsourcePartGroup::LeftWing,
            EastsourcePartGroup::RightWing,
            EastsourcePartGroup::LeftEngine,
            EastsourcePartGroup::RightEngine
        };
        const Stage5ModelTransform transform = EastsourceTransform(boss);
        const EastsourceModelState state = EastsourceState(boss);

        // 描画と同じ26パーツから集約した各グループ境界へ線分判定する
        for (int index = BossNose; index <= BossRightEngine; ++index) {
            if (boss.bossPartHp[index] <= 0) continue;
            const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
                transform, state, Groups[index]);
            if (!bounds.valid || !Hit3DSegment(
                ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;
            part = static_cast<BossPart>(index);
            return true;
        }
        return false;
    }

    if (m_stageNumber == 2) {
        constexpr float ModelScale = 1.92f;
        constexpr Vector3 PartPosition[] = {
            {-2.30f, 2.08f, 0.0f}, {-0.55f, 2.60f, -0.92f},
            {0.65f, 2.60f, 0.0f}, {2.85f, 2.18f, 0.92f}, {0.25f, 1.42f, 0.0f}
        };
        constexpr float PartRadius[] = {2.20f, 1.05f, 1.05f, 1.05f, 1.35f};
        bool weaponsDestroyed = true;
        for (int i = 0; i < BossPartCount; ++i) {
            if (i != BossRightEngine && boss.bossPartHp[i] > 0) weaponsDestroyed = false;
        }
        const bool railMode = IsRailGameplayActive();
        const float battleshipYaw = railMode ? 0.0f : Math::HalfPi;
        const bool separated = boss.phase >= 2.0f;
        const float battleshipPatrolX = separated ?
            std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * RailBlend() : 0.0f;

        // 武装全破壊までは装甲内の接続コアを命中対象にしない
        for (int i = 0; i <= BossRightEngine; ++i) {
            if (boss.bossPartHp[i] <= 0 || (i == BossRightEngine && !weaponsDestroyed)) continue;
            const Vector3& local = PartPosition[i];
            const bool submarinePart = i == BossRightEngine;
            const float yaw = battleshipYaw + (submarinePart && separated ? Math::HalfPi : 0.0f);
            const float cosine = std::cos(yaw);
            const float sine = std::sin(yaw);
            const float unitOffsetX = submarinePart ? ToWorldX(boss.sandSubmarineOffsetX) :
                ToWorldX(boss.landBattleshipOffsetX) + battleshipPatrolX;
            const float worldY = submarinePart ?
                Stage2SubmarineWorldY(boss) :
                Stage2BattleshipWorldY(boss);
            const float unitOffsetZ = submarinePart ? boss.sandSubmarineOffsetZ : boss.landBattleshipOffsetZ;
            const Vector3 world {
                ToWorldX(boss.x) + unitOffsetX + (local.x * cosine + local.z * sine) * ModelScale,
                worldY + local.y * ModelScale,
                boss.z + unitOffsetZ + (-local.x * sine + local.z * cosine) * ModelScale
            };
            const bool hit = railMode ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    world.x, world.y, world.z, PartRadius[i]) :
                Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(world.x), FromWorldY(world.y),
                    PartRadius[i] / WorldXScale);
            if (!hit) continue;
            part = static_cast<BossPart>(i);
            return true;
        }

        // 側面のオレンジ色の小窓十二基を、描画と同じローカル座標で個別判定する
        for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
            const int partIndex = BossFunnelHatch0 + hatch;
            if (boss.bossPartHp[partIndex] <= 0) continue;
            const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
            const float localZ = (hatch < 6 ? -1.0f : 1.0f) * 1.80f;
            const float submarineYaw = battleshipYaw + (separated ? Math::HalfPi : 0.0f);
            const float cosine = std::cos(submarineYaw);
            const float sine = std::sin(submarineYaw);
            const Vector3 world {
                ToWorldX(boss.x + boss.sandSubmarineOffsetX) +
                    (localX * cosine + localZ * sine) * ModelScale,
                Stage2SubmarineWorldY(boss) - 0.22f * ModelScale,
                boss.z + boss.sandSubmarineOffsetZ +
                    (-localX * sine + localZ * cosine) * ModelScale
            };
            const bool hit = railMode ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    world.x, world.y, world.z, 0.58f) :
                Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(world.x), FromWorldY(world.y),
                    0.58f / WorldXScale);
            if (!hit) continue;
            part = static_cast<BossPart>(partIndex);
            return true;
        }
        return false;
    }

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
    if (m_audio) m_audio->PlayMMLSE("t240 o6 l32 v7 c>c");
}

void SideScrollingShooter::PlayHitSound() {
    if (m_audio) m_audio->PlayMMLSE("t180 o4 l32 v10 g e c");
}

/**
 * @brief 鋭い放電音と短い雷鳴を重ねたStage2レールガンSEを再生する
 * @return なし
 */
void SideScrollingShooter::PlayRailgunSound() {
    if (!m_audio) return;

    // 高周波の亀裂音を急降下させて雷の鋭い立ち上がりを作る
    Audio::SfxrParams crack;
    crack.waveType = Audio::SfxrWaveType::Sawtooth;
    crack.startFrequency = 0.92f;
    crack.minFrequency = 0.06f;
    crack.slide = -0.72f;
    crack.sustainTime = 0.025f;
    crack.decayTime = 0.16f;
    crack.masterVolume = 0.75f;
    m_audio->PlaySE(crack);

    // 短いノイズを重ねて雷撃の破裂感を足す
    Audio::SfxrParams thunder;
    thunder.waveType = Audio::SfxrWaveType::Noise;
    thunder.startFrequency = 0.68f;
    thunder.minFrequency = 0.04f;
    thunder.slide = -0.58f;
    thunder.sustainTime = 0.035f;
    thunder.decayTime = 0.22f;
    thunder.masterVolume = 0.82f;
    m_audio->PlaySE(thunder);
}
