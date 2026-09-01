#pragma once

#include "Stage1EnemySheet.h"

/**
 * @brief ステージ2の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage2EnemySheet : public SideScrollingShooter::Stage1EnemySheet {
public:
    /** @brief ステージ2ボス機体で個別に破壊できる部位 */
    enum BossPart {
        BossNose,
        BossLeftWing,
        BossRightWing,
        BossLeftEngine,
        BossRightEngine,
        BossPartCount
    };
    static_assert(BossPartCount == 5);

    /** @brief ステージ2ボス戦の攻撃フェーズ */
    enum BossPhase {
        BossNormalPhase1,
        BossSpecialPhase1,
        BossNormalPhase2,
        BossSpecialPhase2,
        BossPhaseCount
    };
    static_assert(BossPhaseCount == 4);

    static constexpr int BossPhaseHp[] = {480, 360, 240, 120};

    int StageIndex() const override {
        return 2;
    }

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
            "NORMAL 1", "SPECIAL 1", "NORMAL 2", "SPECIAL 2"
        };
        return PhaseLabels[phase % BossPhaseCount];
    }

    /**
     * @brief ステージ2ボスの最大HPを取得する
     * @return ステージ2ボスの最大HP
     */
    int BossMaxHp() const override {
        return BossPhaseHp[0];
    }

    /**
     * @brief ステージ1と同じ敵性能でステージ2の敵を設定する
     * @param shooter ゲーム本体
     * @param enemy 設定する敵
     * @param enemyType 敵構成種別
     * @param frame 現在のステージフレーム
     * @param kills 現在の撃破数
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureEnemy(SideScrollingShooter& shooter, Enemy& enemy,
        int enemyType, int frame, int kills, bool railMode) const override {
        EnemyBehaviorForType(enemyType).ConfigureSpawn(shooter, enemy, frame, kills, railMode, 1);
    }

    /**
     * @brief ステージ1と同じボス性能でステージ2のボスを設定する
     * @param boss 生成するボス
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, 1);
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
     * @brief ステージ2ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(int phase) const override {
        return phase == BossNormalPhase1 || phase == BossNormalPhase2 ? 120 : 84;
    }

    /**
     * @brief ステージ2ボスの本体HPから攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ステージ2ボスの攻撃フェーズ
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        (void)maxHp;
        if (hp > BossPhaseHp[1]) return BossNormalPhase1;
        if (hp > BossPhaseHp[2]) return BossSpecialPhase1;
        if (hp > BossPhaseHp[3]) return BossNormalPhase2;
        return BossSpecialPhase2;
    }

    /**
     * @brief ステージ2ボス部位破壊時に本体へ与えるダメージを取得する
     * @param part 破壊されたボス部位
     * @return 本体へ与えるダメージ
     */
    int BossPartBreakDamage(int part) const override {
        (void)part;
        return 120;
    }

    /**
     * @brief ステージ2ボスの指定部位・フェーズの弾数を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossPartBulletCount(int part, int phase, bool railMode) const override {
        if (part == BossNose) return BossBulletCount(railMode);
        if (part == BossLeftWing || part == BossRightWing) {
            return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 3 : 2;
        }
        return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 2 : 1;
    }

    /**
     * @brief ステージ2ボスの指定部位・フェーズの弾を取得する
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
        if (phase == BossSpecialPhase1 || phase == BossSpecialPhase2) {
            bullet.vx *= 1.35f;
            bullet.vy += (index - BossPartBulletCount(part, phase, railMode) / 2) * 0.010f;
        }
        return bullet;
    }
};
