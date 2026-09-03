#pragma once

#include "../Common/StageDefinition.h"
#include "Stage2Module.h"

/**
 * @brief ステージ2の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage2EnemySheet : public SideScrollingShooter::Stage {
public:
    /** @brief ステージ番号を取得する @return ステージ番号2 */
    int StageIndex() const override { return 2; }

    /** @brief ボス戦開始距離を取得する @return ボス戦開始距離 */
    float BossStartDistance() const override {
        return SideScrollingShooter::BossStartDistance;
    }

    /** @brief Stage 2ボス最大HPを取得する @return Stage 2ボス最大HP */
    int BossMaxHp() const override { return 3600; }

    /** @brief 飛散部品の重力設定を取得する @return 重力を適用しないためfalse */
    bool HasDebrisGravity() const override { return false; }

    /**
     * @brief 指定チャプターの終了フレームを取得する
     * @param chapterNumber チャプター番号
     * @return 指定チャプターの終了フレーム
     */
    int ChapterEndFrame(int chapterNumber) const override {
        return chapterNumber * ChapterFrameLength();
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
        EnemyBehaviorForType(enemyType).ConfigureSpawn(
            shooter, enemy, frame, kills, railMode, 1);
    }

    /**
     * @brief Stage 2専用モジュールでボスを設定する
     * @param boss 生成するボス
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        Stage2Module::ConfigureBossSpawn(boss, railMode, StageIndex());
        boss.hp = BossMaxHp();
        boss.maxHp = BossMaxHp();
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
    }

    /** @brief Stage 2ボスの3D基準点を設定する @param boss 設定するボス @return なし */
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

    /** @brief Stage 2ボスの2D基準点を設定する @param boss 設定するボス @return なし */
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

    /** @brief Stage 2ボス部位HPを設定する @param boss 設定するボス @return なし */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = {600, 100, 100, 100, 300,
            30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
    }

    /** @brief 部位破壊ダメージを取得する @param part 破壊された部位 @return ダメージ120 */
    int BossPartBreakDamage(BossPart part) const override {
        (void)part;
        return 60;
    }

    /**
     * @brief Stage 2専用モジュールで3フェーズ挙動を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        Stage2Module::TickBoss(shooter, boss);
    }

    /**
     * @brief 潜航や主砲チャージ中に通常弾幕を停止するか取得する
     * @param shooter ゲーム本体
     * @param boss 判定するStage 2ボス
     * @return 特殊行動中の場合true、通常行動中の場合false
     */
    bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss) const override {
        return Stage2Module::IsBossSpecialAttackActive(shooter, boss);
    }

    /** @brief Stage 2ボス弾幕の発射間隔を取得する @param phase 攻撃フェーズ @return 発射間隔 */
    int BossAttackInterval(BossPhase phase) const override {
        return phase == BossNormalPhase1 || phase == BossNormalPhase2 ? 120 : 84;
    }

    /** @brief HP割合から攻撃フェーズを取得する @param hp 現在HP @param maxHp 最大HP @return 攻撃フェーズ */
    int BossPhaseForHp(int hp, int maxHp) const override {
        return SideScrollingShooter::BossPhaseForHp(hp, maxHp);
    }

    /** @brief 一回の弾幕に含む弾数を取得する @param railMode レール表示中か @return 弾数3 */
    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 3;
    }

    /** @brief 指定弾を取得する @param index 弾番号 @param railMode レール表示中か @return 弾設定 */
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
     * @brief 部位ごとの弾数を取得する
     * @param part 発射する部位
     * @param phase 攻撃フェーズ
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossPartBulletCount(BossPart part, BossPhase phase, bool railMode) const override {
        if (part >= BossRightEngine) return 0;
        if (part == BossNose) return BossBulletCount(railMode);
        return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 2 : 1;
    }

    /**
     * @brief 部位弾幕の指定弾を取得する
     * @param part 発射する部位
     * @param phase 攻撃フェーズ
     * @param index 弾番号
     * @param railMode レール表示中か
     * @return 弾設定
     */
    BossBullet GetBossPartBullet(
        BossPart part, BossPhase phase, int index, bool railMode) const override {
        const int baseCount = BossBulletCount(railMode);
        const int patternIndex = part == BossNose ? index :
            (part == BossLeftWing ? 0 :
                (part == BossRightWing ? baseCount - 1 : baseCount / 2));
        BossBullet bullet = GetBossBullet(patternIndex, railMode);
        if (phase == BossSpecialPhase1 || phase == BossSpecialPhase2) {
            bullet.vx *= 1.35f;
            bullet.vy +=
                (index - BossPartBulletCount(part, phase, railMode) / 2) * 0.010f;
        }
        return bullet;
    }
};
