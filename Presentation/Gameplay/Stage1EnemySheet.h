#pragma once

#include <cmath>

/**
 * @brief ステージ1の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage1EnemySheet : public SideScrollingShooter::Stage {
public:
    static constexpr int BossPhaseHp[] = {480, 360, 240, 120};

    int StageIndex() const override { return 1; }

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
        boss.phase = 0.0f;
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
        if (shooter.IsRailGameplayActive()) {
            boss.x = std::sin(boss.age * 0.018f) * 0.34f;
            boss.y = std::sin(boss.age * 0.025f) * 0.36f;
            if (boss.z <= 0.0f) {
                boss.z = 48.0f;
            }
        } else {
            if (boss.x > 1.80f) {
                boss.x -= 0.008f;
            }
            boss.z = ToRailZFromSideX(boss.x);
            boss.y = std::sin(boss.age * 0.025f) * 0.48f;
        }
    }

    /**
     * @brief ステージ1ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(BossPhase phase) const override {
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
        if (hp > BossPhaseHp[2]) return BossSpecialPhase1;
        if (hp > BossPhaseHp[3]) return BossNormalPhase2;
        return BossSpecialPhase2;
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
