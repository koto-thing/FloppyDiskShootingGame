#pragma once

#include "Stage1EnemySheet.h"

/**
 * @brief ステージ2の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage2EnemySheet : public SideScrollingShooter::Stage1EnemySheet {
public:
    int StageIndex() const override {
        return 2;
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
};
