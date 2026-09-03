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

static_assert(BombTravelCoordinate(-0.8f, 0, 24) == -0.8f);
static_assert(BombTravelCoordinate(-0.8f, 24, 24) == 0.0f);
}

void SideScrollingShooter::TickPlayer() {
    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    /** @brief 2D画面ではHUDを除くプレイ領域全体を移動可能にする */
    const Vector2 xRange = StageDispatch::PlayerXRange(*this);
    const Vector2 sideYRange = StageDispatch::SidePlayerYRange(*this);
    const float minY = IsRailGameplayActive() ? PlayerRailMinY() : sideYRange.x;
    const float maxY = IsRailGameplayActive() ?
        StageDispatch::RailPlayerMaxY(*this) : sideYRange.y;
    const float speedScale = m_slowMove ? 0.5f : 1.0f;
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f * speedScale, xRange.x, xRange.y);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f * speedScale, minY, maxY);
}

/**
 * @brief 入力中の通常弾・特殊弾発射を更新する
 * @return なし
 */
void SideScrollingShooter::TickPlayerWeapons() {
    // 通常弾と選択機体の特殊弾をそれぞれのクールダウンで発射する
    bool firedPlayerShot = false;
    if (m_fire && m_shotCooldown == 0) {
        SpawnShot(m_playerX + (IsRailGameplayActive() ? 0.0f : 0.12f), m_playerY,
            IsRailGameplayActive() ? 0.0f : 0.045f, 0.0f, false,
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
    if (m_bombRequested && !m_bomb.active) {
        m_bomb = {m_playerX, m_playerY, m_playerX, m_playerY,
            PlayerRailZ + 6.0f, 0, true};
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
        if (visible) enemy.active = false;
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
                if (enemy.type != 2) PlayBossMachineGunSound();
            }
        }

        // ボスは通常・特殊フェーズごとの間隔で、未破壊の各部位から弾幕を発射する
        if (enemy.type == 2 &&
            !StageDispatch::IsBossSpecialAttackActive(*this, enemy) &&
            enemy.age % m_stage->BossAttackInterval(static_cast<BossPhase>(enemy.bossPhase)) == 0) {
            StageDispatch::FireBossPartBarrage(*this, enemy);
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
    TickLinkedEnemyLasers();
}

void SideScrollingShooter::TickLinkedEnemyLasers() {
    if (m_invincible > 0) return;

    for (const auto& upper : m_enemies) {
        if (!upper.active || upper.type != Stage::LinkedLaserEnemy || upper.laserLinkRole <= 0) continue;
        for (const auto& lower : m_enemies) {
            if (!lower.active || lower.type != Stage::LinkedLaserEnemy ||
                lower.laserLinkId != upper.laserLinkId || lower.laserLinkRole >= 0) continue;
            const bool playerHit = IsRailGameplayActive() ?
                DistancePointToSegment3D({ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ},
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
    // Stage3の遅延点火ミサイルは画面外到達または命中時に爆発へ変換する
    auto DeactivateShot = [this](Shot& shot) {
        if (m_stageNumber == 3 && shot.enemy &&
            shot.stage2.kind == ShooterStages::Stage2::ShotKind::Funnel &&
            shot.stage2.delayedEngine) {
            SpawnExplosion(shot.x, shot.y, shot.z);
        }
        shot.active = false;
    };

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
        if (!cullProtected && IsRailGameplayActive() &&
            (shot.z < 0.0f || shot.z > EnemyRailFarZ ||
            std::abs(shot.x) > 1.2f ||
            shot.y < PlayerRailMinY() - Side2DShotCullMargin ||
            shot.y > StageDispatch::RailPlayerMaxY(*this) + Side2DShotCullMargin)) {
            DeactivateShot(shot);
            continue;
        }

        if (shot.enemy) {
            const bool playerHit = StageDispatch::CanEnemyShotDamagePlayer(*this, shot) &&
                (IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.38f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                    StageDispatch::EnemyShotHitRadius(*this, shot)) :
                Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y,
                    StageDispatch::EnemyShotHitRadius(*this, shot)));
            const bool grazed = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 1.18f,
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
            if (enemy.type == 2 && StageDispatch::BlocksPlayerShot(*this, shot, enemy)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                shot.active = false;
                PlayHitSound();
                break;
            }
            if (enemy.type == 2 && StageDispatch::TryHitBossBody(*this, shot, enemy)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                if (!shot.piercing) shot.active = false;
                if (DamageBoss(enemy, shot.damage)) DefeatBoss(enemy);
                else m_bossHp = enemy.hp;
                PlayHitSound();
                break;
            }
            // 専用部位判定後に本体接触無効中のボスを共通形状判定から除外する
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

        // 初めて画面へ出現した通常敵を永続ギャラリーへ登録する
        switch (enemy.type) {
        case 0: UnlockGallery(GalleryEntry::LightEnemy); break;
        case 1: UnlockGallery(GalleryEntry::HeavyEnemy); break;
        case 4: UnlockGallery(GalleryEntry::ArmoredEnemy); break;
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
        enemy.z = IsRailGameplayActive() ? (std::max)(railZ, EnemyRailFarZ) : ToRailZFromSideX(enemy.x);
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
    shot.barrageIndex = barrageIndex;
    shot.barrageCount = barrageCount;
    shot.enemy = enemy;
    shot.active = true;
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
    if (m_audio) m_audio->PlayMMLSE("t240 o6 l32 v7 c>c");
}

void SideScrollingShooter::PlayHitSound() {
    if (m_audio) m_audio->PlayMMLSE("t180 o4 l32 v10 g e c");
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

    // 短く急降下する高域ノイズで連射時の「タッ」という一発を作る
    Audio::SfxrParams sound;
    sound.waveType = Audio::SfxrWaveType::Noise;
    sound.sustainTime = 0.012f;
    sound.decayTime = 0.055f;
    sound.startFrequency = 0.76f;
    sound.minFrequency = 0.18f;
    sound.slide = -0.55f;
    sound.masterVolume = 0.48f;
    m_audio->PlaySE(sound);
}

/** @brief 生存中の爆発エフェクトを更新する @return なし */
void SideScrollingShooter::TickExplosions() {
    for (auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        if (explosion.effectType == 1 && !explosion.damagedPlayer &&
            explosion.age <= AttackWarningFrames && m_invincible == 0) {
            const bool playerHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.38f,
                    ToWorldX(explosion.x), ToWorldY(explosion.y), explosion.z,
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
 * @return なし
 */
void SideScrollingShooter::SpawnMortarExplosion(float x, float y, float z) {
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {
            x, y, IsRailGameplayActive() ? z : ToRailZFromSideX(x),
            0, false, true, 1, 0.55f
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
