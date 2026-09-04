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
    /** @brief 3Dモード時の自機狙い弾速度 */
    static constexpr float RailAimedShotSpeed3D = 0.22f;
    static_assert(RailAimedShotSpeed3D > 0.0f);

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
        return RailAimedShotSpeed3D;
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
    /** @brief 3Dモード時の自機狙い弾速度 */
    static constexpr float RailAimedShotSpeed3D = 0.28f;
    static_assert(RailAimedShotSpeed3D > 0.0f);

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
        return RailAimedShotSpeed3D;
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
    /** @brief 3Dモード時の自機狙い弾速度 */
    static constexpr float RailAimedShotSpeed3D = 0.24f;
    static_assert(RailAimedShotSpeed3D > 0.0f);

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
        return RailAimedShotSpeed3D;
    }

    int Score(const Enemy&) const override {
        return 320;
    }

    float RenderScale() const override {
        return 1.40f;
    }
};

/**
 * @brief 自機狙い弾を高頻度で発射する直進狙撃機を制御する
 */
class SideScrollingShooter::StraightShooterEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 3;
    }

    int MaxHp() const override {
        return 15;
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
        return 0.38f;
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
 * @brief 高所で停止してから自機位置へ降下突撃する敵を制御する
 */
class SideScrollingShooter::DiveRusherEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 7;
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
        enemy.baseY = HighY();
        enemy.y = enemy.baseY;
        enemy.phase = 0.0f;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (enemy.phase < 1.0f) {
            TickApproach(shooter, enemy);
            return;
        }
        if (enemy.phase < 2.0f) {
            TickStop(enemy);
            return;
        }
        if (enemy.phase < 3.0f) {
            TickDiveCurve(shooter, enemy);
            return;
        }
        TickRush(shooter, enemy);
    }

    int AimedShotInterval() const override {
        return 0;
    }

    int Score(const Enemy&) const override {
        return 220;
    }

    float RenderScale() const override {
        return 1.16f;
    }

    static constexpr float HighY() {
        return 0.96f;
    }

private:
    static constexpr float SideTriggerLeadX = 1.52f;
    static constexpr float RailTriggerLeadZ = 25.0f;
    static constexpr int StopFrames = 24;
    static constexpr int DiveCurveFrames = 42;
    static constexpr float SideApproachSpeed = 0.020f;
    static constexpr float RailApproachSpeed = 0.36f;
    static constexpr float SideRushSpeed = 0.034f;
    static constexpr float RailRushSpeed = 0.72f;
    static_assert(SideTriggerLeadX > 0.0f);
    static_assert(RailTriggerLeadZ > 0.0f);
    static_assert(StopFrames > 0);
    static_assert(DiveCurveFrames > 1);

    void TickApproach(SideScrollingShooter& shooter, Enemy& enemy) const {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= RailApproachSpeed;
            enemy.x = enemy.baseX;
            enemy.y = enemy.baseY;
            if (enemy.z <= SideScrollingShooter::PlayerRailZ + RailTriggerLeadZ) {
                BeginStop(shooter, enemy);
            }
            return;
        }

        enemy.x -= SideApproachSpeed;
        enemy.z = ToRailZFromSideX(enemy.x);
        enemy.y = enemy.baseY;
        if (enemy.x <= shooter.m_playerX + SideTriggerLeadX) {
            BeginStop(shooter, enemy);
        }
    }

    void BeginStop(const SideScrollingShooter& shooter, Enemy& enemy) const {
        enemy.phase = 1.0f;
        enemy.motionAge = 0;
        enemy.actionX = shooter.m_playerX;
        enemy.actionY = shooter.m_playerY;
        enemy.actionZ = shooter.IsRailGameplayActive() ?
            SideScrollingShooter::PlayerRailZ : ToRailZFromSideX(shooter.m_playerX);
    }

    void TickStop(Enemy& enemy) const {
        ++enemy.motionAge;
        if (enemy.motionAge < StopFrames) return;

        enemy.phase = 2.0f;
        enemy.motionAge = 0;
        enemy.turretAimX = enemy.x;
        enemy.turretAimY = enemy.y;
        enemy.turretAimZ = enemy.z;
    }

    void TickDiveCurve(SideScrollingShooter& shooter, Enemy& enemy) const {
        ++enemy.motionAge;
        const float progress = Math::Clamp01(
            static_cast<float>(enemy.motionAge) / static_cast<float>(DiveCurveFrames));
        const float horizontal = 1.0f - std::cos(progress * Math::HalfPi);
        const float vertical = std::sin(progress * Math::HalfPi);
        enemy.x = Math::Lerp(enemy.turretAimX, enemy.actionX, horizontal);
        enemy.y = Math::Lerp(enemy.turretAimY, enemy.actionY, vertical);
        enemy.z = Math::Lerp(enemy.turretAimZ, enemy.actionZ, horizontal);
        if (!shooter.IsRailGameplayActive()) {
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        if (enemy.motionAge < DiveCurveFrames) return;

        enemy.phase = 3.0f;
        enemy.x = enemy.actionX;
        enemy.y = enemy.actionY;
        enemy.z = enemy.actionZ;
    }

    void TickRush(SideScrollingShooter& shooter, Enemy& enemy) const {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= RailRushSpeed;
            return;
        }

        enemy.x -= SideRushSpeed;
        enemy.z = ToRailZFromSideX(enemy.x);
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
        return 30;
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
        if (!shooter.CanSpawnEnemyProjectile(enemy.x, enemy.y, enemy.z)) return;

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
		return 30;
	}

    int Score(const Enemy&) const override {
        return 340;
    }

    void FireSpecial(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (enemy.age < 36 || enemy.age % 72 != 0) {
            return;
        }
        if (!shooter.CanSpawnEnemyProjectile(enemy.x, enemy.y, enemy.z)) return;

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
 * @brief 下側で停止してからミサイルを定期発射する敵を制御する
 */
class SideScrollingShooter::MissileShooterEnemyBehavior final : public SideScrollingShooter::CircleShooterEnemyBehavior {
public:
    int Type() const override {
        return 8;
    }

    int MaxHp() const override {
        return 25;
    }

    int Score(const Enemy&) const override {
        return 300;
    }

    float RenderScale() const override {
        return 1.24f;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            // 3DではZ方向へ侵入して指定位置で停止する
            enemy.x = enemy.baseX;
            enemy.z = (std::max)(enemy.z - ApproachRailSpeed, StopRailZ);
            enemy.y = enemy.baseY;
            FireSpecial(shooter, enemy);
            return;
        }

        // 2Dでは右端から侵入して指定Xで停止する
        if (enemy.x > StopSideX) {
            enemy.x -= ApproachSideSpeed;
        } else {
            enemy.x = StopSideX;
        }
        enemy.z = ToRailZFromSideX(enemy.x);
        enemy.y = enemy.baseY;
        FireSpecial(shooter, enemy);
    }

    void FireSpecial(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (enemy.age < FirstMissileFrame || enemy.age % MissileIntervalFrames != 0) {
            return;
        }
        const float side = (enemy.age / MissileIntervalFrames) % 2 == 0 ? -1.0f : 1.0f;
        SpawnStage2DelayedMissile(shooter, enemy.x, enemy.y, enemy.z + side * MissileSideOffsetZ);
    }

    static constexpr float LowY() {
        return -0.92f;
    }

private:
    static constexpr float StopSideX = 1.75f;
    static constexpr float StopRailZ = SideScrollingShooter::PlayerRailZ + 24.0f;
    static constexpr float ApproachSideSpeed = 0.012f;
    static constexpr float ApproachRailSpeed = 0.30f;
    static constexpr int FirstMissileFrame = 72;
    static constexpr int MissileIntervalFrames = 96;
    static constexpr float MissileLaunchVelocity = 0.09f;
    static constexpr float MissileSideOffsetZ = 0.35f;
    static_assert(StopRailZ < SideScrollingShooter::EnemyRailFarZ);
    static_assert(FirstMissileFrame > 0);
    static_assert(MissileIntervalFrames > 0);

    /**
     * @brief Stage2ボスの遅延点火と同じ形式のミサイルを生成する
     * @param shooter 弾を生成するゲーム本体
     * @param x 発射X座標
     * @param y 発射Y座標
     * @param z 発射Z座標
     * @return なし
     */
    static void SpawnStage2DelayedMissile(
        SideScrollingShooter& shooter, float x, float y, float z) {
        if (!shooter.CanSpawnEnemyProjectile(x, y, z)) return;

        for (int shotIndex = 0; shotIndex < shooter.ActiveShotCapacity(); ++shotIndex) {
            auto& shot = shooter.m_shots[shotIndex];
            if (shot.active) continue;
            shot = {};
            shot.x = x;
            shot.y = y;
            shot.z = z;
            shot.transitionSideX = x;
            shot.transitionSideY = y;
            shot.vx = 0.0f;
            shot.vy = MissileLaunchVelocity;
            shot.vz = 0.0f;
            shot.hitRadius = 0.055f;
            shot.damage = 2;
            shot.enemy = true;
            shot.stage2.kind = ShooterStages::Stage2::ShotKind::Funnel;
            shot.stage2.delayedEngine = true;
            shot.active = true;
            shooter.PlayMissileLaunchSound();
            return;
        }
    }
};

/**
 * @brief 上下2機を接続レーザーで結ぶ敵を制御する
 */
class SideScrollingShooter::LinkedLaserEnemyBehavior final : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 9;
    }

    int MaxHp() const override {
        return 100;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (shooter.IsRailGameplayActive()) {
            enemy.z -= RailMoveSpeed;
            enemy.x = enemy.baseX;
        } else {
            enemy.x -= SideMoveSpeed;
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        enemy.y = enemy.baseY;
    }

    int AimedShotInterval() const override {
        return 0;
    }

    int Score(const Enemy&) const override {
        return 260;
    }

    float RenderScale() const override {
        return 1.08f;
    }

    static constexpr float UpperY() {
        return UpperYValue;
    }

    static constexpr float LowerY() {
        return LowerYValue;
    }

    static constexpr float LaserRadius2D() {
        return LaserRadius2DValue;
    }

    static constexpr float LaserRadius3D() {
        return LaserRadius3DValue;
    }

    static constexpr float LeftRailX() {
        return LeftRailXValue;
    }

    static constexpr float RightRailX() {
        return RightRailXValue;
    }

private:
    static constexpr float UpperYValue = 1.50f;
    static constexpr float LowerYValue = -1.55f;
    static constexpr float LeftRailXValue = -1.0f;
    static constexpr float RightRailXValue = 1.0f;
    static constexpr float LaserRadius2DValue = 0.035f;
    static constexpr float LaserRadius3DValue = 0.24f;
    static constexpr float SideMoveSpeed = 0.012f;
    static constexpr float RailMoveSpeed = 0.24f;
    static_assert(UpperYValue > LowerYValue);
    static_assert(LeftRailXValue < RightRailXValue);
    static_assert(LaserRadius2DValue > 0.0f);
    static_assert(LaserRadius3DValue > 0.0f);
    static_assert(SideMoveSpeed > 0.0f);
    static_assert(RailMoveSpeed > 0.0f);
};

/**
 * @brief NEO AIZU外壁へ接近して左右を巡回する警備ドローンを制御する
 */
class SideScrollingShooter::WallSecurityDroneEnemyBehavior final
    : public SideScrollingShooter::EnemyBehavior {
public:
    int Type() const override {
        return 10;
    }

    int MaxHp() const override {
        return 65;
    }

    void Tick(SideScrollingShooter& shooter, Enemy& enemy) const override {
        if (!shooter.IsRailGameplayActive()) {
            // 2DではPANDD会ビル側へ固定し、外壁上だけを上下巡回する
            enemy.x = SideWallX;
            enemy.y = enemy.baseY +
                std::cos(enemy.phase + static_cast<float>(enemy.age) * PatrolSpeed) * PatrolHeight;
            enemy.z = ToRailZFromSideX(enemy.x);
        } else {
            // 3Dではビル上空から壁面を伝って降下し、到着後に楕円巡回へ移る
            enemy.z = WallPatrolZ;
            const float patrolAge = static_cast<float>(enemy.age);
            const float patrolX = enemy.baseX +
                std::sin(enemy.phase + patrolAge * PatrolSpeed) * PatrolWidth;
            const float patrolY = enemy.baseY +
                std::cos(enemy.phase + patrolAge * PatrolSpeed * 1.35f) * PatrolHeight;
            if (enemy.entersWallFromTop) {
                const float progress =
                    ShooterStages::Stage5::Part2RailDroneEntryProgress(enemy.age);
                enemy.x = Math::Lerp(enemy.baseX, patrolX, progress);
                enemy.y = Math::Lerp(
                    ShooterStages::Stage5::Part2RailDroneEntryY, patrolY, progress);
                if (progress < 1.0f) return;
                enemy.entersWallFromTop = false;
            } else {
                enemy.x = patrolX;
                enemy.y = patrolY;
            }
        }

        // 接触時の照準を連射終了まで固定し、終了後は走査へ戻す
        TickSearchlightAttack(shooter, enemy);
    }

    int AimedShotInterval() const override {
        return 0;
    }

    float RailAimedShotSpeed() const override {
        return 0.52f;
    }

    int Score(const Enemy&) const override {
        return 450;
    }

    float CollisionRadius(const Enemy&) const override {
        return 0.12f;
    }

    float CollisionRadius3D(const Enemy&) const override {
        return 1.15f;
    }

    float ShotHitRadius3D(const Enemy&) const override {
        return 1.05f;
    }

    float RenderScale() const override {
        return 1.45f;
    }

    /**
     * @brief 3D外壁表面の固定Z座標を取得する
     * @return 外壁表面のレール座標Z
     */
    static constexpr float WallSurfaceZ() {
        return WallPatrolZ;
    }

private:
    /**
     * @brief レーザーポインター走査と接触後のマシンガン連射を更新する
     * @param shooter ゲーム本体
     * @param enemy 更新する壁面警備ドローン
     * @return なし
     */
    static void TickSearchlightAttack(SideScrollingShooter& shooter, Enemy& enemy) {
        using namespace ShooterStages::Stage5;

        // 連射中は接触時の方向を維持して一定間隔で発射する
        if (enemy.motionAge > 0) {
            enemy.turretAimX = enemy.attackWarningTargetX;
            enemy.turretAimY = enemy.attackWarningTargetY;
            enemy.attackWarningFrames = enemy.motionAge;
            if (IsDroneMachineGunFireFrame(enemy.motionAge)) {
                FireMachineGun(shooter, enemy);
            }
            if (--enemy.motionAge == 0) {
                enemy.recoilAge = DroneMachineGunCooldownFrames;
                enemy.attackWarningFrames = 0;
            }
            return;
        }

        // 画面内を巡回するレーザーポインターの照射地点を更新する
        const float scanAge = static_cast<float>(enemy.age);
        enemy.turretAimX = std::sin(enemy.phase + scanAge * 0.031f) * 0.88f;
        const float aimWave = std::sin(enemy.phase * 1.7f + scanAge * 0.023f);
        enemy.turretAimY = shooter.IsRailGameplayActive() ?
            Part2RailDroneAimY(aimWave) : aimWave * 0.70f;
        if (enemy.recoilAge > 0) {
            --enemy.recoilAge;
            return;
        }

        // ポインターへ自機が触れた瞬間の地点を連射方向として保存する
        if (!DroneSearchlightTouches(shooter.m_playerX, shooter.m_playerY,
                enemy.turretAimX, enemy.turretAimY, 0.055f)) return;
        enemy.attackWarningTargetX = shooter.m_playerX;
        enemy.attackWarningTargetY = shooter.m_playerY;
        enemy.motionAge = DroneMachineGunBurstFrames;
        enemy.attackWarningFrames = enemy.motionAge;
        FireMachineGun(shooter, enemy);
        --enemy.motionAge;
    }

    /**
     * @brief 接触時に保存した方向へマシンガン弾を一発生成する
     * @param shooter ゲーム本体
     * @param enemy 発射する壁面警備ドローン
     * @return なし
     */
    static void FireMachineGun(SideScrollingShooter& shooter, const Enemy& enemy) {
        if (!shooter.CanSpawnEnemyProjectile(enemy.x, enemy.y, enemy.z)) return;

        const float dx = enemy.attackWarningTargetX - enemy.x;
        const float dy = enemy.attackWarningTargetY - enemy.y;
        const int shotIndex = (ShooterStages::Stage5::DroneMachineGunBurstFrames -
            enemy.motionAge) / ShooterStages::Stage5::DroneMachineGunShotIntervalFrames;
        if (!shooter.IsRailGameplayActive()) {
            const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy));
            shooter.SpawnShotDirect(enemy.x - 0.06f, enemy.y, enemy.z,
                dx / length * SideShotSpeed, dy / length * SideShotSpeed, 0.0f,
                true, shotIndex, 0);
            shooter.PlayBossMachineGunSound();
            return;
        }

        // 3Dではゲーム座標のXYをワールド比率へ変換して固定地点へ飛ばす
        const float worldDx = ToWorldX(enemy.attackWarningTargetX) - ToWorldX(enemy.x);
        const float worldDy = ToWorldY(enemy.attackWarningTargetY) - ToWorldY(enemy.y);
        const float worldDz = shooter.PlayerRailDepth() - enemy.z;
        const float length = (std::max)(0.001f,
            std::sqrt(worldDx * worldDx + worldDy * worldDy + worldDz * worldDz));
        shooter.SpawnShotDirect(enemy.x, enemy.y, enemy.z,
            FromWorldX(worldDx / length * RailShotSpeed),
            FromWorldY(worldDy / length * RailShotSpeed),
            worldDz / length * RailShotSpeed, true, shotIndex, 0);
        shooter.PlayBossMachineGunSound();
    }

    static constexpr float WallPatrolZ = 40.8f;
    static constexpr float SideWallX = 1.55f;
    static constexpr float PatrolSpeed = 0.024f;
    static constexpr float PatrolWidth = 0.34f;
    static constexpr float PatrolHeight = 0.22f;
    static constexpr float SideShotSpeed = 0.022f;
    static constexpr float RailShotSpeed = 0.68f;
    static_assert(WallPatrolZ > ShooterStages::Stage5::Part2PlayerRailZ);
    static_assert(SideWallX > 0.0f);
    static_assert(PatrolWidth > 0.0f && PatrolHeight > 0.0f);
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
