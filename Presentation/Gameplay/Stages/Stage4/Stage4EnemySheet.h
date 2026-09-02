#pragma once

#include "../Common/StageDefinition.h"

/**
 * @brief ステージ4の敵出現とボス弾幕を定義する
 */
class SideScrollingShooter::Stage4EnemySheet final : public SideScrollingShooter::Stage {
public:
    /**
     * @brief ステージ番号を取得する
     * @return Stage 4を表す番号
     */
    int StageIndex() const override {
        return 4;
    }

    /**
     * @brief Stage 4の経過フレームから通常敵の出現を選択する
     * @param frame Stage 4開始からの経過フレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 選択した出現規則の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させるフレームの場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {
            {4, 20, 48, 1.10f, -0.82f, -0.68f, 60.0f},
            {5, 90, 210, 1.14f, -0.28f, 0.86f, 50.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {4, 10, 46, 1.12f, 0.28f, 0.18f, 60.0f},
            {3, 60, 125, 1.10f, 0.82f, -0.88f, 34.0f},
            {1, 130, 190, 1.16f, -0.40f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {5, 10, 110, 1.14f, -0.82f, 0.86f, 50.0f},
            {4, 50, 105, 1.12f, 0.82f, -0.18f, 60.0f},
            {3, 120, 135, 1.10f, 0.28f, -0.88f, 40.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }

    /**
     * @brief Stage 4ボスをStage 2と同じ基準位置へ配置する
     * @param boss 生成するボス
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
        boss.hp = BossMaxHp();
        boss.maxHp = BossMaxHp();
        if (railMode) ConfigureBossRailAnchor(boss);
        else ConfigureBossSideAnchor(boss);
    }

    /** @brief Stage 4ボスの3D基準点を設定する @param boss 設定するボス @return なし */
    void ConfigureBossRailAnchor(Enemy& boss) const override {
        boss.x = 0.0f;
        boss.y = -0.5f;
        boss.z = 48.0f;
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.turretAimX = boss.x;
        boss.turretAimY = boss.y;
        boss.turretAimZ = boss.z;
        boss.phase = 0.0f;
        boss.motionAge = 0;
    }

    /** @brief Stage 4ボスの2D基準点を設定する @param boss 設定するボス @return なし */
    void ConfigureBossSideAnchor(Enemy& boss) const override {
        boss.x = 1.80f;
        boss.y = -0.5f;
        boss.z = ToRailZFromSideX(boss.x);
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.turretAimX = boss.x;
        boss.turretAimY = boss.y;
        boss.turretAimZ = boss.z;
        boss.phase = 0.0f;
        boss.motionAge = 0;
    }

    /** @brief Stage 4ボスの砲部位HPを設定する @param boss 設定するボス @return なし */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = {260, 0, 0, 0, 0,
            90, 90, 90, 90, 90, 90,
            0, 0, 0, 0, 0, 0};
    }

    /**
     * @brief 部位破壊ダメージを取得する
     * @param part 破壊された部位
     * @return 本体へ与えるダメージ
     */
    int BossPartBreakDamage(BossPart part) const override {
        return part == BossNose ? 140 : 70;
    }

    /**
     * @brief Stage 4ボスを基準位置へ固定する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        constexpr float TurretTrackingRate = 0.06f;
        boss.phase = 0.0f;
        boss.motionAge = 0;
        boss.x = boss.baseX;
        boss.y = boss.baseY;
        boss.z = shooter.IsRailGameplayActive() ? boss.baseZ : ToRailZFromSideX(boss.x);
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.turretAimX += (shooter.m_playerX - boss.turretAimX) * TurretTrackingRate;
        boss.turretAimY += (shooter.m_playerY - boss.turretAimY) * TurretTrackingRate;
        const float targetZ = shooter.IsRailGameplayActive() ?
            PlayerRailZ : ToRailZFromSideX(shooter.m_playerX);
        boss.turretAimZ += (targetZ - boss.turretAimZ) * TurretTrackingRate;
    }

    /**
     * @brief Stage 4ボスの一斉射撃数を取得する
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 7;
    }

    /**
     * @brief 部位ごとの弾数を取得する
     * @param part 発射する部位
     * @param phase 攻撃フェーズ
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossPartBulletCount(BossPart part, BossPhase phase, bool railMode) const override {
        (void)railMode;
        if (part == BossNose) {
            return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 5 : 3;
        }
        if (part >= BossFunnelHatch0 && part < BossFunnelHatch0 + 6) {
            return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 2 : 1;
        }
        return 0;
    }

    /**
     * @brief Stage 4ボスの指定番号の弾を取得する
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {
            {-0.12f, 0.0f, -0.022f, -0.027f},
            {-0.12f, 0.0f, -0.025f, -0.018f},
            {-0.12f, 0.0f, -0.027f, -0.009f},
            {-0.12f, 0.0f, -0.029f, 0.0f},
            {-0.12f, 0.0f, -0.027f, 0.009f},
            {-0.12f, 0.0f, -0.025f, 0.018f},
            {-0.12f, 0.0f, -0.022f, 0.027f}
        };
        constexpr BossBullet RailPattern[] = {
            {0.0f, 0.0f, -0.018f, -0.028f},
            {0.0f, 0.0f, -0.012f, -0.018f},
            {0.0f, 0.0f, -0.006f, -0.009f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.006f, 0.009f},
            {0.0f, 0.0f, 0.012f, 0.018f},
            {0.0f, 0.0f, 0.018f, 0.028f}
        };
        return (railMode ? RailPattern : SidePattern)[index % 7];
    }
};
