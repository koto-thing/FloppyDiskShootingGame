#pragma once

#include <algorithm>

/**
 * @brief ステージごとの敵出現と弾幕を定義する基底クラス
 */
class SideScrollingShooter::Stage {
public:
    struct BossBullet {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
    };

    struct EnemySpawnRule {
        int enemyType = 0;
        int firstFrame = 0;
        int interval = 0;
    };

    virtual ~Stage() = default;
    virtual int StageIndex() const = 0;
    virtual float BossStartDistance() const {
        return SideScrollingShooter::BossStartDistance;
    }
    virtual int BossMaxHp() const {
        return BossEnemyBehaviorInstance().MaxHpForStage(StageIndex());
    }
    virtual bool TrySelectEnemyType(int frame, int& enemyType) const = 0;
    virtual void ConfigureEnemy(SideScrollingShooter& shooter, Enemy& enemy,
        int enemyType, int frame, int kills, bool railMode) const {
        EnemyBehaviorForType(enemyType).ConfigureSpawn(
            shooter, enemy, frame, kills, railMode, StageIndex());
    }
    virtual void ConfigureBoss(Enemy& boss, bool railMode) const {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
    }
    virtual int BossBulletCount(bool railMode) const = 0;
    virtual BossBullet GetBossBullet(int index, bool railMode) const = 0;

protected:
    bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount, int frame, int& enemyType) const {
        for (int i = 0; i < ruleCount; ++i) {
            const EnemySpawnRule& rule = rules[i];
            if (frame < rule.firstFrame) {
                continue;
            }
            if (frame == rule.firstFrame ||
                (rule.interval > 0 && (frame - rule.firstFrame) % rule.interval == 0)) {
                enemyType = rule.enemyType;
                return true;
            }
        }
        return false;
    }

    bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount, int frame, int kills, int& enemyType) const {
        (void)kills;
        return TrySelectByRules(rules, ruleCount, frame, enemyType);
    }
};

/**
 * @brief 最初のステージの敵出現と弾幕を定義する
 */
class SideScrollingShooter::Stage1 final : public SideScrollingShooter::Stage {
public:
    int StageIndex() const override {
        return 1;
    }

    bool TrySelectEnemyType(int frame, int& enemyType) const override {
        // { 敵タイプ, 最初の生成フレーム, 以降生成する周期 }
        constexpr EnemySpawnRule Rules[] = {
            {0, 35, 70},
            {3, 160, 360},
            {5, 220, 525},
            {1, 300, 420}
        };
        return TrySelectByRules(Rules, static_cast<int>(sizeof(Rules) / sizeof(Rules[0])), frame, enemyType);
    }

    int BossBulletCount(bool) const override {
        return 3;
    }

    BossBullet GetBossBullet(int index, bool railMode) const override {
        if (railMode) {
            constexpr BossBullet RailPattern[3] = {
                {0.0f, 0.0f, 0.0f, -0.018f},
                {0.0f, 0.0f, 0.0f, 0.000f},
                {0.0f, 0.0f, 0.0f, 0.018f}
            };
            return RailPattern[index % 3];
        }

        constexpr BossBullet SidePattern[3] = {
            {-0.12f, 0.0f, -0.020f, -0.010f},
            {-0.12f, 0.0f, -0.022f, 0.000f},
            {-0.12f, 0.0f, -0.020f, 0.010f}
        };
        return SidePattern[index % 3];
    }
};

/**
 * @brief ステージ2用
 */
class SideScrollingShooter::Stage2 final : public SideScrollingShooter::Stage {
public:
    int StageIndex() const override {
        return 2;
    }

    bool TrySelectEnemyType(int frame, int& enemyType) const override {
        // { 敵タイプ, 最初の生成フレーム, 以降生成する周期 }
        constexpr EnemySpawnRule Rules[] = {
            {0, 30, 62},
            {3, 120, 300},
            {5, 180, 420},
            {4, 260, 360}
        };
        return TrySelectByRules(Rules, static_cast<int>(sizeof(Rules) / sizeof(Rules[0])), frame, enemyType);
    }

    int BossBulletCount(bool) const override {
        return 5;
    }

    BossBullet GetBossBullet(int index, bool railMode) const override {
        if (railMode) {
            constexpr BossBullet RailPattern[5] = {
                {0.0f, 0.0f, -0.010f, -0.020f},
                {0.0f, 0.0f, 0.000f, -0.012f},
                {0.0f, 0.0f, 0.000f, 0.000f},
                {0.0f, 0.0f, 0.000f, 0.012f},
                {0.0f, 0.0f, 0.010f, 0.020f}
            };
            return RailPattern[index % 5];
        }

        constexpr BossBullet SidePattern[5] = {
            {-0.12f, 0.0f, -0.018f, -0.018f},
            {-0.12f, 0.0f, -0.021f, -0.009f},
            {-0.12f, 0.0f, -0.023f, 0.000f},
            {-0.12f, 0.0f, -0.021f, 0.009f},
            {-0.12f, 0.0f, -0.018f, 0.018f}
        };
        return SidePattern[index % 5];
    }
};
