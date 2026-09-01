#pragma once

#include <algorithm>

/**
 * @brief ステージごとの敵出現と弾幕を定義する基底クラス
 */
class SideScrollingShooter::Stage {
public:
    /** @brief 敵の挙動・モデル・弾幕を組み合わせた構成種別 */
    enum EnemyType {
        BasicEnemy = 0,
        HeavyEnemy = 1,
        BossEnemy = 2,
        StraightShooterEnemy = 3,
        ArmoredEnemy = 4,
        CircleShooterEnemy = 5
    };

    struct BossBullet {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
    };

    struct EnemySpawnRule {
        int enemyType = BasicEnemy;
        int firstFrame = 0;
        int interval = 0;
        float sideX = 1.10f;
        float railX = 0.0f;
        float y = 0.0f;
        float railZ = SideScrollingShooter::EnemyRailFarZ;
    };

    struct Chapter {
        int firstFrame = 0;
        int endFrame = 0;
        const EnemySpawnRule* spawnRules = nullptr;
        int spawnRuleCount = 0;
    };

    virtual ~Stage() = default;
    virtual int StageIndex() const = 0;
    virtual float BossStartDistance() const {
        return SideScrollingShooter::BossStartDistance;
    }
    virtual int BossMaxHp() const {
        return BossEnemyBehaviorInstance().MaxHpForStage(StageIndex());
    }
    /**
     * @brief ボス機体で個別に破壊できる部位数を取得する
     * @return ボス部位数
     */
    virtual int BossPartTotal() const {
        return SideScrollingShooter::BossPartCapacity;
    }
    /**
     * @brief ボス戦の攻撃フェーズ数を取得する
     * @return ボスフェーズ数
     */
    virtual int BossPhaseTotal() const {
        return 4;
    }
    /**
     * @brief ボス戦開始時の攻撃フェーズを取得する
     * @return 初期攻撃フェーズ
     */
    virtual int BossInitialPhase() const {
        return 0;
    }
    virtual int BossNosePart() const { return 0; }
    virtual int BossLeftWingPart() const { return 1; }
    virtual int BossRightWingPart() const { return 2; }
    virtual int BossLeftEnginePart() const { return 3; }
    virtual int BossRightEnginePart() const { return 4; }
    virtual const char* BossPhaseLabel(int phase) const {
        constexpr const char* PhaseLabels[] = {
            "NORMAL 1", "SPECIAL 1", "NORMAL 2", "SPECIAL 2"
        };
        return PhaseLabels[phase % 4];
    }
    /**
     * @brief 撃破時の飛散部品に重力を適用するか取得する
     * @return 重力を適用するステージの場合true
     */
    virtual bool HasDebrisGravity() const {
        return false;
    }
    /**
     * @brief 指定チャプターの終了フレームを取得する
     * @param chapterNumber チャプター番号
     * @return 指定チャプターの終了フレーム
     */
    virtual int ChapterEndFrame(int chapterNumber) const {
        return chapterNumber * SideScrollingShooter::ChapterLengthFrames;
    }
    virtual bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const = 0;
    virtual void ConfigureEnemy(SideScrollingShooter& shooter, Enemy& enemy,
        int enemyType, int frame, int kills, bool railMode) const {
        EnemyBehaviorForType(enemyType).ConfigureSpawn(
            shooter, enemy, frame, kills, railMode, StageIndex());
    }
    virtual void ConfigureBoss(Enemy& boss, bool railMode) const {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
    }
    /**
     * @brief ボスの3Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    virtual void ConfigureBossRailAnchor(Enemy& boss) const {
        boss.x = 0.0f;
        boss.y = 0.0f;
        boss.z = 48.0f;
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
    }
    /**
     * @brief ボスの2Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    virtual void ConfigureBossSideAnchor(Enemy& boss) const {
        boss.x = 1.80f;
        boss.y = 0.0f;
        boss.z = ToRailZFromSideX(boss.x);
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
    }
    /**
     * @brief ボス部位HPを設定する
     * @param boss 部位HPを設定するボス
     * @return なし
     */
    virtual void ConfigureBossPartHp(Enemy& boss) const {
        boss.bossPartHp = { 120, 180, 180, 150, 150 };
    }
    /**
     * @brief ボスの移動を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    virtual void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const {
        BossEnemyBehaviorInstance().Tick(shooter, boss);
    }
    /**
     * @brief ボスが弾幕を止める特殊攻撃中か取得する
     * @param boss 判定するボス
     * @return 特殊攻撃中の場合true
     */
    virtual bool IsBossSpecialAttackActive(const Enemy& boss) const {
        (void)boss;
        return false;
    }
    /**
     * @brief ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    virtual int BossAttackInterval(int phase) const {
        return phase == 0 || phase == 2 ? 120 : 84;
    }
    /**
     * @brief 本体HPからボス攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ボス攻撃フェーズ
     */
    virtual int BossPhaseForHp(int hp, int maxHp) const {
        if (maxHp <= 0) return BossInitialPhase();
        const int clampedHp = hp < 0 ? 0 : (hp > maxHp ? maxHp : hp);
        const int phase = (maxHp - clampedHp) * BossPhaseTotal() / maxHp;
        return phase < BossPhaseTotal() ? phase : BossPhaseTotal() - 1;
    }
    /**
     * @brief ボス部位破壊時に本体へ与えるダメージを取得する
     * @param part 破壊されたボス部位
     * @return 本体へ与えるダメージ
     */
    virtual int BossPartBreakDamage(int part) const {
        (void)part;
        return 120;
    }
    virtual int BossBulletCount(bool railMode) const = 0;
    virtual BossBullet GetBossBullet(int index, bool railMode) const = 0;
    /**
     * @brief 指定部位・フェーズの弾数を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param railMode レール表示中か
    * @return 発射する弾数
    */
    virtual int BossPartBulletCount(int part, int phase, bool railMode) const {
        if (part == BossNosePart()) return BossBulletCount(railMode);
        if (part == BossLeftWingPart() || part == BossRightWingPart()) {
            return phase == 1 || phase == 3 ? 3 : 2;
        }
        return phase == 1 || phase == 3 ? 2 : 1;
    }
    /**
     * @brief 指定部位・フェーズの弾幕内の弾を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    virtual BossBullet GetBossPartBullet(int part, int phase, int index, bool railMode) const {
        const int baseCount = BossBulletCount(railMode);
        const int patternIndex = part == BossNosePart() ? index :
            (part == BossLeftWingPart() ? 0 : (part == BossRightWingPart() ? baseCount - 1 : baseCount / 2));
        BossBullet bullet = GetBossBullet(patternIndex, railMode);
        if (phase == 1 || phase == 3) {
            bullet.vx *= 1.35f;
            bullet.vy += (index - BossPartBulletCount(part, phase, railMode) / 2) * 0.010f;
        }
        return bullet;
    }

protected:
    bool TrySelectByChapters(const Chapter* chapters, int chapterCount, int frame,
        EnemySpawnRule& spawn, int& chapterNumber) const {
        for (int chapterIndex = 0; chapterIndex < chapterCount; ++chapterIndex) {
            const Chapter& chapter = chapters[chapterIndex];
            if (frame < chapter.firstFrame || frame >= chapter.endFrame) continue;
            chapterNumber = chapterIndex + 1;
            return TrySelectByRules(chapter.spawnRules, chapter.spawnRuleCount, frame, spawn);
        }
        return false;
    }

    bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount, int frame, EnemySpawnRule& spawn) const {
        for (int i = 0; i < ruleCount; ++i) {
            const EnemySpawnRule& rule = rules[i];
            if (frame < rule.firstFrame) {
                continue;
            }
            if (frame == rule.firstFrame ||
                (rule.interval > 0 && (frame - rule.firstFrame) % rule.interval == 0)) {
                spawn = rule;
                return true;
            }
        }
        return false;
    }

    bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount, int frame, int kills, EnemySpawnRule& spawn) const {
        (void)kills;
        return TrySelectByRules(rules, ruleCount, frame, spawn);
    }
};

/** @brief ステージ3の敵出現と弾幕を定義する */
class SideScrollingShooter::Stage3 final : public SideScrollingShooter::Stage {
public:
    int StageIndex() const override { return 3; }
    bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {{3, 24, 54, 1.10f, -0.82f, 0.88f, 50.0f}, {4, 110, 250, 1.12f, -0.40f, -0.42f, 60.0f}};
        static constexpr EnemySpawnRule Chapter2[] = {{3, 510, 52, 1.10f, 0.28f, -0.88f, 40.0f}, {5, 570, 165, 1.14f, 0.82f, 0.32f, 56.0f}, {1, 650, 230, 1.16f, 0.05f, 0.54f, 60.0f}};
        static constexpr EnemySpawnRule Chapter3[] = {{4, 1010, 140, 1.12f, -0.85f, -0.18f, 60.0f}, {5, 1060, 145, 1.14f, -0.28f, 0.86f, 50.0f}, {1, 1120, 180, 1.16f, 0.82f, -0.68f, 60.0f}};
        constexpr Chapter Chapters[] = {{0, 500, Chapter1, 2}, {500, 1000, Chapter2, 3}, {1000, 1500, Chapter3, 3}};
        return TrySelectByChapters(Chapters, 3, frame, spawn, chapterNumber);
    }
    int BossBulletCount(bool) const override { return 5; }
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {{-0.12f, 0.0f, -0.024f, -0.020f}, {-0.12f, 0.0f, -0.026f, -0.010f}, {-0.12f, 0.0f, -0.027f, 0.0f}, {-0.12f, 0.0f, -0.026f, 0.010f}, {-0.12f, 0.0f, -0.024f, 0.020f}};
        constexpr BossBullet RailPattern[] = {{0.0f, 0.0f, -0.014f, -0.024f}, {0.0f, 0.0f, -0.007f, -0.012f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.007f, 0.012f}, {0.0f, 0.0f, 0.014f, 0.024f}};
        return (railMode ? RailPattern : SidePattern)[index % 5];
    }
};

/** @brief ステージ4の敵出現と弾幕を定義する */
class SideScrollingShooter::Stage4 final : public SideScrollingShooter::Stage {
public:
    int StageIndex() const override { return 4; }
    bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {{4, 20, 48, 1.10f, -0.82f, -0.68f, 60.0f}, {5, 90, 210, 1.14f, -0.28f, 0.86f, 50.0f}};
        static constexpr EnemySpawnRule Chapter2[] = {{4, 510, 46, 1.12f, 0.28f, 0.18f, 60.0f}, {3, 560, 125, 1.10f, 0.82f, -0.88f, 34.0f}, {1, 630, 190, 1.16f, -0.40f, 0.54f, 60.0f}};
        static constexpr EnemySpawnRule Chapter3[] = {{5, 1010, 110, 1.14f, -0.82f, 0.86f, 50.0f}, {4, 1050, 105, 1.12f, 0.82f, -0.18f, 60.0f}, {3, 1120, 135, 1.10f, 0.28f, -0.88f, 40.0f}};
        constexpr Chapter Chapters[] = {{0, 500, Chapter1, 2}, {500, 1000, Chapter2, 3}, {1000, 1500, Chapter3, 3}};
        return TrySelectByChapters(Chapters, 3, frame, spawn, chapterNumber);
    }
    int BossBulletCount(bool) const override { return 7; }
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {{-0.12f, 0.0f, -0.022f, -0.027f}, {-0.12f, 0.0f, -0.025f, -0.018f}, {-0.12f, 0.0f, -0.027f, -0.009f}, {-0.12f, 0.0f, -0.029f, 0.0f}, {-0.12f, 0.0f, -0.027f, 0.009f}, {-0.12f, 0.0f, -0.025f, 0.018f}, {-0.12f, 0.0f, -0.022f, 0.027f}};
        constexpr BossBullet RailPattern[] = {{0.0f, 0.0f, -0.018f, -0.028f}, {0.0f, 0.0f, -0.012f, -0.018f}, {0.0f, 0.0f, -0.006f, -0.009f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.006f, 0.009f}, {0.0f, 0.0f, 0.012f, 0.018f}, {0.0f, 0.0f, 0.018f, 0.028f}};
        return (railMode ? RailPattern : SidePattern)[index % 7];
    }
};

/** @brief ステージ5の敵出現と弾幕を定義する */
class SideScrollingShooter::Stage5 final : public SideScrollingShooter::Stage {
public:
    int StageIndex() const override { return 5; }
    bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {{5, 18, 42, 1.14f, -0.82f, 0.86f, 50.0f}, {4, 75, 180, 1.12f, -0.28f, -0.42f, 60.0f}};
        static constexpr EnemySpawnRule Chapter2[] = {{5, 510, 95, 1.14f, 0.28f, -0.32f, 56.0f}, {3, 550, 105, 1.10f, 0.82f, 0.88f, 34.0f}, {1, 610, 145, 1.16f, -0.85f, 0.54f, 60.0f}};
        static constexpr EnemySpawnRule Chapter3[] = {{4, 1010, 82, 1.12f, -0.82f, -0.68f, 60.0f}, {5, 1040, 80, 1.14f, 0.82f, 0.32f, 56.0f}, {3, 1090, 95, 1.10f, -0.28f, -0.88f, 40.0f}, {1, 1150, 120, 1.16f, 0.28f, 0.68f, 60.0f}};
        constexpr Chapter Chapters[] = {{0, 500, Chapter1, 2}, {500, 1000, Chapter2, 3}, {1000, 1500, Chapter3, 4}};
        return TrySelectByChapters(Chapters, 3, frame, spawn, chapterNumber);
    }
    int BossBulletCount(bool) const override { return 9; }
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {{-0.12f, 0.0f, -0.020f, -0.032f}, {-0.12f, 0.0f, -0.023f, -0.024f}, {-0.12f, 0.0f, -0.026f, -0.016f}, {-0.12f, 0.0f, -0.028f, -0.008f}, {-0.12f, 0.0f, -0.030f, 0.0f}, {-0.12f, 0.0f, -0.028f, 0.008f}, {-0.12f, 0.0f, -0.026f, 0.016f}, {-0.12f, 0.0f, -0.023f, 0.024f}, {-0.12f, 0.0f, -0.020f, 0.032f}};
        constexpr BossBullet RailPattern[] = {{0.0f, 0.0f, -0.022f, -0.032f}, {0.0f, 0.0f, -0.016f, -0.024f}, {0.0f, 0.0f, -0.011f, -0.016f}, {0.0f, 0.0f, -0.005f, -0.008f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.005f, 0.008f}, {0.0f, 0.0f, 0.011f, 0.016f}, {0.0f, 0.0f, 0.016f, 0.024f}, {0.0f, 0.0f, 0.022f, 0.032f}};
        return (railMode ? RailPattern : SidePattern)[index % 9];
    }
};
