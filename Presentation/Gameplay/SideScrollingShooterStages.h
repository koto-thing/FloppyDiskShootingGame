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
        int frameUnit = 1;
        int cycle = 1;
        int remainder = 0;
    };

    virtual ~Stage() = default;
    virtual int StageIndex() const = 0;
    virtual float BossStartDistance() const {
        return SideScrollingShooter::BossStartDistance;
    }
    virtual int BossMaxHp() const {
        return BossEnemyBehaviorInstance().MaxHpForStage(StageIndex());
    }
    virtual int SpawnCooldown(int frame, int kills) const = 0;
    virtual int SelectEnemyType(int frame, int kills) const = 0;
    virtual void ConfigureEnemy(Enemy& enemy, int frame, int kills, bool railMode) const {
        EnemyBehaviorForType(SelectEnemyType(frame, kills)).ConfigureSpawn(
            enemy, frame, kills, railMode, StageIndex());
    }
    virtual void ConfigureBoss(Enemy& boss, bool railMode) const {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
    }
    virtual int BossBulletCount(bool railMode) const = 0;
    virtual BossBullet GetBossBullet(int index, bool railMode) const = 0;

protected:
    int SelectByRules(const EnemySpawnRule* rules, int ruleCount, int frame, int kills) const {
        for (int i = 0; i < ruleCount; ++i) {
            const EnemySpawnRule& rule = rules[i];
            if ((kills + frame / rule.frameUnit) % rule.cycle == rule.remainder) {
                return rule.enemyType;
            }
        }
        return 0;
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

    int SpawnCooldown(int, int kills) const override {
        return (std::max)(28, 70 - kills);
    }

    int SelectEnemyType(int frame, int kills) const override {
        constexpr EnemySpawnRule Rules[] = {
            {3, 90, 6, 2},
            {1, 60, 5, 4}
        };
        return SelectByRules(Rules, static_cast<int>(sizeof(Rules) / sizeof(Rules[0])), frame, kills);
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

    int SpawnCooldown(int frame, int kills) const override {
        return (std::max)(22, 62 - kills + (frame / 180) % 10);
    }

    int SelectEnemyType(int frame, int kills) const override {
        constexpr EnemySpawnRule Rules[] = {
            {3, 75, 5, 1},
            {4, 45, 4, 3}
        };
        return SelectByRules(Rules, static_cast<int>(sizeof(Rules) / sizeof(Rules[0])), frame, kills);
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
