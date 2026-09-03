#include "Stage2Module.h"

#include <algorithm>
#include <cmath>

#include "../../../../Infrastructure/ExternalServices/AudioService.h"
#include "../../SideScrollingShooterEnemies.h"
#include "../Common/StageDefinition.h"
#include "Stage2BossModelView.h"
#include "Stage2EnemySheet.h"
#include "Stage2EnemySheetEasy.h"
#include "Stage2EnemySheetHard.h"
#include "Stage2EnemySheetNormal.h"

namespace {
using ShooterStages::Stage2::BossAction;
using ShooterStages::Stage2::DebrisKind;
using ShooterStages::Stage2::ShotKind;

constexpr int Phase3FunnelEngineStartFrame = 26;
constexpr int FunnelLaunchCullGraceFrames = 45;
constexpr int DefeatExplosionIntervalFrames = 72;
constexpr int DefeatExplosionCount = 3;
constexpr float Phase3FunnelLaunchVelocity = 0.09f;
constexpr float Phase3FunnelGravity = 0.0035f;
constexpr float Phase3FunnelRise = Phase3FunnelEngineStartFrame * Phase3FunnelLaunchVelocity -
    Phase3FunnelGravity * Phase3FunnelEngineStartFrame *
        (Phase3FunnelEngineStartFrame + 1) * 0.5f;
constexpr float DesertBoneColor[4] = {0.88f, 0.78f, 0.56f, 1.0f};

static_assert(Phase3FunnelRise > 0.63f);
static_assert(Phase3FunnelLaunchVelocity -
    Phase3FunnelGravity * Phase3FunnelEngineStartFrame < 0.0f);
static_assert(Phase3FunnelEngineStartFrame < FunnelLaunchCullGraceFrames);

}

/** @brief Stage 2巨大戦艦ボス専用Behavior */
class SideScrollingShooter::Stage2Module::BossBehavior final
    : public SideScrollingShooter::EnemyBehavior {
public:
    /** @brief 敵種別を取得する @return Stage 2ボスの敵種別 */
    int Type() const override { return 2; }

    /** @brief 最大HPを取得する @return Stage 2ボスの最大HP */
    int MaxHp() const override { return 1200; }

    /** @brief 自機狙い弾の間隔を取得する @return 発射間隔フレーム数 */
    int AimedShotInterval() const override { return 78; }

    /** @brief 横視点の自機狙い弾速を取得する @return ゲーム座標系の弾速 */
    float AimedShotSpeed() const override { return 0.016f; }

    /** @brief レール視点の自機狙い弾速を取得する @return ワールド座標系の弾速 */
    float RailAimedShotSpeed() const override { return 0.58f; }

    /**
     * @brief 横視点の接触半径を取得する
     * @param enemy 判定対象
     * @return ゲーム座標系の接触半径
     */
    float CollisionRadius(const Enemy& enemy) const override {
        (void)enemy;
        return 0.72f;
    }

    /**
     * @brief レール視点の接触半径を取得する
     * @param enemy 判定対象
     * @return ワールド座標系の接触半径
     */
    float CollisionRadius3D(const Enemy& enemy) const override {
        (void)enemy;
        return 5.8f;
    }

    /**
     * @brief レール視点の被弾半径を取得する
     * @param enemy 判定対象
     * @return ワールド座標系の被弾半径
     */
    float ShotHitRadius3D(const Enemy& enemy) const override {
        (void)enemy;
        return 5.2f;
    }

    /**
     * @brief Stage 2ボスを初期配置する
     * @param enemy 初期化するボス
     * @param railMode レール表示中の場合true
     * @param stageIndex ステージ番号
     * @return なし
     */
    void ConfigureBossSpawn(Enemy& enemy, bool railMode, int stageIndex) const override {
        Stage2Module::ConfigureBossSpawn(enemy, railMode, stageIndex);
    }

    /**
     * @brief Stage 2ボスの状態を更新する
     * @param shooter 更新するゲーム本体
     * @param enemy 更新するボス
     * @return なし
     */
    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        Stage2Module::TickBoss(shooter, enemy);
    }
};

const SideScrollingShooter::Stage& SideScrollingShooter::Stage2Module::Definition(
    DifficultyType difficulty) {
    static const Stage2EnemySheetEasy easyStage;
    static const Stage2EnemySheetNormal normalStage;
    static const Stage2EnemySheetHard hardStage;
    switch (difficulty) {
    case Hard: return hardStage;
    case Normal: return normalStage;
    default: return easyStage;
    }
}

void SideScrollingShooter::Stage2Module::Reset(SideScrollingShooter& shooter) {
    shooter.m_stage2 = {};
}

const SideScrollingShooter::EnemyBehavior&
SideScrollingShooter::Stage2Module::BossBehaviorInstance() {
    static const BossBehavior behavior;
    return behavior;
}

void SideScrollingShooter::Stage2Module::ConfigureBossSpawn(
    Enemy& boss, bool railMode, int stageIndex) {
    (void)stageIndex;

    // EnemyBehavior::ConfigureStatsと同じ順序でボス固有値を設定する
    boss.type = 2;
    boss.behavior = &BossBehaviorInstance();
    boss.hp = 1200;
    boss.maxHp = boss.hp;
    boss.shotInterval = 78;
    boss.age = 0;
    boss.motionAge = 0;

    // 合体した上下船体を画面右側またはレール奥へ配置する
    boss.active = true;
    boss.x = railMode ? 0.0f : 1.80f;
    boss.y = 0.0f;
    boss.z = railMode ? 48.0f : ToRailZFromSideX(boss.x);
    boss.baseX = boss.x;
    boss.baseY = boss.y;
    boss.baseZ = boss.z;
    boss.phase = 0.0f;
    boss.motionAge = 0;
    boss.collisionEnabled = true;
}

void SideScrollingShooter::Stage2Module::TickBoss(
    SideScrollingShooter& shooter, Enemy& boss) {
    // 砲塔の照準だけを遅れて追従させ、戦艦本体と主人公の動きから慣性を感じられるようにする
    constexpr float TurretTrackingRate = 0.06f;
    boss.turretAimX += (shooter.m_playerX - boss.turretAimX) * TurretTrackingRate;
    boss.turretAimY += (shooter.m_playerY - boss.turretAimY) * TurretTrackingRate;
    const float turretTargetZ = shooter.IsRailGameplayActive() ?
        PlayerRailZ : ToRailZFromSideX(shooter.m_playerX);
    boss.turretAimZ += (turretTargetZ - boss.turretAimZ) * TurretTrackingRate;

    // HP割合で三つの船体状態を切り替える
    const float hpRate = boss.maxHp > 0 ? static_cast<float>(boss.hp) / boss.maxHp : 0.0f;
    const int nextPhase = hpRate > 0.65f ? 1 : (hpRate > 0.35f ? 2 : 3);
    if (static_cast<int>(boss.phase) != nextPhase) {
        const float previousSubmarineWorldY = SubmarineWorldY(shooter, boss);
        const bool startsSeparation = boss.phase < 2.0f && nextPhase >= 2;
        boss.phase = static_cast<float>(nextPhase);
        ChangeBossAction(shooter, nextPhase == 3 ? BossAction::Separating : BossAction::Idle);
        if (nextPhase == 3) {
            // 主砲照準を現在の追従位置から始め、Phase切替時の瞬間的な張り付きを防ぐ
            boss.actionX = boss.turretAimX;
            boss.actionY = boss.turretAimY;
            boss.actionZ = boss.turretAimZ;
            // Phase 3開始時から各ハッチの初回射出を時間差にする
            for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
                shooter.m_stage2.boss.funnelLaunchCooldowns[hatch] = 12 + hatch * 7;
                shooter.m_stage2.boss.funnelLaunchCounts[hatch] = 0;
            }
        }
        boss.collisionEnabled = true;
        boss.x = boss.baseX;
        boss.y = boss.baseY;
        boss.z = shooter.IsRailGameplayActive() ? boss.baseZ : ToRailZFromSideX(boss.x);
        if (startsSeparation) {
            // 接地済みのPhase 1位置を初期オフセットへ移し、分離開始フレームの跳びを防ぐ
            shooter.m_stage2.boss.sandSubmarineOffsetY +=
                previousSubmarineWorldY - SubmarineWorldY(shooter, boss);
        }
    }

    // 行動更新後にactionAgeを加算してCombat側との1フレーム差を維持する
    if (nextPhase == 1) TickBossPhase1(shooter, boss);
    else if (nextPhase == 2) TickBossPhase2(shooter, boss);
    else TickBossPhase3(shooter, boss);
    ++shooter.m_stage2.boss.actionAge;
}

bool SideScrollingShooter::Stage2Module::HandleBossInteractionAfterTick(
    SideScrollingShooter& shooter, Enemy& boss) {
    // POST-incrementのactionAgeで発射フレームのレールガン判定を行う
    const int beamCycle = shooter.m_stage2.boss.actionAge % RailgunCycleFrames;
    if (boss.phase >= 3.0f && boss.bossPartHp[BossNose] > 0 &&
        beamCycle == RailgunFireFrame && shooter.m_invincible == 0) {
        constexpr float BossScale = 1.92f;
        const float yaw = shooter.IsRailGameplayActive() ? 0.0f : Math::HalfPi;
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        const float patrolX = std::sin(static_cast<float>(boss.age) * 0.018f) *
            2.4f * shooter.RailBlend();
        const float battleshipX = ToWorldX(
            boss.x + shooter.m_stage2.boss.landBattleshipOffsetX) + patrolX;
        const float battleshipY = BattleshipWorldY(shooter, boss);
        const float battleshipZ = boss.z + shooter.m_stage2.boss.landBattleshipOffsetZ;
        const float startX = battleshipX - 1.55f * cosine * BossScale;
        const float startY = battleshipY + 2.08f * BossScale;
        const float startZ = battleshipZ + 1.55f * sine * BossScale;
        const float targetX = ToWorldX(boss.actionX);
        const float targetY = ToWorldY(boss.actionY);
        const float targetZ = shooter.IsRailGameplayActive() ? boss.actionZ : SidePlaneZ;
        const Vector3 direction = Vector3 {
            targetX - startX, targetY - startY, targetZ - startZ}.Normalized();
        if (Hit3DSegment(startX, startY, startZ,
            targetX + direction.x * 18.0f,
            targetY + direction.y * 18.0f,
            targetZ + direction.z * 18.0f, 0.52f,
            ToWorldX(shooter.m_playerX), ToWorldY(shooter.m_playerY),
            shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ, 0.38f)) {
            shooter.DamagePlayer();
            return true;
        }
    }

    // 全Phaseで上部戦艦の船体と骨アーチとの接触を判定する
    if (!shooter.m_stage2.boneArchDestroyed) {
        constexpr float BodyLocalX = 0.55f;
        constexpr float BodyLocalY = 0.95f;
        constexpr float ModelScale = 1.92f;
        constexpr float BodyRadius = 4.25f;
        const float yaw = shooter.IsRailGameplayActive() ? 0.0f : Math::HalfPi;
        const float patrolWorldX = std::sin(static_cast<float>(boss.age) * 0.018f) *
            2.4f * shooter.RailBlend();
        const Vector3 bodyCenter {
            ToWorldX(boss.x + shooter.m_stage2.boss.landBattleshipOffsetX) +
                patrolWorldX + std::cos(yaw) * BodyLocalX * ModelScale,
            BattleshipWorldY(shooter, boss) + BodyLocalY * ModelScale,
            boss.z + shooter.m_stage2.boss.landBattleshipOffsetZ -
                std::sin(yaw) * BodyLocalX * ModelScale
        };
        if (HitsHazard(shooter, FromWorldX(bodyCenter.x), FromWorldY(bodyCenter.y),
            bodyCenter.z, BodyRadius / WorldXScale)) {
            DestroyBoneArch(shooter);
            shooter.SpawnExplosion(
                FromWorldX(bodyCenter.x), FromWorldY(bodyCenter.y), bodyCenter.z);
            shooter.PlayHitSound();
        }
    }
    return false;
}

bool SideScrollingShooter::Stage2Module::IsBossSpecialAttackActive(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    (void)boss;
    return shooter.m_stage2.boss.action != BossAction::Idle &&
        shooter.m_stage2.boss.action != BossAction::MainGunCooldown;
}

void SideScrollingShooter::Stage2Module::ChangeBossAction(
    SideScrollingShooter& shooter, BossAction action) {
    shooter.m_stage2.boss.action = action;
    shooter.m_stage2.boss.actionAge = -1;
}

void SideScrollingShooter::Stage2Module::TickBossPhase1(
    SideScrollingShooter& shooter, Enemy& boss) {
    boss.collisionEnabled = true;
    boss.x = boss.baseX + std::sin(boss.age * 0.006f) * 0.045f;
    boss.y = boss.baseY + std::sin(boss.age * 0.012f) * 0.10f;
    if (!shooter.IsRailGameplayActive()) boss.z = ToRailZFromSideX(boss.x);

    // 潜砂艦の左右ハッチから交互に自機狙いミサイルを発射する
    if (boss.age % 90 == 0 || boss.age % 90 == 10) {
        const float side = boss.age % 90 == 0 ? -1.0f : 1.0f;
        SpawnMissile(shooter, boss.x, boss.y - 0.10f, boss.z + side * 1.55f, side);
    }
}

void SideScrollingShooter::Stage2Module::TickBossPhase2(
    SideScrollingShooter& shooter, Enemy& boss) {
    constexpr float SubmarineBuriedOffsetY = -4.4f;
    boss.collisionEnabled = false;
    shooter.m_stage2.boss.sandSubmarineOffsetY +=
        (SubmarineBuriedOffsetY - shooter.m_stage2.boss.sandSubmarineOffsetY) * 0.08f;
    shooter.m_stage2.boss.sandSubmarineOffsetX +=
        (shooter.m_playerX - boss.x - shooter.m_stage2.boss.sandSubmarineOffsetX) * 0.025f;
    const float targetZ = shooter.IsRailGameplayActive() ? PlayerRailZ - boss.z : 0.0f;
    shooter.m_stage2.boss.sandSubmarineOffsetZ +=
        (targetZ - shooter.m_stage2.boss.sandSubmarineOffsetZ) * 0.025f;
    shooter.m_stage2.boss.landBattleshipOffsetY +=
        (-0.45f - shooter.m_stage2.boss.landBattleshipOffsetY) * 0.06f;
    if (boss.age % 120 < 3) {
        const int launchIndex = boss.age % 120;
        LaunchFunnel(shooter, boss, boss.age / 120 * 3 + launchIndex, false);
    }
}

void SideScrollingShooter::Stage2Module::TickBossPhase3(
    SideScrollingShooter& shooter, Enemy& boss) {
    constexpr float SubmarineBuriedOffsetY = -4.4f;
    if (shooter.m_stage2.boss.action == BossAction::Separating) {
        const float t = Math::Clamp01(
            static_cast<float>(shooter.m_stage2.boss.actionAge) / 60.0f);
        const float smooth = t * t * (3.0f - 2.0f * t);
        shooter.m_stage2.boss.landBattleshipOffsetY +=
            (0.45f - shooter.m_stage2.boss.landBattleshipOffsetY) * 0.06f;
        shooter.m_stage2.boss.sandSubmarineOffsetY = Math::Lerp(
            shooter.m_stage2.boss.sandSubmarineOffsetY, SubmarineBuriedOffsetY, smooth);
        // 地中を掘り進む潜砂艦はPhase 3移行中からゆっくり戦艦直下へ近づける
        shooter.m_stage2.boss.sandSubmarineOffsetX *= 0.995f;
        shooter.m_stage2.boss.sandSubmarineOffsetZ *= 0.995f;
        if (shooter.m_stage2.boss.actionAge >= 60) {
            ChangeBossAction(shooter, BossAction::Idle);
        }
        return;
    }

    // 潜砂艦は上面だけを砂上へ残し、生存中の側面ハッチからファンネルを連続射出する
    boss.collisionEnabled = false;
    shooter.m_stage2.boss.sandSubmarineOffsetX *= 0.985f;
    shooter.m_stage2.boss.sandSubmarineOffsetZ *= 0.985f;
    shooter.m_stage2.boss.sandSubmarineOffsetY +=
        (SubmarineBuriedOffsetY - shooter.m_stage2.boss.sandSubmarineOffsetY) * 0.08f;
    const int beamCycle = shooter.m_stage2.boss.actionAge % RailgunCycleFrames;
    if (beamCycle == 0) {
        boss.attackWarningFrames = RailgunFireFrame;
        boss.actionX = boss.turretAimX;
        boss.actionY = boss.turretAimY;
        boss.actionZ = boss.turretAimZ;
    }
    if (beamCycle < RailgunFireFrame) {
        // 予告の前後はゆっくり、中央は素早く追従し、発射時に現在の照準へ固定する
        const float trackingRate = ShooterStages::Stage2::Phase3MainGunTrackingRate(
            beamCycle, RailgunFireFrame);
        boss.actionX += (shooter.m_playerX - boss.actionX) * trackingRate;
        boss.actionY += (shooter.m_playerY - boss.actionY) * trackingRate;
        const float targetZ = shooter.IsRailGameplayActive() ?
            PlayerRailZ : ToRailZFromSideX(shooter.m_playerX);
        boss.actionZ += (targetZ - boss.actionZ) * trackingRate;
    }
    if (beamCycle == RailgunFireFrame && boss.bossPartHp[BossNose] > 0) {
        PlayRailgunSound(shooter);
    }
    // 各ハッチを独立したランダム間隔で待機させ、同一フレームの一斉射を避ける
    for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
        if (boss.bossPartHp[BossFunnelHatch0 + hatch] <= 0) continue;
        int& cooldown = shooter.m_stage2.boss.funnelLaunchCooldowns[hatch];
        if (--cooldown > 0) continue;
        LaunchFunnel(shooter, boss, hatch, true);
        const int launchCount = shooter.m_stage2.boss.funnelLaunchCounts[hatch]++;
        cooldown = ShooterStages::Stage2::Phase3FunnelLaunchInterval(hatch, launchCount);
        break;
    }
    if (boss.age % 72 == 0) FireBossPartBarrage(shooter, boss);
}

void SideScrollingShooter::Stage2Module::LaunchFunnel(
    SideScrollingShooter& shooter, Enemy& boss, int sequence, bool delayedEngine) {
    int hatch = -1;
    for (int offset = 0; offset < BossFunnelHatchCount; ++offset) {
        const int candidate = (sequence + offset) % BossFunnelHatchCount;
        if (boss.bossPartHp[BossFunnelHatch0 + candidate] > 0) {
            hatch = candidate;
            break;
        }
    }
    if (hatch < 0) return;

    // オレンジ色ハッチの外面からファンネル全体が見える位置を親Transformへ合成する
    constexpr float ModelScale = 1.92f;
    constexpr float HatchCenterZ = 1.80f;
    constexpr float HatchHalfDepth = 0.04f;
    constexpr float FunnelRadius = 0.30f;
    constexpr float LaunchSurfaceZ = HatchCenterZ + HatchHalfDepth + FunnelRadius / ModelScale;
    const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
    const float localZ = (hatch < 6 ? -1.0f : 1.0f) * LaunchSurfaceZ;
    const bool railMode = shooter.IsRailGameplayActive();
    const bool separated = boss.phase >= 2.0f;
    const float yaw = (railMode ? 0.0f : Math::HalfPi) +
        (separated ? Math::HalfPi : 0.0f);
    const float offsetWorldX =
        (localX * std::cos(yaw) + localZ * std::sin(yaw)) * ModelScale;
    const float offsetWorldZ =
        (-localX * std::sin(yaw) + localZ * std::cos(yaw)) * ModelScale;
    SpawnFunnel(shooter,
        boss.x + shooter.m_stage2.boss.sandSubmarineOffsetX + FromWorldX(offsetWorldX),
        FromWorldY(SubmarineWorldY(shooter, boss) - 0.22f * ModelScale),
        boss.z + shooter.m_stage2.boss.sandSubmarineOffsetZ + offsetWorldZ,
        delayedEngine);
}

void SideScrollingShooter::Stage2Module::SpawnFunnel(
    SideScrollingShooter& shooter, float x, float y, float z, bool delayedEngine) {
    for (auto& shot : shooter.m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = 0.0f;
        // Phase 3は砂面を越えてから短く落下する高さまで点火前の射出初速を与える
        shot.vy = delayedEngine ? Phase3FunnelLaunchVelocity : 0.018f;
        shot.vz = 0.0f;
        shot.hitRadius = 0.055f;
        shot.damage = 2;
        shot.enemy = true;
        shot.stage2.kind = ShotKind::Funnel;
        shot.stage2.delayedEngine = delayedEngine;
        shot.active = true;

        // 短いノイズを初回だけ合成し、ファンネル射出の風切り音として再生する
        if (shooter.m_audio) {
            static const std::vector<int16_t> launchSound = [] {
                Audio::SfxrParams sound;
                sound.waveType = Audio::SfxrWaveType::Noise;
                sound.attackTime = 0.025f;
                sound.sustainTime = 0.10f;
                sound.decayTime = 0.18f;
                sound.masterVolume = 0.32f;
                return Audio::SfxrGenerator::GeneratePCM(sound);
            }();
            shooter.m_audio->PlaySE(launchSound);
        }
        return;
    }
}

void SideScrollingShooter::Stage2Module::SpawnMissile(
    SideScrollingShooter& shooter, float x, float y, float z, float side) {
    for (auto& shot : shooter.m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        const float dx = ToWorldX(shooter.m_playerX - x);
        const float dy = ToWorldY(shooter.m_playerY - y);
        const float dz = shooter.IsRailGameplayActive() ? PlayerRailZ - z : 0.0f;
        const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
        constexpr float MissileSpeed = 0.68f;
        shot.vx = FromWorldX(dx / length * MissileSpeed) + side * 0.006f;
        shot.vy = FromWorldY(dy / length * MissileSpeed);
        shot.vz = dz / length * MissileSpeed;
        shot.hitRadius = 0.045f;
        shot.damage = 2;
        shot.enemy = true;
        shot.stage2.kind = ShotKind::Missile;
        shot.active = true;
        shooter.PlayMissileLaunchSound();
        return;
    }
}

bool SideScrollingShooter::Stage2Module::HitsHazard(
    const SideScrollingShooter& shooter, float x, float y, float z, float radius) {
    constexpr int BoneCount = 13;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    if (shooter.m_stage2.boneArchDestroyed) return false;
    const float phase = std::fmod(shooter.m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(shooter.m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 描画と同じ十三関節を横視点またはレール視点の球として判定する
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) /
            static_cast<float>(BoneCount - 1);
        if (shooter.IsRailGameplayActive()) {
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                std::cos(angle) * RailRadius,
                RailCenterY + std::sin(angle) * RailRadius, railZ, 1.35f)) {
                return true;
            }
        } else if (Hit(x, y, radius, sideCenterX,
            -1.30f + static_cast<float>(i) * 0.24f, 0.32f)) {
            return true;
        }
    }
    return false;
}

bool SideScrollingShooter::Stage2Module::TryDamageStageTarget(
    SideScrollingShooter& shooter, Shot& shot) {
    if (shot.enemy) return false;
    if (!shooter.m_stage2.boneArchDestroyed &&
        HitsHazard(shooter, shot.x, shot.y, shot.z, shot.hitRadius)) {
        shooter.SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        shooter.m_stage2.boneArchHp -= shot.damage;
        if (shooter.m_stage2.boneArchHp <= 0) DestroyBoneArch(shooter);
        shooter.PlayHitSound();
        return true;
    }
    return false;
}

void SideScrollingShooter::Stage2Module::DestroyBoneArch(SideScrollingShooter& shooter) {
    constexpr int BoneCount = 13;
    constexpr int Lifetime = 150;
    constexpr int ShrinkStartAge = 90;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    constexpr float SideZ = SidePlaneZ + 1.2f;
    const float railWeight = shooter.RailBlend();
    const float phase = std::fmod(shooter.m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(shooter.m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 描画中の各関節位置から小さな骨を外向きに飛散させる
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) /
            static_cast<float>(BoneCount - 1);
        const float x = Math::Lerp(ToWorldX(sideCenterX),
            std::cos(angle) * RailRadius, railWeight);
        const float y = Math::Lerp(
            ToWorldY(-1.30f + static_cast<float>(i) * 0.24f),
            RailCenterY + std::sin(angle) * RailRadius, railWeight);
        const float z = Math::Lerp(SideZ, railZ, railWeight);
        const float scatterAngle = angle + static_cast<float>(i % 3 - 1) * 0.35f;
        const float size = 0.30f + static_cast<float>(i % 3) * 0.08f;
        shooter.SpawnDebrisPiece(x, y, z,
            std::cos(scatterAngle) * 0.055f,
            0.055f + static_cast<float>(i % 4) * 0.012f,
            railWeight > 0.5f ? std::sin(scatterAngle) * 0.055f : 0.0f,
            angle, (i % 2 == 0 ? 0.08f : -0.08f) * (1.0f + i * 0.04f),
            5, size, size * 1.35f, size * 0.75f, DesertBoneColor,
            Lifetime, ShrinkStartAge, true);
    }

    shooter.m_stage2.boneArchHp = 0;
    shooter.m_stage2.boneArchDestroyed = true;
}

float SideScrollingShooter::Stage2Module::Phase1SubmarineWorldY(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    constexpr float SideGroundTopY = -6.0f;
    constexpr float RailGroundTopY = -3.65f;
    constexpr float CutterTopLocalY = -0.99f;
    constexpr float CutterEmbedDepth = 0.55f;
    constexpr float ModelScale = 1.92f;
    constexpr float Phase1BobWorldAmplitude = 0.10f * WorldYScale;
    static_assert(CutterEmbedDepth > Phase1BobWorldAmplitude);

    // 2Dと3Dの地面上面を補間し、上下動の最高点でも切削爪を地中へ残す
    const float groundTopY = Math::Lerp(
        SideGroundTopY, RailGroundTopY, shooter.RailBlend());
    return groundTopY - CutterTopLocalY * ModelScale - CutterEmbedDepth + ToWorldY(boss.y);
}

float SideScrollingShooter::Stage2Module::SubmarineWorldY(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    const float assembledY = boss.phase < 2.0f ? Phase1SubmarineWorldY(shooter, boss) :
        ToWorldY(boss.y) - 1.45f + shooter.m_stage2.boss.sandSubmarineOffsetY;
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return assembledY;
    }

    // 前半で地中から砂面直下まで掘り進み、後半で上部戦艦との合体位置まで浮上する
    const float approach = SmoothStep(static_cast<float>(shooter.m_bossIntroductionTimer) /
        static_cast<float>(BossApproachFrames));
    const float assembly = SmoothStep(
        static_cast<float>(shooter.m_bossIntroductionTimer - BossApproachFrames) /
        static_cast<float>(BossAssemblyFrames));
    return assembledY + Math::Lerp(-7.0f, -2.4f, approach) * (1.0f - assembly);
}

float SideScrollingShooter::Stage2Module::BattleshipWorldY(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    constexpr float SideGroundTopY = -6.0f;
    constexpr float RailGroundTopY = -3.65f;
    constexpr float HullBottomLocalY = 0.02f;
    constexpr float HoverHeight = 0.18f;
    constexpr float ModelScale = 1.92f;
    constexpr float AssembledUnitOffsetY = 0.77f;
    if (boss.phase >= 2.0f && boss.phase < 3.0f) {
        // Phase 2開始時の高度から地表付近まで滑らかに下降する
        constexpr float DescentFrames = 120.0f;
        const float t = Math::Clamp01(
            static_cast<float>(shooter.m_stage2.boss.actionAge) / DescentFrames);
        const float smooth = t * t * (3.0f - 2.0f * t);
        const float startY = Phase1SubmarineWorldY(shooter, boss) + AssembledUnitOffsetY;
        const float targetY = Math::Lerp(
            SideGroundTopY, RailGroundTopY, shooter.RailBlend()) +
            HoverHeight - HullBottomLocalY * ModelScale;
        return Math::Lerp(startY, targetY, smooth);
    }
    const float assembledY = boss.phase < 2.0f ?
        Phase1SubmarineWorldY(shooter, boss) + AssembledUnitOffsetY :
        ToWorldY(boss.y) + 1.32f + shooter.m_stage2.boss.landBattleshipOffsetY;
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return assembledY;
    }

    // 前半で上空から降下し、後半で潜砂艦との合体位置へ接近する
    const float approach = SmoothStep(static_cast<float>(shooter.m_bossIntroductionTimer) /
        static_cast<float>(BossApproachFrames));
    const float assembly = SmoothStep(
        static_cast<float>(shooter.m_bossIntroductionTimer - BossApproachFrames) /
        static_cast<float>(BossAssemblyFrames));
    return assembledY + Math::Lerp(13.0f, 4.2f, approach) * (1.0f - assembly);
}

bool SideScrollingShooter::Stage2Module::TryHitBossBody(
    const SideScrollingShooter& shooter, const Shot& shot, const Enemy& boss) {
    constexpr float BodyLocalX = 0.55f;
    constexpr float BodyLocalY = 0.95f;
    constexpr float ModelScale = 1.92f;
    constexpr float BodyRadius = 4.25f;
    const bool railMode = shooter.IsRailGameplayActive();
    const float yaw = railMode ? 0.0f : Math::HalfPi;
    const float patrolX = boss.phase >= 2.0f ?
        std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * shooter.RailBlend() : 0.0f;
    const Vector3 center {
        ToWorldX(boss.x + shooter.m_stage2.boss.landBattleshipOffsetX) + patrolX +
            std::cos(yaw) * BodyLocalX * ModelScale,
        BattleshipWorldY(shooter, boss) + BodyLocalY * ModelScale,
        boss.z + shooter.m_stage2.boss.landBattleshipOffsetZ -
            std::sin(yaw) * BodyLocalX * ModelScale
    };
    return railMode ?
        Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            center.x, center.y, center.z, BodyRadius) :
        Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(center.x), FromWorldY(center.y),
            BodyRadius / WorldXScale);
}

bool SideScrollingShooter::Stage2Module::TryHitBossPart(
    const SideScrollingShooter& shooter, const Shot& shot,
    const Enemy& boss, BossPart& part) {
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
    const bool railMode = shooter.IsRailGameplayActive();
    const float battleshipYaw = railMode ? 0.0f : Math::HalfPi;
    const bool separated = boss.phase >= 2.0f;
    const float battleshipPatrolX = separated ?
        std::sin(static_cast<float>(boss.age) * 0.018f) *
            2.4f * shooter.RailBlend() : 0.0f;

    // 武装全破壊までは装甲内の接続コアを命中対象にしない
    for (int i = 0; i <= BossRightEngine; ++i) {
        if (boss.bossPartHp[i] <= 0 || (i == BossRightEngine && !weaponsDestroyed)) continue;
        const Vector3& local = PartPosition[i];
        const bool submarinePart = i == BossRightEngine;
        const float yaw = battleshipYaw +
            (submarinePart && separated ? Math::HalfPi : 0.0f);
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        const float unitOffsetX = submarinePart ?
            ToWorldX(shooter.m_stage2.boss.sandSubmarineOffsetX) :
            ToWorldX(shooter.m_stage2.boss.landBattleshipOffsetX) + battleshipPatrolX;
        const float worldY = submarinePart ?
            SubmarineWorldY(shooter, boss) : BattleshipWorldY(shooter, boss);
        const float unitOffsetZ = submarinePart ?
            shooter.m_stage2.boss.sandSubmarineOffsetZ :
            shooter.m_stage2.boss.landBattleshipOffsetZ;
        const Vector3 world {
            ToWorldX(boss.x) + unitOffsetX +
                (local.x * cosine + local.z * sine) * ModelScale,
            worldY + local.y * ModelScale,
            boss.z + unitOffsetZ +
                (-local.x * sine + local.z * cosine) * ModelScale
        };
        const bool hit = railMode ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy),
                shot.z - shot.vz, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale,
                world.x, world.y, world.z, PartRadius[i]) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y), PartRadius[i] / WorldXScale);
        if (!hit) continue;
        part = static_cast<BossPart>(i);
        return true;
    }

    // 側面のオレンジ色の小窓十二基を描画と同じローカル座標で個別判定する
    for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
        const int partIndex = BossFunnelHatch0 + hatch;
        if (boss.bossPartHp[partIndex] <= 0) continue;
        const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
        const float localZ = (hatch < 6 ? -1.0f : 1.0f) * 1.80f;
        const float submarineYaw = battleshipYaw + (separated ? Math::HalfPi : 0.0f);
        const float cosine = std::cos(submarineYaw);
        const float sine = std::sin(submarineYaw);
        const Vector3 world {
            ToWorldX(boss.x + shooter.m_stage2.boss.sandSubmarineOffsetX) +
                (localX * cosine + localZ * sine) * ModelScale,
            SubmarineWorldY(shooter, boss) - 0.22f * ModelScale,
            boss.z + shooter.m_stage2.boss.sandSubmarineOffsetZ +
                (-localX * sine + localZ * cosine) * ModelScale
        };
        const bool hit = railMode ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy),
                shot.z - shot.vz, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, world.x, world.y, world.z, 0.58f) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y), 0.58f / WorldXScale);
        if (!hit) continue;
        part = static_cast<BossPart>(partIndex);
        return true;
    }
    return false;
}

void SideScrollingShooter::Stage2Module::FireBossPartBarrage(
    SideScrollingShooter& shooter, const Enemy& boss) {
    constexpr float ModelScale = 1.92f;
    constexpr Vector3 PartPosition[] = {
        {-2.30f, 2.08f, 0.0f}, {-0.55f, 2.60f, -0.92f},
        {0.65f, 2.60f, 0.0f}, {2.85f, 2.18f, 0.92f}, {0.25f, 1.42f, 0.0f}
    };
    const bool railMode = shooter.IsRailGameplayActive();
    const float yaw = railMode ? 0.0f : Math::HalfPi;
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    const float patrolX = boss.phase >= 2.0f ?
        std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f *
            Math::Clamp01(1.0f - yaw / Math::HalfPi) : 0.0f;
    const Vector3 battleshipPosition {
        ToWorldX(boss.x + shooter.m_stage2.boss.landBattleshipOffsetX) + patrolX,
        BattleshipWorldY(shooter, boss),
        boss.z + shooter.m_stage2.boss.landBattleshipOffsetZ
    };
    BossModelTransform battleship {
        battleshipPosition, {}, yaw, ModelScale,
        boss.phase >= 3.0f && shooter.m_stage2.boss.action != BossAction::Separating, true
    };
    battleship.secondaryAimTarget = {
        ToWorldX(boss.turretAimX), ToWorldY(boss.turretAimY),
        railMode ? boss.turretAimZ : SidePlaneZ
    };

    // 毎フレーム移動する上部戦艦の描画位置へローカル砲塔座標を合成する
    for (int part = 0; part < BossRightEngine; ++part) {
        if (part == BossNose && boss.phase < 3.0f) continue;
        if (boss.bossPartHp[part] <= 0) continue;
        const Vector3& local = PartPosition[part];
        const Vector3 partWorld {
            battleshipPosition.x + (local.x * cosine + local.z * sine) * ModelScale,
            battleshipPosition.y + local.y * ModelScale,
            battleshipPosition.z + (-local.x * sine + local.z * cosine) * ModelScale
        };
        // 副砲は全Phaseで描画と同じ向きと砲身長を使い、砲口先端から射出する
        const Vector3 world = part >= BossLeftWing && part <= BossLeftEngine ?
            LandBattleshipView::SecondaryGunMuzzlePosition(battleship, part - BossLeftWing) :
            partWorld;
        const int bulletCount = shooter.m_stage->BossPartBulletCount(
            static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), railMode);
        for (int index = 0; index < bulletCount; ++index) {
            const Stage::BossBullet bullet = shooter.m_stage->GetBossPartBullet(
                static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase),
                index, railMode);
            shooter.SpawnShot(
                FromWorldX(world.x) + bullet.offsetX,
                FromWorldY(world.y) + bullet.offsetY,
                bullet.vx, bullet.vy, true, world.z,
                boss.behavior->RailAimedShotSpeed());
        }
    }
}

void SideScrollingShooter::Stage2Module::TickSpecialShotBeforeMove(
    SideScrollingShooter& shooter, Shot& shot) {
    if (shot.stage2.dustAge >= 0) ++shot.stage2.dustAge;
    if (shot.stage2.kind == ShotKind::Funnel) ++shot.age;

    // Phase 3ファンネルは短く自由落下してから補助エンジンで自機へ向かう
    if (shot.enemy && shot.stage2.kind == ShotKind::Funnel && shot.stage2.delayedEngine) {
        if (shot.age < Phase3FunnelEngineStartFrame) {
            shot.vy -= Phase3FunnelGravity;
        } else {
            constexpr float FunnelSpeed = 0.72f;
            constexpr float EngineAcceleration = 0.085f;
            if (shot.age == Phase3FunnelEngineStartFrame) {
                // 点火時の自機位置から進行方向を一度だけ固定する
                const float dx = ToWorldX(shooter.m_playerX - shot.x);
                const float dy = ToWorldY(shooter.m_playerY - shot.y);
                const float dz = shooter.IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
                const float length = (std::max)(
                    0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                shot.stage2.engineVx = FromWorldX(dx / length * FunnelSpeed);
                shot.stage2.engineVy = FromWorldY(dy / length * FunnelSpeed);
                shot.stage2.engineVz = dz / length * FunnelSpeed;
            }
            shot.vx += (shot.stage2.engineVx - shot.vx) * EngineAcceleration;
            shot.vy += (shot.stage2.engineVy - shot.vy) * EngineAcceleration;
            shot.vz += (shot.stage2.engineVz - shot.vz) * EngineAcceleration;
        }
    }

    // Phase 2ファンネルは進路を変えず上方向へ徐々に加速する
    if (shot.enemy && shot.stage2.kind == ShotKind::Funnel && !shot.stage2.delayedEngine) {
        shot.vy = (std::min)(0.115f, shot.vy + 0.0018f);
    }
}

void SideScrollingShooter::Stage2Module::TickSpecialShotAfterMove(
    SideScrollingShooter& shooter, Shot& shot, float previousY) {
    if (shot.stage2.kind != ShotKind::Funnel || shot.stage2.dustAge >= 0) return;

    // 座標加算後に砂面を横切ったファンネルへ一度だけ砂埃の発生地点を記録する
    const float groundY = FromWorldY(Math::Lerp(-6.0f, -3.65f, shooter.RailBlend()));
    if (previousY < groundY && shot.y >= groundY) {
        shot.stage2.dustAge = 0;
        shot.stage2.dustX = shot.x;
        shot.stage2.dustY = groundY;
        shot.stage2.dustZ = shot.z;
    }
}

bool SideScrollingShooter::Stage2Module::IsShotCullProtected(const Shot& shot) {
    return shot.stage2.kind == ShotKind::Funnel &&
        shot.age <= FunnelLaunchCullGraceFrames;
}

float SideScrollingShooter::Stage2Module::EnemyShotHitRadius(
    const Shot& shot, bool railMode) {
    return railMode ?
        (shot.stage2.kind == ShotKind::Funnel ? 0.42f : 0.28f) :
        (shot.stage2.kind == ShotKind::Funnel ? 0.055f : 0.022f);
}

bool SideScrollingShooter::Stage2Module::TickSpecialDebris(
    SideScrollingShooter& shooter, Debris& debris) {
    static_assert((DefeatExplosionCount - 1) * DefeatExplosionIntervalFrames <
        ResurfaceStartFrame);
    if (!debris.active || debris.stage2.kind == DebrisKind::None) return false;

    // ponytail: 固定長プールの先頭船体を都度探索する。容量拡大時はTickDebris開始時の参照キャッシュへ置換する
    Debris* lowerHull = nullptr;
    for (auto& candidate : shooter.m_debris) {
        if (candidate.active && candidate.stage2.kind == DebrisKind::Sink) {
            lowerHull = &candidate;
            break;
        }
    }

    // 下部船体は一旦沈み、最後の再浮上後に上部船体へ押し戻される
    if (debris.stage2.kind == DebrisKind::Sink) {
        // 沈下中は短い大爆発音を約1.2秒間隔で鳴らす
        if (debris.age > 0 &&
            debris.age < DefeatExplosionCount * DefeatExplosionIntervalFrames &&
            debris.age % DefeatExplosionIntervalFrames == 0) {
            PlayDefeatSound(shooter, false);
        }
        const float groundTopY = Math::Lerp(-6.0f, -3.65f, shooter.RailBlend());
        const float buriedY = groundTopY - debris.height * 0.55f;
        if (debris.stage2.effectAge >= 0) {
            debris.y -= 0.018f;
            ++debris.stage2.effectAge;
        } else if (debris.age < FirstSinkEndFrame) {
            // 残りフレーム数で割り、船体上面まで確実に砂面下へ沈める
            debris.y += (buriedY - debris.y) /
                static_cast<float>(FirstSinkEndFrame - debris.age);
        } else if (debris.age >= ResurfaceStartFrame) {
            const float resurfaceY = groundTopY + debris.height * 0.18f;
            debris.y = (std::min)(resurfaceY, debris.y + 0.065f);
        }
        if (++debris.age >= debris.lifetime) debris.active = false;
        return true;
    }

    // 船底が再浮上した下部船体の上面へ届くまで上部船体をゆっくり降下させる
    if ((debris.stage2.kind == DebrisKind::Impact ||
        debris.stage2.kind == DebrisKind::ImpactPiece) &&
        debris.stage2.effectAge < 0) {
        const bool lowerHullHit = lowerHull != nullptr &&
            debris.stage2.kind == DebrisKind::Impact &&
            lowerHull->age >= ResurfaceStartFrame &&
            debris.y - debris.height * 0.5f <=
                lowerHull->y + lowerHull->height * 0.5f;
        const bool collisionStarted = lowerHull != nullptr &&
            lowerHull->stage2.effectAge >= 0;
        if (lowerHullHit) {
            // 主船体がeffectAgeを開始した同フレームに後続部品も衝突済みとして扱う
            lowerHull->stage2.effectAge = 0;
            shooter.SpawnExplosion(
                FromWorldX(debris.x), FromWorldY(debris.y), debris.z, true);
            PlayDefeatSound(shooter, true);
        }
        if (!lowerHullHit && !collisionStarted) {
            debris.y -= 0.032f;
            debris.yaw += debris.spin * 0.18f;
            if (++debris.age >= debris.lifetime) debris.active = false;
            return true;
        }
    }

    // 特殊デブリはここで共通移動まで所有して二重更新を防ぐ
    debris.x += debris.vx;
    debris.y += debris.vy;
    debris.z += debris.vz;
    if (debris.gravity || shooter.m_stage->HasDebrisGravity()) debris.vy -= 0.006f;
    debris.yaw += debris.spin;

    // 衝突後は上部船体を大きく飛散させ、主船体の衝突時刻を爆発演出へ渡す
    if (debris.stage2.kind == DebrisKind::Impact ||
        debris.stage2.kind == DebrisKind::ImpactPiece) {
        if (debris.stage2.effectAge < 0) {
            debris.stage2.effectAge = 0;
            debris.vx += debris.stage2.kind == DebrisKind::Impact ? 0.025f :
                (debris.x < 0.0f ? -0.055f : 0.055f);
            debris.vy = 0.075f;
            debris.vz += debris.stage2.kind == DebrisKind::Impact ? 0.0f :
                (debris.z < 0.0f ? -0.040f : 0.040f);
            debris.spin *= 3.5f;
        }
        const float groundTopY = Math::Lerp(-6.0f, -3.65f, shooter.RailBlend());
        const float minimumY = groundTopY + debris.height * 0.5f;
        if (debris.y <= minimumY) {
            debris.y = minimumY;
            debris.vx *= 0.86f;
            debris.vy = 0.0f;
            debris.vz *= 0.86f;
            debris.spin *= 0.78f;
        }
        ++debris.stage2.effectAge;
    }
    if (++debris.age >= debris.lifetime) debris.active = false;
    return true;
}

bool SideScrollingShooter::Stage2Module::HandleBossDefeat(
    SideScrollingShooter& shooter, Enemy& boss) {
    if (!boss.active) return false;

    // 上下船体の沈下、再浮上、衝突、大破まで見届ける専用クリア待機へ移る
    shooter.SpawnExplosion(boss.x, boss.y, boss.z, true);
    SpawnBossDebris(shooter, boss);
    boss.active = false;
    shooter.SpawnPowerItem(boss.x, boss.y, boss.z, 1.00f);
    shooter.m_bossHp = 0;
    shooter.m_score += 5000;
    shooter.m_clear = true;
    shooter.m_clearTimer = 440;
    PlayDefeatSound(shooter, false);
    return true;
}

void SideScrollingShooter::Stage2Module::PlayRailgunSound(SideScrollingShooter& shooter) {
    if (!shooter.m_audio) return;

    // 高周波の亀裂音を急降下させて雷の鋭い立ち上がりを作る
    Audio::SfxrParams crack;
    crack.waveType = Audio::SfxrWaveType::Sawtooth;
    crack.startFrequency = 0.92f;
    crack.minFrequency = 0.06f;
    crack.slide = -0.72f;
    crack.sustainTime = 0.025f;
    crack.decayTime = 0.16f;
    crack.masterVolume = 0.75f;
    shooter.m_audio->PlaySE(crack);

    // 短いノイズを重ねて雷撃の破裂感を足す
    Audio::SfxrParams thunder;
    thunder.waveType = Audio::SfxrWaveType::Noise;
    thunder.startFrequency = 0.68f;
    thunder.minFrequency = 0.04f;
    thunder.slide = -0.58f;
    thunder.sustainTime = 0.035f;
    thunder.decayTime = 0.22f;
    thunder.masterVolume = 0.82f;
    shooter.m_audio->PlaySE(thunder);
}

void SideScrollingShooter::Stage2Module::PlayDefeatSound(
    SideScrollingShooter& shooter, bool finalExplosion) {
    if (!shooter.m_audio) return;

    // 短い低域ノイズで間隔のある大爆発音と最後のドカーン音を作る
    Audio::SfxrParams sound;
    sound.waveType = Audio::SfxrWaveType::Noise;
    sound.startFrequency = finalExplosion ? 0.24f : 0.20f;
    sound.minFrequency = finalExplosion ? 0.01f : 0.025f;
    sound.slide = finalExplosion ? -0.32f : -0.24f;
    sound.attackTime = 0.0f;
    sound.sustainTime = finalExplosion ? 0.42f : 0.18f;
    sound.decayTime = finalExplosion ? 0.90f : 0.38f;
    sound.masterVolume = finalExplosion ? 0.95f : 0.78f;
    shooter.m_audio->PlaySE(sound);

    // 最終爆発だけ低い衝撃音を足して爆発の芯を強める
    if (finalExplosion) shooter.m_audio->PlayMMLSE("t72 o1 l2 v15 c g c");
}

int SideScrollingShooter::Stage2Module::BossIntroductionFrames() {
    return BossApproachFrames + BossAssemblyFrames;
}
