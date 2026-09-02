#pragma once

#include <algorithm>
#include <cmath>

/**
 * @brief 敵ごとの初期値、移動、発射間隔を定義する基底クラス
 */
class SideScrollingShooter::EnemyBehavior {
public:
	// @brief 敵の初期配置候補
    // { 2DのX, 3DのX, 上下Y, 3DのZ }
    struct EntryCandidate {
        float sideX = 1.08f;
        float railX = 0.0f;
        float y = 0.0f;
        float railZ = EnemyRailFarZ;
    };

    virtual ~EnemyBehavior() = default;
    virtual int Type() const = 0;
    virtual int MaxHp() const = 0;
    virtual void Tick(SideScrollingShooter& shooter, Enemy& enemy) const = 0;
    virtual int AimedShotInterval() const = 0;
    virtual float AimedShotSpeed() const {
        return 0.018f;
    }
    virtual float RailAimedShotSpeed() const {
        return 0.42f;
    }
    virtual int MaxHpForStage(int) const {
        return MaxHp();
    }
    virtual int AimedShotIntervalForStage(int) const {
        return AimedShotInterval();
    }
    virtual void FireSpecial(SideScrollingShooter&, Enemy&) const {
    }
    virtual void ConfigureSpawn(SideScrollingShooter& shooter, Enemy& enemy,
        int frame, int kills, bool railMode, int stageIndex) const {
        (void)shooter;
        ConfigureStats(enemy, stageIndex);
        ConfigureDefaultEntry(enemy, frame, kills, railMode, stageIndex);
        enemy.phase = static_cast<float>((frame + (stageIndex - 1) * 17) % 47) * 0.17f;
    }
    virtual void ConfigureBossSpawn(Enemy& enemy, bool railMode, int stageIndex) const {
        ConfigureStats(enemy, stageIndex);
        enemy.active = true;
        enemy.collisionEnabled = true;
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
        return enemy.type == 2 ? 1.5f : 0.66f;
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
        enemy.motionAge = 0;
    }

    int SelectEntryCandidateIndex(int frame, int kills, int stageIndex) const {
        return (frame / 30 + kills * 2 + Type() * 3 + stageIndex) % EntryCandidateCount(stageIndex);
    }

    virtual int EntryCandidateCount(int stageIndex) const {
        return stageIndex >= 2 ? 5 : 4;
    }

    virtual EntryCandidate EntryCandidateAt(int index, int stageIndex) const {
        // { 2DのX, 3DのX, 上下Y, 3DのZ }
        constexpr EntryCandidate Stage1Candidates[] = {
            {1.08f, -0.72f, -0.54f, EnemyRailFarZ},
            {1.10f, -0.22f, 0.36f, EnemyRailFarZ},
            {1.12f, 0.42f, -0.12f, EnemyRailFarZ},
            {1.14f, 0.70f, 0.58f, EnemyRailFarZ}
        };
        constexpr EntryCandidate Stage2Candidates[] = {
            {1.08f, -0.85f, -0.68f, EnemyRailFarZ},
            {1.10f, -0.40f, 0.54f, EnemyRailFarZ},
            {1.12f, 0.05f, -0.18f, EnemyRailFarZ},
            {1.14f, 0.48f, 0.18f, EnemyRailFarZ},
            {1.16f, 0.82f, 0.68f, EnemyRailFarZ}
        };
        if (stageIndex >= 2) {
            return Stage2Candidates[index % (sizeof(Stage2Candidates) / sizeof(Stage2Candidates[0]))];
        }
        return Stage1Candidates[index % (sizeof(Stage1Candidates) / sizeof(Stage1Candidates[0]))];
    }

    int EdgeEntryCandidateCount() const {
        return 4;
    }

    EntryCandidate EdgeEntryCandidateAt(int index, int stageIndex) const {
        const bool lateStage = stageIndex >= 2;
        constexpr EntryCandidate Stage1Candidates[] = {
            {1.10f, -0.78f, 0.86f, 48.0f},
            {1.12f, -0.18f, -0.86f, 42.0f},
            {1.14f, 0.35f, 0.25f, 54.0f},
            {1.16f, 0.78f, -0.25f, 36.0f}
        };
        constexpr EntryCandidate Stage2Candidates[] = {
            {1.10f, -0.82f, 0.88f, 50.0f},
            {1.12f, -0.28f, -0.88f, 40.0f},
            {1.14f, 0.28f, 0.88f, 56.0f},
            {1.16f, 0.82f, -0.88f, 34.0f}
        };
        if (lateStage) {
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
        enemy.z = railMode ? candidate.railZ : ToRailZFromSideX(enemy.x);
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
        return 10;
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

    float AimedShotSpeed() const override {
        return 0.018f;
    }

    float RailAimedShotSpeed() const override {
        return 0.42f;
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
        return 30;
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

    float AimedShotSpeed() const override {
        return 0.017f;
    }

    float RailAimedShotSpeed() const override {
        return 0.58f;
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
        return 50;
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

    float AimedShotSpeed() const override {
        return 0.016f;
    }

    float RailAimedShotSpeed() const override {
        return 0.54f;
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
        return 20;
    }

    void ConfigureSpawn(SideScrollingShooter& shooter, Enemy& enemy,
        int frame, int kills, bool railMode, int stageIndex) const override {
        (void)shooter;
        ConfigureStats(enemy, stageIndex);
        const EntryCandidate candidate = EntryCandidateAt(
            SelectEntryCandidateIndex(frame, kills, stageIndex), stageIndex);
        enemy.baseX = railMode ? candidate.railX : candidate.sideX;
        enemy.x = enemy.baseX;
        enemy.z = railMode ? candidate.railZ : ToRailZFromSideX(enemy.x);
        enemy.baseY = candidate.y;
        enemy.y = enemy.baseY;
        enemy.phase = 0.0f;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= 0.36f;
            enemy.x = enemy.baseX;
        } else {
            enemy.x -= 0.020f;
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        enemy.y = enemy.baseY;
    }

    int AimedShotInterval() const override {
        return 32;
    }

    float AimedShotSpeed() const override {
        return 0.020f;
    }

    float RailAimedShotSpeed() const override {
        return 0.58f;
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
        return EdgeEntryCandidateCount();
    }

    EntryCandidate EntryCandidateAt(int index, int stageIndex) const override {
        return EdgeEntryCandidateAt(index, stageIndex);
    }
};

/**
 * @brief 停止後に円形弾幕を撃つ砲台機を制御する
 */
class SideScrollingShooter::CircleShooterEnemyBehavior : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 5;
    }

    int MaxHp() const override {
        return 50;
    }

    void ConfigureSpawn(SideScrollingShooter& shooter, Enemy& enemy,
        int frame, int kills, bool railMode, int stageIndex) const override {
        ConfigureStats(enemy, stageIndex);
        const EntryCandidate candidate = SelectFreeCandidate(shooter, enemy, frame, kills, stageIndex);
        enemy.baseX = railMode ? candidate.railX : candidate.sideX;
        enemy.x = enemy.baseX;
        enemy.z = railMode ? candidate.railZ : ToRailZFromSideX(enemy.x);
        enemy.baseY = candidate.y;
        enemy.y = enemy.baseY;
        enemy.phase = 0.0f;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            // 3DではXはレーン位置として固定し、奥行き方向へ進んでから砲撃位置で停止する
            constexpr float StopZ = PlayerRailZ + 24.0f;
            static_assert(StopZ < EnemyRailFarZ);
            enemy.x = enemy.baseX;
            enemy.z = (std::max)(enemy.z - 0.30f, StopZ);
            enemy.y = enemy.baseY;
            FireSpecial(shooter, enemy);
            return;
        }

        // 2Dでは画面右端から侵入し、砲撃後は左へ離脱する
        constexpr float StopX = 0.75f;
        constexpr int ExitAge = 288;
        static_assert(ExitAge > 72);
        if (enemy.age >= ExitAge || enemy.x > StopX) {
            enemy.x -= 0.012f;
        } else {
            enemy.x = StopX;
        }
        enemy.z = ToRailZFromSideX(enemy.x);
        enemy.y = enemy.baseY;
        FireSpecial(shooter, enemy);
    }

    int AimedShotInterval() const override {
        return 0;
    }

    float RingShotSpeed() const {
        return 0.58f;
    }

    float RingSpreadSpeed() const {
        return 0.010f;
    }

    int Score(const Enemy&) const override {
        return 280;
    }

    float RenderScale() const override {
        return 1.22f;
    }

    void FireSpecial(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (enemy.age < 36 || enemy.age % 72 != 0) {
            return;
        }

        constexpr int BulletCount = 8;
        if (shooter.IsRailGameplayActive()) {
            constexpr float Radius = 0.22f;
            for (int i = 0; i < BulletCount; ++i) {
                const float angle = static_cast<float>(i) * Math::TwoPi / static_cast<float>(BulletCount);
                const float cx = std::cos(angle);
                const float sy = std::sin(angle);
                shooter.SpawnShotDirect(enemy.x + cx * Radius, enemy.y + sy * Radius, enemy.z,
                    cx * RingSpreadSpeed(), sy * RingSpreadSpeed() * 1.4f, -RingShotSpeed(), true, i, BulletCount);
            }
            shooter.PlayBossMachineGunSound();
            return;
        }

        for (int i = 0; i < BulletCount; ++i) {
            const float offsetY = (static_cast<float>(i) - 3.5f) * 0.075f;
            shooter.SpawnShotDirect(enemy.x - 0.06f, enemy.y + offsetY, ToRailZFromSideX(enemy.x),
                -AimedShotSpeed(), 0.0f, 0.0f, true, i, BulletCount);
        }
        shooter.PlayBossMachineGunSound();
    }

protected:
    int EntryCandidateCount(int) const override {
        return EdgeEntryCandidateCount();
    }

    EntryCandidate EntryCandidateAt(int index, int stageIndex) const override {
        const bool lateStage = stageIndex >= 2;
        constexpr EntryCandidate Stage1Candidates[] = {
            {1.10f, -0.78f, 0.86f, 48.0f},
            {1.12f, -0.18f, 0.32f, 42.0f},
            {1.14f, 0.35f, -0.32f, 54.0f},
            {1.16f, 0.78f, -0.86f, 36.0f}
        };
        constexpr EntryCandidate Stage2Candidates[] = {
            {1.10f, -0.82f, 0.86f, 50.0f},
            {1.12f, -0.28f, 0.32f, 40.0f},
            {1.14f, 0.28f, -0.32f, 56.0f},
            {1.16f, 0.82f, -0.86f, 34.0f}
        };
        if (lateStage) {
            return Stage2Candidates[index % (sizeof(Stage2Candidates) / sizeof(Stage2Candidates[0]))];
        }
        return Stage1Candidates[index % (sizeof(Stage1Candidates) / sizeof(Stage1Candidates[0]))];
    }

	// 同じ場所に出現している敵がいないか確認し、空いている候補を返す
    EntryCandidate SelectFreeCandidate(const SideScrollingShooter& shooter,
        const Enemy& spawningEnemy, int frame, int kills, int stageIndex) const {
        const int firstIndex = SelectEntryCandidateIndex(frame, kills, stageIndex);
        const int candidateCount = EntryCandidateCount(stageIndex);
        for (int offset = 0; offset < candidateCount; ++offset) {
            const EntryCandidate candidate = EntryCandidateAt(firstIndex + offset, stageIndex);
            if (!IsLaneOccupied(shooter, spawningEnemy, candidate.y)) {
                return candidate;
            }
        }
        return EntryCandidateAt(firstIndex, stageIndex);
    }

    bool IsLaneOccupied(const SideScrollingShooter& shooter,
        const Enemy& spawningEnemy, float laneY) const {
        for (const auto& enemy : shooter.m_enemies) {
            if (&enemy == &spawningEnemy || !enemy.active || enemy.type != Type()) {
                continue;
            }
            if (std::abs(enemy.baseY - laneY) < 0.05f) {
                return true;
            }
        }
        return false;
    }
};

/**
 * @brief 停止後に正方形型の拡散弾幕を撃つStage2用砲台機を制御する
 */
class SideScrollingShooter::SquareShooterEnemyBehavior final : public SideScrollingShooter::CircleShooterEnemyBehavior {
public:
    int Type() const override {
        return 6;
    }

	int MaxHp() const override {
		return 80;
	}

    int Score(const Enemy&) const override {
        return 340;
    }

    void FireSpecial(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (enemy.age < 36 || enemy.age % 72 != 0) {
            return;
        }

        if (shooter.IsRailGameplayActive()) {
            // 3Dでは同一点から5x5の格子へ徐々に拡散させる
            constexpr int GridSize = 5;
            constexpr int BulletCount = GridSize * GridSize;
            constexpr float SpreadSpeed = 0.010f;
            constexpr float RowScale = 1.4f;
            for (int i = 0; i < BulletCount; ++i) {
                const int column = i % GridSize;
                const int row = i / GridSize;
                const float vx = static_cast<float>(column - GridSize / 2) * SpreadSpeed;
                const float vy = static_cast<float>(row - GridSize / 2) * SpreadSpeed * RowScale;
                shooter.SpawnShotDirect(enemy.x, enemy.y, enemy.z,
                    vx, vy, -RingShotSpeed(), true, i, -BulletCount);
            }
            shooter.PlayBossMachineGunSound();
            return;
        }

        // 2Dでは同一点から縦1列5発へ徐々に拡散させる
        constexpr int BulletCount = 5;
        constexpr float SpreadSpeed = 0.010f;
        for (int i = 0; i < BulletCount; ++i) {
            const float vy = static_cast<float>(i - BulletCount / 2) * SpreadSpeed;
            shooter.SpawnShotDirect(enemy.x - 0.06f, enemy.y, ToRailZFromSideX(enemy.x),
                -AimedShotSpeed(), vy, 0.0f, true, i, -BulletCount);
        }
        shooter.PlayBossMachineGunSound();
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

    float AimedShotSpeed() const override {
        return 0.018f;
    }

    float RailAimedShotSpeed() const override {
        return 0.62f;
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
