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
     * @brief 部位破壊時にボス本体へ与えるダメージを取得する
     * @param part 破壊された部位
     * @return 本体へ与えるダメージ
     */
    virtual int BossPartBreakDamage(BossPart part) const {
        (void)part;
        return 120;
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
    virtual int BossAttackInterval(BossPhase phase) const {
        return phase == BossNormalPhase1 || phase == BossNormalPhase2 ? 120 : 84;
    }
    /**
     * @brief 本体HPからボス攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ボス攻撃フェーズ
     */
    virtual int BossPhaseForHp(int hp, int maxHp) const {
        return SideScrollingShooter::BossPhaseForHp(hp, maxHp);
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
    virtual int BossPartBulletCount(BossPart part, BossPhase phase, bool railMode) const {
        if (part == BossNose) return BossBulletCount(railMode);
        if (part == BossLeftWing || part == BossRightWing) {
            return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 3 : 2;
        }
        return phase == BossSpecialPhase1 || phase == BossSpecialPhase2 ? 2 : 1;
    }
    /**
     * @brief 指定部位・フェーズの弾幕内の弾を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    virtual BossBullet GetBossPartBullet(BossPart part, BossPhase phase, int index, bool railMode) const {
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
    /**
     * @brief ステージ番号を取得する
     * @return Stage 5を表す番号
     */
    int StageIndex() const override { return 5; }
    /**
     * @brief EASTSOURCEの最大HPを取得する
     * @return EASTSOURCEの最大HP
     */
    int BossMaxHp() const override { return SideScrollingShooter::EastsourceMaxHp; }
    /**
     * @brief Stage 5の経過フレームから通常敵の出現を選択する
     * @param frame Stage 5開始からの経過フレーム
     * @param spawn 選択した出現規則の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させるフレームの場合true
     */
    bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {{5, 18, 42, 1.14f, -0.82f, 0.86f, 50.0f}, {4, 75, 180, 1.12f, -0.28f, -0.42f, 60.0f}};
        static constexpr EnemySpawnRule Chapter2[] = {{5, 510, 95, 1.14f, 0.28f, -0.32f, 56.0f}, {3, 550, 105, 1.10f, 0.82f, 0.88f, 34.0f}, {1, 610, 145, 1.16f, -0.85f, 0.54f, 60.0f}};
        static constexpr EnemySpawnRule Chapter3[] = {{4, 1010, 82, 1.12f, -0.82f, -0.68f, 60.0f}, {5, 1040, 80, 1.14f, 0.82f, 0.32f, 56.0f}, {3, 1090, 95, 1.10f, -0.28f, -0.88f, 40.0f}, {1, 1150, 120, 1.16f, 0.28f, 0.68f, 60.0f}};
        constexpr Chapter Chapters[] = {{0, 500, Chapter1, 2}, {500, 1000, Chapter2, 3}, {1000, 1500, Chapter3, 4}};
        return TrySelectByChapters(Chapters, 3, frame, spawn, chapterNumber);
    }
    /**
     * @brief 汎用ボス弾幕の弾数を取得する
     * @param railMode 3Dレール表示の場合true
     * @return 弾数
     */
    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 9;
    }
    /**
     * @brief 汎用ボス弾幕の一発を取得する
     * @param index 弾番号
     * @param railMode 3Dレール表示の場合true
     * @return 弾の発射位置と速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        constexpr BossBullet SidePattern[] = {{-0.12f, 0.0f, -0.020f, -0.032f}, {-0.12f, 0.0f, -0.023f, -0.024f}, {-0.12f, 0.0f, -0.026f, -0.016f}, {-0.12f, 0.0f, -0.028f, -0.008f}, {-0.12f, 0.0f, -0.030f, 0.0f}, {-0.12f, 0.0f, -0.028f, 0.008f}, {-0.12f, 0.0f, -0.026f, 0.016f}, {-0.12f, 0.0f, -0.023f, 0.024f}, {-0.12f, 0.0f, -0.020f, 0.032f}};
        constexpr BossBullet RailPattern[] = {{0.0f, 0.0f, -0.022f, -0.032f}, {0.0f, 0.0f, -0.016f, -0.024f}, {0.0f, 0.0f, -0.011f, -0.016f}, {0.0f, 0.0f, -0.005f, -0.008f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.005f, 0.008f}, {0.0f, 0.0f, 0.011f, 0.016f}, {0.0f, 0.0f, 0.016f, 0.024f}, {0.0f, 0.0f, 0.022f, 0.032f}};
        return (railMode ? RailPattern : SidePattern)[index % 9];
    }
    /**
     * @brief EASTSOURCEを戦闘開始状態へ設定する
     * @param boss 設定するEASTSOURCE
     * @param railMode 3Dレール表示の場合true
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
        boss.hp = SideScrollingShooter::EastsourceMaxHp;
        boss.maxHp = boss.hp;
        boss.shotInterval = 0;
        if (railMode) ConfigureBossRailAnchor(boss);
    }
    /**
     * @brief EASTSOURCEの部位HPを設定する
     * @param boss 設定するEASTSOURCE
     * @return なし
     */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = {
            SideScrollingShooter::EastsourceNoseHp,
            SideScrollingShooter::EastsourceWingHp,
            SideScrollingShooter::EastsourceWingHp,
            SideScrollingShooter::EastsourceEngineHp,
            SideScrollingShooter::EastsourceEngineHp
        };
    }
    /**
     * @brief EASTSOURCE部位破壊時の本体ダメージを取得する
     * @param part 破壊された部位
     * @return 本体へ与えるダメージ
     */
    int BossPartBreakDamage(BossPart part) const override {
        return part == BossNose ? 75 : (part == BossLeftWing || part == BossRightWing ? 90 : 65);
    }
    /**
     * @brief EASTSOURCEの移動と攻撃を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するEASTSOURCE
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        shooter.TickEastsource(boss);
    }
    /**
     * @brief EASTSOURCE専用攻撃が有効か取得する
     * @param boss 判定するEASTSOURCE
     * @return 常にtrue
     */
    bool IsBossSpecialAttackActive(const Enemy& boss) const override {
        (void)boss;
        // EASTSOURCEは専用の予告付き4フェーズ攻撃だけを使用する
        return true;
    }
};
