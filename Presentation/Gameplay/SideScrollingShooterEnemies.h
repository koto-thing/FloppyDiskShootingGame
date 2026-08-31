#pragma once

#include <algorithm>
#include <cmath>

/**
 * @brief 敵ごとの初期値、移動、発射間隔を定義する基底クラス
 */
class SideScrollingShooter::EnemyBehavior {
public:
    struct EntryCandidate {
        float sideX = 1.08f;
        float railX = 0.0f;
        float y = 0.0f;
    };

    virtual ~EnemyBehavior() = default;
    virtual int Type() const = 0;
    virtual int MaxHp() const = 0;
    virtual void Tick(SideScrollingShooter& shooter, Enemy& enemy) const = 0;
    virtual int AimedShotInterval() const = 0;
    virtual int MaxHpForStage(int) const {
        return MaxHp();
    }
    virtual int AimedShotIntervalForStage(int) const {
        return AimedShotInterval();
    }
    virtual void ConfigureSpawn(Enemy& enemy, int frame, int kills, bool railMode, int stageIndex) const {
        ConfigureStats(enemy, stageIndex);
        ConfigureDefaultEntry(enemy, frame, kills, railMode, stageIndex);
        enemy.phase = static_cast<float>((frame + (stageIndex - 1) * 17) % 47) * 0.17f;
    }
    virtual void ConfigureBossSpawn(Enemy& enemy, bool railMode, int stageIndex) const {
        ConfigureStats(enemy, stageIndex);
        enemy.active = true;
        enemy.x = 1.16f;
        enemy.y = 0.0f;
        enemy.z = railMode ? (stageIndex >= 2 ? 52.0f : 48.0f) : ToRailZFromSideX(enemy.x);
        enemy.baseX = 0.0f;
        enemy.baseY = 0.0f;
        enemy.phase = 0.0f;
    }
    virtual float CollisionRadius(const Enemy& enemy) const {
        return enemy.type == 2 ? 0.18f : 0.065f;
    }
    virtual float CollisionRadius3D(const Enemy& enemy) const {
        return enemy.type == 2 ? 1.6f : 0.7f;
    }
    virtual float ShotHitRadius3D(const Enemy& enemy) const {
        return enemy.type == 2 ? 1.5f : 0.55f;
    }
    virtual int Score(const Enemy& enemy) const {
        return enemy.type == 0 ? 100 : 250;
    }
    virtual float RenderScale() const {
        return 1.0f;
    }

protected:
    void ConfigureStats(Enemy& enemy, int stageIndex) const {
        enemy.type = Type();
        enemy.behavior = this;
        enemy.hp = MaxHpForStage(stageIndex);
        enemy.maxHp = enemy.hp;
        enemy.shotInterval = AimedShotIntervalForStage(stageIndex);
        enemy.age = 0;
    }

    int SelectEntryCandidateIndex(int frame, int kills, int stageIndex) const {
        return (frame / 30 + kills * 2 + Type() * 3 + stageIndex) % EntryCandidateCount(stageIndex);
    }

    virtual int EntryCandidateCount(int stageIndex) const {
        return stageIndex >= 2 ? 5 : 4;
    }

    virtual EntryCandidate EntryCandidateAt(int index, int stageIndex) const {
        constexpr EntryCandidate Stage1Candidates[] = {
            {1.08f, -0.72f, -0.54f},
            {1.10f, -0.22f, 0.36f},
            {1.12f, 0.42f, -0.12f},
            {1.14f, 0.70f, 0.58f}
        };
        constexpr EntryCandidate Stage2Candidates[] = {
            {1.08f, -0.85f, -0.68f},
            {1.10f, -0.40f, 0.54f},
            {1.12f, 0.05f, -0.18f},
            {1.14f, 0.48f, 0.18f},
            {1.16f, 0.82f, 0.68f}
        };
        if (stageIndex >= 2) {
            return Stage2Candidates[index % (sizeof(Stage2Candidates) / sizeof(Stage2Candidates[0]))];
        }
        return Stage1Candidates[index % (sizeof(Stage1Candidates) / sizeof(Stage1Candidates[0]))];
    }

    void ConfigureDefaultEntry(Enemy& enemy, int frame, int kills, bool railMode, int stageIndex) const {
        const EntryCandidate candidate = EntryCandidateAt(
            SelectEntryCandidateIndex(frame, kills, stageIndex), stageIndex);
        enemy.baseX = railMode ? candidate.railX : candidate.sideX;
        enemy.x = enemy.baseX;
        enemy.baseY = candidate.y;
        enemy.y = enemy.baseY;
        enemy.z = railMode ? EnemyRailFarZ : ToRailZFromSideX(enemy.x);
    }
};

/**
 * @brief 小型の通常敵を制御する
 */
class SideScrollingShooter::BasicEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 0;
    }

    int MaxHp() const override {
        return 1;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= 0.42f;
            enemy.x = enemy.baseX + std::sin(enemy.phase + enemy.age * 0.045f) * 0.16f;
        } else {
            enemy.x -= 0.010f;
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        enemy.y = enemy.baseY + std::sin(enemy.phase + enemy.age * 0.055f) * 0.10f;
    }

    int AimedShotInterval() const override {
        return 105;
    }

    int AimedShotIntervalForStage(int stageIndex) const override {
        return stageIndex >= 2 ? 92 : AimedShotInterval();
    }

    int Score(const Enemy&) const override {
        return 100;
    }
};

/**
 * @brief 耐久力のある通常敵を制御する
 */
class SideScrollingShooter::HeavyEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 1;
    }

    int MaxHp() const override {
        return 3;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= 0.30f;
            enemy.x = enemy.baseX + std::sin(enemy.phase + enemy.age * 0.045f) * 0.24f;
        } else {
            enemy.x -= 0.007f;
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        enemy.y = enemy.baseY + std::sin(enemy.phase + enemy.age * 0.055f) * 0.18f;
    }

    int AimedShotInterval() const override {
        return 72;
    }

    int AimedShotIntervalForStage(int stageIndex) const override {
        return stageIndex >= 2 ? 64 : AimedShotInterval();
    }

    int Score(const Enemy&) const override {
        return 250;
    }

    float RenderScale() const override {
        return 1.28f;
    }
};

/**
 * @brief 高耐久の重装敵を制御する
 */
class SideScrollingShooter::ArmoredEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 4;
    }

    int MaxHp() const override {
        return 4;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= 0.25f;
            enemy.x = enemy.baseX + std::sin(enemy.phase + enemy.age * 0.038f) * 0.30f;
        } else {
            enemy.x -= 0.006f;
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        enemy.y = enemy.baseY + std::sin(enemy.phase + enemy.age * 0.050f) * 0.16f;
    }

    int AimedShotInterval() const override {
        return 64;
    }

    int Score(const Enemy&) const override {
        return 320;
    }

    float RenderScale() const override {
        return 1.40f;
    }
};

/**
 * @brief 上下端から出現する直進狙撃機を制御する
 */
class SideScrollingShooter::StraightShooterEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 3;
    }

    int MaxHp() const override {
        return 2;
    }

    void ConfigureSpawn(Enemy& enemy, int frame, int kills, bool railMode, int stageIndex) const override {
        ConfigureStats(enemy, stageIndex);
        const EntryCandidate candidate = EntryCandidateAt(
            SelectEntryCandidateIndex(frame, kills, stageIndex), stageIndex);
        enemy.baseX = railMode ? candidate.railX : candidate.sideX;
        enemy.x = enemy.baseX;
        enemy.z = railMode ? EnemyRailFarZ : ToRailZFromSideX(enemy.x);
        enemy.baseY = candidate.y;
        enemy.y = enemy.baseY;
        enemy.phase = 0.0f;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        enemy.z -= 0.36f;
        enemy.x = shooter.IsRailGameplayActive() ? enemy.baseX : ToSideXFromRailZ(enemy.z);
        enemy.y = enemy.baseY;
    }

    int AimedShotInterval() const override {
        return 32;
    }

    int AimedShotIntervalForStage(int stageIndex) const override {
        return stageIndex >= 2 ? 28 : AimedShotInterval();
    }

    int Score(const Enemy&) const override {
        return 180;
    }

    float RenderScale() const override {
        return 1.12f;
    }

protected:
    int EntryCandidateCount(int) const override {
        return 4;
    }

    EntryCandidate EntryCandidateAt(int index, int stageIndex) const override {
        const bool lateStage = stageIndex >= 2;
        constexpr EntryCandidate Stage1Candidates[] = {
            {1.10f, -0.78f, 0.86f},
            {1.12f, -0.18f, -0.86f},
            {1.14f, 0.35f, 0.86f},
            {1.16f, 0.78f, -0.86f}
        };
        constexpr EntryCandidate Stage2Candidates[] = {
            {1.10f, -0.82f, 0.88f},
            {1.12f, -0.28f, -0.88f},
            {1.14f, 0.28f, 0.88f},
            {1.16f, 0.82f, -0.88f}
        };
        if (lateStage) {
            return Stage2Candidates[index % (sizeof(Stage2Candidates) / sizeof(Stage2Candidates[0]))];
        }
        return Stage1Candidates[index % (sizeof(Stage1Candidates) / sizeof(Stage1Candidates[0]))];
    }
};

/**
 * @brief ボスの移動と通常狙い弾間隔を制御する
 */
class SideScrollingShooter::BossEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 2;
    }

    int MaxHp() const override {
        return SideScrollingShooter::BossMaxHp;
    }

    int MaxHpForStage(int stageIndex) const override {
        return stageIndex >= 2 ? SideScrollingShooter::BossMaxHp + 16 : MaxHp();
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            enemy.x = std::sin(enemy.age * 0.018f) * 0.34f;
            enemy.y = std::sin(enemy.age * 0.025f) * 0.36f;
            if (enemy.z <= 0.0f) {
                enemy.z = 48.0f;
            }
        } else {
            if (enemy.x > 0.70f) {
                enemy.x -= 0.008f;
            }
            enemy.z = ToRailZFromSideX(enemy.x);
            enemy.y = std::sin(enemy.age * 0.025f) * 0.48f;
        }
    }

    int AimedShotInterval() const override {
        return 42;
    }

    int AimedShotIntervalForStage(int stageIndex) const override {
        return stageIndex >= 2 ? 36 : AimedShotInterval();
    }

    float CollisionRadius(const Enemy&) const override {
        return 0.18f;
    }

    float CollisionRadius3D(const Enemy&) const override {
        return 1.6f;
    }

    float ShotHitRadius3D(const Enemy&) const override {
        return 1.5f;
    }
};
