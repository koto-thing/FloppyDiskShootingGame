#pragma once

#include <cmath>

#include "../Common/StageDefinition.h"

/**
 * @brief ステージ1の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage1EnemySheet : public SideScrollingShooter::Stage {
public:
    static constexpr int BossPhaseHp[] = {800, 600, 400, 200};

    int StageIndex() const override { return 1; }

    /**
     * @brief ステージ1ボスの最大HPを取得する
     * @return ステージ1ボスの最大HP
     */
    int BossMaxHp() const override {
        return BossPhaseHp[0];
    }

    /** @brief 部位破壊ダメージを取得する @param part 破壊された部位 @return ダメージ120 */
    int BossPartBreakDamage(BossPart part) const override {
        (void)part;
        return 70;
    }

    /**
     * @brief ステージ1ボスの生成位置を設定する
     * @param boss 生成するボス
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
        boss.hp = BossMaxHp();
        boss.maxHp = BossMaxHp();
        boss.x = 2.6f;
        boss.y = 0.0f;
        boss.z = railMode ? 48.0f : ToRailZFromSideX(boss.x);
        boss.baseX = 0.0f;
        boss.baseY = 0.0f;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.phase = 0.0f;
        boss.motionAge = 0;
    }

    /**
     * @brief ステージ1ボスの3Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    void ConfigureBossRailAnchor(Enemy& boss) const override {
        boss.x = 0.0f;
        boss.y = 0.0f;
        boss.z = 48.0f;
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.phase = 0.0f;
        boss.motionAge = 0;
    }

    /**
     * @brief ステージ1ボスの2Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    void ConfigureBossSideAnchor(Enemy& boss) const override {
        boss.x = 1.80f;
        boss.y = 0.0f;
        boss.z = ToRailZFromSideX(boss.x);
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.phase = 0.0f;
        boss.motionAge = 0;
    }

    /**
     * @brief ステージ1ボスの部位HPを設定する
     * @param boss 部位HPを設定するボス
     * @return なし
     */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = { 120, 180, 180, 150, 150 };
    }

    /**
     * @brief ステージ1ボスの移動を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        constexpr int RushIntervalFrames = 400;
        constexpr int RushWindupFrames = 42;
        constexpr int RushChargeFrames = 56;
        constexpr int RushWaitFrames = 42;
        constexpr int RushReturnFrames = 72;
        constexpr int RushTotalFrames = RushWindupFrames + RushChargeFrames + RushWaitFrames + RushReturnFrames;
        constexpr float SideBackDistance = 0.45f;
        constexpr float SideOffscreenX = -3.85f;
        constexpr float SideEntryX = 3.00f;
        constexpr float RailEntryX = 8.20f;
        constexpr float RailBackDistance = 8.0f;
        constexpr float RailOffscreenZ = -30.0f;

        // フェーズ3では一定間隔で後退、突進、待機、復帰を行う
        if ((boss.bossPhase == BossNormalPhase2 || boss.bossPhase == BossSpecialPhase2) &&
            (boss.phase > 0.0f || boss.age % RushIntervalFrames == 0)) {
            int rushFrame = static_cast<int>(boss.phase);
            if (rushFrame <= 0) {
                boss.actionX = boss.x;
                boss.actionY = boss.y;
                boss.actionZ = boss.z;
                rushFrame = 1;
            }

            if (shooter.IsRailGameplayActive()) {
                boss.y = boss.actionY;
                if (rushFrame <= RushWindupFrames) {
                    const float t = static_cast<float>(rushFrame) / static_cast<float>(RushWindupFrames);
                    boss.x = boss.actionX;
                    boss.z = boss.actionZ + RailBackDistance * t;
                } else if (rushFrame <= RushWindupFrames + RushChargeFrames) {
                    const float t = static_cast<float>(rushFrame - RushWindupFrames) /
                        static_cast<float>(RushChargeFrames);
                    boss.x = boss.actionX;
                    boss.z = boss.actionZ + RailBackDistance + (RailOffscreenZ - boss.actionZ - RailBackDistance) * t * t;
                } else if (rushFrame <= RushWindupFrames + RushChargeFrames + RushWaitFrames) {
                    boss.x = boss.actionX;
                    boss.z = RailOffscreenZ;
                } else {
                    const float t = static_cast<float>(rushFrame - RushWindupFrames - RushChargeFrames - RushWaitFrames) /
                        static_cast<float>(RushReturnFrames);
                    boss.x = RailEntryX + (boss.actionX - RailEntryX) * t;
                    boss.z = boss.actionZ;
                }
            } else {
                boss.y = boss.actionY;
                if (rushFrame <= RushWindupFrames) {
                    const float t = static_cast<float>(rushFrame) / static_cast<float>(RushWindupFrames);
                    boss.x = boss.actionX + SideBackDistance * t;
                } else if (rushFrame <= RushWindupFrames + RushChargeFrames) {
                    const float t = static_cast<float>(rushFrame - RushWindupFrames) /
                        static_cast<float>(RushChargeFrames);
                    boss.x = boss.actionX + SideBackDistance +
                        (SideOffscreenX - boss.actionX - SideBackDistance) * t * t;
                } else if (rushFrame <= RushWindupFrames + RushChargeFrames + RushWaitFrames) {
                    boss.x = SideOffscreenX;
                } else {
                    const float t = static_cast<float>(rushFrame - RushWindupFrames - RushChargeFrames - RushWaitFrames) /
                        static_cast<float>(RushReturnFrames);
                    boss.x = SideEntryX + (boss.actionX - SideEntryX) * t;
                }
                boss.z = ToRailZFromSideX(boss.x);
            }

            boss.phase = rushFrame < RushTotalFrames ? static_cast<float>(rushFrame + 1) : 0.0f;
            return;
        }

        boss.phase = 0.0f;
        const float currentMotionAge = static_cast<float>(++boss.motionAge);

        if (shooter.IsRailGameplayActive()) {
            boss.x = boss.baseX + std::sin(currentMotionAge * 0.038f) * 1.40f;
            boss.y = boss.baseY + std::sin(currentMotionAge * 0.045f) * 0.56f;
            if (boss.z <= 0.0f) {
                boss.z = 48.0f;
            }
        } else {
            if (boss.x > 1.80f) {
                boss.x -= 0.008f;
            }
            boss.z = ToRailZFromSideX(boss.x);
            boss.y = boss.baseY + std::sin(currentMotionAge * 0.045f) * 0.68f;
        }
    }

    /**
     * @brief ステージ1ボスが突進攻撃中か取得する
     * @param shooter ゲーム本体
     * @param boss 判定するボス
     * @return 突進攻撃中の場合true、通常攻撃中の場合false
     */
    bool IsBossSpecialAttackActive(const SideScrollingShooter& shooter, const Enemy& boss) const override {
        (void)shooter;
        return (boss.bossPhase == BossNormalPhase2 || boss.bossPhase == BossSpecialPhase2) && boss.phase > 0.0f;
    }

    /**
     * @brief ステージ1ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(BossPhase phase) const override {
        return phase == BossNormalPhase1 || phase == BossNormalPhase2 ? 120 : 74;
    }

    /**
     * @brief ステージ1ボスの本体HPから攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ステージ1ボスの攻撃フェーズ
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        return SideScrollingShooter::BossPhaseForHp(hp, maxHp);
    }

    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 3;
    }

    /**
     * @brief ボス弾幕の指定弾を取得する
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        if (railMode) {
            constexpr BossBullet RailPattern[] = {
                {0.0f, 0.0f, 0.0f, -0.018f},
                {0.0f, 0.0f, 0.0f, 0.000f},
                {0.0f, 0.0f, 0.0f, 0.018f}
            };
            return RailPattern[index % 3];
        }

        constexpr BossBullet SidePattern[] = {
            {-0.12f, 0.0f, -0.020f, -0.010f},
            {-0.12f, 0.0f, -0.022f, 0.000f},
            {-0.12f, 0.0f, -0.020f, 0.010f}
        };
        return SidePattern[index % 3];
    }
};
