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

    virtual ~Stage() = default;
    virtual float BossStartDistance() const { return SideScrollingShooter::BossStartDistance; }
    virtual int BossMaxHp() const { return SideScrollingShooter::BossMaxHp; }
    virtual int SpawnCooldown(int frame, int kills) const = 0;
    virtual void ConfigureEnemy(Enemy& enemy, int frame, int kills, bool railMode) const = 0;
    virtual void ConfigureBoss(Enemy& boss, bool railMode) const = 0;
    virtual int AimedShotInterval(int enemyType) const = 0;
    virtual int BossBulletCount(bool railMode) const = 0;
    virtual BossBullet GetBossBullet(int index, bool railMode) const = 0;
};

/**
 * @brief 最初のステージの敵出現と弾幕を定義する
 */
class SideScrollingShooter::Stage1 final : public SideScrollingShooter::Stage {
public:
    int SpawnCooldown(int, int kills) const override {
        return (std::max)(28, 70 - kills);
    }

    void ConfigureEnemy(Enemy& enemy, int frame, int kills, bool railMode) const override {
        enemy.baseX = railMode ? -0.72f + static_cast<float>((frame * 53) % 145) / 100.0f : 1.05f;
        enemy.x = enemy.baseX;
        enemy.baseY = railMode ? -0.52f + static_cast<float>((frame * 37) % 105) / 100.0f :
            -0.60f + static_cast<float>((frame * 37) % 120) / 100.0f;
        enemy.y = enemy.baseY;
        enemy.z = railMode ? EnemyRailFarZ : ToRailZFromSideX(enemy.x);
        enemy.phase = static_cast<float>(frame % 31) * 0.2f;
        enemy.type = ((kills + frame / 60) % 5 == 4) ? 1 : 0;
        enemy.hp = enemy.type == 0 ? 1 : 3;
        enemy.maxHp = enemy.hp;
        enemy.age = 0;
    }

    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        boss.active = true;
        boss.x = 1.16f;
        boss.y = 0.0f;
        boss.z = railMode ? 48.0f : ToRailZFromSideX(boss.x);
        boss.baseX = 0.0f;
        boss.baseY = 0.0f;
        boss.phase = 0.0f;
        boss.type = 2;
        boss.hp = BossMaxHp();
        boss.maxHp = boss.hp;
        boss.age = 0;
    }

    int AimedShotInterval(int enemyType) const override {
        return enemyType == 2 ? 42 : (enemyType == 0 ? 105 : 72);
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
    int BossMaxHp() const override {
        return SideScrollingShooter::BossMaxHp + 16;
    }

    int SpawnCooldown(int frame, int kills) const override {
        return (std::max)(22, 62 - kills + (frame / 180) % 10);
    }

    void ConfigureEnemy(Enemy& enemy, int frame, int kills, bool railMode) const override {
        enemy.baseX = railMode ? -0.85f + static_cast<float>((frame * 41) % 170) / 100.0f : 1.08f;
        enemy.x = enemy.baseX;
        enemy.baseY = railMode ? -0.62f + static_cast<float>((frame * 29) % 125) / 100.0f :
            -0.70f + static_cast<float>((frame * 29) % 140) / 100.0f;
        enemy.y = enemy.baseY;
        enemy.z = railMode ? EnemyRailFarZ : ToRailZFromSideX(enemy.x);
        enemy.phase = static_cast<float>((frame + 17) % 47) * 0.17f;
        enemy.type = ((kills + frame / 45) % 4 == 3) ? 1 : 0;
        enemy.hp = enemy.type == 0 ? 1 : 4;
        enemy.maxHp = enemy.hp;
        enemy.age = 0;
    }

    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        boss.active = true;
        boss.x = 1.16f;
        boss.y = 0.0f;
        boss.z = railMode ? 52.0f : ToRailZFromSideX(boss.x);
        boss.baseX = 0.0f;
        boss.baseY = 0.0f;
        boss.phase = 0.0f;
        boss.type = 2;
        boss.hp = BossMaxHp();
        boss.maxHp = boss.hp;
        boss.age = 0;
    }

    int AimedShotInterval(int enemyType) const override {
        return enemyType == 2 ? 36 : (enemyType == 0 ? 92 : 64);
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
