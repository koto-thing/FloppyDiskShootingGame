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
     * @brief 部位破壊段階を最後まで成立させるStage2ボス最大HPを取得する
     * @return Stage2ボス最大HP
     */
    int BossMaxHp() const override {
        return 1200;
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
     * @brief Stage2専用Behaviorでボスを設定する
     * @param boss 生成するボス
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        Stage2BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
        boss.hp = BossMaxHp();
        boss.maxHp = BossMaxHp();
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
    }

    /**
     * @brief Stage2専用Behaviorで3フェーズ挙動を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        Stage2BossEnemyBehaviorInstance().Tick(shooter, boss);
    }

    /**
     * @brief 潜航や主砲チャージ中に通常弾幕を停止するか取得する
     * @param boss 判定するStage2ボス
     * @return 特殊行動中の場合true
     */
    bool IsBossSpecialAttackActive(const Enemy& boss) const override {
        return boss.stage2BossAction != Stage2BossAction::Idle &&
            boss.stage2BossAction != Stage2BossAction::MainGunCooldown;
    }

    /**
     * @brief 主砲、副砲三基、接続コア、側面ファンネルハッチ十二基の耐久値を設定する
     * @param boss 部位HPを設定するStage2ボス
     * @return なし
     */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = {300, 160, 160, 160, 240,
            30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
    }

    /**
     * @brief Stage2ボスの部位ごとの弾数を取得する
     * @param part 発射する部位
     * @param phase 現在の攻撃フェーズ
     * @param railMode レール表示中か
     * @return 発射する弾数、接続コアは常に0
     */
    int BossPartBulletCount(BossPart part, BossPhase phase, bool railMode) const override {
        if (part >= BossRightEngine) return 0;
        if (part == BossNose) return BossBulletCount(railMode);
        return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 2 : 1;
    }

    /**
     * @brief Stage2ボスの最大HPに対する割合から攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return 現在の攻撃フェーズ
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        return SideScrollingShooter::BossPhaseForHp(hp, maxHp);
    }
};
