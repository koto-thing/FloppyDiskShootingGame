#pragma once

#include <cmath>

#include "SideScrollingShooterStages.h"

/**
 * @brief ステージ2の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage2EnemySheet : public SideScrollingShooter::Stage {
public:
    /** @brief ステージ2ボス機体で個別に破壊できる部位 */
    enum BossPart {
        BossNose,
        BossLeftWing,
        BossRightWing,
        BossLeftEngine,
        BossRightEngine,
        BossPartCount
    };
    static_assert(BossPartCount == 5);

    /** @brief ステージ2ボス戦の攻撃フェーズ */
    enum BossPhase {
        BossNormalPhase1,
        BossNormalPhase2,
        BossPhaseCount
    };
    static_assert(BossPhaseCount == 2);

    static constexpr int BossPhaseHp[] = {480, 240};
    static constexpr float BossSideEntryX = 3.20f;
    static constexpr float BossSideAnchorX = 1.05f;
    static constexpr float BossRailEntryZ = 88.0f;
    static constexpr float BossRailAnchorZ = 52.0f;
    static constexpr float BossAnchorY = -0.58f;
    static constexpr float BossSideYaw = 0.0f;
    static constexpr float BossRailYaw = -1.57079632679f;
    static constexpr float BossSideIdleBackDistance = 0.32f;
    static constexpr float BossRailIdleBackDistance = 10.2f;
    static constexpr int BossIdleMoveFrames = 75;
    static constexpr int BossIdleBackHoldFrames = 140;
    static constexpr int BossIdleFrontHoldFrames = 140;
    static constexpr int BossIdleMotionFrames =
        BossIdleMoveFrames * 2 + BossIdleBackHoldFrames + BossIdleFrontHoldFrames;
    static constexpr int BossTransitionHoldFrames = 90;
    static constexpr int BossKnifeAttackIntervalFrames = 400;
    static constexpr int BossCannonKnifeAttackIntervalFrames = 180;
    static constexpr float BossCannonKnifeHitRadius = 0.13f;
    static constexpr float BossCannonKnifeSpeed = 0.024f;
    static constexpr int BossCannonKnifeBurstCount = 14;
    static constexpr float BossCannonKnifeBurstSpeed = 0.022f;

    int StageIndex() const override {
        return 2;
    }

    int BossPartTotal() const override { return BossPartCount; }
    int BossPhaseTotal() const override { return BossPhaseCount; }
    int BossInitialPhase() const override { return BossNormalPhase1; }
    int BossNosePart() const override { return BossNose; }
    int BossLeftWingPart() const override { return BossLeftWing; }
    int BossRightWingPart() const override { return BossRightWing; }
    int BossLeftEnginePart() const override { return BossLeftEngine; }
    int BossRightEnginePart() const override { return BossRightEngine; }
    const char* BossPhaseLabel(int phase) const override {
        constexpr const char* PhaseLabels[] = {
            "NORMAL 1", "NORMAL 2"
        };
        return PhaseLabels[phase % BossPhaseCount];
    }

    /**
     * @brief ステージ2ボスの2D表示時初期Y軸回転を取得する
     * @return Y軸回転角度
     */
    float BossSideModelYaw() const override {
        return BossSideYaw;
    }

    /**
     * @brief ステージ2ボスの3D表示時初期Y軸回転を取得する
     * @return Y軸回転角度
     */
    float BossRailModelYaw() const override {
        return BossRailYaw;
    }

    /**
     * @brief ステージ2ボス部位ソケットのローカル座標倍率を取得する
     * @return ローカル座標からワールド座標への倍率
     */
    float BossPartSocketScale() const override {
        return 1.92f;
    }

    /**
     * @brief ステージ2ボスの指定部位ソケットを取得する
     * @param part 取得するボス部位
     * @return 巨大艦モデル基準の部位ソケット
     */
    BossPartSocket GetBossPartSocket(int part) const override {
        constexpr BossPartSocket Sockets[] = {
            {-5.70f, 2.86f, 0.00f, 2.30f},
            {-0.60f, 1.18f, 1.74f, 1.95f},
            {-0.60f, 1.18f, -1.74f, 1.95f},
            {4.80f, -0.46f, 1.42f, 1.55f},
            {4.80f, -0.46f, -1.42f, 1.55f}
        };
        return Sockets[part % BossPartCount];
    }

    /**
     * @brief ステージ2ボスの最大HPを取得する
     * @return ステージ2ボスの最大HP
     */
    int BossMaxHp() const override {
        return BossPhaseHp[0];
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
        EnemyBehaviorForType(enemyType).ConfigureSpawn(shooter, enemy, frame, kills, railMode, 1);
    }

    /**
     * @brief ステージ2巨大艦ボスの生成位置を設定する
     * @param boss 生成するボス
     * @param railMode レール表示中か
     * @return なし
     */
    void ConfigureBoss(Enemy& boss, bool railMode) const override {
        BossEnemyBehaviorInstance().ConfigureBossSpawn(boss, railMode, StageIndex());
        boss.hp = BossMaxHp();
        boss.maxHp = BossMaxHp();
        boss.x = railMode ? 0.0f : BossSideEntryX;
        boss.y = BossAnchorY;
        boss.z = railMode ? BossRailEntryZ : ToRailZFromSideX(boss.x);
        boss.baseX = railMode ? 0.0f : BossSideAnchorX;
        boss.baseY = BossAnchorY;
        boss.baseZ = railMode ? BossRailAnchorZ : ToRailZFromSideX(BossSideAnchorX);
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.phase = 0.0f;
        boss.motionAge = 0;
    }

    /**
     * @brief ステージ2ボスの3Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    void ConfigureBossRailAnchor(Enemy& boss) const override {
        boss.x = 0.0f;
        boss.y = BossAnchorY;
        boss.z = BossRailAnchorZ;
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.phase = -static_cast<float>(BossTransitionHoldFrames);
        boss.motionAge = 0;
    }

    /**
     * @brief ステージ2ボスの2Dモード基準点を設定する
     * @param boss 基準点へ移動するボス
     * @return なし
     */
    void ConfigureBossSideAnchor(Enemy& boss) const override {
        boss.x = BossSideAnchorX;
        boss.y = BossAnchorY;
        boss.z = ToRailZFromSideX(boss.x);
        boss.baseX = boss.x;
        boss.baseY = boss.y;
        boss.baseZ = boss.z;
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
        boss.phase = -static_cast<float>(BossTransitionHoldFrames);
        boss.motionAge = 0;
    }

    /**
     * @brief ステージ2ボスの部位HPを設定する
     * @param boss 部位HPを設定するボス
     * @return なし
     */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = { 180, 240, 240, 160, 160 };
    }

    /**
     * @brief ステージ2ボスの登場移動を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        // モード切替直後は基準点で待機し、補間中の目標位置を動かさない
        if (boss.phase < 0.0f) {
            boss.x = boss.baseX;
            boss.y = boss.baseY;
            boss.z = shooter.IsRailGameplayActive() ? boss.baseZ : ToRailZFromSideX(boss.baseX);
            boss.phase += 1.0f;
            if (boss.phase >= 0.0f) {
                boss.phase = 1.0f;
            }
            return;
        }

        // 2Dでは画面右側、3Dでは奥から基準点へゆっくり進入する
        if (shooter.IsRailGameplayActive()) {
            if (boss.phase == 0.0f && boss.z > boss.baseZ) {
                boss.z = (std::max)(boss.baseZ, boss.z - 0.20f);
            }
            if (boss.phase == 0.0f && boss.x > boss.baseX) {
                boss.x = (std::max)(boss.baseX, boss.x - 0.012f);
            } else if (boss.phase == 0.0f && boss.x < boss.baseX) {
                boss.x = (std::min)(boss.baseX, boss.x + 0.012f);
            }
            if (boss.z <= boss.baseZ && boss.x == boss.baseX) {
                boss.phase = 1.0f;
                boss.motionAge = 0;
            }
            if (boss.phase > 0.0f) {
                boss.z = boss.baseZ + BossRailIdleBackDistance * IdleMotionRate(++boss.motionAge);
            }
        } else {
            if (boss.phase == 0.0f && boss.x > boss.baseX) {
                boss.x = (std::max)(boss.baseX, boss.x - 0.012f);
            }
            if (boss.x <= boss.baseX) {
                boss.phase = 1.0f;
                boss.motionAge = 0;
            }
            if (boss.phase > 0.0f) {
                boss.x = boss.baseX + BossSideIdleBackDistance * IdleMotionRate(++boss.motionAge);
            }
            boss.z = ToRailZFromSideX(boss.x);
        }
        boss.y = boss.baseY;
    }


    /**
     * @brief ステージ2ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(int phase) const override {
        return phase == BossNormalPhase1 ? 144 : 104;
    }

    /**
     * @brief ステージ2ボスの本体HPから攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return ステージ2ボスの攻撃フェーズ
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        (void)maxHp;
        if (hp > BossPhaseHp[1]) return BossNormalPhase1;
        return BossNormalPhase2;
    }

    /**
     * @brief ステージ2ボス部位破壊時に本体へ与えるダメージを取得する
     * @param part 破壊されたボス部位
     * @return 本体へ与えるダメージ
     */
    int BossPartBreakDamage(int part) const override {
        (void)part;
        return 90;
    }

    /**
     * @brief ステージ2ボス本体のメイン攻撃間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射しない場合0、発射する場合は間隔フレーム
     */
    int BossMainAttackInterval(int phase) const override {
        return phase == BossNormalPhase1 ? BossKnifeAttackIntervalFrames : BossCannonKnifeAttackIntervalFrames;
    }

    /**
     * @brief ステージ2ボス本体の回転ナイフ弾を主砲から発射する
     * @param shooter ゲーム本体
     * @param boss 発射元のボス
     * @return なし
     */
    void FireBossMainAttack(SideScrollingShooter& shooter, const Enemy& boss) const override {
        const BossPartSocket socket = GetBossPartSocket(BossNose);
        if (boss.bossPhase == BossNormalPhase1) {
            shooter.SpawnBossKnifeShot(boss, socket.localX, socket.localY, socket.localZ, BossPartSocketScale());
            return;
        }
        if (!shooter.IsRailGameplayActive()) {
            shooter.SpawnBossCannonKnifeShot(boss, socket.localX, socket.localY, socket.localZ,
                BossPartSocketScale(), BossCannonKnifeHitRadius, BossCannonKnifeSpeed,
                BossCannonKnifeBurstCount, BossCannonKnifeBurstSpeed);
        }
    }

    int BossBulletCount(bool railMode) const override {
        (void)railMode;
        return 5;
    }

    /**
     * @brief ステージ2ボス弾幕の指定弾を取得する
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossBullet(int index, bool railMode) const override {
        if (railMode) {
            constexpr BossBullet RailPattern[] = {
                {0.0f, 0.0f, -0.016f, -0.020f},
                {0.0f, 0.0f, -0.008f, -0.010f},
                {0.0f, 0.0f, 0.000f, 0.000f},
                {0.0f, 0.0f, 0.008f, 0.010f},
                {0.0f, 0.0f, 0.016f, 0.020f}
            };
            return RailPattern[index % 5];
        }

        constexpr BossBullet SidePattern[] = {
            {-0.18f, 0.0f, -0.016f, -0.018f},
            {-0.18f, 0.0f, -0.020f, -0.009f},
            {-0.18f, 0.0f, -0.023f, 0.000f},
            {-0.18f, 0.0f, -0.020f, 0.009f},
            {-0.18f, 0.0f, -0.016f, 0.018f}
        };
        return SidePattern[index % 5];
    }

    /**
     * @brief ステージ2ボスの指定部位・フェーズの弾数を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param railMode レール表示中か
     * @return 発射する弾数
     */
    int BossPartBulletCount(int part, int phase, bool railMode) const override {
        if (part == BossNose) return BossBulletCount(railMode);
        if (part == BossLeftWing || part == BossRightWing) {
            return phase == BossNormalPhase2 ? 3 : 2;
        }
        return phase == BossNormalPhase2 ? 2 : 1;
    }

    /**
     * @brief ステージ2ボスの指定部位・フェーズの弾を取得する
     * @param part 発射するボス部位
     * @param phase 現在のボス攻撃フェーズ
     * @param index 弾幕内の弾番号
     * @param railMode レール表示中か
     * @return 発射位置オフセットと速度
     */
    BossBullet GetBossPartBullet(int part, int phase, int index, bool railMode) const override {
        const int baseCount = BossBulletCount(railMode);
        const int patternIndex = part == BossNose ? index :
            (part == BossLeftWing ? 0 : (part == BossRightWing ? baseCount - 1 : baseCount / 2));
        BossBullet bullet = GetBossBullet(patternIndex, railMode);
        if (phase == BossNormalPhase2) {
            bullet.vx *= 1.35f;
            bullet.vy += (index - BossPartBulletCount(part, phase, railMode) / 2) * 0.010f;
        }
        return bullet;
    }

private:
    /**
     * @brief 後退、後方停止、前進、前方停止を行う待機モーションの割合を取得する
     * @param frame 待機モーション開始後フレーム
     * @return 基準点から後退方向への割合
     */
    static float IdleMotionRate(int frame) {
        const int cycleFrame = frame % BossIdleMotionFrames;
        if (cycleFrame < BossIdleMoveFrames) {
            return EaseOut(static_cast<float>(cycleFrame) / static_cast<float>(BossIdleMoveFrames));
        }
        const int backHoldEnd = BossIdleMoveFrames + BossIdleBackHoldFrames;
        if (cycleFrame < backHoldEnd) {
            return 1.0f;
        }
        const int forwardFrame = cycleFrame - backHoldEnd;
        if (forwardFrame < BossIdleMoveFrames) {
            return 1.0f - EaseOut(static_cast<float>(forwardFrame) / static_cast<float>(BossIdleMoveFrames));
        }
        return 0.0f;
    }

    /**
     * @brief 0から1の値へEaseOutを適用する
     * @param value 補間率
     * @return EaseOut後の補間率
     */
    static float EaseOut(float value) {
        return 1.0f - (1.0f - value) * (1.0f - value);
    }
};
