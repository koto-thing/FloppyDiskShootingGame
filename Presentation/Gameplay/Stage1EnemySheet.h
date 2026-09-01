#pragma once

#include <cmath>

/**
 * @brief ステージ1の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage1EnemySheet : public SideScrollingShooter::Stage {
public:
    /** @brief ステージ1ボス機体で個別に破壊できる部位 */
    enum BossPart {
        BossNose,
        BossLeftWing,
        BossRightWing,
        BossLeftEngine,
        BossRightEngine,
        BossPartCount
    };
    static_assert(BossPartCount == 5);

    /** @brief ステージ1ボス戦の攻撃フェーズ */
    enum BossPhase {
        BossNormalPhase1,
        BossNormalPhase2,
        BossPhaseCount
    };
    static_assert(BossPhaseCount == 2);

    static constexpr int BossPhaseHp[] = {480,240};

    int StageIndex() const override { return 1; }

    int BossPartTotal() const override { return BossPartCount; }
    int BossPhaseTotal() const override { return BossPhaseCount; }
    int BossInitialPhase() const override { return BossNormalPhase1; }
    int BossNosePart() const override { return BossNose; }
    int BossLeftWingPart() const override { return BossLeftWing; }
    int BossRightWingPart() const override { return BossRightWing; }
    int BossLeftEnginePart() const override { return BossLeftEngine; }
    int BossRightEnginePart() const override { return BossRightEngine; }
    const char* BossPhaseLabel(int phase) const override {
        constexpr const char* PhaseLabels[] = {
            "NORMAL 1", "NORMAL 2"
        };
        return PhaseLabels[phase % BossPhaseCount];
    }

    /**
     * @brief ステージ1ボスの最大HPを取得する
     * @return ステージ1ボスの最大HP
     */
    int BossMaxHp() const override {
        return BossPhaseHp[0];
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
        if ((boss.bossPhase == BossNormalPhase2) &&
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
     * @param boss 判定するボス
     * @return 突進攻撃中の場合true
     */
    bool IsBossSpecialAttackActive(const Enemy& boss) const override {
        return (boss.bossPhase == BossNormalPhase2) && boss.phase > 0.0f;
    }

    /**
     * @brief ステージ1ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(int phase) const override {
        return phase == BossNormalPhase1 || phase == BossNormalPhase2 ? 120 : 84;
    }

    /**
     * @brief ステージ1ボスの本体HPから攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ステージ1ボスの攻撃フェーズ
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        (void)maxHp;
        if (hp > BossPhaseHp[1]) return BossNormalPhase1;
        return BossNormalPhase2;
    }

    /**
     * @brief ステージ1ボス部位破壊時に本体へ与えるダメージを取得する
     * @param part 破壊されたボス部位
     * @return 本体へ与えるダメージ
     */
    int BossPartBreakDamage(int part) const override {
        (void)part;
        return 60;
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

    /**
     * @brief ステージ1ボスの指定部位・フェーズの弾数を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossPartBulletCount(int part, int phase, bool railMode) const override {
        if (part == BossNose) return BossBulletCount(railMode);
        if (part == BossLeftWing || part == BossRightWing) {
            return phase == BossNormalPhase2 ? 3 : 2;
        }
        return phase == BossNormalPhase2 ? 2 : 1;
    }

    /**
     * @brief ステージ1ボスの指定部位・フェーズの弾を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossPartBullet(int part, int phase, int index, bool railMode) const override {
        const int baseCount = BossBulletCount(railMode);
        const int patternIndex = part == BossNose ? index :
            (part == BossLeftWing ? 0 : (part == BossRightWing ? baseCount - 1 : baseCount / 2));
        BossBullet bullet = GetBossBullet(patternIndex, railMode);
        if (phase == BossNormalPhase2) {
            bullet.vx *= 1.35f;
            bullet.vy += (index - BossPartBulletCount(part, phase, railMode) / 2) * 0.010f;
        }
        return bullet;
    }
};
