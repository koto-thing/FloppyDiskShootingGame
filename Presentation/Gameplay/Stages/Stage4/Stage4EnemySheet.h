#pragma once

#include <cmath>

#include "../Common/StageDefinition.h"
#include "Stage4Module.h"

/**
 * @brief ステージ4の難易度共通設定を定義する
 */
class SideScrollingShooter::Stage4EnemySheet : public SideScrollingShooter::Stage {
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
        EnemySpawnRule& spawn, int& chapterNumber) const override = 0;

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
        const float rushPhase = boss.phase;
        const int recoilAge = boss.recoilAge;
        const int recoilType = boss.recoilType;
        const bool rushing = IsRushPhase(static_cast<BossPhase>(boss.bossPhase)) && rushPhase > 0.0f;
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
        boss.recoilAge = recoilAge;
        boss.recoilType = recoilType;
        if (rushing) {
            boss.phase = rushPhase;
            ApplyRushPosition(boss, true, static_cast<int>(rushPhase));
        }
    }

    /** @brief Stage 4ボスの2D基準点を設定する @param boss 設定するボス @return なし */
    void ConfigureBossSideAnchor(Enemy& boss) const override {
        const float rushPhase = boss.phase;
        const int recoilAge = boss.recoilAge;
        const int recoilType = boss.recoilType;
        const bool rushing = IsRushPhase(static_cast<BossPhase>(boss.bossPhase)) && rushPhase > 0.0f;
        boss.x = 1.80f;
        // 履帯を道路帯の内側へ収めて路面上を走って見える高さに置く
        boss.y = -0.80f;
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
        boss.recoilAge = recoilAge;
        boss.recoilType = recoilType;
        if (rushing) {
            boss.phase = rushPhase;
            ApplyRushPosition(boss, false, static_cast<int>(rushPhase));
        }
    }

    /** @brief Stage 4ボスの砲部位HPを設定する @param boss 設定するボス @return なし */
    void ConfigureBossPartHp(Enemy& boss) const override {
        boss.bossPartHp = {260, 0, 0, 0, 0,
            90, 90, 90, 90, 90, 90,
            0, 0, 0, 0, 0, 0};
    }

    /**
     * @brief 本体HPからStage 4固有の三段階攻撃フェーズを取得する
     * @param hp 現在HP
     * @param maxHp 最大HP
     * @return Phase 1からPhase 3に対応する番号
     */
    int BossPhaseForHp(int hp, int maxHp) const override {
        return ShooterStages::Stage4::BossPhaseForHp(hp, maxHp);
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

        // 交換中は戦車を基準位置へ止めて攻撃用の移動処理を行わない
        if (Stage4Module::TickWeaponSwap(shooter, boss)) {
            boss.motionAge = 0;
            boss.phase = 0.0f;
            return;
        }

        // Phase1は一定間隔で後退、突進、待機、復帰を行う
        Stage4Module::TickSiegeMortarAim(shooter);
        bool rushedThisFrame = false;
        if (IsRushPhase(static_cast<BossPhase>(boss.bossPhase)) &&
            (boss.phase > 0.0f || boss.age % RushIntervalFrames == 0)) {
            int rushFrame = static_cast<int>(boss.phase);
            if (rushFrame <= 0) {
                boss.actionX = boss.baseX;
                boss.actionY = boss.baseY;
                boss.actionZ = boss.baseZ;
                shooter.m_stage4.rushAimX = boss.turretAimX;
                shooter.m_stage4.rushAimY = boss.turretAimY;
                shooter.m_stage4.rushAimZ = boss.turretAimZ;
                rushFrame = 1;
            }

            boss.motionAge = 0;
            ApplyRushPosition(boss, shooter.IsRailGameplayActive(), rushFrame);
            rushedThisFrame = true;

            boss.phase = rushFrame < RushTotalFrames ?
                static_cast<float>(rushFrame + 1) : 0.0f;
        } else if (shooter.m_stage4.currentWeapon == ShooterStages::Stage4::MainWeaponType::SiegeMortar) {
            boss.phase = 0.0f;
            ApplySiegeMortarBodyMotion(boss, shooter.IsRailGameplayActive());
        } else {
            boss.motionAge = 0;
            boss.phase = 0.0f;
            boss.x = boss.baseX;
            boss.y = boss.baseY;
            boss.z = shooter.IsRailGameplayActive() ? boss.baseZ : ToRailZFromSideX(boss.x);
            boss.actionX = boss.x;
            boss.actionY = boss.y;
            boss.actionZ = boss.z;
        }
        ApplyMainCannonRecoil(boss, shooter.IsRailGameplayActive());

        // 砲の通常照準は次の攻撃に向けて自機位置へ追従する
        if (rushedThisFrame) return;
        boss.turretAimX += (shooter.m_playerX - boss.turretAimX) * TurretTrackingRate;
        boss.turretAimY += (shooter.m_playerY - boss.turretAimY) * TurretTrackingRate;
        const float targetZ = shooter.IsRailGameplayActive() ?
            PlayerRailZ : ToRailZFromSideX(shooter.m_playerX);
        boss.turretAimZ += (targetZ - boss.turretAimZ) * TurretTrackingRate;
    }

    /**
     * @brief Stage 4ボスが突進攻撃中か取得する
     * @param shooter ゲーム本体
     * @param boss 判定するボス
     * @return 突進攻撃中の場合true、通常攻撃中の場合false
     */
    bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss) const override {
        return Stage4Module::IsWeaponSwapActive(shooter) ||
            (IsRushPhase(static_cast<BossPhase>(boss.bossPhase)) && boss.phase > 0.0f);
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
            (void)phase;
            return 1;
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

private:
    inline static constexpr int RushIntervalFrames = 700;
    inline static constexpr int RushWindupFrames = 42;
    inline static constexpr int RushChargeFrames = 61;
    inline static constexpr int RushWaitFrames = 48;
    inline static constexpr int RushReturnFrames = 96;
    inline static constexpr int RushTotalFrames = RushWindupFrames + RushChargeFrames +
        RushWaitFrames + RushReturnFrames;
    inline static constexpr float SideBackDistance = 0.28f;
    inline static constexpr float SideRushEndX = -3.35f;
    inline static constexpr float RailBackDistance = 8.0f;
    inline static constexpr float RailRushEndZ = -30.0f;
    inline static constexpr int MainCannonRecoilFrames = 40;
    inline static constexpr int RomanceCannonRecoilFrames = 158;
    inline static constexpr float SideMainCannonRecoilDistance = 0.10f;
    inline static constexpr float RailMainCannonRecoilDistance = 1.40f;
    inline static constexpr float SideRomanceCannonRecoilDistance = 1.64f;
    inline static constexpr float RailRomanceCannonRecoilDistance = 20.10f;
    inline static constexpr float SiegeBodyMotionRate = 0.045f;
    inline static constexpr float SideSiegeBodyMotionDistance = 0.35f;
    inline static constexpr float RailSiegeBodyMotionDistance = 3.50f;
    inline static constexpr int BossAttackIntervalFrames = 150;
    inline static constexpr int RomanceCannonAttackIntervalFrames = 260;

    /**
     * @brief 突進攻撃を行うフェーズか取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return Phase1の場合true、それ以外の場合false
     */
    static constexpr bool IsRushPhase(BossPhase phase) {
        return phase == BossNormalPhase1;
    }
    static_assert(BossNormalPhase1 != BossSpecialPhase1);

    /**
     * @brief 突進フレームから現在表示モードの突進位置を反映する
     * @param boss 更新するボス
     * @param railMode 3Dレール表示中か
     * @param rushFrame 現在の突進フレーム
     * @return なし
     */
    static void ApplyRushPosition(Enemy& boss, bool railMode, int rushFrame) {
        boss.y = boss.actionY;
        if (railMode) {
            boss.x = boss.actionX;
            if (rushFrame <= RushWindupFrames) {
                const float t = static_cast<float>(rushFrame) /
                    static_cast<float>(RushWindupFrames);
                boss.z = boss.actionZ + RailBackDistance * t;
            } else if (rushFrame <= RushWindupFrames + RushChargeFrames) {
                const float t = static_cast<float>(rushFrame - RushWindupFrames) /
                    static_cast<float>(RushChargeFrames);
                boss.z = boss.actionZ + RailBackDistance +
                    (RailRushEndZ - boss.actionZ - RailBackDistance) * t * t;
            } else if (rushFrame <= RushWindupFrames + RushChargeFrames + RushWaitFrames) {
                boss.z = RailRushEndZ;
            } else {
                const float t = static_cast<float>(rushFrame - RushWindupFrames -
                    RushChargeFrames - RushWaitFrames) /
                    static_cast<float>(RushReturnFrames);
                boss.z = RailRushEndZ + (boss.actionZ - RailRushEndZ) * t;
            }
            return;
        }

        if (rushFrame <= RushWindupFrames) {
            const float t = static_cast<float>(rushFrame) /
                static_cast<float>(RushWindupFrames);
            boss.x = boss.actionX + SideBackDistance * t;
        } else if (rushFrame <= RushWindupFrames + RushChargeFrames) {
            const float t = static_cast<float>(rushFrame - RushWindupFrames) /
                static_cast<float>(RushChargeFrames);
            boss.x = boss.actionX + SideBackDistance +
                (SideRushEndX - boss.actionX - SideBackDistance) * t * t;
        } else if (rushFrame <= RushWindupFrames + RushChargeFrames + RushWaitFrames) {
            boss.x = SideRushEndX;
        } else {
            const float t = static_cast<float>(rushFrame - RushWindupFrames -
                RushChargeFrames - RushWaitFrames) /
                static_cast<float>(RushReturnFrames);
            boss.x = SideRushEndX + (boss.actionX - SideRushEndX) * t;
        }
        boss.z = ToRailZFromSideX(boss.x);
    }

    /**
     * @brief SiegeMortar形態の車体往復移動を反映する
     * @param boss 更新するボス
     * @param railMode レール表示中か
     * @return なし
     */
    static void ApplySiegeMortarBodyMotion(Enemy& boss, bool railMode) {
        const float motion = std::sin(static_cast<float>(++boss.motionAge) *
            SiegeBodyMotionRate);
        boss.y = boss.baseY;
        if (railMode) {
            boss.x = boss.baseX;
            boss.z = boss.baseZ + motion * RailSiegeBodyMotionDistance;
        } else {
            boss.x = boss.baseX + motion * SideSiegeBodyMotionDistance;
            boss.z = ToRailZFromSideX(boss.x);
        }
        boss.actionX = boss.x;
        boss.actionY = boss.y;
        boss.actionZ = boss.z;
    }

    /**
     * @brief 主砲発射反動をボス本体位置へ加算する
     * @param boss 反動を適用するボス
     * @param railMode レール表示中か
     * @return なし
     */
    static void ApplyMainCannonRecoil(Enemy& boss, bool railMode) {
        if (boss.recoilAge == 0) return;

        // 主砲3は高速にのけぞり、等速でゆっくり戻る
        const bool romance = boss.recoilType == 1;
        const int recoilFrames = romance ? RomanceCannonRecoilFrames : MainCannonRecoilFrames;
        const float sideDistance = romance ?
            SideRomanceCannonRecoilDistance : SideMainCannonRecoilDistance;
        const float railDistance = romance ?
            RailRomanceCannonRecoilDistance : RailMainCannonRecoilDistance;
        const int frame = recoilFrames - boss.recoilAge + 1;
        const float t = static_cast<float>(frame) /
            static_cast<float>(recoilFrames);
        const float recoilTime = 0.22f;
        const float easeOut = 1.0f - (1.0f - Math::Clamp01(t / recoilTime)) *
            (1.0f - Math::Clamp01(t / recoilTime));
        const float returnLinear = 1.0f - Math::Clamp01((t - recoilTime) / (1.0f - recoilTime));
        const float offset = romance ? (t < recoilTime ? easeOut : returnLinear) :
            SmoothStep(Math::Clamp01(t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f));
        if (railMode) {
            boss.z += railDistance * offset;
        } else {
            boss.x += sideDistance * offset;
            boss.z = ToRailZFromSideX(boss.x);
        }
        if (--boss.recoilAge <= 0) {
            boss.recoilAge = 0;
            boss.recoilType = 0;
        }
    }

    /**
     * @brief ステージ4ボス弾幕の発射間隔を取得する
     * @param phase 現在のボス攻撃フェーズ
     * @return 発射間隔
     */
    int BossAttackInterval(BossPhase phase) const override {
        return phase == BossNormalPhase2 ?
            RomanceCannonAttackIntervalFrames : BossAttackIntervalFrames;
    }

public:
    /**
     * @brief 主砲種別に対応する反動フレーム数を取得する
     * @param weapon 主砲種別
     * @return 反動フレーム数
     */
    static constexpr int MainCannonRecoilFramesForWeapon(
        ShooterStages::Stage4::MainWeaponType weapon) {
        return weapon == ShooterStages::Stage4::MainWeaponType::RomanceCannon ?
            RomanceCannonRecoilFrames : MainCannonRecoilFrames;
    }
};
