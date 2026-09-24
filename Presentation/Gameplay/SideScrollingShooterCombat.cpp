#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"
#include "Stages/Common/StageDispatch.h"
#include "Stages/Stage4/Stage4Module.h"
#include "Stages/Stage2/Stage2Module.h"
#include "Stages/Stage5/Stage5Module.h"

#include "SideScrollingShooterEnemies.h"
#include "Models/AircraftModelView.h"
#include "GameplayRandom.h"
#include "Stages/Common/StageDefinition.h"

namespace {
/** @brief ボスに命中したボム一発分の最大HP比ダメージを求める @param maxHp ボス最大HP @param piercing レーザーボムならtrue @return 一発分のダメージ */
constexpr int BossBombDamage(int maxHp, bool piercing) {
    return (std::max)(1, (maxHp * (piercing ? 5 : 20) + 500) / 1000);
}

static_assert(BossBombDamage(800, false) * 10 == 160);
static_assert(BossBombDamage(1200, true) * 30 == 180);
constexpr float MortarExplosionDepthHitRadius = 0.85f;

/**
 * @brief 3D距離が敵発射体の接近禁止範囲外か判定する
 * @param dx 発射元と自機のX距離
 * @param dy 発射元と自機のY距離
 * @param dz 発射元と自機のZ距離
 * @return 指定した接近禁止距離より離れている場合true
 */
constexpr bool IsOutsideEnemyProjectileNoFireRange(
    float dx, float dy, float dz, float noFireDistance) {
    return dx * dx + dy * dy + dz * dz >
        noFireDistance * noFireDistance;
}

static_assert(!IsOutsideEnemyProjectileNoFireRange(0.0f, 0.0f, 0.0f, 12.0f));
static_assert(!IsOutsideEnemyProjectileNoFireRange(0.0f, 0.0f, 12.0f, 12.0f));
static_assert(IsOutsideEnemyProjectileNoFireRange(0.0f, 0.0f, 12.01f, 12.0f));

/**
 * @brief 距離を優先し、現在の標的への小さな距離変化で切り替わることを防ぐ
 * @param distanceSquared 自機から標的までの距離の二乗
 * @param locked 現在の標的の場合true
 * @return 小さいほど優先する評価値
 */
constexpr float HomingTargetScore(float distanceSquared, bool locked) {
    return distanceSquared * (locked ? 0.75f : 1.0f);
}

/**
 * @brief 等速の標的へ弾が到達する最短時間を求める
 * @param delta 発射点から標的への相対位置
 * @param velocity 標的の1フレームの移動量
 * @param speed 弾の1フレームの移動距離
 * @return 到達までのフレーム数、迎撃不能なら0
 */
float InterceptFrames(const Vector3& delta, const Vector3& velocity, float speed) {
    // 二次方程式の小さい正の解を選び、同速時と桁落ちも処理する
    const double a = static_cast<double>(velocity.LengthSquared()) - static_cast<double>(speed) * speed;
    const double b = 2.0 * Vector3::Dot(delta, velocity);
    const double c = delta.LengthSquared();
    if (std::abs(a) < 0.000001) return b < -0.000001 ? static_cast<float>(-c / b) : 0.0f;
    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) return 0.0f;
    const double q = -0.5 * (b + std::copysign(std::sqrt(discriminant), b));
    const double first = q / a;
    const double second = q != 0.0 ? c / q : -1.0;
    const double time = first > 0.0 && second > 0.0 ? (std::min)(first, second) : (std::max)(first, second);
    return time > 0.0 && std::isfinite(time) ? static_cast<float>(time) : 0.0f;
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

static_assert(PerspectiveDepthScale(-13.5f, 35.0f, 10.0f) > 0.0f);
static_assert(PerspectiveDepthScale(-13.5f, 35.0f, 10.0f) < 1.0f);
static_assert(HomingTargetScore(1.0f, false) < HomingTargetScore(9.0f, false));
static_assert(HomingTargetScore(1.1f, true) < HomingTargetScore(1.0f, false));
static_assert(HomingTargetScore(4.0f, true) > HomingTargetScore(1.0f, false));
}

void SideScrollingShooter::TickPlayer() {
    if (Player().m_playerDestructionTimer > 0) return;
    float dx = static_cast<float>(Player().m_moveRight) - static_cast<float>(Player().m_moveLeft);
    float dy = static_cast<float>(Player().m_moveUp) - static_cast<float>(Player().m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    const float speedScale = Player().m_slowMove ? 0.5f : 1.0f;
    if (IsTayamaBattle()) {
        // 3Dはボス中心の周回、2Dは切替時に固定したカメラ面内の横移動として扱う
        if (IsTayamaOrbitViewActive() && m_activePlayer == 0) {
            m_stage5.tayamaOrbitAngle = std::remainder(
                m_stage5.tayamaOrbitAngle + dx * ShooterStages::Stage5::TayamaOrbitSpeed * speedScale,
                Math::TwoPi);
            Player().m_playerX = 0.0f;
        } else {
            Player().m_playerX = (std::clamp)(Player().m_playerX + dx * 0.023f * speedScale, -1.2f, 1.2f);
        }
        Player().m_playerY = (std::clamp)(Player().m_playerY + dy * 0.058f * speedScale,
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
    Player().m_playerX = (std::clamp)(Player().m_playerX + dx * 0.023f * speedScale, xRange.x, xRange.y);
    Player().m_playerY = (std::clamp)(Player().m_playerY + dy * 0.029f * speedScale, minY, maxY);
}

/**
 * @brief 入力中の通常弾・特殊弾発射を更新する
 * @return なし
 */
void SideScrollingShooter::TickPlayerWeapons() {
    UpdateAimSnap(false);
    if (Player().m_playerDestructionTimer > 0) return;
    // 通常弾と選択機体の特殊弾をそれぞれのクールダウンで発射する
    bool firedPlayerShot = false;
    if (Player().m_fire && Player().m_shotCooldown == 0) {
        const bool verticalRoute = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
        SpawnShot(Player().m_playerX + (IsRailGameplayActive() || verticalRoute ? 0.0f : 0.12f),
            Player().m_playerY + (verticalRoute ? 0.12f : 0.0f),
            IsRailGameplayActive() || verticalRoute ? 0.0f : 0.045f,
            verticalRoute ? 0.045f : 0.0f, false,
            -1.0f, -1.0f, 1 + PowerLevel());
        Player().m_shotCooldown = (std::max)(3, 7 - PowerLevel());
        PlayShotSound();
    }
    if (Player().m_fire && Player().m_specialShotCooldown == 0) {
        FireSpecialShots();
        const auto& config = PlayerShotConfigs[static_cast<size_t>(Player().m_playerType)];
        Player().m_specialShotCooldown = config.fireIntervalFrames;
        firedPlayerShot = true;
    }
    if (firedPlayerShot) PlayShotSound();
}

/** @brief 機体別ボムの発動、移動、持続時間を更新する @return なし */
void SideScrollingShooter::TickBomb() {
    auto& bomb = Player().m_bomb;
    const bool rail = IsRailGameplayActive();
    const bool spatial = rail && !UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    const Shot forwardShot = MakeNormalPlayerShot(false);
    const Vector3 forward = Vector3 {ToWorldX(forwardShot.vx), ToWorldY(forwardShot.vy), forwardShot.vz}.Normalized();
    const Vector3 player = PlayerWorldPosition();

    // 一人につき一個を消費し、ミサイル用の十個の専用枠も初期化する
    if (!m_chapterResultActive && !m_clear && Player().m_playerDestructionTimer == 0 &&
        Player().m_bombRequested && !bomb.active && Player().m_bombCount > 0) {
        bomb = {};
        bomb.type = Player().m_playerType;
        bomb.rail = rail;
        bomb.active = true;
        --Player().m_bombCount;
        PlayShotSound();
        ShakeScreen(bomb.type == Spread ? 0.06f : 0.16f, 12);
    }
    if (!bomb.active) return;
    // シールドは時間で終了せず、表示用の位相だけを循環させる
    bomb.age = bomb.type == Spread ? (bomb.age + 1) % 360 : bomb.age + 1;
    if ((bomb.type == Homing && bomb.age > BombMissileFrames) ||
        (bomb.type == Piercing && bomb.age > BombLaserFrames)) { bomb.active = false; return; }
    const Vector3 origin = player + forward * (bomb.type == Piercing ? AircraftModelView::NoseTipZ : 0.0f);
    bomb.x = FromWorldX(origin.x);
    bomb.y = FromWorldY(origin.y);
    bomb.z = rail ? origin.z : SidePlaneZ;
    bomb.direction = forward;
    if (bomb.type != Homing) { bomb.rail = rail; return; }

    // Stage2ボスと同じ打ち上げ→点火の順で、一発ずつ発射して個別に追尾する
    bool flying = false;
    for (int i = 0; i < static_cast<int>(bomb.missiles.size()); ++i) {
        auto& missile = bomb.missiles[i];
        if (bomb.age == i * BombMissileInterval + 1) {
            missile = {};
            missile.x = bomb.x;
            missile.y = bomb.y;
            missile.z = bomb.z;
            // 発射時に一度だけ乱数を固定し、同期プレイでも同じ曲線を再現する
            missile.homingLaunchDirection = {GameplayRandom::Range(-1.0f, 1.0f),
                GameplayRandom::Range(-1.0f, 1.0f), GameplayRandom::Range(-1.0f, 1.0f)};
            missile.hitRadius = BombMissileRadius / WorldXScale;
            missile.damage = 80;
            missile.owner = m_activePlayer;
            missile.special = missile.bomb = missile.active = true;
            PlayMissileLaunchSound();
        }
        if (!missile.active) continue;
        flying = true;
        if (bomb.rail != rail && missile.age > 0) {
            // 発射し直さず、既に飛んでいる弾だけを新しい視点の前方軸へ移す
            if (!UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase)) {
                if (rail) { missile.z = ToRailZFromSideX(missile.x); missile.x = Player().m_playerX; }
                else { missile.x = ToSideXFromRailZ(missile.z); missile.z = SidePlaneZ; }
            } else missile.z = rail ? player.z : SidePlaneZ;
            missile.vx = FromWorldX(forward.x * BombMissileSpeed);
            missile.vz = forward.z * BombMissileSpeed;
        }
        ++missile.age;
        if (bomb.age <= BombMissileLaunchFrames) {
            // 全弾が出るまでは重力で上昇から落下へ移り、低い終端速度で画面内に留める
            missile.vx = FromWorldX(missile.homingLaunchDirection.x * 0.025f);
            missile.vz = spatial ? missile.homingLaunchDirection.z * 0.025f : 0.0f;
            missile.vy = FromWorldY((std::max)(-0.07f,
                0.17f + missile.homingLaunchDirection.y * 0.015f - 0.006f * missile.age));
        } else {
            Vector3 target;
            missile.homingTarget = FindShotTarget(missile, target);
            const Vector3 position {ToWorldX(missile.x), ToWorldY(missile.y), spatial ? missile.z : 0.0f};
            Vector3 desired = missile.homingTarget >= 0 ? (target - position).Normalized() : forward;
            if (!spatial) desired.z = 0.0f;
            const float boostFrames = static_cast<float>(bomb.age - BombMissileLaunchFrames);
            const float speed = (spatial ? BombMissileSpeed : BombMissileSpeed * 0.4f) * Math::Clamp01(boostFrames / 18.0f);
            Vector3 velocity {ToWorldX(missile.vx), ToWorldY(missile.vy), spatial ? missile.vz : 0.0f};
            const float distance = missile.homingTarget >= 0 ? (target - position).Length() : 10.0f;
            // 弾ごとに異なる横向きの加速を徐々に収め、標的の手前では正確な追尾へ戻す
            Vector3 curve = missile.homingLaunchDirection;
            if (!spatial) curve.z = 0.0f;
            curve -= desired * Vector3::Dot(curve, desired);
            desired = (desired + curve * (1.8f * Math::Clamp01(1.0f - boostFrames / 72.0f) *
                Math::Clamp01(distance / 6.0f))).Normalized();
            const float turn = std::clamp(speed * 2.0f / (std::max)(0.01f, distance), 0.14f, 1.0f);
            velocity = Vector3::Lerp(velocity, desired * speed, turn);
            missile.vx = FromWorldX(velocity.x);
            missile.vy = FromWorldY(velocity.y);
            missile.vz = velocity.z;
        }
        missile.x += missile.vx;
        missile.y += missile.vy;
        missile.z += missile.vz;
    }
    bomb.rail = rail;
    if (!flying && bomb.age > (static_cast<int>(bomb.missiles.size()) - 1) * BombMissileInterval)
        bomb.active = false;
}

/** @brief ボムの攻撃範囲を共通の弾判定へ変換する @return 攻撃線分と威力を持つ弾 */
SideScrollingShooter::Shot SideScrollingShooter::MakeBombShot() const {
    const auto& bomb = Player().m_bomb;
    Shot shot;
    shot.x = bomb.x;
    shot.y = bomb.y;
    shot.z = bomb.z;
    const float length = bomb.type == Piercing ? BombLaserLength : 0.0f;
    shot.vx = FromWorldX(bomb.direction.x * length);
    shot.vy = FromWorldY(bomb.direction.y * length);
    shot.vz = bomb.direction.z * length;
    if (bomb.type == Piercing) {
        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
    }
    shot.hitRadius = (bomb.type == Piercing ? BombLaserRadius : BombShieldRadius) / WorldXScale;
    shot.damage = 8;
    shot.playerType = bomb.type;
    shot.owner = m_activePlayer;
    shot.piercing = bomb.type == Piercing;
    shot.bomb = true;
    shot.active = true;
    return shot;
}

/** @brief ボムと敵弾の移動軌跡が重なるか判定する @param shot 敵弾 @return 消去対象ならtrue */
bool SideScrollingShooter::BombClearsShot(const Shot& shot) {
    auto& bomb = Player().m_bomb;
    if (!bomb.active || !shot.enemy) return false;
    if (bomb.type == Homing) {
        for (const auto& missile : bomb.missiles)
            if (missile.active && BombShotIntersects(missile, shot)) return true;
        return false;
    }
    if (!BombShotIntersects(MakeBombShot(), shot)) return false;
    // 一度の被弾を肩代わりし、残りの敵弾で同時に撃墜されない猶予を付ける
    if (bomb.type == Spread && StageDispatch::CanEnemyShotDamagePlayer(*this, shot)) DamagePlayer();
    return true;
}

/** @brief 二つの弾の掃引範囲が接触するか判定する @param bomb 弾消し側 @param shot 敵弾 @return 接触時true */
bool SideScrollingShooter::BombShotIntersects(const Shot& bomb, const Shot& shot) const {
    const bool rail = IsRailGameplayActive();
    const Vector3 end {ToWorldX(bomb.x), ToWorldY(bomb.y), rail ? bomb.z : 0.0f};
    const Vector3 axis {ToWorldX(bomb.vx), ToWorldY(bomb.vy), rail ? bomb.vz : 0.0f};
    const Vector3 start = end - axis;
    const Vector3 point {ToWorldX(shot.x), ToWorldY(shot.y), rail ? shot.z : 0.0f};
    const Vector3 velocity {ToWorldX(shot.vx), ToWorldY(shot.vy), rail ? shot.vz : 0.0f};
    const Vector3 previous = point - velocity;
    const float radius = bomb.hitRadius * WorldXScale +
        StageDispatch::EnemyShotHitRadius(*this, shot) * (rail ? 1.0f : WorldXScale);

    // 両線分の端点と内部の最近点を調べ、高速弾が防御範囲を飛び越すのを防ぐ
    float distance = (std::min)({DistancePointToSegment3D(point, start, end),
        DistancePointToSegment3D(previous, start, end),
        DistancePointToSegment3D(start, previous, point),
        DistancePointToSegment3D(end, previous, point)});
    const Vector3 delta = start - previous;
    const float aa = axis.LengthSquared();
    const float bb = Vector3::Dot(axis, velocity);
    const float cc = velocity.LengthSquared();
    const float dd = Vector3::Dot(axis, delta);
    const float ee = Vector3::Dot(velocity, delta);
    const float denominator = aa * cc - bb * bb;
    if (denominator > 0.000001f) {
        const float t = (bb * ee - cc * dd) / denominator;
        const float u = (aa * ee - bb * dd) / denominator;
        if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
            distance = (std::min)(distance, (delta + axis * t - velocity * u).Length());
    }
    return distance <= radius;
}

void SideScrollingShooter::TickEnemies() {
    // 第2部最終ムービー前は通常更新を止めて全敵を画面下へ退避させる
    if (m_stageNumber == 5 && ShooterStages::Stage5::IsPart2PlayerFlyingAway(
        m_stage5.phase, m_stage5.phaseTimer)) {
        TickChapterExitEnemies();
        return;
    }

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
        const bool canSpawnAimedProjectile = CanSpawnEnemyProjectile(
            enemy.x, enemy.y, enemy.z);
        if (aimedShotInterval > AttackWarningFrames && canUseAimedShot && canSpawnAimedProjectile &&
            enemy.age % aimedShotInterval == aimedShotInterval - AttackWarningFrames) {
            // 発射時の追尾を防ぐため、予告した地点を狙い弾の目標として固定する
            enemy.attackWarningTargetX = Player().m_playerX;
            enemy.attackWarningTargetY = Player().m_playerY;
            enemy.attackWarningFrames = AttackWarningFrames;
        }
        if (aimedShotInterval > 0 && enemy.age % aimedShotInterval == 0 &&
            canUseAimedShot && canSpawnAimedProjectile) {
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
        ForEachPlayer([&] {
            if (Player().m_playerDestructionTimer > 0) return;
            const Vector3 playerPosition = PlayerWorldPosition();
            const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
            const bool playerHit = IsRailGameplayActive() ?
                Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 0.42f,
                    ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.behavior->CollisionRadius3D(enemy)) :
                Hit(Player().m_playerX, Player().m_playerY, 0.055f, enemy.x, enemy.y, enemyRadius);
            if (enemy.active && Player().m_invincible == 0 && playerHit) {
                if (enemy.type != 2) {
                    SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
                    SpawnEnemyDebris(enemy);
                    enemy.active = false;
                }
                DamagePlayer();
                return;
            }
        });
    }
    ForEachPlayer([&] { TickLinkedEnemyLasers(); });
}

void SideScrollingShooter::TickLinkedEnemyLasers() {
    if (Player().m_invincible > 0 || Player().m_playerDestructionTimer > 0) return;
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
                DistancePointToSegment2D({Player().m_playerX, Player().m_playerY},
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
    // 得点済みの弾も含め、今フレームの接近状態を取り直す
    m_grazing = false;
    // Stage3以降の遅延点火ミサイルは画面外到達または命中時に爆発へ変換する
    auto DeactivateShot = [this](Shot& shot) {
        if ((m_stageNumber == 3 || m_stageNumber == 4 || m_stageNumber == 5) && shot.enemy &&
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

        // 追尾弾を最寄りの前方敵へ旋回させる
        if (!shot.enemy && shot.special && shot.playerType == Homing) {
            ForEachPlayer([&] { if (m_activePlayer == shot.owner) UpdateHomingShot(shot); });
        }

        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
        // Spreadは実移動距離で減衰し、2D用の疑似奥行きや視点切替の座標変換を加算しない
        if (!shot.enemy && shot.special && shot.playerType == Spread) {
            const float dx = ToWorldX(shot.x - previousX);
            const float dy = ToWorldY(shot.y - previousY);
            const float dz = IsRailGameplayActive() ? shot.z - previousZ : 0.0f;
            shot.travelDistance += std::sqrt(dx * dx + dy * dy + dz * dz);
            constexpr float NearRange = 3.5f;
            constexpr float FarRange = 7.0f;
            shot.damage = PlayerShotConfigs[Spread].damage -
                (shot.travelDistance >= NearRange ? 1 : 0) -
                (shot.travelDistance >= FarRange ? 1 : 0);
        }
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
        // 包囲弾は自機周囲の円から進入するため、通常の画面端より外まで保持する
        const bool outsideOrbit = shot.tayamaDragonOrbit &&
            Vector3::Distance(playerPosition,
                {ToWorldX(shot.x), ToWorldY(shot.y), shot.z}) >
                ShooterStages::Stage5::TayamaDragonOrbitRadius + 1.0f;
        const bool outsideRail = !IsTayamaBattle() && (shot.tayamaDragonOrbit ? outsideOrbit :
            (shot.z < 0.0f || shot.z > railShotFarZ ||
                std::abs(shot.x) > (std::max)(1.2f, StageDispatch::PlayerXRange(*this).y) ||
                shot.y < railShotMinY - Side2DShotCullMargin ||
                shot.y > railShotMaxY + Side2DShotCullMargin));
        if (!cullProtected && IsRailGameplayActive() &&
            (outsideTayamaArena || outsideRail)) {
            DeactivateShot(shot);
            continue;
        }

        if (shot.enemy) {
            ForEachPlayer([&] { if (shot.active && BombClearsShot(shot)) shot.active = false; });
            if (!shot.active) continue;
            ForEachPlayer([&] {
                if (!shot.active || Player().m_playerDestructionTimer > 0) return;
                const Vector3 playerPosition = PlayerWorldPosition();
                const bool playerHit = StageDispatch::CanEnemyShotDamagePlayer(*this, shot) &&
                    (IsRailGameplayActive() ?
                    Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 0.38f,
                        ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                        StageDispatch::EnemyShotHitRadius(*this, shot)) :
                    Hit(Player().m_playerX, Player().m_playerY, 0.050f, shot.x, shot.y,
                        StageDispatch::EnemyShotHitRadius(*this, shot)));
                const bool grazed = IsRailGameplayActive() ?
                    Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 1.18f,
                        ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.28f) :
                    Hit(Player().m_playerX, Player().m_playerY, 0.200f, shot.x, shot.y, 0.022f);
                m_grazing |= !playerHit && grazed;
                if (!playerHit && grazed && !shot.grazed) {
                    shot.grazed = true;
                    ++m_chapterResult.grazeCount;

                    // 自機の内側方向へ交互に飛ばし、グレイズ直後も見える時間を確保する
                    constexpr float GrazeScoreSpeedX = 0.040f;
                    constexpr float GrazeScoreSpeedY = 0.045f;
                    constexpr int GrazeScorePickupDelay = 12;
                    const float scoreVx = (m_chapterResult.grazeCount & 1) != 0 ?
                        GrazeScoreSpeedX : -GrazeScoreSpeedX;
                    const float scoreVy = Player().m_playerY <= 0.0f ? GrazeScoreSpeedY : -GrazeScoreSpeedY;
                    SpawnScoreItem(Player().m_playerX, Player().m_playerY, playerPosition.z, 100,
                        scoreVx, scoreVy, GrazeScorePickupDelay);
                }
                if (Player().m_invincible == 0 && playerHit) {
                    DeactivateShot(shot);
                    DamagePlayer();
                    return;
                }
            });
            continue;
        }

        HitPlayerShotTargets(shot);
    }
    // 全敵弾を処理してから、各ミサイルとレーザーの命中を個別に解決する
    ForEachPlayer([&] {
        auto& bomb = Player().m_bomb;
        if (!bomb.active || bomb.type == Spread) return;
        if (bomb.type == Homing) {
            for (auto& missile : bomb.missiles) {
                if (!bomb.active) break;
                if (!missile.active) continue;
                HitPlayerShotTargets(missile);
                if (!missile.active) SpawnExplosion(missile.x, missile.y, missile.z, true);
            }
        } else if ((bomb.age - 1) % 6 == 0) {
            Shot shot = MakeBombShot();
            HitPlayerShotTargets(shot);
        }
    });
    // 撃破済みの敵へ枠を残さず、描画前にスナップ先を再評価する
    ForEachPlayer([&] { UpdateAimSnap(); });
}

/** @brief 自機弾とボムに共通の敵・部位へのダメージを適用する @param shot 自機弾 @return なし */
void SideScrollingShooter::HitPlayerShotTargets(Shot& shot) {
    const bool verticalRouteShot = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    const bool part2RailNormalShot = verticalRouteShot && IsRailGameplayActive() && !shot.special;
    if (StageDispatch::TryDamageStageTarget(*this, shot) && (!shot.bomb || !shot.active)) return;

    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        if (m_chapterResultActive && !enemy.collisionEnabled) continue;
        if (enemy.type == 2 && !enemy.collisionEnabled &&
            !StageDispatch::CanHitBossWhileCollisionDisabled(*this)) continue;
        if (enemy.behavior == nullptr) {
            enemy.behavior = &EnemyBehaviorForType(enemy.type);
        }
        // ボスへのボムは最大HP比で計算する。ミサイル全10発で約20%、レーザー全30ヒットで約15%
        const int damage = shot.bomb && enemy.type == 2 && enemy.maxHp > 0 ?
            BossBombDamage(enemy.maxHp, shot.piercing) : shot.damage;
        BossPart hitPart = BossNose;
        if (enemy.type == 2 && TryHitBossPart(shot, enemy, hitPart)) {
            SpawnExplosion(shot.bomb ? enemy.x : shot.x, shot.bomb ? enemy.y : shot.y, shot.bomb ? enemy.z : shot.z);
            shot.RegisterHit();
            enemy.bossPartHitFlashFrames[hitPart] = BossPartHitFlashFrames;
            enemy.bossPartHp[hitPart] -= shot.bomb ? (std::min)(damage, shot.damage) : damage;
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
            if (shot.bomb && shot.piercing) continue;
            break;
        }
        if (enemy.type == 2 && StageDispatch::BlocksPlayerShot(*this, shot, enemy)) {
            SpawnExplosion(shot.bomb ? enemy.x : shot.x, shot.bomb ? enemy.y : shot.y, shot.bomb ? enemy.z : shot.z);
            shot.active = false;
            PlayHitSound();
            break;
        }
        if (enemy.type == 2 && StageDispatch::TryHitBossBody(*this, shot, enemy)) {
            SpawnExplosion(shot.bomb ? enemy.x : shot.x, shot.bomb ? enemy.y : shot.y, shot.bomb ? enemy.z : shot.z);
            shot.RegisterHit();
            if (DamageBoss(enemy, damage)) DefeatBoss(enemy);
            else m_bossHp = enemy.hp;
            PlayHitSound();
            if (shot.bomb && shot.piercing) continue;
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
        if (verticalRouteShot && shot.special) {
            // 敵を自機弾の奥行き平面へ透視投影し、画面上で重なった場合だけ命中させる
            const Vector3 cameraPosition {
                ToWorldX(m_players[0].m_playerX) * 0.18f,
                ToWorldY(m_players[0].m_playerY) * 0.12f + 1.72f,
                PlayerRailDepth() - 21.5f
            };
            const float projectionScale = PerspectiveDepthScale(
                cameraPosition.z, enemy.z, shot.z);
            railTarget = cameraPosition + (railTarget - cameraPosition) * projectionScale;
            railTargetRadius *= projectionScale;
        }
        // 第2部3Dの通常ショットは敵の演出用奥行きに依存せず縦画面座標で判定する
        const bool enemyHit = part2RailNormalShot ?
            HitShotCircle(shot, enemy.x, enemy.y, enemyRadius) :
            IsRailGameplayActive() ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                railTarget.x, railTarget.y, railTarget.z, railTargetRadius) :
            HitShotCircle(shot, enemy.x, enemy.y, enemyRadius);
        if (!enemyHit) continue;
        SpawnExplosion(shot.bomb ? enemy.x : shot.x, shot.bomb ? enemy.y : shot.y, shot.bomb ? enemy.z : shot.z);
        shot.RegisterHit();
        if (enemy.type == 2) DamageBoss(enemy, damage);
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
        if (shot.bomb && shot.piercing) continue;
        break;
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

        // 生成時の飛び出し速度を減衰させながら反映する
        item.x += item.vx;
        item.y += item.vy;
        item.vx *= 0.88f;
        item.vy *= 0.88f;
        if (item.pickupDelay > 0) --item.pickupDelay;

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
            item.z = part2Route ? (std::max)(item.z - 0.28f, PlayerRailDepth()) : item.z - 0.28f;
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

        // 生存中の最寄り自機へ寄せ、一つのアイテムを一人だけに渡す
        int recipient = -1;
        float nearest = 1.0e30f;
        ForEachPlayer([&] {
            if (Player().m_playerDestructionTimer > 0) return;
            const Vector3 position = PlayerWorldPosition();
            const float dx = IsRailGameplayActive() ? position.x - ToWorldX(item.x) : Player().m_playerX - item.x;
            const float dy = IsRailGameplayActive() ? position.y - ToWorldY(item.y) : Player().m_playerY - item.y;
            const float dz = IsRailGameplayActive() ? position.z - item.z : 0.0f;
            const float distance = dx * dx + dy * dy + dz * dz;
            if (distance < nearest) { nearest = distance; recipient = m_activePlayer; }
        });
        ForEachPlayer([&] {
            if (m_activePlayer != recipient) return;
            const Vector3 playerPosition = PlayerWorldPosition();
            // 自機が近づいたアイテムだけを強く追尾させる
            const float dx = FromWorldX(playerPosition.x) - item.x;
            const float dy = FromWorldY(playerPosition.y) - item.y;
            const bool followsPlayer = IsRailGameplayActive() ?
                Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 3.5f,
                    ToWorldX(item.x), ToWorldY(item.y), item.z, 0.0f) :
                Hit(Player().m_playerX, Player().m_playerY, 0.45f, item.x, item.y, 0.0f);
            if (followsPlayer && item.pickupDelay == 0) {
                item.x += dx * 0.45f;
                item.y += dy * 0.45f;
                if (IsRailGameplayActive()) {
                    item.z += (playerPosition.z - item.z) * 0.45f;
                }
            }
            const bool collected = IsRailGameplayActive() ?
                Hit3D(playerPosition.x, playerPosition.y, playerPosition.z, 0.52f,
                    ToWorldX(item.x), ToWorldY(item.y), item.z, 0.38f) :
                Hit(Player().m_playerX, Player().m_playerY, 0.075f, item.x, item.y, 0.045f);
            if (!collected || item.pickupDelay > 0) return;

            if (item.type == ItemType::Power) {
                const int previousPowerLevel = PowerLevel();
                Player().m_power = (std::min)(MaxPower, Player().m_power + item.power);
                if (PowerLevel() > previousPowerLevel) Player().m_powerUpTimer = 120;
            } else {
                m_chapterResult.score += item.score;
                m_score += item.score;
            }
            item.active = false;
        });
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
    const float spawnZ = IsRailGameplayActive() ?
        (z >= 0.0f ? z : PlayerRailDepth() + 2.0f) : ToRailZFromSideX(x);
    if (enemy && !CanSpawnEnemyProjectile(x, y, spawnZ)) return;

    for (int shotIndex = 0; shotIndex < ActiveShotCapacity(); ++shotIndex) {
        auto& shot = m_shots[shotIndex];
        if (shot.active) continue;
        // 通常弾の生成と照準予測で同じ発射位置・速度・スナップ補正を使う
        if (!enemy) {
            shot = MakeNormalPlayerShot();
            shot.damage = damage;
            shot.active = true;
            return;
        }
        shot = {};
        shot.owner = m_activePlayer;
        shot.x = x;
        shot.y = y;
        shot.z = spawnZ;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.vz = 0.0f;
        shot.damage = damage;
        if (IsRailGameplayActive()) {
            const Vector3 player = PlayerWorldPosition();
            const float targetX = IsTayamaBattle() ? FromWorldX(player.x) : Player().m_playerX + vx * 12.0f;
            const float targetY = IsTayamaBattle() ? FromWorldY(player.y) : Player().m_playerY + vy * 12.0f;
            const float targetZ = player.z;
            const float dx = ToWorldX(targetX) - ToWorldX(x);
            const float dy = ToWorldY(targetY) - ToWorldY(y);
            const float dz = targetZ - shot.z;
            const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
            const float EnemyShotSpeed = railSpeed >= 0.0f ? railSpeed : 0.62f;
            shot.vx = FromWorldX(dx / length * EnemyShotSpeed);
            shot.vy = FromWorldY(dy / length * EnemyShotSpeed);
            shot.vz = dz / length * EnemyShotSpeed;
        }
        if (!IsRailGameplayActive() && m_stageNumber == 5 &&
            ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase)) {
            AimShotGroundward(shot.vx, shot.vy);
        }
        shot.enemy = enemy;
        shot.active = true;
        return;
    }
}

bool SideScrollingShooter::CanSpawnEnemyProjectile(float x, float y, float z) const {
    if (!IsRailGameplayActive()) return true;

    // ゲーム座標のXYをワールド座標へ揃えて自機との3D距離を判定する
    bool outside = true;
    ForEachPlayer([&] {
        if (Player().m_playerDestructionTimer > 0) return;
        const Vector3 player = PlayerWorldPosition();
        outside = outside && IsOutsideEnemyProjectileNoFireRange(
            ToWorldX(x) - player.x, ToWorldY(y) - player.y, z - player.z,
            EnemyProjectileNoFireDistance3D);
    });
    return outside;
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
 * @param vx 生成直後のX方向速度
 * @param vy 生成直後のY方向速度
 * @param pickupDelay 取得を開始するまでのフレーム数
 * @return なし
 */
void SideScrollingShooter::SpawnScoreItem(float x, float y, float z, int value,
    float vx, float vy, int pickupDelay) {
    for (auto& item : m_items) {
        if (item.active) continue;
        item = {};
        item.x = x;
        item.y = y;
        item.z = IsRailGameplayActive() ? z : ToRailZFromSideX(x);
        item.vx = vx;
        item.vy = vy;
        item.score = value;
        item.pickupDelay = pickupDelay;
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
    // 第2形態の包囲射撃だけ自機に近い円周からの発射を許可する
    const bool tayamaDragonOrbit = enemy && firedByBoss && m_stageNumber == 5 &&
        m_stage5.phase == Stage5Phase::TayamaDragonBattle &&
        m_stage5.tayamaDragonAttack == ShooterStages::Stage5::TayamaDragonAttack::Orbit;
    if (enemy && !tayamaDragonOrbit && !CanSpawnEnemyProjectile(x, y, z)) return;

    Shot* available = nullptr;
    for (int shotIndex = 0; shotIndex < ActiveShotCapacity(); ++shotIndex) {
        auto& shot = m_shots[shotIndex];
        if (!shot.active) {
            available = &shot;
            break;
        }
    }

    // ステージ側が予約済み攻撃の欠落回避を要求した場合だけ古い自機弾を置換する
    if (!available && StageDispatch::CanReplacePlayerShot(*this, enemy)) {
        for (int shotIndex = 0; shotIndex < ActiveShotCapacity(); ++shotIndex) {
            auto& shot = m_shots[shotIndex];
            if (!shot.enemy) {
                available = &shot;
                break;
            }
        }
    }
    if (!available) return;

    Shot& shot = *available;
    shot = {};
    shot.owner = m_activePlayer;
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
    shot.firedByBoss = firedByBoss;
    shot.tayamaDragonOrbit = tayamaDragonOrbit;
    shot.active = true;
}



/** @brief 選択中の機体タイプに対応する特殊弾を生成する */
void SideScrollingShooter::FireSpecialShots() {
    const auto& config = PlayerShotConfigs[static_cast<size_t>(Player().m_playerType)];
    const int powerLevel = PowerLevel();
    const int projectileCount = Player().m_playerType == Spread ? config.projectileCount + powerLevel : config.projectileCount + powerLevel;
    const int damage = config.damage;
    constexpr float DegreesToRadians = 3.1415926535f / 180.0f;

    // 弾数に応じて左右対称の角度と発射位置を求める
    for (int i = 0; i < projectileCount; ++i) {
        const float centeredIndex = static_cast<float>(i) -
            static_cast<float>(projectileCount - 1) * 0.5f;
        const float angleStep = projectileCount > 1
            ? config.spreadAngleDegrees / static_cast<float>(projectileCount - 1)
            : 0.0f;
        const float angle = centeredIndex * angleStep * DegreesToRadians;
        const bool railGameplay = IsRailGameplayActive();
        const bool verticalRoute = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
        const float spawnY = verticalRoute ? Player().m_playerY + config.spawnOffsetX :
            (railGameplay ? Player().m_playerY :
                Player().m_playerY + centeredIndex * config.spawnOffsetY);
        const float railSpawnOffsetX = config.spawnOffsetY > 0.0f ? config.spawnOffsetY : 0.05f;

        // 空きスロットへ機体タイプ固有の属性を設定する
        for (int shotIndex = 0; shotIndex < ActiveShotCapacity(); ++shotIndex) {
            auto& shot = m_shots[shotIndex];
            if (shot.active) continue;
            shot = {};
            shot.owner = m_activePlayer;
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
                shot.x = verticalRoute ? Player().m_playerX + centeredIndex * config.spawnOffsetY :
                    (railGameplay ? Player().m_playerX + centeredIndex * railSpawnOffsetX :
                        Player().m_playerX + config.spawnOffsetX);
                shot.y = spawnY;
                shot.z = railGameplay ? PlayerRailDepth() + 2.0f : ToRailZFromSideX(shot.x);
                shot.transitionSideX = shot.x;
                shot.transitionSideY = shot.y;
                if (verticalRoute) {
                    // Stage 5第2部は視点に関わらず画面上方向を基準に拡散する
                    shot.vx = std::sin(angle) * config.speed;
                    shot.vy = std::cos(angle) * config.speed;
                    shot.vz = 0.0f;
                } else if (railGameplay) {
                    // 3Dレールでは特殊弾を奥行き方向へ進ませ、拡散角を横移動へ適用する
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
            shot.playerType = Player().m_playerType;
            shot.barrageIndex = i;
            // 発射口の側へ開き、対になる弾は同じ曲線で斉射ごとに巻き返す時刻をずらす
            if (shot.playerType == Homing) {
                const int pair = (std::min)(i, projectileCount - 1 - i);
                const bool negativeSide = verticalRoute ? centeredIndex > 0.0f : centeredIndex < 0.0f;
                shot.barrageIndex = (pair + m_frame / config.fireIntervalFrames % 3) * 2 + negativeSide;
            }
            shot.special = true;
            shot.piercing = config.piercing;
            ApplyAimSnap(shot);
            shot.active = true;
            break;
        }
    }
}

/**
 * @brief 追尾または画面上の照準距離で攻撃可能な標的を選ぶ
 * @param shot 標的を選ぶ自機弾
 * @param target 選んだ標的のワールド位置
 * @param aimCamera 非nullなら十字照準の近傍だけを選ぶ
 * @return 標的ID、対象なしなら-1
 */
int SideScrollingShooter::FindShotTarget(const Shot& shot, Vector3& target, const Camera3D* aimCamera) {
    const bool verticalRoute = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    const bool spatial = (IsRailGameplayActive() && !verticalRoute) || IsTayamaBattle();
    const Vector3 origin {ToWorldX(shot.x), ToWorldY(shot.y), spatial ? shot.z : 0.0f};
    Vector3 player = PlayerWorldPosition();
    if (!spatial) player = {ToWorldX(Player().m_playerX), ToWorldY(Player().m_playerY), 0.0f};
    Vector3 forward = IsTayamaBattle() ?
        Vector3 {-player.x, 0.0f, ShooterStages::Stage5::TayamaArenaCenterZ - player.z} :
        spatial ? Vector3 {0.0f, 0.0f, 1.0f} :
        verticalRoute ? Vector3 {0.0f, 1.0f, 0.0f} : Vector3 {1.0f, 0.0f, 0.0f};
    Vector2 aimScreen;
    if (aimCamera && !aimCamera->TryWorldToScreen(PlayerAimPoint(), aimScreen)) return -1;
    // 周回半径が通常の索敵距離を超えるアリーナでは対岸まで候補に含める
    float bestScore = IsTayamaBattle() ?
        4.0f * ShooterStages::Stage5::TayamaOrbitRadius * ShooterStages::Stage5::TayamaOrbitRadius : 10000.0f;
    int selected = -1;
    // 解像度に依存しない画面高比で、低難易度ほど広い範囲を捕捉する
    if (aimCamera) {
        const float radius = m_difficulty == Easy ? 0.09f : m_difficulty == Hard ? 0.035f : 0.06f;
        bestScore = radius * radius;
    }

    /** @brief 前方の候補を距離と継続性で比較する @param position ワールド中心 @param id 標的識別子 @return なし */
    const auto consider = [&](Vector3 position, int id) {
        const Vector3 visiblePosition = position;
        if (!spatial) position.z = 0.0f;
        const Vector3 delta = position - origin;
        if (!shot.bomb && delta.x * forward.x + delta.y * forward.y + delta.z * forward.z <= 0.0f) return;
        float score = HomingTargetScore((position - player).LengthSquared(), shot.homingTarget == id);
        if (aimCamera) {
            Vector2 screen;
            float depth;
            if (!aimCamera->TryWorldToScreen(visiblePosition, screen, &depth) || depth < 0.0f || depth > 1.0f ||
                !aimCamera->GetViewport().Contains(screen)) return;
            const Vector2 offset = (screen - aimScreen) / static_cast<float>(aimCamera->GetViewport().height);
            score = offset.LengthSquared();
        }
        if (score >= bestScore) return;
        bestScore = score;
        target = aimCamera ? visiblePosition : position;
        selected = id;
    };

    // 部位座標は衝突判定から取得し、攻撃不能な部位や導入中のボスを除外する
    for (int index = 0; index < static_cast<int>(m_enemies.size()); ++index) {
        const auto& enemy = m_enemies[index];
        if (!enemy.active || enemy.hp <= 0 || (m_chapterResultActive && !enemy.collisionEnabled)) continue;
        if (aimCamera && enemy.type != 2 && !enemy.collisionEnabled) continue;
        const int baseId = index * (BossPartCount + 1);
        if (enemy.type == 2) {
            if (m_bossIntroductionPhase != BossIntroductionPhase::None ||
                (!enemy.collisionEnabled && !StageDispatch::CanHitBossWhileCollisionDisabled(*this))) continue;
            bool hasPart = false;
            for (int i = 0; i < BossPartCount; ++i) {
                BossPart part = static_cast<BossPart>(i);
                Vector3 position;
                if (!TryHitBossPart(shot, enemy, part, &position)) continue;
                hasPart = true;
                consider(position, baseId + i + 1);
            }
            if (hasPart) continue;
            if (m_stageNumber == 2) {
                Vector3 position;
                if (Stage2Module::TryHitBossBody(*this, shot, enemy, &position)) consider(position, baseId);
                continue;
            }
            if (!enemy.collisionEnabled) continue;
        }
        Vector3 position {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z};
        // 第2部の追尾弾は衝突判定と同じ自機弾平面へ透視投影する
        if (verticalRoute && IsRailGameplayActive() && !aimCamera) {
            const Vector3 camera {ToWorldX(m_players[0].m_playerX) * 0.18f,
                ToWorldY(m_players[0].m_playerY) * 0.12f + 1.72f, PlayerRailDepth() - 21.5f};
            position = camera + (position - camera) * PerspectiveDepthScale(camera.z, enemy.z, shot.z);
        }
        consider(position, baseId);
    }

    // 通常敵プールに存在しない専用ボスも、実際の弱点や節を照準候補にする
    const int specialBase = static_cast<int>(m_enemies.size()) * (BossPartCount + 1);
    if (m_stageNumber == 5) {
        for (int i = 0; i < (std::max)(TayamaWeakpointCount, ShooterStages::Stage5::TayamaDragonSegmentCount); ++i) {
            Vector3 position;
            if (Stage5Module::GetHomingTarget(*this, i, position)) consider(position, specialBase + i);
        }
    }
    // 反射ファンネルも通常敵と同じ距離基準で選択する
    if (m_stageNumber == 3) {
        for (int i = 0; i < static_cast<int>(m_stage3.reflectFunnels.size()); ++i) {
            const auto& funnel = m_stage3.reflectFunnels[i];
            if (funnel.active && funnel.hp > 0)
                consider({ToWorldX(funnel.x), ToWorldY(funnel.y), funnel.z}, specialBase + 100 + i);
        }
    }
    if (m_stageNumber == 5 && m_stage5.phase == Stage5Phase::TayamaDragonBattle) {
        for (int i = 0; i < static_cast<int>(m_stage5.tayamaReflectFunnels.size()); ++i) {
            const auto& funnel = m_stage5.tayamaReflectFunnels[i];
            if (funnel.active && funnel.hp > 0)
                consider({ToWorldX(funnel.x), ToWorldY(funnel.y), funnel.z}, specialBase + 100 + i);
        }
    }
    return selected;
}

/** @brief 通常の十字照準のワールド位置を取得する @return 照準位置 */
Vector3 SideScrollingShooter::PlayerAimPoint() const {
    const Shot shot = MakeNormalPlayerShot(false);
    return Vector3 {ToWorldX(shot.x), ToWorldY(shot.y), shot.z} +
        Vector3 {ToWorldX(shot.vx), ToWorldY(shot.vy), shot.vz} * 15.0f;
}

/** @brief 発射と照準予測に共通の通常弾を作る @param snap スナップ補正を適用する場合true @return 通常弾 */
SideScrollingShooter::Shot SideScrollingShooter::MakeNormalPlayerShot(bool snap) const {
    const bool rail = IsRailGameplayActive();
    const bool vertical = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    Shot shot;
    shot.owner = m_activePlayer;
    shot.x = Player().m_playerX + (rail || vertical ? 0.0f : 0.12f);
    shot.y = Player().m_playerY + (vertical ? 0.12f : 0.0f);
    shot.z = rail ? PlayerRailDepth() + 2.0f : ToRailZFromSideX(shot.x);
    shot.vx = rail || vertical ? 0.0f : 0.045f;
    shot.vy = vertical ? 0.045f : 0.0f;
    shot.vz = rail && !vertical ? 1.45f : 0.0f;
    // 周回戦では実際の機首からアリーナ中心へ発射する
    if (IsTayamaBattle()) {
        const Vector3 player = PlayerWorldPosition();
        const Vector3 forward = Vector3 {-player.x, 0.0f, ShooterStages::Stage5::TayamaArenaCenterZ - player.z}.Normalized();
        const Vector3 origin = player + forward * 2.0f;
        shot.x = FromWorldX(origin.x);
        shot.y = FromWorldY(origin.y);
        shot.z = origin.z;
        shot.vx = FromWorldX(forward.x * 1.45f);
        shot.vy = 0.0f;
        shot.vz = forward.z * 1.45f;
    }
    shot.transitionSideX = shot.x;
    shot.transitionSideY = shot.y;
    if (snap) ApplyAimSnap(shot);
    return shot;
}

/** @brief 3D照準の対象を更新する @param advance 枠と弾道の補間を1フレーム進める場合true @return なし */
void SideScrollingShooter::UpdateAimSnap(bool advance) {
    Player().m_aimSnapped = false;
    if (m_viewMode != ViewMode::Rail3D || m_viewTransitionTimer > 0 || m_clear ||
        Player().m_playerDestructionTimer > 0 || StageDispatch::IsCinematic(*this)) {
        Player().m_aimSnapOffset = {};
        Player().m_aimSnapBlend = 0.0f;
        Player().m_aimSampleId = -1;
        Player().m_aimTargetVelocity = {};
        Player().m_crosshairInitialized = false;
        Player().m_crosshairVelocity = {};
        return;
    }

    // 協力プレイも描画と同じ1P基準のカメラから各自の照準距離を測る
    Camera3D camera;
    const int owner = m_activePlayer;
    m_activePlayer = 0;
    ConfigureRailCamera(camera, m_aimViewport);
    m_activePlayer = owner;
    const Vector3 player = PlayerWorldPosition();
    Shot probe;
    probe.x = FromWorldX(player.x);
    probe.y = FromWorldY(player.y);
    probe.z = player.z;
    const int targetId = FindShotTarget(probe, Player().m_aimSnapTarget, &camera);
    Player().m_aimSnapped = targetId >= 0;
    if (targetId != Player().m_aimSampleId) Player().m_aimTargetVelocity = {};

    // 発射前と命中判定後の再検索で二重に進めず、毎フレーム枠と補正量を滑らかに追従させる
    if (!advance) return;
    // 対象変更時は速度を引き継がず、発射前の再検索では観測履歴を進めない
    if (targetId >= 0 && targetId == Player().m_aimSampleId)
        Player().m_aimTargetVelocity = Player().m_aimSnapTarget - Player().m_aimSamplePosition;
    Player().m_aimSampleId = targetId;
    Player().m_aimSamplePosition = Player().m_aimSnapTarget;
    const Vector3 offset = Player().m_aimSnapped ? Player().m_aimSnapTarget - PlayerAimPoint() : Vector3::Zero;
    Player().m_aimSnapOffset = Vector3::Lerp(Player().m_aimSnapOffset, offset, 0.25f);
    // 高難易度では最大補正を弱め、自力で十字を合わせる余地を残す
    const float strength = m_difficulty == Easy ? 1.0f : m_difficulty == Hard ? 0.45f : 0.75f;
    const float blend = Player().m_aimSnapped ? strength : 0.0f;
    Player().m_aimSnapBlend = Math::Lerp(Player().m_aimSnapBlend, blend, 0.25f);
    if (!Player().m_aimSnapped && Player().m_aimSnapBlend < 0.001f) {
        Player().m_aimSnapOffset = {};
        Player().m_aimSnapBlend = 0.0f;
    }

    // 発射15フレーム後の通過点を画面へ投影し、速度を持つ十字を加速・減速させる
    const Shot preview = MakeNormalPlayerShot();
    const Vector3 future = Vector3 {ToWorldX(preview.x), ToWorldY(preview.y), preview.z} +
        Vector3 {ToWorldX(preview.vx), ToWorldY(preview.vy), preview.vz} * 15.0f;
    Vector2 screen;
    if (camera.TryWorldToScreen(future, screen)) {
        const Vector2 desired {screen.x / m_aimViewport.width * 2.0f - 1.0f,
            1.0f - screen.y / m_aimViewport.height * 2.0f};
        if (!Player().m_crosshairInitialized) {
            Player().m_crosshairPosition = desired;
            Player().m_crosshairInitialized = true;
        }
        Player().m_crosshairVelocity = Player().m_crosshairVelocity * 0.5f +
            (desired - Player().m_crosshairPosition) * 0.08f;
        Player().m_crosshairPosition += Player().m_crosshairVelocity;
    }
}

/** @brief 発射時の弾道を標的へ補正する @param shot 発射する自機弾 @return なし */
void SideScrollingShooter::ApplyAimSnap(Shot& shot) const {
    if (Player().m_aimSnapBlend <= 0.0f || m_viewMode != ViewMode::Rail3D || m_viewTransitionTimer > 0) return;
    const bool vertical = UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase);
    // 描画枠の追従遅れを射撃へ持ち込まず、捕捉中は最新の標的位置を使う
    Vector3 target = Player().m_aimSnapped ? Player().m_aimSnapTarget : PlayerAimPoint() + Player().m_aimSnapOffset;
    Vector3 targetVelocity = Player().m_aimSnapped ? Player().m_aimTargetVelocity : Vector3::Zero;
    const Vector3 origin {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};
    Vector3 forward = (PlayerAimPoint() - PlayerWorldPosition()).Normalized();
    if (vertical) {
        // 壁面の通常弾と特殊弾で異なる命中平面を、それぞれの衝突判定へ合わせる
        if (shot.special) {
            const Vector3 camera {ToWorldX(m_players[0].m_playerX) * 0.18f,
                ToWorldY(m_players[0].m_playerY) * 0.12f + 1.72f, PlayerRailDepth() - 21.5f};
            const Vector3 nextTarget = target + targetVelocity;
            const Vector3 projectedNext = camera + (nextTarget - camera) * PerspectiveDepthScale(camera.z, nextTarget.z, shot.z);
            target = camera + (target - camera) * PerspectiveDepthScale(camera.z, target.z, shot.z);
            targetVelocity = projectedNext - target;
        }
        target.z = shot.z;
        targetVelocity.z = 0.0f;
        forward = Vector3::Up;
    }
    // ponytail: 現在速度で等速予測するため急旋回は予測外、必要なら敵の軌道式で置き換える
    const Vector3 shotVelocity {ToWorldX(shot.vx), ToWorldY(shot.vy), shot.vz};
    target += targetVelocity * InterceptFrames(target - origin, targetVelocity, shotVelocity.Length());
    const Vector3 direction = Vector3::Lerp(forward, (target - origin).Normalized(), Player().m_aimSnapBlend).Normalized();
    if (Vector3::Dot(forward, direction) <= 0.0f) return;

    // 弾速と拡散角を維持したまま発射軸だけを回転する
    const Vector3 axis = Vector3::Cross(forward, direction);
    const Quaternion rotation = Quaternion {axis.x, axis.y, axis.z,
        1.0f + Vector3::Dot(forward, direction)}.Normalized();
    const Vector3 velocity = rotation.Rotate(shotVelocity);
    shot.vx = FromWorldX(velocity.x);
    shot.vy = FromWorldY(velocity.y);
    shot.vz = velocity.z;
}

/** @brief 自機から放射状に開いて巻き返し、うねりを収めながら標的へ追尾する @param shot 更新する追尾弾 @return なし */
void SideScrollingShooter::UpdateHomingShot(Shot& shot) {
    const bool spatial = (IsRailGameplayActive() && !UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase)) || IsTayamaBattle();
    const int view = (spatial ? 2 : 0) + (IsRailGameplayActive() ? 1 : 0);
    const bool changedPlane = shot.homingView != view;
    if (changedPlane) shot.homingTrailCount = 0;
    shot.homingView = view;
    // 実際に通った位置を残し、視点の座標系が変わった際は古い残光を破棄する
    shot.homingTrail[shot.age % shot.homingTrail.size()] = {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};
    shot.homingTrailCount = (std::min)(shot.homingTrailCount + 1, static_cast<int>(shot.homingTrail.size()));
    ++shot.age;
    // 発射後90フレームで誘導を終了し、再捕捉や視点切替後も現在の速度で画面外へ進む
    constexpr int HomingLifetimeFrames = 90;
    if (shot.age > HomingLifetimeFrames) {
        shot.homingTarget = -1;
        return;
    }
    const int previousTarget = shot.homingTarget;
    Vector3 target;
    shot.homingTarget = FindShotTarget(shot, target);
    const Vector3 position = spatial ? Vector3 {ToWorldX(shot.x), ToWorldY(shot.y), shot.z} :
        Vector3 {shot.x, shot.y, 0.0f};
    Vector3 velocity = spatial ? Vector3 {ToWorldX(shot.vx), ToWorldY(shot.vy), shot.vz} :
        Vector3 {shot.vx, shot.vy, 0.0f};
    const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
    const float speed = spatial ? 1.45f : config.speed;
    // 曲線の広がりを2Dの弾速と横方向のワールド倍率から決める
    const float curveSpeed = config.speed * (spatial ? WorldXScale : 1.0f);
    const int index = (std::max)(0, shot.barrageIndex);
    const int variant = index / 2 % 3;
    const float side = (index & 1) ? -1.0f : 1.0f;
    const int launchFrames = 18 + variant * 4;

    // 敵の位置に依存しない発射軸を保存し、再捕捉や視点切替で発射演出を繰り返さない
    if (shot.age == 1) {
        shot.homingOrigin = position;
        shot.homingLaunchDirection = velocity.Normalized();
    } else if (changedPlane || (previousTarget >= 0 && shot.homingTarget < 0)) {
        shot.homingLaunchDirection = {};
    }
    const bool launching = shot.age <= launchFrames && shot.homingLaunchDirection.LengthSquared() > 0.5f;
    if (!launching && shot.homingTarget < 0) return;
    Vector3 delta = spatial ? target - position :
        Vector3 {FromWorldX(target.x) - shot.x, FromWorldY(target.y) - shot.y, 0.0f};
    const float distance = delta.Length();
    if (!launching && distance <= 0.000001f) return;
    const Vector3 forward = launching ? shot.homingLaunchDirection : delta / distance;
    const Vector3 lateral = (spatial ? Vector3::Cross(
        std::abs(forward.y) < 0.95f ? Vector3::Up : Vector3::Right, forward).Normalized() :
        Vector3 {-forward.y, forward.x, 0.0f}) * side;
    if (launching) {
        // 狭い発射口から横・後方へ開き、先端が内側へ巻き返すエルミート曲線
        const float t = static_cast<float>(shot.age) / launchFrames;
        const float endWeight = t * t * (3.0f - 2.0f * t);
        const float startWeight = t * (t - 1.0f) * (t - 1.0f);
        const float endTangentWeight = t * t * (t - 1.0f);
        const float outward = (0.42f - 0.07f * variant) * endWeight +
            (0.95f - 0.20f * variant) * startWeight - 0.45f * endTangentWeight;
        const float advance = -(0.03f + 0.055f * variant) * endWeight -
            (0.25f + 0.35f * variant) * startWeight + 1.15f * endTangentWeight;
        const Vector3 next = shot.homingOrigin + (lateral * outward + forward * advance) * (curveSpeed * 18.0f);
        velocity = next - position;
    } else {
        // 巻き返し後のうねりを時間と距離で減衰させ、小さい部位へも収束する
        const float remaining = std::clamp(static_cast<float>(launchFrames + 42 - shot.age) / 42.0f, 0.0f, 1.0f);
        const float fade = remaining * std::clamp(distance / (speed * 6.0f) - 1.0f, 0.0f, 1.0f);
        const float phase = (shot.age - launchFrames) * 0.24f + variant * 0.8f;
        delta -= lateral * (std::cos(phase) * distance * 1.6f * fade * curveSpeed / speed);
        const float strength = (std::clamp)(speed * 2.0f / distance, config.homingStrength, 1.0f);
        velocity = velocity * (1.0f - strength) + delta.Normalized() * (speed * strength);
        const float length = velocity.Length();
        velocity = length > 0.000001f ? velocity * (speed / length) : forward * speed;
    }
    shot.vx = spatial ? FromWorldX(velocity.x) : velocity.x;
    shot.vy = spatial ? FromWorldY(velocity.y) : velocity.y;
    if (spatial) shot.vz = velocity.z;
}

/**
 * @brief 未破壊部位への衝突判定または攻撃可能な部位中心の取得を行う
 * @param shot 判定する自機弾
 * @param boss 判定するボス
 * @param part 命中部位の出力先、座標取得時は部位番号の入力
 * @param aimPosition 非nullなら衝突判定せず部位のワールド中心を出力する
 * @return 命中または座標取得に成功した場合true
 */
bool SideScrollingShooter::TryHitBossPart(
    const Shot& shot, const Enemy& boss, BossPart& part, Vector3* aimPosition) const {
    return StageDispatch::TryHitBossPart(*this, shot, boss, part, aimPosition);
}

/**
 * @brief 未破壊部位への衝突判定または攻撃可能な部位中心の取得を行う
 * @param shot 判定する自機弾
 * @param boss 判定するボス
 * @param part 命中部位の出力先、座標取得時は部位番号の入力
 * @param aimPosition 非nullなら衝突判定せず部位のワールド中心を出力する
 * @return 命中または座標取得に成功した場合true
 */
bool SideScrollingShooter::TryHitDefaultBossPart(
    const Shot& shot, const Enemy& boss, BossPart& part, Vector3* aimPosition) const {


    // 既存ボスモデルのローカル座標に対応する、破壊可能部位の中心と当たり判定半径
    constexpr float ModelScale = 0.14f;
    constexpr float PartX[] = { 0.0f, 17.0f, -17.0f, 6.0f, -6.0f };
    constexpr float PartY[] = { 3.0f, 2.0f, 2.0f, -6.0f, -6.0f };
    constexpr float PartZ[] = { -17.5f, 0.0f, 0.0f, 13.0f, 13.0f };
    constexpr float PartRadius[] = { 0.50f, 1.20f, 1.20f, 0.58f, 0.58f };

    for (int i = 0; i <= BossRightEngine; ++i) {
        if (boss.bossPartHp[i] <= 0 || (aimPosition && part != i)) continue;
        if (IsRailGameplayActive()) {
            const float partX = ToWorldX(boss.x) + PartX[i] * ModelScale;
            const float partY = ToWorldY(boss.y) + PartY[i] * ModelScale;
            const float partZ = boss.z + PartZ[i] * ModelScale;
            if (aimPosition) { *aimPosition = {partX, partY, partZ}; return true; }
            if (!HitShotSphere(shot, partX, partY, partZ, PartRadius[i])) {
                continue;
            }
        } else {
            // 2D表示ではY軸回転済みモデルの奥行きを画面X座標へ投影する
            const float partX = boss.x + PartZ[i] * ModelScale / WorldXScale;
            const float partY = boss.y + PartY[i] * ModelScale / WorldYScale;
            if (aimPosition) { *aimPosition = {ToWorldX(partX), ToWorldY(partY), boss.z}; return true; }
            if (!HitShotCircle(shot, partX, partY, PartRadius[i] / WorldXScale)) {
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
        ForEachPlayer([&] {
            if (Player().m_playerDestructionTimer > 0) return;
            if (explosion.effectType == 1 && (explosion.damagedPlayerMask & (1 << m_activePlayer)) == 0 &&
                explosion.age <= AttackWarningFrames && Player().m_invincible == 0) {
                const Vector3 player = PlayerWorldPosition();
                const float depthDistance = std::abs(player.z - explosion.z);
                const bool playerHit = IsRailGameplayActive() ?
                    depthDistance <= MortarExplosionDepthHitRadius &&
                        Hit(player.x, player.y, 0.38f,
                            ToWorldX(explosion.x), ToWorldY(explosion.y),
                            explosion.hitRadius * WorldXScale) :
                    Hit(Player().m_playerX, Player().m_playerY, 0.050f, explosion.x, explosion.y,
                        explosion.hitRadius);
                if (playerHit) {
                    explosion.damagedPlayerMask |= static_cast<unsigned char>(1 << m_activePlayer);
                    DamagePlayer();
                    return;
                }
            }
        });
        const int lifetime = explosion.effectType == 1 ? MortarExplosionLifetimeFrames :
            (explosion.destruction ? DestructionExplosionLifetimeFrames : ExplosionLifetimeFrames);
        if (++explosion.age >= lifetime) explosion.active = false;
    }
}

/** @brief 飛散中の機体部品を固定長プール順に更新する @return なし */
void SideScrollingShooter::TickDebris() {
    for (auto& debris : m_debris) {
        if (!debris.active || StageDispatch::TickSpecialDebris(*this, debris)) continue;
        const Vector3 previous {debris.x, debris.y, debris.z};
        debris.x += debris.vx;
        debris.y += debris.vy;
        debris.z += debris.vz;
        if (debris.gravity || m_stage->HasDebrisGravity()) debris.vy -= 0.006f;
        debris.yaw += debris.spin;

        // 危険ながれきだけを回転Boxの包含球で連続判定する
        ForEachPlayer([&] {
            if (Player().m_playerDestructionTimer > 0) return;
            if (debris.active && debris.damagesPlayer && Player().m_invincible == 0) {
                const Vector3 player = PlayerWorldPosition();
                const float debrisRadius = std::sqrt(debris.width * debris.width +
                    debris.height * debris.height + debris.depth * debris.depth) * 0.5f;
                if (Hit3DSegment(previous.x, previous.y, previous.z,
                    debris.x, debris.y, debris.z, debrisRadius,
                    player.x, player.y, player.z, 0.38f)) {
                    debris.active = false;
                    DamagePlayer();
                    return;
                }
            }
        });
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
 * @param damagesPlayer 自機との接触時に被弾させる場合true
 * @return 生成したデブリ、プール満杯の場合nullptr
 */
SideScrollingShooter::Debris* SideScrollingShooter::SpawnDebrisPiece(
    float x, float y, float z, float vx, float vy, float vz,
    float yaw, float spin, int shape, float width, float height, float depth,
    const float color[4], int lifetime, int shrinkStartAge, bool gravity,
    bool damagesPlayer) {
    for (auto& debris : m_debris) {
        if (debris.active) continue;
        debris = {x, y, z, vx, vy, vz, yaw, spin, width, height, depth,
            {color[0], color[1], color[2], color[3]}, shape, 0, lifetime,
            shrinkStartAge, {}, gravity, damagesPlayer, true};
        return &debris;
    }
    return nullptr;
}
