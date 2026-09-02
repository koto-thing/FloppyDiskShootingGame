#include "Stage3Module.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../SideScrollingShooterEnemies.h"
#include "../../SideScrollingShooterShared.h"
#include "../Common/StageDefinition.h"
#include "Stage3BossModelView.h"
#include "Stage3BarrierCageView.h"
#include "Stage3FunnelModelView.h"

namespace {
using SideScrollingShooterShared::SideCameraFieldOfView;
using SideScrollingShooterShared::SideCameraZ;

constexpr float DaySkyColor[4] = {0.30f, 0.68f, 0.92f, 1.0f};
constexpr float NightSkyColor[4] = {0.015f, 0.03f, 0.12f, 1.0f};
constexpr float WaterColor[4] = {0.04f, 0.34f, 0.60f, 1.0f};
constexpr float WaveColor[4] = {0.20f, 0.74f, 0.86f, 1.0f};
constexpr float FoamColor[4] = {0.78f, 0.94f, 0.92f, 1.0f};
constexpr float CloudColor[4] = {0.90f, 0.95f, 0.96f, 1.0f};
constexpr float SunColor[4] = {1.00f, 0.82f, 0.20f, 1.0f};
constexpr float SeaSerpentColor[4] = {0.05f, 0.24f, 0.20f, 1.0f};
constexpr float SeaSerpentBellyColor[4] = {0.28f, 0.62f, 0.48f, 1.0f};
constexpr float SeaSerpentEyeColor[4] = {1.00f, 0.84f, 0.16f, 1.0f};
constexpr float SeaSerpentSideEyeSurfaceOffset = 0.90f;
constexpr float SeaSerpentRailEyeSurfaceOffset = 1.70f;
constexpr int DawnStartFrame = 500;
constexpr int DawnFrame = 750;
constexpr int BossAscentFrames = 150;
constexpr int BossRevealFrames = 150;
constexpr int BossFlyOverFrames = 180;
constexpr int BossIntroductionFrameCount =
    BossAscentFrames + BossRevealFrames + BossFlyOverFrames;
constexpr int BossSectionTransitionFrames = 150;
constexpr int BossPhase2TravelFrames = 4 * 60;
constexpr int BossPhase2DeployFrames = 4 * 60;
constexpr int BossMissileEngineStartFrame = 26;
constexpr int BossMissileCullGraceFrames = 45;
constexpr int BossDirectMissileHomingFrames = 42;
constexpr int BossMachineGunCycleFrames = 180;
constexpr int BossMachineGunBurstFrames = 36;
constexpr int BossMachineGunRandomStartFrame = 90;
constexpr int BossMachineGunShotInterval = 5;
constexpr int BossTurretHp = 100;
constexpr int BossPhase1StartHp = 1200;
constexpr int BossTurretBreakDamage = 100;
constexpr float BossModelScale = 4.0f;
constexpr float BossSideModelScale = 1.8f;
constexpr float BossSideFocusX = 6.0f;
constexpr float BossFinalWorldY = -11.60f;
constexpr float BossFinalWorldZ = 60.0f;
constexpr float BossPhase2SideWorldY = -3.6f;
constexpr float BossPhase2RailWorldY = 23.0f;
constexpr float BossPhase2WorldZ = 20.0f;
constexpr float BossPhase2SideAboveY = 18.0f;
constexpr float BossPhase2RailAboveY = 50.0f;
constexpr float BossPhase2SideRetreatZ = 40.0f;
constexpr float BossPhase2RailRetreatZ = 60.0f;
constexpr float BossDeckTopY = 0.0f;
constexpr float BossSeaDropDistance = 18.0f;
constexpr float BossMissileSpeed = 0.58f;
constexpr float BossMissileLaunchVelocity = 0.09f;
constexpr float BossMissileGravity = 0.0035f;
constexpr float BossMissileEngineAcceleration = 0.085f;
constexpr float BossDirectMissileTurnRate = 0.055f;
constexpr float BossMachineGunSpeed = 0.29f;
constexpr float BossLaterPhaseSpeedScale = 0.25f;
constexpr int BossLaterPhaseCullGraceFrames =
    static_cast<int>(90.0f / BossLaterPhaseSpeedScale);
constexpr float BossTurretMuzzleLocalOffset = 1.45f;
constexpr float BossSideZOffset = 2.2f;
constexpr float BossSidePrimitiveDepth = 0.28f;
constexpr float BossPhase2GroundY = BossPhase2RailWorldY +
    (Stage3BarrierCageView::BarrierTopY - Stage3BarrierCageView::BarrierHeight) *
        BossModelScale;
constexpr float BossPhase2RailTopY = BossPhase2RailWorldY +
    Stage3BarrierCageView::BarrierTopY * BossModelScale;
constexpr float BossPhase2RailCenterY =
    (BossPhase2GroundY + BossPhase2RailTopY) * 0.5f;
constexpr float BossPhase2SideBottomY = BossPhase2SideWorldY +
    (Stage3BarrierCageView::BarrierTopY - Stage3BarrierCageView::BarrierHeight) *
        BossSideModelScale;
constexpr float BossPhase2SideTopY = BossPhase2SideWorldY +
    Stage3BarrierCageView::BarrierTopY * BossSideModelScale;
constexpr float BossPhase2SideCenterY =
    (BossPhase2SideBottomY + BossPhase2SideTopY) * 0.5f;
constexpr float BossBarrierPlayerMarginY = 0.35f;
constexpr float BossPhase2Travel = 3.0f;
constexpr float BossPhase2Deploy = 4.0f;
constexpr float BossPhase2Survival = 5.0f;
constexpr float BossPhase3Survival = 6.0f;
constexpr float BossSectionAdvanceDistance = 16.0f;
constexpr float BossCameraAdvanceRate = 0.35f;
constexpr int ReflectFunnelHp = 30;
constexpr int ReflectFunnelPortCooldownFrames = 5 * 60;
constexpr int ReflectFunnelLaunchFrames = 60;
constexpr float ReflectShotSpeed = 0.44f * BossLaterPhaseSpeedScale;
constexpr int ReflectFunnelMissileIntervalFrames = 10 * 60;
constexpr int ReflectFunnelMissileLifetimeFrames = 8 * 60;
constexpr float ReflectFunnelMissileSpeed = 0.045f;
constexpr float ReflectFunnelMissileTurnRate = 0.008f;
constexpr Vector3 ReflectFunnelTargetLocal[ShooterStages::Stage3::ReflectFunnelCount] = {
    {-5.2f, -6.5f, -1.6f},
    {-2.6f, -8.8f, 1.5f},
    {0.0f, -7.4f, -1.2f},
    {2.6f, -9.8f, 1.4f},
    {5.2f, -8.2f, -1.5f}
};

/**
 * @brief Phase3ファンネル配置がバリア内か判定する
 * @return 全配置がバリア内の場合true
 */
constexpr bool AreReflectFunnelTargetsInsideBarrier() {
    for (const Vector3& target : ReflectFunnelTargetLocal) {
        if (target.x <= -Stage3BarrierCageView::BarrierHalfLength ||
            target.x >= Stage3BarrierCageView::BarrierHalfLength ||
            target.y >= Stage3BarrierCageView::BarrierTopY ||
            target.y <= Stage3BarrierCageView::BarrierTopY -
                Stage3BarrierCageView::BarrierHeight ||
            target.z <= -Stage3BarrierCageView::BarrierHalfWidth ||
            target.z >= Stage3BarrierCageView::BarrierHalfWidth) return false;
    }
    return true;
}
static_assert(AreReflectFunnelTargetsInsideBarrier());
static_assert(ReflectFunnelMissileLifetimeFrames < ReflectFunnelMissileIntervalFrames);

/**
 * @brief Phase1進行量から戦艦を手前へ送る距離を取得する
 * @param sectionProgress 艦尾区画を0とする連続進行量
 * @return 戦艦へ加算するワールドZ移動量
 */
constexpr float BossSectionAdvanceZ(float sectionProgress) {
    return -BossSectionAdvanceDistance * sectionProgress;
}

/**
 * @brief 整数Seedから決定的な-1以上1以下の散布値を取得する
 * @param seed 発射フレームと砲台番号から作るSeed
 * @return -1以上1以下の散布値
 */
constexpr float BossMachineGunSpread(std::uint32_t seed) {
    seed ^= seed >> 16;
    seed *= 0x7FEB352Du;
    seed ^= seed >> 15;
    return static_cast<float>(seed & 1023u) / 511.5f - 1.0f;
}
static_assert(SeaSerpentSideEyeSurfaceOffset > 1.35f * 1.25f * 0.5f);
static_assert(SeaSerpentRailEyeSurfaceOffset > 2.50f * 1.25f * 0.5f);
static_assert(DawnStartFrame < DawnFrame);
static_assert(BossPhase1StartHp >
    Stage3BossModelView::TopGunCount * BossTurretBreakDamage);
static_assert(BossIntroductionFrameCount == 8 * 60);
static_assert(BossMissileEngineStartFrame < BossMissileCullGraceFrames);
static_assert(BossMissileEngineStartFrame * BossMissileLaunchVelocity -
    BossMissileGravity * BossMissileEngineStartFrame *
        (BossMissileEngineStartFrame + 1) * 0.5f > 0.63f);
static_assert(BossMissileLaunchVelocity -
    BossMissileGravity * BossMissileEngineStartFrame < 0.0f);
static_assert(BossDirectMissileHomingFrames < BossSectionTransitionFrames);
static_assert(BossSideModelScale < BossModelScale);
static_assert(BossSideZOffset - BossSidePrimitiveDepth * 0.5f > 1.0f);
static_assert(BossSectionAdvanceZ(0.0f) == 0.0f);
static_assert(BossSectionAdvanceZ(1.0f) == -BossSectionAdvanceDistance);
static_assert(BossMachineGunRandomStartFrame + BossMachineGunBurstFrames <
    BossMachineGunCycleFrames);
static_assert(BossMachineGunSpread(1u) >= -1.0f && BossMachineGunSpread(1u) <= 1.0f);

}

struct SideScrollingShooter::Stage3Module::SeaSerpentMotion {
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

struct SideScrollingShooter::Stage3Module::SeaSerpentSegment {
    float progress;
    float elevation;
    float sideX;
    float railX;
    float railZ;
    float scale;
};

/** @brief Stage 3の敵出現とボス弾幕を定義する */
class SideScrollingShooter::Stage3Module::StageDefinitionImpl final : public SideScrollingShooter::Stage {
public:
    /**
     * @brief ステージ番号を取得する
     * @return Stage 3を表す番号
     */
    int StageIndex() const override {
        return 3;
    }

    /**
     * @brief Stage3ボス本体の初期HPを取得する
     * @return Phase1後も船体が残るHP
     */
    int BossMaxHp() const override {
        return BossPhase1StartHp;
    }

    /**
     * @brief Stage3ボスを専用の導入演出位置へ初期化する
     * @param boss 初期化するボス
     * @param railMode レール表示中の場合true
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        Stage3Module::ConfigureBossSpawn(boss, railMode, StageIndex());
    }

    /**
     * @brief Stage3ボスを飛行戦艦上の基準位置へ配置する
     * @param boss 配置するボス
     * @return なし
     */
    void ConfigureBossRailAnchor(Enemy& boss) const override {
        boss.x = 0.0f;
        boss.y = FromWorldY(BossFinalWorldY);
        boss.z = BossFinalWorldZ;
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
    }

    /**
     * @brief Stage3ボスの上部砲台6基へHPを設定する
     * @param boss 設定するボス
     * @return なし
     */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = {
            BossTurretHp, BossTurretHp, BossTurretHp,
            BossTurretHp, BossTurretHp, BossTurretHp
        };
    }

    /**
     * @brief 上部砲台破壊時にStage3ボス本体へ与えるダメージを取得する
     * @param part 破壊された部位
     * @return 固定ダメージ
     */
    int BossPartBreakDamage(BossPart part) const override {
        (void)part;
        return BossTurretBreakDamage;
    }

    /**
     * @brief Stage3ボスのPhase1区画進行とPhase2耐久戦を更新する
     * @param shooter 更新するゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        Stage3Module::TickBoss(shooter, boss);
    }

    /**
     * @brief Stage3専用処理中に共通ボス砲撃を停止する
     * @param shooter 判定するゲーム本体
     * @param boss 判定するボス
     * @return 砲撃を停止する場合true
     */
    bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss) const override {
        return Stage3Module::IsBossSpecialAttackActive(shooter, boss);
    }

    /**
     * @brief Stage3の攻撃フェーズをPhase1へ固定する
     * @param hp 現在HP
     * @param maxHp 最大HP
     * @return 通常Phase1
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        (void)hp;
        (void)maxHp;
        return BossNormalPhase1;
    }

    /**
     * @brief Stage3上部砲台の斉射間隔を取得する
     * @param phase 現在フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(BossPhase phase) const override {
        (void)phase;
        return 96;
    }

    /**
     * @brief Stage 3の経過フレームから通常敵の出現を選択する
     * @param frame Stage 3開始からの経過フレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 選択した出現規則の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させるフレームの場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {
            {3, 24, 54, 1.10f, -0.82f, 0.88f, 50.0f},
            {4, 110, 250, 1.12f, -0.40f, -0.42f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {3, 10, 52, 1.10f, 0.28f, -0.88f, 40.0f},
            {5, 70, 165, 1.14f, 0.82f, 0.32f, 56.0f},
            {1, 150, 230, 1.16f, 0.05f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {4, 10, 140, 1.12f, -0.85f, -0.18f, 60.0f},
            {5, 60, 145, 1.14f, -0.28f, 0.86f, 50.0f},
            {1, 120, 180, 1.16f, 0.82f, -0.68f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }

    /**
     * @brief Stage 3ボスの一斉射撃数を取得する
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 5;
    }

    /**
     * @brief Stage 3ボスの指定番号の弾を取得する
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {
            {-0.12f, 0.0f, -0.024f, -0.020f},
            {-0.12f, 0.0f, -0.026f, -0.010f},
            {-0.12f, 0.0f, -0.027f, 0.0f},
            {-0.12f, 0.0f, -0.026f, 0.010f},
            {-0.12f, 0.0f, -0.024f, 0.020f}
        };
        constexpr BossBullet RailPattern[] = {
            {0.0f, 0.0f, -0.014f, -0.024f},
            {0.0f, 0.0f, -0.007f, -0.012f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.007f, 0.012f},
            {0.0f, 0.0f, 0.014f, 0.024f}
        };
        return (railMode ? RailPattern : SidePattern)[index % 5];
    }
};

const SideScrollingShooter::Stage& SideScrollingShooter::Stage3Module::Definition() {
    static_assert(BossPhase2GroundY > -3.65f - BossSeaDropDistance);
    static_assert(BossPhase2GroundY - (-3.65f - BossSeaDropDistance) < 0.5f);
    static_assert(BossPhase2SideBottomY > -6.0f - BossSeaDropDistance);
    static_assert(BossPhase2SideBottomY - (-6.0f - BossSeaDropDistance) < 0.6f);
    static_assert(BossPhase2WorldZ -
        Stage3BarrierCageView::BarrierHalfLength * BossModelScale < PlayerRailZ - 15.5f);
    static_assert(BossPhase2WorldZ +
        Stage3BarrierCageView::BarrierHalfLength * BossModelScale > PlayerRailZ + 22.0f);
    static_assert(Phase2SurvivalFrames == 30 * 60);
    static_assert(Phase2HpForRemainingFrames(600, 900) == 300);
    static_assert(Phase2HpForRemainingFrames(600, 0) == 0);
    static_assert(Phase3SurvivalFrames == 60 * 60);
    static_assert(Phase3HpForRemainingFrames(600, 1800) == 300);
    static_assert(Phase3HpForRemainingFrames(600, 0) == 0);
    static const StageDefinitionImpl definition;
    return definition;
}

void SideScrollingShooter::Stage3Module::Reset(SideScrollingShooter& shooter) {
    shooter.m_stage3 = {};
}

void SideScrollingShooter::Stage3Module::ConfigureBossSpawn(
    Enemy& boss, bool railMode, int stageIndex) {
    BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, stageIndex);

    // 全景を固定したまま、boss.phaseを現在の甲板区画として使用する
    boss.hp = BossPhase1StartHp;
    boss.maxHp = boss.hp;
    boss.x = 0.0f;
    boss.y = FromWorldY(BossFinalWorldY);
    boss.z = BossFinalWorldZ;
    boss.baseX = boss.x;
    boss.baseY = boss.y;
    boss.baseZ = boss.z;
    boss.phase = 0.0f;
    boss.motionAge = 0;
    boss.shotInterval = (std::numeric_limits<int>::max)();
    boss.collisionEnabled = false;
}

void SideScrollingShooter::Stage3Module::TickBoss(
    SideScrollingShooter& shooter, Enemy& boss) {
    // 飛行戦艦のゲーム座標は固定し、描画Transformで攻略地点を移動する
    boss.x = 0.0f;
    boss.y = FromWorldY(BossFinalWorldY);
    boss.z = BossFinalWorldZ;

    // 奥へ退避して上空へ回る間は攻撃せず、2Dを初期視点とする
    if (boss.phase == BossPhase2Travel) {
        if (boss.motionAge > 0) {
            --boss.motionAge;
            return;
        }
        boss.phase = BossPhase2Deploy;
        boss.motionAge = BossPhase2DeployFrames;
        return;
    }

    // ファンネル射出とバリア展開が完了してから耐久時間を開始する
    if (boss.phase == BossPhase2Deploy) {
        if (boss.motionAge > 0) {
            --boss.motionAge;
            return;
        }
        boss.phase = BossPhase2Survival;
        boss.motionAge = Phase2SurvivalFrames;
        boss.actionX = static_cast<float>(boss.hp);
        return;
    }

    // ゴンドラ全武装を周期発射し、30秒で残りHPを均等に減らす
    if (boss.phase == BossPhase2Survival) {
        FireBossPartBarrage(shooter, boss);
        boss.motionAge = (std::max)(0, boss.motionAge - 1);
        boss.hp = Phase2HpForRemainingFrames(
            static_cast<int>(boss.actionX), boss.motionAge);
        shooter.m_bossHp = boss.hp;
        if (boss.hp > 0) return;

        for (auto& shot : shooter.m_shots) {
            if (!shot.active || !shot.enemy) continue;
            if (shot.stage2.delayedEngine) {
                shooter.SpawnExplosion(shot.x, shot.y, shot.z);
            }
            shot.active = false;
        }
        // Phase2終了時にゴンドラ武装だけを爆破し、射出口を残してPhase3へ移行する
        const BossModelTransform transform = BossTransform(shooter, boss);
        auto ExplodeMount = [&](const Stage3BossWeaponMount& mount) {
            const float cosine = std::cos(transform.yaw);
            const float sine = std::sin(transform.yaw);
            const Vector3& local = mount.localPosition;
            const Vector3 world {
                transform.position.x + (local.x * cosine + local.z * sine) * transform.scale,
                transform.position.y + local.y * transform.scale,
                transform.position.z + (-local.x * sine + local.z * cosine) * transform.scale};
            shooter.SpawnExplosion(FromWorldX(world.x), FromWorldY(world.y), world.z, true);
        };
        for (int i = 0; i < Stage3BossModelView::GondolaMachineGunCount; ++i) {
            ExplodeMount(Stage3BossModelView::GondolaMachineGunMount(i));
        }
        for (int i = 0; i < Stage3BossModelView::HeavyCannonCount; ++i) {
            ExplodeMount(Stage3BossModelView::HeavyCannonMount(i));
        }
        for (int i = 0; i < Stage3BossModelView::MissilePodCount; ++i) {
            ExplodeMount(Stage3BossModelView::MissilePodMount(i));
        }
        shooter.m_stage3 = {};
        boss.phase = BossPhase3Survival;
        boss.motionAge = Phase3SurvivalFrames;
        boss.hp = static_cast<int>(boss.actionX);
        shooter.m_bossHp = boss.hp;
        return;
    }

    // Phase3は射出口ごとの再装填を管理し、最大5基で反射攻撃を継続する
    if (boss.phase == BossPhase3Survival) {
        boss.motionAge = (std::max)(0, boss.motionAge - 1);
        boss.hp = Phase3HpForRemainingFrames(static_cast<int>(boss.actionX), boss.motionAge);
        shooter.m_bossHp = boss.hp;

        const BossModelTransform transform = BossTransform(shooter, boss);
        auto WorldPosition = [&transform](const Vector3& local) {
            const float cosine = std::cos(transform.yaw);
            const float sine = std::sin(transform.yaw);
            return Vector3 {
                transform.position.x + (local.x * cosine + local.z * sine) * transform.scale,
                transform.position.y + local.y * transform.scale,
                transform.position.z + (-local.x * sine + local.z * cosine) * transform.scale
            };
        };

        // 視点とボスTransformに追従するバリア内のローカル座標へ展開する
        int activeFunnels = 0;
        for (auto& funnel : shooter.m_stage3.reflectFunnels) {
            if (!funnel.active) continue;
            ++activeFunnels;
            ++funnel.age;
            const int index = static_cast<int>(&funnel - shooter.m_stage3.reflectFunnels.data());
            const Vector3 target = WorldPosition(ReflectFunnelTargetLocal[index]);
            funnel.x += (FromWorldX(target.x) - funnel.x) * 0.08f;
            funnel.y += (FromWorldY(target.y) - funnel.y) * 0.08f;
            funnel.z += (target.z - funnel.z) * 0.08f;
        }
        for (int& cooldown : shooter.m_stage3.funnelPortCooldowns) {
            if (cooldown > 0) --cooldown;
        }

        if (activeFunnels < ShooterStages::Stage3::ReflectFunnelCount) {
            for (int offset = 0; offset < Stage3BossModelView::FunnelPodCount; ++offset) {
                const int port = (shooter.m_stage3.nextFunnelPort + offset) %
                    Stage3BossModelView::FunnelPodCount;
                if (shooter.m_stage3.funnelPortCooldowns[port] > 0) continue;
                auto& funnel = *std::find_if(shooter.m_stage3.reflectFunnels.begin(),
                    shooter.m_stage3.reflectFunnels.end(), [](const auto& item) { return !item.active; });
                const Vector3 local = Stage3BossModelView::FunnelLaunchLocalPosition(port);
                const Vector3 launch = WorldPosition(local);
                funnel = {FromWorldX(launch.x), FromWorldY(launch.y), launch.z,
                    ReflectFunnelHp, 0, port, true};
                shooter.m_stage3.funnelPortCooldowns[port] = ReflectFunnelPortCooldownFrames;
                shooter.m_stage3.nextFunnelPort = (port + 1) % Stage3BossModelView::FunnelPodCount;
                break;
            }
        }

        // 配置完了した各ファンネルが一発ずつ次のファンネルへ渡す
        for (int owner = 0; owner < ShooterStages::Stage3::ReflectFunnelCount; ++owner) {
            const auto& source = shooter.m_stage3.reflectFunnels[owner];
            if (!source.active || source.age < ReflectFunnelLaunchFrames) continue;
            bool alreadyHasShot = false;
            for (const auto& shot : shooter.m_shots) {
                if (shot.active && shot.enemy &&
                    (shot.stage2.kind == ShooterStages::Stage2::ShotKind::ReflectPass ||
                        shot.stage2.kind == ShooterStages::Stage2::ShotKind::ReflectAttack) &&
                    shot.barrageIndex == owner) alreadyHasShot = true;
            }
            if (alreadyHasShot) continue;
            int target = -1;
            for (int offset = 1; offset < ShooterStages::Stage3::ReflectFunnelCount; ++offset) {
                const int candidate = (owner + offset) % ShooterStages::Stage3::ReflectFunnelCount;
                if (shooter.m_stage3.reflectFunnels[candidate].active) { target = candidate; break; }
            }
            if (target < 0) continue;
            const auto& destination = shooter.m_stage3.reflectFunnels[target];
            const float dx = ToWorldX(destination.x - source.x);
            const float dy = ToWorldY(destination.y - source.y);
            const float dz = shooter.IsRailGameplayActive() ? destination.z - source.z : 0.0f;
            const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
            for (auto& shot : shooter.m_shots) {
                if (shot.active) continue;
                shot = {};
                shot.x = source.x; shot.y = source.y; shot.z = source.z;
                shot.transitionSideX = shot.x; shot.transitionSideY = shot.y;
                shot.vx = FromWorldX(dx / length * ReflectShotSpeed);
                shot.vy = FromWorldY(dy / length * ReflectShotSpeed);
                shot.vz = dz / length * ReflectShotSpeed;
                shot.hitRadius = 0.045f; shot.damage = 2; shot.enemy = true; shot.special = true;
                shot.barrageIndex = owner; shot.barrageCount = target;
                shot.stage2.kind = ShooterStages::Stage2::ShotKind::ReflectPass;
                shot.active = true;
                break;
            }
        }

        // 各ファンネル砲塔から10秒ごとに小型追尾ミサイルを1発ずつ放つ
        for (int owner = 0; owner < ShooterStages::Stage3::ReflectFunnelCount; ++owner) {
            const auto& source = shooter.m_stage3.reflectFunnels[owner];
            if (!source.active || source.age < ReflectFunnelMissileIntervalFrames ||
                source.age % ReflectFunnelMissileIntervalFrames != 0) continue;
            const float dx = ToWorldX(shooter.m_playerX - source.x);
            const float dy = ToWorldY(shooter.m_playerY - source.y);
            const float dz = shooter.IsRailGameplayActive() ? PlayerRailZ - source.z : 0.0f;
            const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
            for (auto& shot : shooter.m_shots) {
                if (shot.active) continue;
                shot = {};
                shot.x = source.x;
                shot.y = source.y;
                shot.z = source.z;
                shot.transitionSideX = shot.x;
                shot.transitionSideY = shot.y;
                shot.vx = FromWorldX(dx / length * ReflectFunnelMissileSpeed);
                shot.vy = FromWorldY(dy / length * ReflectFunnelMissileSpeed);
                shot.vz = dz / length * ReflectFunnelMissileSpeed;
                shot.hitRadius = 0.030f;
                shot.damage = 1;
                shot.enemy = true;
                shot.special = true;
                shot.barrageIndex = owner;
                shot.stage2.kind = ShooterStages::Stage2::ShotKind::FunnelMissile;
                shot.active = true;
                break;
            }
        }
        if (boss.hp > 0) return;
        shooter.DefeatBoss(boss);
        return;
    }

    if (boss.motionAge > 0) {
        --boss.motionAge;
        return;
    }

    // 自機狙いとランダム散射を半周期ずらした短いバーストとして交互に撃つ
    const int machineGunFrame = boss.age % BossMachineGunCycleFrames;
    if (machineGunFrame < BossMachineGunBurstFrames &&
        machineGunFrame % BossMachineGunShotInterval == 0) {
        FireBossMachineGun(shooter, boss, true);
    } else if (machineGunFrame >= BossMachineGunRandomStartFrame &&
        machineGunFrame < BossMachineGunRandomStartFrame + BossMachineGunBurstFrames &&
        (machineGunFrame - BossMachineGunRandomStartFrame) % BossMachineGunShotInterval == 0) {
        FireBossMachineGun(shooter, boss, false);
    }

    const int section = (std::clamp)(static_cast<int>(boss.phase), 0,
        Stage3BossModelView::Phase1SectionCount - 1);
    if (boss.phase >= static_cast<float>(Stage3BossModelView::Phase1SectionCount) ||
        !Stage3BossModelView::IsPhase1SectionDestroyed(section, boss.bossPartHp)) {
        return;
    }

    // 二基を破壊したら敵弾を消し、次の二基へゆっくりカメラを送る
    for (auto& shot : shooter.m_shots) {
        if (!shot.active || !shot.enemy) continue;
        if (shot.stage2.delayedEngine) {
            shooter.SpawnExplosion(shot.x, shot.y, shot.z);
        }
        shot.active = false;
    }
    if (section + 1 < Stage3BossModelView::Phase1SectionCount) {
        boss.phase = static_cast<float>(section + 1);
        boss.motionAge = BossSectionTransitionFrames;
    } else {
        boss.phase = BossPhase2Travel;
        boss.motionAge = BossPhase2TravelFrames;
        boss.bossPhase = BossNormalPhase2;
        boss.collisionEnabled = false;
        if (shooter.IsRailGameplayActive()) {
            shooter.RequestViewMode(ViewMode::Side2D);
        }
    }
}

bool SideScrollingShooter::Stage3Module::IsBossSpecialAttackActive(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    (void)shooter;
    return boss.motionAge > 0 || boss.phase >= BossPhase2Travel;
}

BossModelTransform SideScrollingShooter::Stage3Module::BossTransform(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    const float advanceZ = Phase1AdvanceZ(boss);
    const Vector3 finalPosition {0.0f, BossFinalWorldY, BossFinalWorldZ + advanceZ};
    const float sideX = BossSideFocusX +
        (finalPosition.z - Phase1FocusZ(boss)) * BossSideModelScale / BossModelScale;
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        // 一度奥へ退避して画面外の上空へ回り、バリア展開と同時に降下する
        if (boss.phase >= BossPhase2Travel) {
            const Vector3 sideStart {
                sideX,
                0.0f,
                SideScrollingShooter::SidePlaneZ + BossSideZOffset
            };
            const Vector3 sideEnd {0.0f, BossPhase2SideWorldY, sideStart.z};
            const Vector3 railEnd {0.0f, BossPhase2RailWorldY, BossPhase2WorldZ};
            auto TravelRoute = [](const Vector3& start, const Vector3& end,
                float aboveY, float retreatZ, float progress) {
                const Vector3 behind {start.x, start.y, start.z + retreatZ};
                const Vector3 behindAbove {start.x, aboveY, behind.z};
                const Vector3 above {end.x, aboveY, end.z};
                if (progress < 0.40f) {
                    return Vector3::Lerp(start, behind, SmoothStep(progress / 0.40f));
                }
                if (progress < 0.70f) {
                    return Vector3::Lerp(behind, behindAbove,
                        SmoothStep((progress - 0.40f) / 0.30f));
                }
                return Vector3::Lerp(behindAbove, above,
                    SmoothStep((progress - 0.70f) / 0.30f));
            };
            Vector3 sidePosition = sideEnd;
            Vector3 railPosition = railEnd;
            if (boss.phase == BossPhase2Travel) {
                const float travel = 1.0f - static_cast<float>(boss.motionAge) /
                    static_cast<float>(BossPhase2TravelFrames);
                sidePosition = TravelRoute(sideStart, sideEnd,
                    BossPhase2SideAboveY, BossPhase2SideRetreatZ, travel);
                railPosition = TravelRoute(finalPosition, railEnd,
                    BossPhase2RailAboveY, BossPhase2RailRetreatZ, travel);
            } else if (boss.phase == BossPhase2Deploy) {
                const float descent = SmoothStep(1.0f -
                    static_cast<float>(boss.motionAge) /
                        static_cast<float>(BossPhase2DeployFrames));
                sidePosition = Vector3::Lerp(
                    {sideEnd.x, BossPhase2SideAboveY, sideEnd.z}, sideEnd, descent);
                railPosition = Vector3::Lerp(
                    {railEnd.x, BossPhase2RailAboveY, railEnd.z}, railEnd, descent);
            }
            const float railWeight = shooter.RailBlend();
            return {Vector3::Lerp(sidePosition, railPosition, railWeight),
                {}, Math::Lerp(Math::Pi, Math::HalfPi, railWeight),
                Math::Lerp(BossSideModelScale, BossModelScale, railWeight)};
        }

        // 2Dでは反対舷から攻略区画を右寄りに見せ、3Dでは船体長を奥行きへ向ける
        const float railWeight = shooter.RailBlend();
        const Vector3 sidePosition {
            sideX,
            0.0f,
            SideScrollingShooter::SidePlaneZ + BossSideZOffset
        };
        return {Vector3::Lerp(sidePosition, finalPosition, railWeight), {},
            Math::Lerp(Math::Pi, Math::HalfPi, railWeight),
            Math::Lerp(BossSideModelScale, BossModelScale, railWeight)};
    }

    // 海面下降後に画面後方の下側から巨体を浮上させる
    const float reveal = SmoothStep(Math::Clamp01(
        static_cast<float>(shooter.m_bossIntroductionTimer - BossAscentFrames) /
        static_cast<float>(BossRevealFrames)));
    const Vector3 hiddenPosition {0.0f, -18.0f, -14.0f};
    return {Vector3::Lerp(hiddenPosition, finalPosition, reveal),
        {}, Math::HalfPi, BossModelScale};
}

float SideScrollingShooter::Stage3Module::Phase1SectionProgress(const Enemy& boss) {
    if (boss.phase >= BossPhase2Travel) {
        return static_cast<float>(Stage3BossModelView::Phase1SectionCount - 1);
    }
    const int section = (std::clamp)(static_cast<int>(boss.phase), 0,
        Stage3BossModelView::Phase1SectionCount - 1);
    if (boss.motionAge <= 0 || section == 0) return static_cast<float>(section);
    const float progress = SmoothStep(1.0f -
        static_cast<float>(boss.motionAge) /
        static_cast<float>(BossSectionTransitionFrames));
    return static_cast<float>(section - 1) + progress;
}

float SideScrollingShooter::Stage3Module::Phase1AdvanceZ(const Enemy& boss) {
    return BossSectionAdvanceZ(Phase1SectionProgress(boss));
}

float SideScrollingShooter::Stage3Module::Phase1FocusZ(const Enemy& boss) {
    auto SectionFocus = [](int section) {
        const int first = Stage3BossModelView::Phase1TopGunIndex(section, 0);
        const int second = Stage3BossModelView::Phase1TopGunIndex(section, 1);
        const float localX = (Stage3BossModelView::TopGunMount(first).localPosition.x +
            Stage3BossModelView::TopGunMount(second).localPosition.x) * 0.5f;
        return BossFinalWorldZ - localX * BossModelScale;
    };

    const float sectionProgress = Phase1SectionProgress(boss);
    const int section = (std::clamp)(static_cast<int>(sectionProgress), 0,
        Stage3BossModelView::Phase1SectionCount - 1);
    const int nextSection = (std::min)(
        section + 1, Stage3BossModelView::Phase1SectionCount - 1);
    const float progress = sectionProgress - static_cast<float>(section);
    return Math::Lerp(SectionFocus(section), SectionFocus(nextSection), progress) +
        BossSectionAdvanceZ(sectionProgress);
}

bool SideScrollingShooter::Stage3Module::TryHitBossPart(
    const SideScrollingShooter& shooter, const Shot& shot,
    const Enemy& boss, BossPart& part) {
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::None ||
        boss.motionAge > 0 ||
        boss.phase >= static_cast<float>(Stage3BossModelView::Phase1SectionCount)) {
        return false;
    }

    const int section = static_cast<int>(boss.phase);
    const BossModelTransform transform = BossTransform(shooter, boss);
    const bool railMode = shooter.IsRailGameplayActive();
    for (int slot = 0; slot < Stage3BossModelView::Phase1TurretsPerSection; ++slot) {
        const int partIndex = Stage3BossModelView::Phase1PartIndex(section, slot);
        if (boss.bossPartHp[partIndex] <= 0) continue;
        const int topGunIndex = Stage3BossModelView::Phase1TopGunIndex(section, slot);
        const Vector3 local = Stage3BossModelView::TopGunMount(topGunIndex).localPosition;
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 world {
            transform.position.x + (local.x * cosine + local.z * sine) * transform.scale,
            transform.position.y + local.y * transform.scale,
            transform.position.z + (-local.x * sine + local.z * cosine) * transform.scale
        };
        const bool hit = railMode ?
            Hit3DSegment(
                ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, world.x, world.y, world.z, 1.05f) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y), 1.05f / WorldXScale);
        if (!hit) continue;
        part = static_cast<BossPart>(partIndex);
        return true;
    }
    return false;
}

bool SideScrollingShooter::Stage3Module::BlocksPlayerShot(
    const SideScrollingShooter& shooter, const Shot& shot, const Enemy& boss) {
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::None ||
        boss.phase >= static_cast<float>(Stage3BossModelView::Phase1SectionCount)) {
        return false;
    }

    // 描画に使う三つの主船体と同じ中心を球で覆い、装甲へ入った弾を止める
    constexpr Vector3 HullCenters[] = {
        {0.0f, 0.0f, 0.0f}, {-6.15f, 0.0f, 0.0f}, {6.10f, -0.02f, 0.0f}
    };
    constexpr float HullRadii[] = {2.95f, 2.45f, 2.30f};
    const BossModelTransform transform = BossTransform(shooter, boss);
    const float cosine = std::cos(transform.yaw);
    const float sine = std::sin(transform.yaw);
    const bool railMode = shooter.IsRailGameplayActive();
    for (std::size_t index = 0; index < std::size(HullCenters); ++index) {
        const Vector3 local = HullCenters[index];
        const Vector3 center {
            transform.position.x + (local.x * cosine + local.z * sine) * transform.scale,
            transform.position.y + local.y * transform.scale,
            transform.position.z + (-local.x * sine + local.z * cosine) * transform.scale
        };
        const float radius = HullRadii[index] * transform.scale;
        const bool hit = railMode ?
            Hit3DSegment(
                ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, center.x, center.y, center.z, radius) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(center.x), FromWorldY(center.y), radius / WorldXScale);
        if (hit) {
            return true;
        }
    }
    return false;
}

bool SideScrollingShooter::Stage3Module::TryDamageStageTarget(
    SideScrollingShooter& shooter, Shot& shot) {
    if (shot.enemy || shooter.m_enemies.empty() ||
        shooter.m_enemies[0].phase != BossPhase3Survival) return false;

    for (int index = 0; index < ShooterStages::Stage3::ReflectFunnelCount; ++index) {
        auto& funnel = shooter.m_stage3.reflectFunnels[index];
        if (!funnel.active) continue;
        const bool hit = shooter.IsRailGameplayActive() ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                ToWorldX(funnel.x), ToWorldY(funnel.y), funnel.z, 0.72f) :
            Hit(shot.x, shot.y, shot.hitRadius, funnel.x, funnel.y, 0.11f);
        if (!hit) continue;

        shooter.SpawnExplosion(shot.x, shot.y, shot.z);
        if (!shot.piercing) shot.active = false;
        funnel.hp -= shot.damage;
        if (funnel.hp > 0) return true;

        shooter.SpawnExplosion(funnel.x, funnel.y, funnel.z, true);
        funnel.active = false;
        for (auto& enemyShot : shooter.m_shots) {
            if (!enemyShot.active || !enemyShot.enemy) continue;
            const bool reflect = enemyShot.stage2.kind == ShooterStages::Stage2::ShotKind::ReflectPass ||
                enemyShot.stage2.kind == ShooterStages::Stage2::ShotKind::ReflectAttack;
            if (reflect && (enemyShot.barrageIndex == index || enemyShot.barrageCount == index)) {
                enemyShot.active = false;
            }
        }
        shooter.PlayHitSound();
        return true;
    }
    return false;
}

void SideScrollingShooter::Stage3Module::FireBossPartBarrage(
    SideScrollingShooter& shooter, const Enemy& boss) {
    if (boss.phase == BossPhase2Survival) {
        const int elapsed = Phase2SurvivalFrames - boss.motionAge;
        const int cycleLength = elapsed < Phase2SurvivalFrames / 2 ? 360 : 300;
        const int cycle = elapsed % cycleLength;
        const bool machineGunFrame = cycle == 0 || cycle == 32 || cycle == 64;
        const bool heavyCannonFrame = cycle == 132;
        const bool missileFrame = cycle == 220;
        if (!machineGunFrame && !heavyCannonFrame && !missileFrame) return;

        const BossModelTransform transform = BossTransform(shooter, boss);
        const Vector3 target {
            ToWorldX(shooter.m_playerX),
            ToWorldY(shooter.m_playerY),
            shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ
        };
        auto WorldPosition = [&transform](const Vector3& local) {
            const float cosine = std::cos(transform.yaw);
            const float sine = std::sin(transform.yaw);
            return Vector3 {
                transform.position.x + (local.x * cosine + local.z * sine) * transform.scale,
                transform.position.y + local.y * transform.scale,
                transform.position.z + (-local.x * sine + local.z * cosine) * transform.scale
            };
        };
        auto SpawnAimed = [&](const Vector3& local, float speed, int damage,
            ShooterStages::Stage2::ShotKind kind, float hitRadius) {
            const Vector3 mount = WorldPosition(local);
            const float dx = target.x - mount.x;
            const float dy = target.y - mount.y;
            const float dz = shooter.IsRailGameplayActive() ? target.z - mount.z : 0.0f;
            const float length = (std::max)(0.001f,
                std::sqrt(dx * dx + dy * dy + dz * dz));
            const Vector3 direction {dx / length, dy / length, dz / length};
            for (auto& shot : shooter.m_shots) {
                if (shot.active) continue;
                shot = {};
                shot.x = FromWorldX(mount.x);
                shot.y = FromWorldY(mount.y);
                shot.z = mount.z;
                shot.transitionSideX = shot.x;
                shot.transitionSideY = shot.y;
                shot.vx = FromWorldX(direction.x * speed);
                shot.vy = FromWorldY(direction.y * speed);
                shot.vz = direction.z * speed;
                shot.hitRadius = hitRadius;
                shot.damage = damage;
                shot.enemy = true;
                shot.special = true;
                shot.stage2.kind = kind;
                shot.active = true;
                break;
            }
        };

        // 各周期で機銃、大口径砲、追尾ミサイルを順番に使用する
        if (machineGunFrame) {
            for (int i = 0; i < Stage3BossModelView::GondolaMachineGunCount; ++i) {
                SpawnAimed(Stage3BossModelView::GondolaMachineGunMount(i).localPosition,
                    0.50f * BossLaterPhaseSpeedScale, 1,
                    ShooterStages::Stage2::ShotKind::None, 0.025f);
            }
        } else if (heavyCannonFrame) {
            for (int i = 0; i < Stage3BossModelView::HeavyCannonCount; ++i) {
                SpawnAimed(Stage3BossModelView::HeavyCannonMount(i).localPosition,
                    0.32f * BossLaterPhaseSpeedScale, 3,
                    ShooterStages::Stage2::ShotKind::Missile, 0.070f);
            }
        } else {
            for (int i = 0; i < Stage3BossModelView::MissilePodCount; ++i) {
                SpawnAimed(Stage3BossModelView::MissilePodMount(i).localPosition,
                    0.40f * BossLaterPhaseSpeedScale, 2,
                    ShooterStages::Stage2::ShotKind::Funnel, 0.055f);
            }
        }
        return;
    }

    if (boss.motionAge > 0 ||
        boss.phase >= BossPhase2Travel) {
        return;
    }

    const int section = static_cast<int>(boss.phase);
    const BossModelTransform transform = BossTransform(shooter, boss);
    const Vector3 target {
        ToWorldX(shooter.m_playerX),
        ToWorldY(shooter.m_playerY),
        shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ
    };
    for (int slot = 0; slot < Stage3BossModelView::Phase1TurretsPerSection; ++slot) {
        const int partIndex = Stage3BossModelView::Phase1PartIndex(section, slot);
        if (boss.bossPartHp[partIndex] <= 0) continue;
        const int topGunIndex = Stage3BossModelView::Phase1TopGunIndex(section, slot);
        const Vector3 mount = Stage3BossModelView::TopGunWorldPosition(topGunIndex, transform);
        const float dx = target.x - mount.x;
        const float dy = target.y - mount.y;
        const float dz = shooter.IsRailGameplayActive() ? target.z - mount.z : 0.0f;
        const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
        const Vector3 direction {dx / length, dy / length, dz / length};
        const Vector3 muzzle = mount + direction *
            (BossTurretMuzzleLocalOffset * transform.scale);

        // Stage2ファンネル外観を使う小型ミサイルを一基ずつ射出する
        for (auto& shot : shooter.m_shots) {
            if (shot.active) continue;
            shot = {};
            shot.x = FromWorldX(muzzle.x);
            shot.y = FromWorldY(muzzle.y);
            shot.z = muzzle.z;
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            shot.vx = 0.0f;
            shot.vy = BossMissileLaunchVelocity;
            shot.vz = 0.0f;
            shot.hitRadius = 0.055f;
            shot.damage = 2;
            shot.enemy = true;
            shot.stage2.kind = ShooterStages::Stage2::ShotKind::Funnel;
            shot.stage2.delayedEngine = true;
            shot.active = true;
            break;
        }
    }
}

void SideScrollingShooter::Stage3Module::FireBossMachineGun(
    SideScrollingShooter& shooter, const Enemy& boss, bool aimed) {
    if (boss.motionAge > 0 ||
        boss.phase >= static_cast<float>(Stage3BossModelView::Phase1SectionCount)) {
        return;
    }

    const int section = static_cast<int>(boss.phase);
    const BossModelTransform transform = BossTransform(shooter, boss);
    const Vector3 target {
        ToWorldX(shooter.m_playerX),
        ToWorldY(shooter.m_playerY),
        shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ
    };
    for (int slot = 0; slot < Stage3BossModelView::Phase1TurretsPerSection; ++slot) {
        const int partIndex = Stage3BossModelView::Phase1PartIndex(section, slot);
        if (boss.bossPartHp[partIndex] <= 0) continue;

        // 描画と同じ砲台支点から自機方向を求め、砲身先端まで進めて生成する
        const int topGunIndex = Stage3BossModelView::Phase1TopGunIndex(section, slot);
        const Vector3 mount = Stage3BossModelView::TopGunWorldPosition(topGunIndex, transform);
        Vector3 direction {
            target.x - mount.x,
            target.y - mount.y,
            shooter.IsRailGameplayActive() ? target.z - mount.z : 0.0f
        };
        float length = (std::max)(0.001f, std::sqrt(
            direction.x * direction.x + direction.y * direction.y + direction.z * direction.z));
        direction = direction * (1.0f / length);
        if (!aimed) {
            const std::uint32_t seed = static_cast<std::uint32_t>(boss.age) * 17u +
                static_cast<std::uint32_t>(topGunIndex) * 131u;
            direction.x += BossMachineGunSpread(seed) * 0.62f;
            direction.y += BossMachineGunSpread(seed + 53u) * 0.48f;
            if (shooter.IsRailGameplayActive()) {
                direction.z += BossMachineGunSpread(seed + 101u) * 0.10f;
            }
            length = (std::max)(0.001f, std::sqrt(
                direction.x * direction.x + direction.y * direction.y + direction.z * direction.z));
            direction = direction * (1.0f / length);
        }
        const Vector3 muzzle = mount + direction *
            (BossTurretMuzzleLocalOffset * transform.scale);

        // 通常敵弾モデルを小口径弾として固定長Poolへ追加する
        for (auto& shot : shooter.m_shots) {
            if (shot.active) continue;
            shot = {};
            shot.x = FromWorldX(muzzle.x);
            shot.y = FromWorldY(muzzle.y);
            shot.z = muzzle.z;
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            shot.vx = FromWorldX(direction.x * BossMachineGunSpeed);
            shot.vy = FromWorldY(direction.y * BossMachineGunSpeed);
            shot.vz = direction.z * BossMachineGunSpeed;
            shot.hitRadius = 0.022f;
            shot.enemy = true;
            shot.active = true;
            break;
        }
    }
}

void SideScrollingShooter::Stage3Module::TickSpecialShotBeforeMove(
    SideScrollingShooter& shooter, Shot& shot) {
    if (!shot.enemy) return;

    if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::FunnelMissile) {
        if (++shot.age >= ReflectFunnelMissileLifetimeFrames) {
            shot.active = false;
            return;
        }
        const float dx = ToWorldX(shooter.m_playerX - shot.x);
        const float dy = ToWorldY(shooter.m_playerY - shot.y);
        const float dz = shooter.IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
        const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
        const float desiredVx = FromWorldX(dx / length * ReflectFunnelMissileSpeed);
        const float desiredVy = FromWorldY(dy / length * ReflectFunnelMissileSpeed);
        const float desiredVz = dz / length * ReflectFunnelMissileSpeed;
        shot.vx += (desiredVx - shot.vx) * ReflectFunnelMissileTurnRate;
        shot.vy += (desiredVy - shot.vy) * ReflectFunnelMissileTurnRate;
        shot.vz += (desiredVz - shot.vz) * ReflectFunnelMissileTurnRate;
        return;
    }

    if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::ReflectPass) {
        ++shot.age;
        const int target = shot.barrageCount;
        if (target < 0 || target >= ShooterStages::Stage3::ReflectFunnelCount ||
            !shooter.m_stage3.reflectFunnels[target].active) {
            shot.active = false;
            return;
        }
        const auto& funnel = shooter.m_stage3.reflectFunnels[target];
        const float dx = ToWorldX(funnel.x - shot.x);
        const float dy = ToWorldY(funnel.y - shot.y);
        const float dz = shooter.IsRailGameplayActive() ? funnel.z - shot.z : 0.0f;
        const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length > ReflectShotSpeed) return;

        // 受け取ったファンネルから現在の自機位置へ反射する
        shot.x = funnel.x; shot.y = funnel.y; shot.z = funnel.z;
        const float playerDx = ToWorldX(shooter.m_playerX - shot.x);
        const float playerDy = ToWorldY(shooter.m_playerY - shot.y);
        const float playerDz = shooter.IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
        const float playerLength = (std::max)(0.001f,
            std::sqrt(playerDx * playerDx + playerDy * playerDy + playerDz * playerDz));
        shot.vx = FromWorldX(playerDx / playerLength * ReflectShotSpeed);
        shot.vy = FromWorldY(playerDy / playerLength * ReflectShotSpeed);
        shot.vz = playerDz / playerLength * ReflectShotSpeed;
        shot.stage2.kind = ShooterStages::Stage2::ShotKind::ReflectAttack;
        return;
    }
    if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::ReflectAttack) {
        ++shot.age;
        return;
    }

    // Phase2弾は画面外のゴンドラ砲から侵入するまでの経過時間を共有する
    if (shot.special) ++shot.age;
    else if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::Funnel) ++shot.age;
    if (shot.stage2.kind != ShooterStages::Stage2::ShotKind::Funnel) return;

    // Phase1ミサイルは上昇後に短く落下し、点火時の自機位置へ補助エンジンで加速する
    if (shot.stage2.delayedEngine) {
        if (shot.age < BossMissileEngineStartFrame) {
            shot.vy -= BossMissileGravity;
            return;
        }
        if (shot.age == BossMissileEngineStartFrame) {
            const float dx = ToWorldX(shooter.m_playerX - shot.x);
            const float dy = ToWorldY(shooter.m_playerY - shot.y);
            const float dz = shooter.IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
            const float length = (std::max)(
                0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
            shot.stage2.engineVx = FromWorldX(dx / length * BossMissileSpeed);
            shot.stage2.engineVy = FromWorldY(dy / length * BossMissileSpeed);
            shot.stage2.engineVz = dz / length * BossMissileSpeed;
        }
        shot.vx += (shot.stage2.engineVx - shot.vx) * BossMissileEngineAcceleration;
        shot.vy += (shot.stage2.engineVy - shot.vy) * BossMissileEngineAcceleration;
        shot.vz += (shot.stage2.engineVz - shot.vz) * BossMissileEngineAcceleration;
        return;
    }

    if (shot.age > BossDirectMissileHomingFrames) return;

    // 発射直後だけ現在の自機方向へ緩く旋回し、その後は得た進路を維持する
    const float dx = ToWorldX(shooter.m_playerX - shot.x);
    const float dy = ToWorldY(shooter.m_playerY - shot.y);
    const float dz = shooter.IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
    const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
    const float speed = BossMissileSpeed *
        (shot.special ? BossLaterPhaseSpeedScale : 1.0f);
    const float desiredVx = FromWorldX(dx / length * speed);
    const float desiredVy = FromWorldY(dy / length * speed);
    const float desiredVz = dz / length * speed;
    shot.vx += (desiredVx - shot.vx) * BossDirectMissileTurnRate;
    shot.vy += (desiredVy - shot.vy) * BossDirectMissileTurnRate;
    shot.vz += (desiredVz - shot.vz) * BossDirectMissileTurnRate;
}

bool SideScrollingShooter::Stage3Module::IsShotCullProtected(const Shot& shot) {
    return shot.enemy && ((shot.special && shot.age <= BossLaterPhaseCullGraceFrames) ||
        (shot.stage2.kind == ShooterStages::Stage2::ShotKind::Funnel &&
            shot.age <= (shot.stage2.delayedEngine ? BossMissileCullGraceFrames : 24)));
}

float SideScrollingShooter::Stage3Module::EnemyShotHitRadius(
    const Shot& shot, bool railMode) {
    if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::FunnelMissile) {
        return railMode ? 0.24f : 0.030f;
    }
    if (shot.stage2.kind != ShooterStages::Stage2::ShotKind::Funnel) {
        if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::Missile) {
            return railMode ? 0.55f : 0.070f;
        }
        return railMode ? 0.28f : 0.025f;
    }
    return railMode ? 0.42f : 0.055f;
}

bool SideScrollingShooter::Stage3Module::CanEnemyShotDamagePlayer(const Shot& shot) {
    return shot.stage2.kind != ShooterStages::Stage2::ShotKind::ReflectPass;
}

bool SideScrollingShooter::Stage3Module::DrawSpecialShot(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Shot& shot, float yaw) {
    if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::FunnelMissile) {
        constexpr float Body[4] = {0.92f, 0.36f, 0.10f, 1.0f};
        constexpr float Glow[4] = {1.00f, 0.72f, 0.18f, 0.85f};
        DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
            0.07f, 0.07f, 0.24f, Body, yaw);
        DrawModelPrimitive(renderer, camera, 1, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
            0.10f, 0.10f, 0.08f, Glow, yaw);
        return true;
    }
    if (shot.stage2.kind != ShooterStages::Stage2::ShotKind::ReflectPass &&
        shot.stage2.kind != ShooterStages::Stage2::ShotKind::ReflectAttack) return false;
    (void)shooter;
    constexpr float Core[4] = {1.00f, 0.32f, 0.04f, 1.0f};
    constexpr float Glow[4] = {1.00f, 0.72f, 0.16f, 0.82f};
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.22f, 0.22f, 0.64f, Glow, yaw);
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.12f, 0.12f, 0.40f, Core, yaw);
    return true;
}

bool SideScrollingShooter::Stage3Module::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& boss, int bossPart) {
    if (bossPart < 0 || bossPart >= Stage3BossModelView::TopGunCount) return false;

    const int section = bossPart / Stage3BossModelView::Phase1TurretsPerSection;
    const int slot = bossPart % Stage3BossModelView::Phase1TurretsPerSection;
    const int topGunIndex = Stage3BossModelView::Phase1TopGunIndex(section, slot);
    const BossModelTransform transform = BossTransform(shooter, boss);
    int piece = 0;
    auto SpawnPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float yaw, float pitch) {
        (void)pitch;
        const float direction = piece++ % 2 == 0 ? -1.0f : 1.0f;
        shooter.SpawnDebrisPiece(position.x, position.y, position.z,
            direction * 0.045f, 0.055f + static_cast<float>(piece) * 0.006f,
            direction * 0.030f, yaw, direction * 0.11f,
            shape, scale.x, scale.y, scale.z, color, 54, 42, false);
    };
    Stage3BossModelView::DrawTopGun(topGunIndex, transform, {}, false, SpawnPart);
    return true;
}

bool SideScrollingShooter::Stage3Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& boss, float yaw) {
    if (boss.type != Stage::BossEnemy) return false;
    (void)yaw;

    const BossModelTransform transform = BossTransform(shooter, boss);
    const float railWeight = shooter.RailBlend();
    auto DrawPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float partYaw, float partPitch) {
        // 2Dでは巨大船体の奥行きを固定Zへ畳み、プレイヤー描画面への突き抜けを防ぐ
        const Vector3 drawPosition {
            position.x,
            position.y,
            Math::Lerp(transform.position.z, position.z, railWeight)
        };
        const Vector3 drawScale {
            scale.x,
            scale.y,
            Math::Lerp((std::min)(scale.z, BossSidePrimitiveDepth), scale.z, railWeight)
        };
        DrawModelPrimitive(renderer, camera, shape,
            drawPosition.x, drawPosition.y, drawPosition.z,
            drawScale.x, drawScale.y, drawScale.z, color, partYaw, partPitch);
    };

    // 巨大船体と未破壊の全砲台を常時描画し、現在区画だけを発光させる
    Stage3BossModelView::DrawStaticBody(transform, DrawPart);
    Stage3BossModelView::DrawGondolaBody(transform, DrawPart);
    const int section = (std::clamp)(static_cast<int>(boss.phase), 0,
        Stage3BossModelView::Phase1SectionCount - 1);
    const Vector3 target {
        ToWorldX(shooter.m_playerX),
        ToWorldY(shooter.m_playerY),
        Math::Lerp(transform.position.z, PlayerRailZ, railWeight)
    };
    for (int topGunIndex = 0; topGunIndex < Stage3BossModelView::TopGunCount; ++topGunIndex) {
        const int partIndex = Stage3BossModelView::Phase1PartIndexForTopGun(topGunIndex);
        if (boss.bossPartHp[partIndex] <= 0) continue;
        const bool active = boss.motionAge <= 0 &&
            boss.phase < static_cast<float>(Stage3BossModelView::Phase1SectionCount) &&
            partIndex / Stage3BossModelView::Phase1TurretsPerSection == section;
        const Vector3 aim = Stage3BossModelView::TopGunAimRotation(
            topGunIndex, transform, target);
        Stage3BossModelView::DrawTopGun(topGunIndex, transform, aim, active, DrawPart);
    }
    if (boss.phase < BossPhase2Travel) return true;

    auto AimRotation = [&transform, &target](const Stage3BossWeaponMount& mount) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 world {
            transform.position.x +
                (mount.localPosition.x * cosine + mount.localPosition.z * sine) * transform.scale,
            transform.position.y + mount.localPosition.y * transform.scale,
            transform.position.z +
                (-mount.localPosition.x * sine + mount.localPosition.z * cosine) * transform.scale
        };
        const float dx = target.x - world.x;
        const float dy = target.y - world.y;
        const float dz = target.z - world.z;
        const float localX = dx * cosine - dz * sine;
        const float localZ = dx * sine + dz * cosine;
        const float horizontal = (std::max)(0.001f,
            std::sqrt(localX * localX + localZ * localZ));
        return Vector3 {
            (std::clamp)(-std::atan2(dy, horizontal),
                -Math::ToRadians(55.0f), Math::ToRadians(55.0f)),
            std::atan2(localZ, -localX), 0.0f
        };
    };
    if (boss.phase < BossPhase3Survival) {
        for (int i = 0; i < Stage3BossModelView::GondolaMachineGunCount; ++i) {
            Stage3BossModelView::DrawGondolaMachineGun(i, transform,
                AimRotation(Stage3BossModelView::GondolaMachineGunMount(i)), DrawPart);
        }
        for (int i = 0; i < Stage3BossModelView::HeavyCannonCount; ++i) {
            Stage3BossModelView::DrawHeavyCannon(i, transform,
                AimRotation(Stage3BossModelView::HeavyCannonMount(i)), DrawPart);
        }
    }

    const float deployElapsed = boss.phase == BossPhase2Travel ? 0.0f :
        (boss.phase == BossPhase2Deploy ?
            static_cast<float>(BossPhase2DeployFrames - boss.motionAge) :
            static_cast<float>(BossPhase2DeployFrames));
    const float podOpen = Math::Clamp01(deployElapsed / 45.0f);
    if (boss.phase < BossPhase3Survival) {
        for (int i = 0; i < Stage3BossModelView::MissilePodCount; ++i) {
            Stage3BossModelView::DrawMissilePod(i, transform, podOpen, DrawPart);
        }
    }
    for (int i = 0; i < Stage3BossModelView::FunnelPodCount; ++i) {
        Stage3BossModelView::DrawFunnelPod(i, transform, podOpen, DrawPart);
    }

    // Phase3では破壊可能な反射ファンネルだけを追加描画する
    if (boss.phase == BossPhase3Survival) {
        for (const auto& state : shooter.m_stage3.reflectFunnels) {
            if (!state.active) continue;
            const Vector3 position {ToWorldX(state.x), ToWorldY(state.y),
                Math::Lerp(transform.position.z, state.z, railWeight)};
            const Vector3 playerTarget {ToWorldX(shooter.m_playerX), ToWorldY(shooter.m_playerY),
                Math::Lerp(transform.position.z, PlayerRailZ, railWeight)};
            const float dx = playerTarget.x - position.x;
            const float dy = playerTarget.y - position.y;
            const float dz = playerTarget.z - position.z;
            const float horizontal = (std::max)(0.001f, std::sqrt(dx * dx + dz * dz));
            const BossModelTransform funnel {position, {}, 0.0f, 1.6f};
            Stage3FunnelModelView::DrawReflectShot(funnel, std::atan2(dz, -dx),
                -std::atan2(dy, horizontal), 0.0f, DrawPart);
        }
    }

    // 三基を時間差で射出口から所定位置へ送り、到着後にEmitterを開く
    const Vector3 funnelTargets[Stage3BossModelView::FunnelPodCount] = {
        {-Stage3BarrierCageView::BarrierHalfLength,
            Stage3BarrierCageView::BarrierTopY,
            -Stage3BarrierCageView::BarrierHalfWidth},
        {0.0f,
            Stage3BarrierCageView::BarrierTopY - Stage3BarrierCageView::BarrierHeight,
            Stage3BarrierCageView::BarrierHalfWidth},
        {Stage3BarrierCageView::BarrierHalfLength,
            Stage3BarrierCageView::BarrierTopY,
            -Stage3BarrierCageView::BarrierHalfWidth}
    };
    auto WorldPosition = [&transform](const Vector3& local) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        return Vector3 {
            transform.position.x + (local.x * cosine + local.z * sine) * transform.scale,
            transform.position.y + local.y * transform.scale,
            transform.position.z + (-local.x * sine + local.z * cosine) * transform.scale
        };
    };
    for (int i = 0; i < Stage3BossModelView::FunnelPodCount; ++i) {
        const float launchFrame = 45.0f + static_cast<float>(i) * 24.0f;
        if (deployElapsed < launchFrame) continue;
        const float flight = SmoothStep(Math::Clamp01(
            (deployElapsed - launchFrame) / 75.0f));
        const Vector3 local = Vector3::Lerp(
            Stage3BossModelView::FunnelLaunchLocalPosition(i), funnelTargets[i], flight);
        const BossModelTransform funnel {WorldPosition(local), {}, transform.yaw, 1.6f};
        const float emitterOpen = Math::Clamp01(
            (deployElapsed - launchFrame - 75.0f) / 30.0f);
        Stage3FunnelModelView::DrawBarrier(funnel, emitterOpen, DrawPart);
    }

    // ファンネル配置後に5面バリアを上端から下へ展開する
    if (deployElapsed >= 165.0f) {
        Stage3BarrierCagePose pose;
        pose.openAmount = Math::Clamp01((deployElapsed - 165.0f) / 75.0f);
        pose.scrollOffset = static_cast<float>(boss.age) * 0.16f;
        pose.flicker = 0.5f + std::sin(static_cast<float>(boss.age) * 0.12f) * 0.5f;
        Stage3BarrierCageView::Draw(transform, pose, DrawPart);
    }
    return true;
}

void SideScrollingShooter::Stage3Module::ApplyCameraCorrection(
    const SideScrollingShooter& shooter,
    Vector3& railPosition, Vector3& railTarget) {
    if (!shooter.m_bossBattle || !shooter.m_enemies[0].active) return;

    const Vector3 defaultPosition = railPosition;
    const Vector3 defaultTarget = railTarget;

    const bool entrance =
        shooter.m_bossIntroductionPhase == BossIntroductionPhase::Entrance;
    const float ascent = entrance ? SmoothStep(Math::Clamp01(
        static_cast<float>(shooter.m_bossIntroductionTimer) /
        static_cast<float>(BossAscentFrames))) : 1.0f;
    const float flyOver = entrance ? SmoothStep(Math::Clamp01(
        static_cast<float>(shooter.m_bossIntroductionTimer -
            BossAscentFrames - BossRevealFrames) /
        static_cast<float>(BossFlyOverFrames))) : 1.0f;

    // 前半は海面を置き去りにして上昇し、後半は艦尾甲板を見下ろす
    Vector3 ascentPosition = railPosition;
    Vector3 ascentTarget = railTarget;
    ascentPosition.y += 5.0f * ascent;
    ascentTarget.y += 5.5f * ascent;
    const float focusZ = Phase1FocusZ(shooter.m_enemies[0]);
    const float advanceZ = Phase1AdvanceZ(shooter.m_enemies[0]);
    const Vector3 deckPosition {
        ToWorldX(shooter.m_playerX) * 0.12f,
        11.5f,
        -22.0f - advanceZ * BossCameraAdvanceRate
    };
    const Vector3 deckTarget {0.0f, BossDeckTopY + 1.0f, focusZ};
    railPosition = Vector3::Lerp(ascentPosition, deckPosition, flyOver);
    railTarget = Vector3::Lerp(ascentTarget, deckTarget, flyOver);

    // Phase2移動に合わせて甲板カメラから通常レールカメラへ戻す
    const Enemy& boss = shooter.m_enemies[0];
    if (boss.phase >= BossPhase2Travel) {
        const float progress = boss.phase == BossPhase2Travel ? SmoothStep(
            1.0f - static_cast<float>(boss.motionAge) /
                static_cast<float>(BossPhase2TravelFrames)) : 1.0f;
        const Vector3 barrierPosition {
            defaultPosition.x, BossPhase2RailCenterY, defaultPosition.z};
        const Vector3 barrierTarget {
            defaultTarget.x, BossPhase2RailCenterY, defaultTarget.z};
        railPosition = Vector3::Lerp(railPosition, barrierPosition, progress);
        railTarget = Vector3::Lerp(railTarget, barrierTarget, progress);
    }
}

float SideScrollingShooter::Stage3Module::SideCameraY(
    const SideScrollingShooter& shooter) {
    if (!shooter.m_bossBattle || !shooter.m_enemies[0].active) return 0.0f;
    const Enemy& boss = shooter.m_enemies[0];
    if (boss.phase < BossPhase2Travel) return 0.0f;
    const float progress = boss.phase == BossPhase2Travel ? SmoothStep(
        1.0f - static_cast<float>(boss.motionAge) /
            static_cast<float>(BossPhase2TravelFrames)) : 1.0f;
    return Math::Lerp(0.0f, BossPhase2SideCenterY, progress);
}

Vector2 SideScrollingShooter::Stage3Module::SidePlayerYRange(
    const SideScrollingShooter& shooter) {
    if (!shooter.m_bossBattle || !shooter.m_enemies[0].active) {
        return {Side2DPlayerMinY, Side2DPlayerMaxY};
    }
    const Enemy& boss = shooter.m_enemies[0];
    if (boss.phase < BossPhase2Travel) return {Side2DPlayerMinY, Side2DPlayerMaxY};
    const float progress = boss.phase == BossPhase2Travel ? SmoothStep(
        1.0f - static_cast<float>(boss.motionAge) /
            static_cast<float>(BossPhase2TravelFrames)) : 1.0f;
    return {
        Math::Lerp(Side2DPlayerMinY,
            FromWorldY(BossPhase2SideBottomY + BossBarrierPlayerMarginY), progress),
        Math::Lerp(Side2DPlayerMaxY,
            FromWorldY(BossPhase2SideTopY - BossBarrierPlayerMarginY), progress)
    };
}

float SideScrollingShooter::Stage3Module::RailGroundY(
    const SideScrollingShooter& shooter) {
    if (!shooter.m_bossBattle) return -3.65f;
    if (shooter.m_enemies[0].active &&
        shooter.m_enemies[0].phase >= BossPhase2Travel) {
        return BossPhase2GroundY;
    }
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return BossDeckTopY;
    }
    const float deckArrival = SmoothStep(Math::Clamp01(
        static_cast<float>(shooter.m_bossIntroductionTimer -
            BossAscentFrames - BossRevealFrames) /
        static_cast<float>(BossFlyOverFrames)));
    return Math::Lerp(-3.65f, BossDeckTopY, deckArrival);
}

float SideScrollingShooter::Stage3Module::RailPlayerMaxY(
    const SideScrollingShooter& shooter) {
    if (!shooter.m_bossBattle || !shooter.m_enemies[0].active ||
        shooter.m_enemies[0].phase < BossPhase2Travel) return 0.9f;
    return FromWorldY(BossPhase2RailTopY - BossBarrierPlayerMarginY);
}

bool SideScrollingShooter::Stage3Module::ShouldDrawEnemy(
    const SideScrollingShooter& shooter, const Enemy& enemy) {
    if (enemy.type != Stage::BossEnemy ||
        shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return true;
    }
    return shooter.m_bossIntroductionTimer >= BossAscentFrames - 30;
}

bool SideScrollingShooter::Stage3Module::IsViewLocked(
    const SideScrollingShooter& shooter) {
    (void)shooter;
    return false;
}

void SideScrollingShooter::Stage3Module::TickBossIntroduction(
    SideScrollingShooter& shooter) {
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) return;
    if (!shooter.IsRailGameplayActive()) {
        shooter.RequestViewMode(ViewMode::Rail3D);
    }

    // 出現演出中も移動と射撃を受け付け、生成済みの自機弾と演出を進める
    shooter.m_shotCooldown = (std::max)(0, shooter.m_shotCooldown - 1);
    shooter.m_specialShotCooldown =
        (std::max)(0, shooter.m_specialShotCooldown - 1);
    shooter.m_invincible = (std::max)(0, shooter.m_invincible - 1);
    shooter.TickPlayer();
    shooter.TickPlayerWeapons();
    shooter.TickShots();
    shooter.TickExplosions();
    shooter.TickDebris();
    shooter.TickItems();
}

int SideScrollingShooter::Stage3Module::BossIntroductionFrames() {
    return BossIntroductionFrameCount;
}

float SideScrollingShooter::Stage3Module::BossSeaDrop(
    const SideScrollingShooter& shooter) {
    if (!shooter.m_bossBattle) return 0.0f;
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return BossSeaDropDistance;
    }
    const float progress = SmoothStep(Math::Clamp01(
        static_cast<float>(shooter.m_bossIntroductionTimer) /
        static_cast<float>(BossAscentFrames)));
    return BossSeaDropDistance * progress;
}

float SideScrollingShooter::Stage3Module::WrapNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) {
        wrapped += 2.0f;
    }
    return wrapped - 1.0f;
}

float SideScrollingShooter::Stage3Module::NightBlend(int frame) {
    return 1.0f - Math::Clamp01(static_cast<float>(frame - DawnStartFrame) /
        static_cast<float>(DawnFrame - DawnStartFrame));
}

bool SideScrollingShooter::Stage3Module::GetSeaSerpentMotion(
    int frame, SeaSerpentMotion& motion) {
    constexpr int CycleFrames = 420;
    constexpr int Duration[] = {112, 138, 172};
    constexpr int Segments[] = {14, 16, 23};
    constexpr float Elevation[] = {8.2f, 6.4f, 12.0f};
    constexpr float Travel[] = {12.0f, 17.0f, 14.0f};
    constexpr float Spacing[] = {0.90f, 0.78f, 1.12f};
    constexpr float Delay[] = {0.055f, 0.042f, 0.050f};
    constexpr float Scale[] = {1.70f, 2.00f, 3.20f};
    static_assert(Segments[2] > Segments[0] &&
        Elevation[2] > Elevation[0] && Scale[2] > Scale[0]);
    const int cycle = frame / CycleFrames;
    const int action = cycle % 3;
    const int cycleFrame = frame % CycleFrames;
    const int startFrame = 48 + (cycle * 97) % 170;
    if (cycleFrame < startFrame || cycleFrame >= startFrame + Duration[action]) {
        return false;
    }

    motion.segmentCount = Segments[action];
    motion.progress = static_cast<float>(cycleFrame - startFrame) /
        static_cast<float>(Duration[action] - 1);
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

constexpr float SideScrollingShooter::Stage3Module::GetSeaSerpentSegmentProgress(
    float progress, int segmentCount, float segmentDelay, int segmentIndex) {
    const float tailDelay = static_cast<float>(segmentCount - 1) * segmentDelay;
    return Math::Clamp01(progress * (1.0f + tailDelay) -
        static_cast<float>(segmentIndex) * segmentDelay);
}

SideScrollingShooter::Stage3Module::SeaSerpentSegment
SideScrollingShooter::Stage3Module::GetSeaSerpentSegment(
    const SeaSerpentMotion& motion, int segmentIndex) {
    static_assert(GetSeaSerpentSegmentProgress(1.0f, 23, 0.05f, 22) >=
        1.0f - Math::Epsilon);
    const float progress = GetSeaSerpentSegmentProgress(
        motion.progress, motion.segmentCount, motion.segmentDelay, segmentIndex);
    return {
        progress,
        std::sin(Math::Pi * progress) *
            (motion.elevation - static_cast<float>(segmentIndex) * 0.16f),
        motion.sideOriginX + motion.direction *
            (progress * motion.travel - static_cast<float>(segmentIndex) * motion.segmentSpacing),
        motion.railOriginX +
            std::sin(progress * 4.0f + static_cast<float>(segmentIndex) * 0.6f) * 6.0f,
        motion.railOriginZ + motion.railDirection *
            (progress * motion.railTravel - static_cast<float>(segmentIndex) * 2.6f),
        motion.scale * (segmentIndex == 0 ? 1.25f : 1.0f)
    };
}

void SideScrollingShooter::Stage3Module::DrawSky(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    const float nightBlend = NightBlend(shooter.m_frame);
    const ColorF skyColor {
        Math::Lerp(DaySkyColor[0], NightSkyColor[0], nightBlend),
        Math::Lerp(DaySkyColor[1], NightSkyColor[1], nightBlend),
        Math::Lerp(DaySkyColor[2], NightSkyColor[2], nightBlend),
        1.0f
    };
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, skyColor);
}

void SideScrollingShooter::Stage3Module::DrawBackground2D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();
    const float nightBlend = NightBlend(shooter.m_frame);
    const float dayBlend = 1.0f - nightBlend;
    const float waterColor[4] = {
        Math::Lerp(WaterColor[0], WaterColor[0] * 0.30f, nightBlend),
        Math::Lerp(WaterColor[1], WaterColor[1] * 0.36f, nightBlend),
        Math::Lerp(WaterColor[2], WaterColor[2] * 0.48f, nightBlend),
        1.0f
    };
    const float waveColor[4] = {
        Math::Lerp(WaveColor[0], WaveColor[0] * 0.38f, nightBlend),
        Math::Lerp(WaveColor[1], WaveColor[1] * 0.42f, nightBlend),
        Math::Lerp(WaveColor[2], WaveColor[2] * 0.55f, nightBlend),
        1.0f
    };
    const float foamColor[4] = {
        FoamColor[0], FoamColor[1], FoamColor[2], 1.0f - nightBlend * 0.35f
    };
    const float cloudColor[4] = {CloudColor[0], CloudColor[1], CloudColor[2], dayBlend};
    const float sunColor[4] = {SunColor[0], SunColor[1], SunColor[2], dayBlend};
    const float seaDrop = BossSeaDrop(shooter);

    // 朝に合わせて太陽を昇らせ、Cubeの雲を空の上部へ流す
    if (dayBlend > 0.01f) {
        const float sunX = backgroundHalfWidth * 0.56f;
        const float sunY = backgroundHalfHeight * (0.26f + dayBlend * 0.30f);
        constexpr float SkyZ = SidePlaneZ + 11.6f;
        DrawModelPrimitive(renderer, camera, 1, sunX, sunY, SkyZ,
            1.05f, 1.05f, 0.14f, sunColor);
        DrawModelPrimitive(renderer, camera, 1, sunX - 0.62f, sunY, SkyZ,
            0.28f, 0.28f, 0.15f, sunColor);
        DrawModelPrimitive(renderer, camera, 1, sunX + 0.62f, sunY, SkyZ,
            0.28f, 0.28f, 0.15f, sunColor);
        for (int i = 0; i < 7; ++i) {
            const float x = WrapNdcX(i * 0.53f - shooter.m_scroll * 0.06f) * backgroundHalfWidth;
            const float y = backgroundHalfHeight *
                (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
            DrawModelPrimitive(renderer, camera, 1, x, y, SkyZ,
                1.15f, 0.18f, 0.16f, cloudColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.62f, y - 0.07f, SkyZ,
                0.62f, 0.13f, 0.16f, cloudColor);
            DrawModelPrimitive(renderer, camera, 1, x + 0.68f, y - 0.05f, SkyZ,
                0.54f, 0.12f, 0.16f, cloudColor);
        }
    }

    // 海面の上端Y=-6を保ち、ドット絵調の波と泡を横スクロールさせる
    DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f - seaDrop, SidePlaneZ + 14.0f,
        60.0f, 10.0f, 0.3f, waterColor);
    for (int i = 0; i < 24; ++i) {
        const float x = WrapNdcX(i * 0.29f -
            shooter.m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
        const float y = -6.25f - seaDrop -
            static_cast<float>((i * 37) % 42) / 10.0f;
        const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
        constexpr float Z = SidePlaneZ + 13.6f;
        DrawModelPrimitive(renderer, camera, 1, x, y, Z,
            width, 0.10f, 0.18f, waveColor);
        if (i % 3 == 0) {
            DrawModelPrimitive(renderer, camera, 1,
                x - width * 0.18f, y + 0.16f, Z - 0.02f,
                width * 0.42f, 0.07f, 0.19f, foamColor);
        }
    }
    if (!shooter.m_bossBattle) DrawSeaSerpent(shooter, renderer, camera, 0.0f);
}

void SideScrollingShooter::Stage3Module::DrawBackground3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth = sideBackgroundHalfHeight * renderer.AspectRatio();
    const float nightBlend = NightBlend(shooter.m_frame);
    const float dayBlend = 1.0f - nightBlend;
    const float waterColor[4] = {
        Math::Lerp(WaterColor[0], WaterColor[0] * 0.30f, nightBlend),
        Math::Lerp(WaterColor[1], WaterColor[1] * 0.36f, nightBlend),
        Math::Lerp(WaterColor[2], WaterColor[2] * 0.48f, nightBlend),
        1.0f
    };
    const float waveColor[4] = {
        Math::Lerp(WaveColor[0], WaveColor[0] * 0.38f, nightBlend),
        Math::Lerp(WaveColor[1], WaveColor[1] * 0.42f, nightBlend),
        Math::Lerp(WaveColor[2], WaveColor[2] * 0.55f, nightBlend),
        1.0f
    };
    const float foamColor[4] = {
        FoamColor[0], FoamColor[1], FoamColor[2], 1.0f - nightBlend * 0.35f
    };
    const float cloudColor[4] = {CloudColor[0], CloudColor[1], CloudColor[2], dayBlend};
    const float sunColor[4] = {SunColor[0], SunColor[1], SunColor[2], dayBlend};
    const float seaDrop = BossSeaDrop(shooter);

    // 太陽と雲は横視点の配置からレール空間へ補間する
    if (dayBlend > 0.01f) {
        const float sideSunX = sideBackgroundHalfWidth * 0.56f;
        const float sideSunY = sideBackgroundHalfHeight * (0.26f + dayBlend * 0.30f);
        const float sunX = Math::Lerp(sideSunX, 32.0f, railWeight);
        const float sunY = Math::Lerp(sideSunY, 14.0f, railWeight);
        const float sunZ = Math::Lerp(SidePlaneZ + 11.6f, 72.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 1, sunX, sunY, sunZ,
            Math::Lerp(1.05f, 2.8f, railWeight),
            Math::Lerp(1.05f, 2.8f, railWeight),
            Math::Lerp(0.14f, 0.35f, railWeight), sunColor);
        DrawModelPrimitive(renderer, camera, 1,
            sunX - Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
        DrawModelPrimitive(renderer, camera, 1,
            sunX + Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.28f, 0.70f, railWeight),
            Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
        for (int i = 0; i < 7; ++i) {
            const float sideX = WrapNdcX(i * 0.53f - shooter.m_scroll * 0.06f) *
                sideBackgroundHalfWidth;
            const float sideY = sideBackgroundHalfHeight *
                (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
            const float x = Math::Lerp(sideX,
                -42.0f + static_cast<float>((i * 73) % 840) / 10.0f, railWeight);
            const float y = Math::Lerp(sideY,
                8.0f + static_cast<float>((i * 7) % 10), railWeight);
            const float z = Math::Lerp(SidePlaneZ + 11.6f,
                32.0f + static_cast<float>((i * 37) % 70), railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z,
                Math::Lerp(1.15f, 2.2f, railWeight),
                Math::Lerp(0.18f, 0.30f, railWeight),
                Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x - Math::Lerp(0.62f, 1.2f, railWeight),
                y - Math::Lerp(0.07f, 0.12f, railWeight), z,
                Math::Lerp(0.62f, 1.1f, railWeight),
                Math::Lerp(0.13f, 0.22f, railWeight),
                Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
            DrawModelPrimitive(renderer, camera, 1,
                x + Math::Lerp(0.68f, 1.3f, railWeight),
                y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                Math::Lerp(0.54f, 1.0f, railWeight),
                Math::Lerp(0.12f, 0.20f, railWeight),
                Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
        }
    }

    // 横視点の海面と同じ波配置をレール空間へ補間する
    DrawModelPrimitive(renderer, camera, 1, 0.0f,
        Math::Lerp(-11.0f, -4.0f, railWeight) - seaDrop,
        Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
        Math::Lerp(60.0f, 140.0f, railWeight),
        Math::Lerp(10.0f, 0.7f, railWeight),
        Math::Lerp(0.3f, 140.0f, railWeight), waterColor);
    for (int i = 0; i < 24; ++i) {
        const float sideX = WrapNdcX(i * 0.29f -
            shooter.m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
        const float sideY = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
        const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
        const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
        const float railZ = 8.0f + std::fmod(
            static_cast<float>(i * 43) - shooter.m_scroll * 28.0f + 110.0f, 110.0f);
        const float x = Math::Lerp(sideX, railX, railWeight);
        const float y = Math::Lerp(sideY, -3.65f + 0.045f * 0.5f, railWeight) - seaDrop;
        const float z = Math::Lerp(SidePlaneZ + 13.6f, railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 1, x, y, z,
            Math::Lerp(width, 1.5f, railWeight),
            Math::Lerp(0.10f, 0.045f, railWeight),
            Math::Lerp(0.18f, 0.70f, railWeight), waveColor);
        if (i % 3 == 0) {
            const float foamY = Math::Lerp(
                sideY + 0.16f, -3.65f + 0.045f + 0.03f * 0.5f, railWeight) - seaDrop;
            DrawModelPrimitive(renderer, camera, 1,
                x - Math::Lerp(width * 0.18f, 0.25f, railWeight),
                foamY, z - Math::Lerp(0.02f, 0.08f, railWeight),
                Math::Lerp(width * 0.42f, 0.65f, railWeight),
                Math::Lerp(0.07f, 0.03f, railWeight),
                Math::Lerp(0.19f, 0.72f, railWeight), foamColor);
        }
    }
    if (!shooter.m_bossBattle) DrawSeaSerpent(shooter, renderer, camera, railWeight);
}

bool SideScrollingShooter::Stage3Module::HitsHazard(
    const SideScrollingShooter& shooter, float x, float y, float z, float radius) {
    if (shooter.m_bossBattle) return false;
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(shooter.m_frame, motion)) {
        return false;
    }

    // 描画と同じ胴体節配置を使い、横視点とレール視点の双方で判定する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const SeaSerpentSegment segment = GetSeaSerpentSegment(motion, i);
        if (shooter.IsRailGameplayActive()) {
            const float railHeight = 2.50f * segment.scale;
            const float visibleHeight = Math::Clamp01(segment.elevation / railHeight) * railHeight;
            if (visibleHeight <= 0.0f) {
                continue;
            }
            const float visibleScale = segment.scale * std::sqrt(visibleHeight / railHeight);
            const float railY = -3.65f + (segment.elevation < railHeight ?
                visibleHeight * 0.5f : segment.elevation - railHeight * 0.5f);
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                segment.railX, railY, segment.railZ, 1.25f * visibleScale)) {
                return true;
            }
        } else {
            const float sideHeight = 1.35f * segment.scale;
            const float visibleHeight = Math::Clamp01(segment.elevation / sideHeight) * sideHeight;
            if (visibleHeight <= 0.0f) {
                continue;
            }
            const float visibleWidth = 1.18f * segment.scale *
                std::sqrt(visibleHeight / sideHeight);
            const float hitWidth = visibleWidth * 0.5f + radius * WorldXScale;
            const float hitHeight = visibleHeight * 0.5f + radius * WorldYScale;
            const float dx = (ToWorldX(x) - segment.sideX) / hitWidth;
            const float sideY = -6.0f + (segment.elevation < sideHeight ?
                visibleHeight * 0.5f : segment.elevation - sideHeight * 0.5f);
            const float dy = (ToWorldY(y) - sideY) / hitHeight;
            if (dx * dx + dy * dy <= 1.0f) {
                return true;
            }
        }
    }
    return false;
}

void SideScrollingShooter::Stage3Module::DrawSeaSerpent(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(shooter.m_frame, motion)) {
        return;
    }

    // 通常跳躍、低空横断、超巨大ジャンプを判定と同じ胴体節配置で描画する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const SeaSerpentSegment segment = GetSeaSerpentSegment(motion, i);
        const float sideWidth = 1.18f * segment.scale;
        const float sideHeight = 1.35f * segment.scale;
        const float sideDepth = 0.72f * segment.scale;
        const float railSize = 2.50f * segment.scale;
        const float sideVisibleHeight = Math::Clamp01(segment.elevation / sideHeight) * sideHeight;
        const float railVisibleHeight = Math::Clamp01(segment.elevation / railSize) * railSize;
        if (sideVisibleHeight <= 0.0f && railVisibleHeight <= 0.0f) {
            continue;
        }
        const float sideVisibleScale = std::sqrt(sideVisibleHeight / sideHeight);
        const float railVisibleScale = std::sqrt(railVisibleHeight / railSize);
        const float x = Math::Lerp(segment.sideX, segment.railX, railWeight);
        const float sideY = -6.0f + (segment.elevation < sideHeight ?
            sideVisibleHeight * 0.5f : segment.elevation - sideHeight * 0.5f);
        const float railY = -3.65f + (segment.elevation < railSize ?
            railVisibleHeight * 0.5f : segment.elevation - railSize * 0.5f);
        const float y = Math::Lerp(sideY, railY, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.1f, segment.railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(sideWidth * sideVisibleScale,
                railSize * railVisibleScale, railWeight),
            Math::Lerp(sideVisibleHeight, railVisibleHeight, railWeight),
            Math::Lerp(sideDepth * sideVisibleScale,
                railSize * railVisibleScale, railWeight), SeaSerpentColor);
        if (i % 2 == 1 &&
            (sideVisibleHeight >= sideHeight || railVisibleHeight >= railSize)) {
            DrawModelPrimitive(renderer, camera, 5,
                x, y - Math::Lerp(0.48f, 0.82f, railWeight), z - 0.05f,
                Math::Lerp(0.52f, 1.15f, railWeight),
                Math::Lerp(0.28f, 0.48f, railWeight),
                Math::Lerp(0.18f, 0.42f, railWeight), SeaSerpentBellyColor);
        }
    }

    const SeaSerpentSegment head = GetSeaSerpentSegment(motion, 0);
    const float headSideVisibleHeight = Math::Clamp01(
        head.elevation / (1.35f * head.scale)) * 1.35f * head.scale;
    const float headRailVisibleHeight = Math::Clamp01(
        head.elevation / (2.50f * head.scale)) * 2.50f * head.scale;
    const float headX = Math::Lerp(head.sideX, head.railX, railWeight);
    const float headSideY = -6.0f + (head.elevation < 1.35f * head.scale ?
        headSideVisibleHeight * 0.5f : head.elevation - 1.35f * head.scale * 0.5f);
    const float headRailY = -3.65f + (head.elevation < 2.50f * head.scale ?
        headRailVisibleHeight * 0.5f : head.elevation - 2.50f * head.scale * 0.5f);
    const float headY = Math::Lerp(headSideY, headRailY, railWeight);
    const float headZ = Math::Lerp(SidePlaneZ + 13.1f, head.railZ, railWeight);

    // 頭が海面を出入りする短い時間だけ、水滴を初速と重力による放物線で飛ばす
    const float emergeProgress = std::asin((std::min)(
        1.0f, 1.35f * head.scale / motion.elevation)) / Math::HalfPi;
    const float reentryProgress = 1.0f - emergeProgress;
    const float splashCenter = head.progress < 0.5f ? emergeProgress : reentryProgress;
    const float splashTime = Math::Clamp01((head.progress -
        (splashCenter - emergeProgress)) / (emergeProgress * 2.0f));
    const bool splashActive = std::abs(head.progress - splashCenter) <= emergeProgress;
    if (splashActive) {
        for (int i = 0; i < 17; ++i) {
            const float spread = static_cast<float>(i - 8) *
                0.22f * motion.scale * splashTime;
            const float launchVelocity = (1.05f +
                static_cast<float>((i * 5) % 4) * 0.22f) * motion.scale;
            const float gravity = launchVelocity * 2.0f;
            const float dropletHeight = launchVelocity * splashTime -
                gravity * splashTime * splashTime * 0.5f;
            const float sideSplashX = head.sideX - motion.direction * spread;
            const float railSplashX = head.railX - motion.direction * spread;
            DrawModelPrimitive(renderer, camera, 5,
                Math::Lerp(sideSplashX, railSplashX, railWeight),
                Math::Lerp(-5.75f + dropletHeight, -3.45f + dropletHeight, railWeight),
                Math::Lerp(SidePlaneZ + 13.0f,
                    head.railZ - motion.railDirection * spread, railWeight),
                Math::Lerp(0.16f, 0.42f, railWeight) * motion.scale,
                Math::Lerp(0.28f, 0.65f, railWeight) * motion.scale,
                Math::Lerp(0.10f, 0.32f, railWeight) * motion.scale, FoamColor);
        }
    }
    if (headSideVisibleHeight >= 1.35f * head.scale ||
        headRailVisibleHeight >= 2.50f * head.scale) {
        // 目をカメラ側の頭部表面より前へ置き、移動中も胴体へ埋まらないようにする
        const Vector3 headPosition {headX, headY, headZ};
        const Vector3 eyePosition = headPosition +
            camera.Right() * (Math::Lerp(0.36f, 0.82f, railWeight) * motion.scale) +
            camera.Up() * (Math::Lerp(0.30f, 0.72f, railWeight) * motion.scale) +
            (camera.Position() - headPosition).Normalized() *
                (Math::Lerp(SeaSerpentSideEyeSurfaceOffset,
                    SeaSerpentRailEyeSurfaceOffset, railWeight) * motion.scale);
        DrawModelPrimitive(renderer, camera, 5,
            eyePosition.x, eyePosition.y, eyePosition.z,
            Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale,
            Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale,
            Math::Lerp(0.10f, 0.20f, railWeight) * motion.scale,
            SeaSerpentEyeColor);
    }
}
