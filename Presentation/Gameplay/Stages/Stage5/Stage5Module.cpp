#include "Stage5Module.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../../../Engine/Input/Input.h"
#include "../../../../Engine/Input/KeyCode.h"
#include "../../../../Infrastructure/ExternalServices/AudioService.h"
#include "../../SideScrollingShooterShared.h"
#include "../../Voices/VoiceDpcmDecoder.h"

/**
 * @brief 値を一フレームの最大移動量以内で目標へ近づける
 * @param current 現在値
 * @param target 目標値
 * @param maxDelta 最大移動量
 * @return 更新後の値
 */
float SideScrollingShooter::Stage5Module::MoveTowards(
    float current, float target, float maxDelta) {
    return current < target ? (std::min)(current + maxDelta, target) :
        (std::max)(current - maxDelta, target);
}

#include "../../SideScrollingShooterEnemies.h"
#include "../Common/StageDefinition.h"
#include "Stage5EnemySheetEasy.h"
#include "Stage5EnemySheetHard.h"
#include "Stage5EnemySheetNormal.h"
#include "Stage5ModelView.h"

/** @brief Stage 5の敵出現、弾幕、ボス設定を保持する不変定義 */
class SideScrollingShooter::Stage5Module::StageDefinitionImpl final
    : public SideScrollingShooter::Stage {
public:
    explicit StageDefinitionImpl(const Stage5EnemySheet& enemySheet)
        : m_enemySheet(enemySheet) {
    }

    /**
     * @brief ステージ番号を取得する
     * @return Stage 5を表す番号
     */
    int StageIndex() const override { return 5; }

    /**
     * @brief EASTSOURCEの最大HPを取得する
     * @return EASTSOURCEの最大HP
     */
    int BossMaxHp() const override { return ShooterStages::Stage5::EastsourceMaxHp; }

    /**
     * @brief Stage 5の経過フレームから通常敵の出現を選択する
     * @param frame Stage 5開始からの経過フレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 選択した出現規則の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させるフレームの場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        return m_enemySheet.TrySelectEnemySpawn(frame, spawnIndex, spawn, chapterNumber);
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
        constexpr BossBullet SidePattern[] = {
            {-0.12f, 0.0f, -0.020f, -0.032f},
            {-0.12f, 0.0f, -0.023f, -0.024f},
            {-0.12f, 0.0f, -0.026f, -0.016f},
            {-0.12f, 0.0f, -0.028f, -0.008f},
            {-0.12f, 0.0f, -0.030f, 0.0f},
            {-0.12f, 0.0f, -0.028f, 0.008f},
            {-0.12f, 0.0f, -0.026f, 0.016f},
            {-0.12f, 0.0f, -0.023f, 0.024f},
            {-0.12f, 0.0f, -0.020f, 0.032f}
        };
        constexpr BossBullet RailPattern[] = {
            {0.0f, 0.0f, -0.022f, -0.032f},
            {0.0f, 0.0f, -0.016f, -0.024f},
            {0.0f, 0.0f, -0.011f, -0.016f},
            {0.0f, 0.0f, -0.005f, -0.008f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.005f, 0.008f},
            {0.0f, 0.0f, 0.011f, 0.016f},
            {0.0f, 0.0f, 0.016f, 0.024f},
            {0.0f, 0.0f, 0.022f, 0.032f}
        };
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
        boss.hp = ShooterStages::Stage5::EastsourceMaxHp;
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
            ShooterStages::Stage5::EastsourceNoseHp,
            ShooterStages::Stage5::EastsourceWingHp,
            ShooterStages::Stage5::EastsourceWingHp,
            ShooterStages::Stage5::EastsourceEngineHp,
            ShooterStages::Stage5::EastsourceEngineHp
        };
    }

    /**
     * @brief EASTSOURCE部位破壊時の本体ダメージを取得する
     * @param part 破壊された部位
     * @return 本体へ与えるダメージ
     */
    int BossPartBreakDamage(BossPart part) const override {
        return part == BossNose ? 75 :
            (part == BossLeftWing || part == BossRightWing ? 90 : 65);
    }

    /**
     * @brief EASTSOURCEの移動と攻撃を更新する
     * @param shooter ゲーム本体
     * @param boss 更新するEASTSOURCE
     * @return なし
     */
    void TickBoss(SideScrollingShooter& shooter, Enemy& boss) const override {
        Stage5Module::TickBoss(shooter, boss);
    }

    /**
     * @brief EASTSOURCE専用攻撃が有効か取得する
     * @param shooter ゲーム本体
     * @param boss 判定するEASTSOURCE
     * @return 常にtrue、falseは返さない
     */
    bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss) const override {
        (void)shooter;
        (void)boss;
        // EASTSOURCEは専用の予告付き4フェーズ攻撃だけを使用する
        return true;
    }

private:
    const Stage5EnemySheet& m_enemySheet;
};

/**
 * @brief Stage 5の不変ステージ定義を取得する
 * @return Stage 5定義
 */
const SideScrollingShooter::Stage& SideScrollingShooter::Stage5Module::Definition(
    DifficultyType difficulty) {
    static const Stage5EnemySheetEasy easySheet;
    static const Stage5EnemySheetNormal normalSheet;
    static const Stage5EnemySheetHard hardSheet;
    static const StageDefinitionImpl easyDefinition(easySheet);
    static const StageDefinitionImpl normalDefinition(normalSheet);
    static const StageDefinitionImpl hardDefinition(hardSheet);
    switch (difficulty) {
    case Hard: return hardDefinition;
    case Normal: return normalDefinition;
    default: return easyDefinition;
    }
}

/**
 * @brief Stage 5専用デバッグ入力を処理する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::ProcessDebugInput(SideScrollingShooter& shooter) {
#ifdef _DEBUG
    // F6からF12は現行と同じStage 5後半の直接開始位置を維持する
    if (Input::GetKeyDown(KeyCode::F6)) StartDebugPhase(shooter, Stage5Phase::WallClimbLower);
    if (Input::GetKeyDown(KeyCode::F7)) StartDebugPhase(shooter, Stage5Phase::WallClimbMiddle);
    if (Input::GetKeyDown(KeyCode::F8)) StartDebugPhase(shooter, Stage5Phase::WallClimbUpper);
    if (Input::GetKeyDown(KeyCode::F9)) StartDebugPhase(shooter, Stage5Phase::TayamaFireControl);
    if (Input::GetKeyDown(KeyCode::F10)) StartDebugPhase(shooter, Stage5Phase::TayamaLiftEngines);
    if (Input::GetKeyDown(KeyCode::F11)) StartDebugPhase(shooter, Stage5Phase::TayamaCommandCore);
    if (Input::GetKeyDown(KeyCode::F12)) StartDebugPhase(shooter, Stage5Phase::TayamaCollapse);
#else
    (void)shooter;
#endif
}

/**
 * @brief Stage 5ボス戦をデバッグ開始状態へ設定する
 * @param shooter 更新対象
 * @return Stage 5専用開始処理を完了した場合true、falseは返さない
 */
bool SideScrollingShooter::Stage5Module::StartDebugBoss(SideScrollingShooter& shooter) {
    // 現行のStage 5ボス開始と同じくミッション表示を消してレール視点を確定する
    shooter.m_missionStartTimer = 0;
    shooter.m_viewMode = ViewMode::Rail3D;
    shooter.m_nextViewMode = ViewMode::Rail3D;
    StartPhase(shooter, Stage5Phase::EastsourceBattle);
    return true;
}

/**
 * @brief m_frame加算前のStage 5スクリプトを更新する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickBeforeFrame(SideScrollingShooter& shooter) {
    // Stage 5後半は絶対フレームではなく専用状態と経過時間で進行する
    if (shooter.m_stage5.phase != Stage5Phase::Approach) {
        TickStateMachine(shooter);
    } else if (shooter.m_chapterNumber == 3 && shooter.m_frame % 150 == 30) {
        // TAYAMA浮上に同期して道路上の小型構造物を決定的な間隔で崩す
        const float side = (shooter.m_frame / 150) % 2 == 0 ? -1.0f : 1.0f;
        shooter.SpawnExplosion(side * 0.88f, -0.72f, 52.0f, true);
    }
}

/**
 * @brief m_frame加算後のStage 5環境音を更新する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickAfterFrame(SideScrollingShooter& shooter) {
    // 現行と同じ位置でクールダウンを一度だけ減算する
    shooter.m_stage5.soundCooldown = (std::max)(0, shooter.m_stage5.soundCooldown - 1);
    if (shooter.m_stage5.phase < Stage5Phase::TayamaCommandCore &&
        shooter.m_stage5.soundCooldown == 0) {
        if (shooter.m_frame % 397 == 0) {
            PlayCue(shooter, ShooterStages::Stage5::DistantThunder);
        }
        if (shooter.m_chapterNumber >= 2 && shooter.m_frame % 241 == 0) {
            PlayCue(shooter, ShooterStages::Stage5::Thunder);
        }
    }
}

/**
 * @brief チャプター結果終了後のStage 5専用遷移を処理する
 * @param shooter 更新対象
 * @return 専用遷移を完了して共通処理を中断する場合true、共通処理を続ける場合false
 */
bool SideScrollingShooter::Stage5Module::HandleChapterResult(SideScrollingShooter& shooter) {
    // Chapter 3完了時だけ通常ボス待機を使わずEASTSOURCE登場へ直結する
    if (shooter.m_chapterNumber != 3) return false;
    StartPhase(shooter, Stage5Phase::EastsourceIntro);
    return true;
}

/**
 * @brief 新しいチャプターの復帰地点を保存する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::OnChapterStarted(SideScrollingShooter& shooter) {
    // 現行のChapter 2または3開始時スナップショットを保持する
    SaveCheckpoint(shooter, shooter.m_chapterNumber == 2 ?
        Stage5Checkpoint::Chapter2 : Stage5Checkpoint::Chapter3);
}

/**
 * @brief Stage 5後半で表示モード切り替えをロックするか判定する
 * @param shooter 判定対象
 * @return 表示モードを固定する場合true、切り替えを許可する場合false
 */
bool SideScrollingShooter::Stage5Module::IsViewLocked(const SideScrollingShooter& shooter) {
    return shooter.m_stage5.phase != Stage5Phase::Approach;
}

/**
 * @brief 現在のStage 5状態で背景スクロールを進めるか判定する
 * @param shooter 判定対象
 * @return 背景スクロールを進める場合true、進行を止める場合false
 */
bool SideScrollingShooter::Stage5Module::ShouldAdvanceStageScroll(
    const SideScrollingShooter& shooter) {
    return shooter.m_stage5.phase <= Stage5Phase::WallClimbUpper ||
        shooter.m_stage5.phase == Stage5Phase::EastsourceFall;
}

/**
 * @brief 現在のStage 5状態で通常チャプター進行を行うか判定する
 * @param shooter 判定対象
 * @return チャプター終了判定と通常敵生成を行う場合true、Stage 5側が進行を所有する場合false
 */
bool SideScrollingShooter::Stage5Module::UsesChapterTimeline(
    const SideScrollingShooter& shooter) {
    return shooter.m_stage5.phase == Stage5Phase::Approach;
}

/**
 * @brief 現在のStage 5状態でプレイヤー被弾を無効にするか判定する
 * @param shooter 判定対象
 * @return 被弾を無効にする場合true、共通ダメージを適用する場合false
 */
bool SideScrollingShooter::Stage5Module::IsPlayerDamageIgnored(
    const SideScrollingShooter& shooter) {
    return shooter.m_stage5.phase == Stage5Phase::TayamaCollapse ||
        shooter.m_stage5.phase == Stage5Phase::EndingReady;
}

/**
 * @brief Stage 5終幕まで完了したか判定する
 * @param shooter 判定対象
 * @return 全ゲームクリアとして扱う場合true、進行中の場合false
 */
bool SideScrollingShooter::Stage5Module::IsGameCleared(const SideScrollingShooter& shooter) {
    return shooter.m_stage5.phase == Stage5Phase::EndingReady;
}

/**
 * @brief EASTSOURCEの移動と攻撃を更新する
 * @param shooter 更新対象
 * @param eastsource EASTSOURCE本体
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickBoss(
    SideScrollingShooter& shooter, Enemy& eastsource) {
    TickEastsource(shooter, eastsource);
}

/**
 * @brief 自機弾をStage 5固有ターゲットへ適用する
 * @param shooter 更新対象
 * @param shot 判定する自機弾
 * @return Stage 5固有ターゲットへ命中して共通敵判定を省略する場合true、共通敵判定へ進む場合false
 */
bool SideScrollingShooter::Stage5Module::TryDamageStageTarget(
    SideScrollingShooter& shooter, Shot& shot) {
    // 現行と同じくTAYAMAを壁面サーチライトより先に判定する
    return TryDamageTayama(shooter, shot) || TryDamageWallSearchlight(shooter, shot);
}

/**
 * @brief EASTSOURCE撃破後の専用遷移を処理する
 * @param shooter 更新対象
 * @param boss 撃破されたボス
 * @return 専用撃破遷移を完了して共通撃破処理を省略する場合true、共通撃破処理を使用する場合false
 */
bool SideScrollingShooter::Stage5Module::HandleBossDefeat(
    SideScrollingShooter& shooter, Enemy& boss) {
    if (!boss.active || shooter.m_stage5.phase != Stage5Phase::EastsourceBattle) return false;
    DefeatEastsource(shooter, boss);
    return true;
}

/**
 * @brief 敵弾用スロット不足時に自機弾を置換できるか判定する
 * @param enemy 生成対象が敵弾の場合true
 * @return Stage 5の予告済み敵弾として置換を許可する場合true、プールを変更しない場合false
 */
bool SideScrollingShooter::Stage5Module::CanReplacePlayerShot(bool enemy) {
    return enemy;
}

/**
 * @brief 自機弾とEASTSOURCE部位の衝突を判定する
 * @param shooter 判定対象
 * @param shot 判定する自機弾
 * @param boss EASTSOURCE本体
 * @param part 命中部位の格納先
 * @return EASTSOURCEの専用部位へ命中した場合true、命中しない場合false
 */
bool SideScrollingShooter::Stage5Module::TryHitBossPart(
    const SideScrollingShooter& shooter, const Shot& shot, const Enemy& boss, BossPart& part) {
    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };
    const Stage5ModelTransform transform = EastsourceTransform(shooter, boss);
    const EastsourceModelState state = EastsourceState(boss);

    // 描画と同じ26パーツから集約した各グループ境界へ線分判定する
    for (int index = BossNose; index <= BossRightEngine; ++index) {
        if (boss.bossPartHp[index] <= 0) continue;
        const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
            transform, state, Groups[index]);
        if (!bounds.valid || !Hit3DSegment(
            ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;
        part = static_cast<BossPart>(index);
        return true;
    }
    return false;
}

/**
 * @brief 指定地点へ向かう敵弾を生成する
 * @param shooter 更新対象
 * @param sourceX 発射元ゲーム座標X
 * @param sourceY 発射元ゲーム座標Y
 * @param sourceZ 発射元レール座標Z
 * @param targetX 固定目標ゲーム座標X
 * @param targetY 固定目標ゲーム座標Y
 * @param targetZ 固定目標レール座標Z
 * @param speed ワールド空間の弾速
 * @return なし
 */
void SideScrollingShooter::Stage5Module::SpawnEnemyShotAt(SideScrollingShooter& shooter,
    float sourceX, float sourceY, float sourceZ,
    float targetX, float targetY, float targetZ, float speed) {
    const float dx = ToWorldX(targetX - sourceX);
    const float dy = ToWorldY(targetY - sourceY);
    const float dz = targetZ - sourceZ;
    const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
    shooter.SpawnShotDirect(sourceX, sourceY, sourceZ,
        FromWorldX(dx / length * speed), FromWorldY(dy / length * speed),
        dz / length * speed, true);
}

/**
 * @brief Stage 5専用状態を初期化する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::Reset(SideScrollingShooter& shooter) {
    shooter.m_stage5.phase = Stage5Phase::Approach;
    shooter.m_stage5.checkpoint = Stage5Checkpoint::Chapter1;
    shooter.m_stage5.phaseTimer = 0;
    shooter.m_stage5.checkpointPower = shooter.m_power;
    shooter.m_stage5.checkpointScore = shooter.m_score;
    shooter.m_stage5.checkpointKills = shooter.m_kills;
    shooter.m_stage5.soundCooldown = 0;
    shooter.m_stage5.attackTimer = 0;
    shooter.m_stage5.guardSpawnCooldown = 0;
    shooter.m_stage5.coreTargetX = 0.0f;
    shooter.m_stage5.coreTargetY = 0.0f;
    shooter.m_stage5.tayamaTransformation = 0.0f;
    shooter.m_stage5.searchlights = {};
    shooter.m_stage5.tayamaWeakpoints = {{
        {TayamaWeakpoint::LeftSearchlight, 180, 180, false, false, 0},
        {TayamaWeakpoint::RightSearchlight, 180, 180, false, false, 0},
        {TayamaWeakpoint::FireControlRadar, 260, 260, false, false, 0},
        {TayamaWeakpoint::LeftLiftEngine, 360, 360, false, false, 0},
        {TayamaWeakpoint::RightLiftEngine, 360, 360, false, false, 0},
        {TayamaWeakpoint::CommandCore, 900, 900, false, false, 0}
    }};
    shooter.ApplyDifficultyToStage5WeakpointHp();
}

/**
 * @brief Stage 5の指定状態からデバッグ開始する
 * @param shooter 更新対象
 * @param phase 開始する状態
 * @return なし
 */
void SideScrollingShooter::Stage5Module::StartDebugPhase(SideScrollingShooter& shooter, Stage5Phase phase) {
    shooter.StartDebugCheckpoint(5, 3, false);
    shooter.m_chapterNumber = 3;
    shooter.m_frame = shooter.m_stage->ChapterEndFrame(3);
    shooter.m_scroll = static_cast<float>(shooter.m_frame) * 0.008f;
    shooter.m_missionStartTimer = 0;
    shooter.m_viewMode = ViewMode::Rail3D;
    shooter.m_nextViewMode = ViewMode::Rail3D;
    shooter.m_viewTransitionTimer = 0;
    shooter.m_viewTransitionProgress = 0.0f;

    // 後半地点は到達済みの弱点を破壊状態へ合わせてから開始する
    if (phase == Stage5Phase::TayamaLiftEngines ||
        phase == Stage5Phase::TayamaCommandCore || phase == Stage5Phase::TayamaCollapse) {
        for (int i = 0; i <= static_cast<int>(TayamaWeakpoint::FireControlRadar); ++i) {
            shooter.m_stage5.tayamaWeakpoints[i].hp = 0;
            shooter.m_stage5.tayamaWeakpoints[i].destroyed = true;
        }
    }
    if (phase == Stage5Phase::TayamaCommandCore || phase == Stage5Phase::TayamaCollapse) {
        for (int i = static_cast<int>(TayamaWeakpoint::LeftLiftEngine);
            i <= static_cast<int>(TayamaWeakpoint::RightLiftEngine); ++i) {
            shooter.m_stage5.tayamaWeakpoints[i].hp = 0;
            shooter.m_stage5.tayamaWeakpoints[i].destroyed = true;
        }
    }
    if (phase == Stage5Phase::TayamaCollapse) {
        TayamaWeakpointState& core =
            shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)];
        core.hp = 0;
        core.destroyed = true;
    }
    shooter.m_stage5.tayamaTransformation = phase >= Stage5Phase::TayamaFireControl ? 1.0f : 0.0f;
    StartPhase(shooter, phase);
#ifdef _DEBUG
    // デバッグ直行後に各フェーズの演出と当たり判定を確認できる時間を確保する
    shooter.m_invincible = (std::max)(shooter.m_invincible, 600);
#endif
}

/**
 * @brief 現在状態をStage 5チェックポイントとして保存する
 * @param shooter 更新対象
 * @param checkpoint 保存するチェックポイント
 * @return なし
 */
void SideScrollingShooter::Stage5Module::SaveCheckpoint(SideScrollingShooter& shooter, Stage5Checkpoint checkpoint) {
    shooter.m_stage5.checkpoint = checkpoint;
    shooter.m_stage5.checkpointPower = shooter.m_power;
    shooter.m_stage5.checkpointScore = shooter.m_score;
    shooter.m_stage5.checkpointKills = shooter.m_kills;
}

/**
 * @brief Stage 5状態を開始する
 * @param shooter 更新対象
 * @param phase 開始する状態
 * @param saveCheckpoint 復帰地点として保存する場合true
 * @return なし
 */
void SideScrollingShooter::Stage5Module::StartPhase(SideScrollingShooter& shooter, Stage5Phase phase, bool saveCheckpoint) {
    shooter.m_stage5.phase = phase;
    shooter.m_stage5.phaseTimer = 0;
    shooter.m_stage5.attackTimer = 0;
    shooter.m_stage5.coreTargetX = shooter.m_playerX;
    shooter.m_stage5.coreTargetY = shooter.m_playerY;
    shooter.m_stage5.guardSpawnCooldown = 0;

    if (phase != Stage5Phase::Approach) shooter.RequestViewMode(ViewMode::Rail3D);

    // TAYAMA攻略は専用初期化へ委譲する
    if (phase == Stage5Phase::TayamaFireControl ||
        phase == Stage5Phase::TayamaLiftEngines ||
        phase == Stage5Phase::TayamaCommandCore) {
        StartTayamaPhase(shooter, phase, true);
        return;
    }

    if (phase == Stage5Phase::EastsourceIntro) {
        shooter.m_shots = {};
        shooter.m_enemies = {};
        shooter.m_items = {};
        shooter.m_bossBattle = true;
        Enemy& eastsource = shooter.m_enemies[0];
        shooter.m_stage->ConfigureBoss(eastsource, true);
        shooter.m_stage->ConfigureBossPartHp(eastsource);
        shooter.ApplyDifficultyToBossHp(eastsource);
        eastsource.x = 1.45f;
        eastsource.y = 0.30f;
        eastsource.z = 66.0f;
        eastsource.collisionEnabled = false;
        shooter.m_bossHp = eastsource.hp;
        shooter.m_displayBossHp = static_cast<float>(shooter.m_bossHp);
        if (saveCheckpoint) SaveCheckpoint(shooter, Stage5Checkpoint::Eastsource);
        PlayCue(shooter, ShooterStages::Stage5::EastsourceEntrance);
        return;
    }
    if (phase == Stage5Phase::EastsourceBattle) {
        StartEastsourceBattle(shooter);
        if (saveCheckpoint) SaveCheckpoint(shooter, Stage5Checkpoint::Eastsource);
        return;
    }
    if (phase == Stage5Phase::EastsourceFall) {
        shooter.m_bossBattle = false;
        for (auto& shot : shooter.m_shots) {
            if (shot.enemy) shot.active = false;
        }
        PlayCue(shooter, ShooterStages::Stage5::SignalLost);
        return;
    }

    // 壁面区画へ入るたび、その区画固有のライトを初期化する
    if (phase >= Stage5Phase::WallClimbTransition && phase <= Stage5Phase::CarrierTransformation) {
        shooter.m_bossBattle = false;
        for (auto& enemy : shooter.m_enemies) enemy.active = false;
        for (auto& shot : shooter.m_shots) {
            if (shot.enemy) shot.active = false;
        }
        int lightCount = 0;
        Stage5Checkpoint checkpoint = Stage5Checkpoint::WallClimbLower;
        if (phase == Stage5Phase::WallClimbLower) {
            lightCount = 1;
            shooter.m_stage5.tayamaTransformation = 0.10f;
        } else if (phase == Stage5Phase::WallClimbMiddle) {
            lightCount = 2;
            checkpoint = Stage5Checkpoint::WallClimbMiddle;
            shooter.m_stage5.tayamaTransformation = 0.34f;
        } else if (phase == Stage5Phase::WallClimbUpper) {
            lightCount = 3;
            checkpoint = Stage5Checkpoint::WallClimbUpper;
            shooter.m_stage5.tayamaTransformation = 0.64f;
        } else if (phase == Stage5Phase::RooftopArrival) {
            shooter.m_stage5.tayamaTransformation = 0.90f;
        } else if (phase == Stage5Phase::CarrierTransformation) {
            shooter.m_stage5.tayamaTransformation = 0.96f;
            PlayCue(shooter, ShooterStages::Stage5::Transformation);
        }
        ResetWallSearchlights(shooter, lightCount);
        if (saveCheckpoint && lightCount > 0) SaveCheckpoint(shooter, checkpoint);
        shooter.m_invincible = (std::max)(shooter.m_invincible, 60);
        return;
    }

    if (phase == Stage5Phase::TayamaCollapse) {
        shooter.m_bossBattle = false;
        shooter.m_bossHp = 0;
        shooter.m_displayBossHp = 0.0f;
        for (auto& enemy : shooter.m_enemies) enemy.active = false;
        for (auto& shot : shooter.m_shots) {
            if (shot.enemy) shot.active = false;
        }
        shooter.m_invincible = TayamaCollapseFrames + Stage5QuietFlightFrames + 60;
        PlayCue(shooter, ShooterStages::Stage5::ChainExplosion);
        return;
    }

    if (phase == Stage5Phase::EndingReady) {
        shooter.m_clear = true;
        shooter.m_clearTimer = 0;
    }
}

/**
 * @brief EASTSOURCE戦を戦闘可能な状態で開始する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::StartEastsourceBattle(SideScrollingShooter& shooter) {
    shooter.m_bossBattle = true;
    Enemy& eastsource = shooter.m_enemies[0];
    if (!eastsource.active || eastsource.type != Stage::BossEnemy) {
        eastsource = {};
        shooter.m_stage->ConfigureBoss(eastsource, true);
        shooter.m_stage->ConfigureBossPartHp(eastsource);
        shooter.ApplyDifficultyToBossHp(eastsource);
    }
    eastsource.collisionEnabled = true;
    eastsource.age = 0;
    eastsource.motionAge = 0;
    eastsource.bossPhase = BossNormalPhase1;
    eastsource.x = 0.42f;
    eastsource.y = 0.0f;
    eastsource.z = 45.0f;
    eastsource.baseX = eastsource.x;
    eastsource.baseY = eastsource.y;
    eastsource.baseZ = eastsource.z;
    shooter.m_bossHp = eastsource.hp;
    shooter.m_displayBossHp = static_cast<float>(shooter.m_bossHp);
    shooter.m_invincible = (std::max)(shooter.m_invincible, 60);
    ResetWallSearchlights(shooter, 1);
}

/**
 * @brief EASTSOURCEの移動と攻撃を更新する
 * @param shooter 更新対象
 * @param eastsource 更新するEASTSOURCE
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickEastsource(SideScrollingShooter& shooter, Enemy& eastsource) {
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceIntro) {
        const float progress = SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / ShooterStages::Stage5::EastsourceIntroFrames);
        eastsource.x = Math::Lerp(1.45f, 0.42f, progress);
        eastsource.y = Math::Lerp(0.30f, 0.0f, progress) + std::sin(progress * Math::Pi * 5.0f) * 0.08f;
        eastsource.z = Math::Lerp(66.0f, 45.0f, progress);
        eastsource.collisionEnabled = false;
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceFall) {
        eastsource.collisionEnabled = false;
        eastsource.x -= 0.004f;
        eastsource.y -= 0.018f + static_cast<float>(shooter.m_stage5.phaseTimer) * 0.00008f;
        eastsource.z += 0.08f;
        return;
    }
    if (shooter.m_stage5.phase != Stage5Phase::EastsourceBattle) return;

    const bool leftEngine = eastsource.bossPartHp[BossLeftEngine] > 0;
    const bool rightEngine = eastsource.bossPartHp[BossRightEngine] > 0;
    const float engineRate = (static_cast<float>(leftEngine) + static_cast<float>(rightEngine)) * 0.5f;
    const float moveAmplitude = 0.20f + engineRate * 0.38f;
    const float moveSpeed = 0.009f + engineRate * 0.009f;
    eastsource.x = std::sin(static_cast<float>(eastsource.age) * moveSpeed) * moveAmplitude;
    eastsource.y = std::sin(static_cast<float>(eastsource.age) * 0.021f) * 0.42f;
    eastsource.z = 43.0f + std::sin(static_cast<float>(eastsource.age) * 0.013f) * 3.0f;

    const bool nose = eastsource.bossPartHp[BossNose] > 0;
    const int phase = eastsource.bossPhase;
    if (phase == BossNormalPhase2 || phase == BossSpecialPhase2) {
        const int pursuitCycle = eastsource.age % 180;
        const bool fromLeft = (eastsource.age / 180) % 2 == 0;
        const BossPart wingPart = fromLeft ? BossLeftWing : BossRightWing;
        const BossPart enginePart = fromLeft ? BossLeftEngine : BossRightEngine;
        const bool wing = eastsource.bossPartHp[wingPart] > 0;
        const bool engine = eastsource.bossPartHp[enginePart] > 0;
        const float side = fromLeft ? -1.0f : 1.0f;
        const int passEnd = 112 + (engine ? 36 : 52);

        // 索敵中は遠ざかり、予告後だけ画面外から固定方向へ高速再進入する
        if (!wing) {
            eastsource.collisionEnabled = true;
        } else if (pursuitCycle < 82) {
            eastsource.z = Math::Lerp(43.0f, 59.0f,
                SmoothStep(static_cast<float>(pursuitCycle) / 82.0f));
        } else if (pursuitCycle < 112) {
            eastsource.x = side * 1.48f;
            eastsource.z = 59.0f;
            eastsource.collisionEnabled = false;
        } else if (pursuitCycle < passEnd) {
            const float pass = SmoothStep(static_cast<float>(pursuitCycle - 112) / (engine ? 36.0f : 52.0f));
            eastsource.x = Math::Lerp(side * 1.48f, -side * 1.48f, pass);
            eastsource.y = eastsource.attackWarningTargetY;
            eastsource.z = Math::Lerp(31.0f, 19.0f, std::sin(pass * Math::Pi));
            eastsource.collisionEnabled = false;
        } else {
            const float settle = SmoothStep(static_cast<float>(pursuitCycle - passEnd) /
                static_cast<float>(180 - passEnd));
            eastsource.x = Math::Lerp(-side * 1.48f, 0.0f, settle);
            eastsource.z = Math::Lerp(28.0f, 43.0f, settle);
            eastsource.collisionEnabled = settle > 0.55f;
        }
    } else {
        eastsource.collisionEnabled = true;
    }
    if (phase == BossNormalPhase1 || phase == BossSpecialPhase2) {
        const int cycleLength = nose ? 118 : 148;
        const int cycle = eastsource.age % cycleLength;
        const int warningFrames = nose ? 34 : 54;
        if (cycle == 0) {
            const float error = nose ? 0.0f : std::sin(static_cast<float>(eastsource.age) * 0.37f) * 0.24f;
            eastsource.attackWarningTargetX = shooter.m_playerX + error;
            eastsource.attackWarningTargetY = shooter.m_playerY - error * 0.45f;
            eastsource.attackWarningFrames = warningFrames;
            PlayCue(shooter, ShooterStages::Stage5::BarrageWarning);
        }
        const int shotCount = nose ? 3 : 1;
        for (int shotIndex = 0; shotIndex < shotCount; ++shotIndex) {
            if (cycle == warningFrames + shotIndex * 10) {
                SpawnEnemyShotAt(shooter, eastsource.x, eastsource.y, eastsource.z,
                    eastsource.attackWarningTargetX, eastsource.attackWarningTargetY,
                    PlayerRailZ, 0.72f);
            }
        }
    }

    if (phase == BossSpecialPhase1 || phase == BossSpecialPhase2) {
        const int cycle = eastsource.age % 96;
        if (cycle == 22 && eastsource.bossPartHp[BossLeftWing] > 0) {
            for (int lane = -2; lane <= 2; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(shooter, eastsource.x - 0.42f, eastsource.y + 0.12f, eastsource.z,
                    shooter.m_playerX + 0.25f, static_cast<float>(lane) * 0.25f, PlayerRailZ, 0.64f);
            }
        }
        if (cycle == 48 && eastsource.bossPartHp[BossRightWing] > 0) {
            for (int lane = -2; lane <= 2; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(shooter, eastsource.x + 0.42f, eastsource.y - 0.12f, eastsource.z,
                    shooter.m_playerX - 0.25f, static_cast<float>(lane) * 0.25f, PlayerRailZ, 0.64f);
            }
        }
    }

    if (phase == BossNormalPhase2 || phase == BossSpecialPhase2) {
        const int cycle = eastsource.age % 180;
        if (cycle == 92) {
            eastsource.attackWarningTargetX = shooter.m_playerX;
            eastsource.attackWarningTargetY = shooter.m_playerY;
            eastsource.attackWarningFrames = 30;
        }
        if (cycle >= 120 && cycle < 138) {
            const bool fromLeft = (eastsource.age / 180) % 2 == 0;
            const bool wing = eastsource.bossPartHp[fromLeft ? BossLeftWing : BossRightWing] > 0;
            const bool engine = eastsource.bossPartHp[fromLeft ? BossLeftEngine : BossRightEngine] > 0;
            if (wing && cycle % (engine ? 4 : 7) == 0) {
                const float sourceX = fromLeft ? -1.25f : 1.25f;
                SpawnEnemyShotAt(shooter, sourceX, eastsource.attackWarningTargetY, 18.0f,
                    eastsource.attackWarningTargetX, eastsource.attackWarningTargetY,
                    PlayerRailZ, engine ? 0.82f : 0.58f);
            }
        }
    }
}

/**
 * @brief EASTSOURCE撃破後の信号消失演出へ移行する
 * @param shooter 更新対象
 * @param eastsource 撃破状態へ移行するEASTSOURCE
 * @return なし
 */
void SideScrollingShooter::Stage5Module::DefeatEastsource(SideScrollingShooter& shooter, Enemy& eastsource) {
    if (shooter.m_stage5.phase != Stage5Phase::EastsourceBattle) return;
    static const auto eastsourceDeathVoice =
        VoiceCodec::DecodeForAudioService(VoiceSamples::eastsourceDeath);
    if (shooter.m_audio) shooter.m_audio->PlaySE(eastsourceDeathVoice);
    shooter.UnlockGallery(GalleryEntry::Eastsource);
    eastsource.hp = 0;
    eastsource.collisionEnabled = false;
    shooter.m_bossHp = 0;
    shooter.m_displayBossHp = 0.0f;
    shooter.m_score += 5000;
    const BossPart detachedPart = eastsource.bossPartHp[BossLeftWing] > 0 ? BossLeftWing :
        (eastsource.bossPartHp[BossRightWing] > 0 ? BossRightWing :
            (eastsource.bossPartHp[BossLeftEngine] > 0 ? BossLeftEngine : BossRightEngine));
    SpawnBossDebris(shooter, eastsource, detachedPart);
    eastsource.bossPartHp[detachedPart] = 0;
    StartPhase(shooter, Stage5Phase::EastsourceFall, false);
}

/**
 * @brief 壁面区画のサーチライトを初期化する
 * @param shooter 更新対象
 * @param activeCount 有効化するサーチライト数
 * @return なし
 */
void SideScrollingShooter::Stage5Module::ResetWallSearchlights(SideScrollingShooter& shooter, int activeCount) {
    shooter.m_stage5.searchlights = {};
    for (int i = 0; i < Stage5SearchlightCount; ++i) {
        SearchlightState& light = shooter.m_stage5.searchlights[i];
        light.beamX = (static_cast<float>(i) - 1.0f) * 0.62f;
        light.beamY = i % 2 == 0 ? 0.34f : -0.28f;
        light.hp = i < activeCount ? 90 : 0;
        light.destroyed = i >= activeCount;
        light.timer = i * 27;
    }
}

/**
 * @brief サーチライトの保存済み地点へ集中砲火を生成する
 * @param shooter 更新対象
 * @param light 集中砲火に使用するサーチライト状態
 * @param lightIndex 発射元を決めるサーチライト番号
 * @return なし
 */
void SideScrollingShooter::Stage5Module::FireSearchlightVolley(SideScrollingShooter& shooter, const SearchlightState& light, int lightIndex) {
    float sourceX = (static_cast<float>(lightIndex) - 1.0f) * 0.72f;
    float sourceY = 0.72f - static_cast<float>(lightIndex) * 0.22f;
    float sourceZ = 46.0f;
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl) {
        const TayamaPartGroup group = lightIndex == 0 ?
            TayamaPartGroup::LeftSearchlight : TayamaPartGroup::RightSearchlight;
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
            TayamaTransform(shooter), shooter.m_stage5.tayamaTransformation, TayamaState(shooter), group);
        if (bounds.valid) {
            sourceX = FromWorldX(bounds.center.x);
            sourceY = FromWorldY(bounds.center.y);
            sourceZ = bounds.center.z;
        }
    }
    for (int bullet = -2; bullet <= 2; ++bullet) {
        const float spread = static_cast<float>(bullet) * 0.065f;
        SpawnEnemyShotAt(shooter, sourceX, sourceY, sourceZ,
            light.lockedX + spread, light.lockedY + std::abs(spread) * 0.35f,
            PlayerRailZ, 0.78f);
    }
}

/**
 * @brief 指定数のサーチライトを更新する
 * @param shooter 更新対象
 * @param activeCount 更新するライト数
 * @param tayamaWeakpoints TAYAMA弱点と破壊状態を共有する場合true
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickSearchlights(SideScrollingShooter& shooter, int activeCount, bool tayamaWeakpoints) {
    const bool radarDestroyed = tayamaWeakpoints &&
        shooter.m_stage5.tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed;
    const int difficultyOffset = shooter.m_difficulty == Easy ? 9 : (shooter.m_difficulty == Hard ? -9 : 0);
    const int lockFrames = SearchlightLockFrames + difficultyOffset + (radarDestroyed ? 18 : 0);

    for (int i = 0; i < activeCount; ++i) {
        SearchlightState& light = shooter.m_stage5.searchlights[i];
        if (tayamaWeakpoints) {
            const TayamaWeakpoint type = i == 0 ? TayamaWeakpoint::LeftSearchlight :
                TayamaWeakpoint::RightSearchlight;
            light.destroyed = shooter.m_stage5.tayamaWeakpoints[static_cast<int>(type)].destroyed;
        }
        if (light.destroyed) continue;

        const float scanTargetX = std::sin(static_cast<float>(shooter.m_stage5.phaseTimer + i * 67) *
            (0.018f + static_cast<float>(i) * 0.002f)) * 0.92f;
        const float scanTargetY = std::sin(static_cast<float>(shooter.m_stage5.phaseTimer + i * 43) *
            (0.013f + static_cast<float>(i) * 0.003f)) * 0.66f;
        if (light.phase == SearchlightPhase::Searching || light.phase == SearchlightPhase::Detecting) {
            const float trackingLimit = radarDestroyed ? 0.010f : 0.016f;
            light.beamX = MoveTowards(light.beamX, scanTargetX, trackingLimit);
            light.beamY = MoveTowards(light.beamY, scanTargetY, trackingLimit * 0.78f);
        }

        const bool illuminated = Hit(shooter.m_playerX, shooter.m_playerY, 0.055f,
            light.beamX, light.beamY, SearchlightDetectionRadius);
        if (light.phase == SearchlightPhase::Searching) {
            if (illuminated) {
                light.phase = SearchlightPhase::Detecting;
                light.detectionFrames = 1;
                PlayCue(shooter, ShooterStages::Stage5::SearchlightDetect);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Detecting) {
            light.detectionFrames = illuminated ? light.detectionFrames + 1 :
                (std::max)(0, light.detectionFrames - 2);
            if (light.detectionFrames == 0) {
                light.phase = SearchlightPhase::Searching;
            } else if (light.detectionFrames >= lockFrames) {
                light.lockedX = shooter.m_playerX;
                light.lockedY = shooter.m_playerY;
                light.phase = SearchlightPhase::Locked;
                light.timer = SearchlightWarningFrames;
                PlayCue(shooter, ShooterStages::Stage5::SearchlightLocked);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Locked) {
            if (--light.timer <= 0) {
                light.phase = SearchlightPhase::Firing;
                light.timer = 0;
                light.volley = 0;
                PlayCue(shooter, ShooterStages::Stage5::BarrageWarning);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Firing) {
            if (light.timer % SearchlightVolleyIntervalFrames == 0 &&
                light.volley < SearchlightVolleyCount) {
                FireSearchlightVolley(shooter, light, i);
                ++light.volley;
            }
            ++light.timer;
            if (light.volley >= SearchlightVolleyCount &&
                light.timer >= SearchlightVolleyIntervalFrames * SearchlightVolleyCount) {
                light.phase = SearchlightPhase::Cooldown;
                light.timer = 90 + i * 24 + (shooter.m_difficulty == Easy ? 30 : 0);
                light.detectionFrames = 0;
            }
            continue;
        }
        if (--light.timer <= 0) light.phase = SearchlightPhase::Searching;
    }
}

/**
 * @brief EASTSOURCEの描画と当たり判定で共有する親Transformを取得する
 * @param shooter 更新対象
 * @param eastsource EASTSOURCE本体
 * @return ワールド座標へ変換する親Transform
 */
Stage5ModelTransform SideScrollingShooter::Stage5Module::EastsourceTransform(const SideScrollingShooter& shooter, const Enemy& eastsource) {
    const float fallRoll = shooter.m_stage5.phase == Stage5Phase::EastsourceFall ?
        static_cast<float>(shooter.m_stage5.phaseTimer) * 0.035f : 0.0f;
    return {{ToWorldX(eastsource.x), ToWorldY(eastsource.y), eastsource.z},
        {0.0f, Math::Lerp(Math::HalfPi, 0.0f, shooter.RailBlend()), fallRoll}, 0.72f};
}

/**
 * @brief EASTSOURCEの部位状態をモデルグループへ変換する
 * @param shooter 更新対象
 * @param eastsource EASTSOURCE本体
 * @return 描画と当たり判定へ渡すモデル状態
 */
EastsourceModelState SideScrollingShooter::Stage5Module::EastsourceState(const Enemy& eastsource) {
    EastsourceModelState state;
    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };

    // 既存BossPartの固定順をEASTSOURCEのモデルグループへ一度だけ写す
    for (int part = BossNose; part <= BossRightEngine; ++part) {
        const std::size_t group = static_cast<std::size_t>(Groups[part]);
        state.destroyed[group] = eastsource.bossPartHp[part] <= 0;
        const int flash = eastsource.bossPartHitFlashFrames[part];
        state.hitFlash[group] = flash > 0 && (flash / 2) % 2 != 0;
    }
    return state;
}

/**
 * @brief TAYAMAの描画と当たり判定で共有する親Transformを取得する
 * @param shooter 更新対象
 * @return 現在の進行に対応する親Transform
 */
Stage5ModelTransform SideScrollingShooter::Stage5Module::TayamaTransform(const SideScrollingShooter& shooter) {
    float y = 3.0f;
    float z = 57.0f;
    float scale = 1.34f;
    float pitch = 0.0f;
    float roll = 0.0f;

    // 通常チャプターでは遠景の都市構造として見せ、後半ほど接近させる
    if (shooter.m_stage5.phase == Stage5Phase::Approach) {
        const int chapterLength = shooter.m_stage != nullptr ? shooter.m_stage->ChapterFrameLength() : ChapterLengthFrames;
        const float chapterProgress = Math::Clamp01(
            static_cast<float>(shooter.m_frame - (shooter.m_chapterNumber - 1) * chapterLength) /
            static_cast<float>(chapterLength));
        y = -9.0f + static_cast<float>(shooter.m_chapterNumber - 1) * 2.4f + chapterProgress * 1.2f;
        z = 88.0f - static_cast<float>(shooter.m_chapterNumber - 1) * 6.0f;
        scale = 0.82f + static_cast<float>(shooter.m_chapterNumber - 1) * 0.12f;
    } else if (shooter.m_stage5.phase <= Stage5Phase::EastsourceFall) {
        const float approach = shooter.m_stage5.phase == Stage5Phase::EastsourceIntro ?
            SmoothStep(Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer) / ShooterStages::Stage5::EastsourceIntroFrames)) : 1.0f;
        y = Math::Lerp(-3.0f, -1.5f, approach);
        z = Math::Lerp(76.0f, 59.0f, approach);
        scale = Math::Lerp(1.06f, 1.42f, approach);
    } else if (shooter.m_stage5.phase <= Stage5Phase::WallClimbUpper) {
        y = -1.5f + shooter.m_stage5.tayamaTransformation * 4.0f;
        z = 59.0f;
        scale = 1.42f;
    } else if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) {
        z = 57.0f + static_cast<float>((std::min)(shooter.m_stage5.phaseTimer, 450)) * 0.004f;
        scale = 1.34f + static_cast<float>((std::max)(0, shooter.m_stage5.phaseTimer - 330)) * 0.0006f;
    }
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival) {
        const float arrival = SmoothStep(Math::Clamp01(
            static_cast<float>(shooter.m_stage5.phaseTimer) / RooftopArrivalFrames));
        pitch = Math::Lerp(0.0f, 0.58f, arrival);
        y = Math::Lerp(2.1f, 0.0f, arrival);
        z = Math::Lerp(59.0f, 57.0f, arrival);
        scale = Math::Lerp(1.42f, 1.08f, arrival);
    } else if (shooter.m_stage5.phase >= Stage5Phase::CarrierTransformation) {
        pitch = 0.58f;
        y = 0.0f;
        scale = shooter.m_stage5.phase == Stage5Phase::TayamaCollapse ?
            1.08f + static_cast<float>((std::max)(0, shooter.m_stage5.phaseTimer - 330)) * 0.0006f : 1.08f;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore) {
        y = Math::Lerp(0.0f, 1.8f, SmoothStep(Math::Clamp01(
            static_cast<float>(shooter.m_stage5.phaseTimer) / 180.0f)));
    } else if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) {
        y = 1.8f - static_cast<float>((std::min)(shooter.m_stage5.phaseTimer, 450)) * 0.002f;
    }

    // 揚力エンジンの片側破壊を艦体ロールへ反映するが入力軸は回転させない
    if (shooter.m_stage5.phase >= Stage5Phase::TayamaLiftEngines) {
        const bool left = shooter.m_stage5.tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed;
        const bool right = shooter.m_stage5.tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed;
        if (left != right) roll = left ? -0.12f : 0.12f;
    }
    return {{0.0f, y, z}, {pitch, 0.0f, roll}, scale};
}

/**
 * @brief 現在の弱点と崩壊状態をTAYAMAモデルグループへ変換する
 * @param shooter 更新対象
 * @return 描画と当たり判定へ渡すモデル状態
 */
TayamaModelState SideScrollingShooter::Stage5Module::TayamaState(const SideScrollingShooter& shooter) {
    TayamaModelState state;
    if (shooter.m_stage5.phase == Stage5Phase::EndingReady) {
        state.visible.fill(false);
        return state;
    }
    constexpr TayamaPartGroup Groups[] = {
        TayamaPartGroup::LeftSearchlight,
        TayamaPartGroup::RightSearchlight,
        TayamaPartGroup::FireControlRadar,
        TayamaPartGroup::LeftLiftEngine,
        TayamaPartGroup::RightLiftEngine,
        TayamaPartGroup::CommandCore
    };

    // 弱点の破壊と点滅を同名モデルグループへ反映する
    for (const TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        const std::size_t index = static_cast<std::size_t>(weakpoint.type);
        const std::size_t group = static_cast<std::size_t>(Groups[index]);
        state.destroyed[group] = weakpoint.destroyed;
        state.hitFlash[group] = weakpoint.hitFlashFrames > 0 &&
            (weakpoint.hitFlashFrames / 2) % 2 != 0;
    }
    state.visible[static_cast<std::size_t>(TayamaPartGroup::CommandCore)] =
        shooter.m_stage5.phase >= Stage5Phase::TayamaCommandCore;
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore) {
        state.visible[static_cast<std::size_t>(TayamaPartGroup::ArmorPanel)] = false;
    }

    if (shooter.m_stage5.phase != Stage5Phase::TayamaCollapse) return state;

    // 大型構造はDebrisへ分解せずグループ単位のTransformで画面内崩壊させる
    const float deckFall = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 180) / 270.0f);
    const float bridgeFall = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 300) / 190.0f);
    const float engineFall = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 60) / 180.0f);
    const float armorFall = Math::Clamp01(static_cast<float>(shooter.m_stage5.phaseTimer - 210) / 210.0f);
    auto SetOffset = [&](TayamaPartGroup group, const Vector3& position,
        const Vector3& rotation, float scale) {
        state.collapseOffsets[static_cast<std::size_t>(group)] =
            {position, rotation, {scale, scale, scale}};
    };
    SetOffset(TayamaPartGroup::LeftFlightDeck,
        {-deckFall * 12.0f, -deckFall * 13.0f, deckFall * 3.0f},
        {deckFall * 0.18f, 0.0f, deckFall * 1.05f}, 1.0f - deckFall * 0.22f);
    SetOffset(TayamaPartGroup::RightFlightDeck,
        {deckFall * 12.0f, -deckFall * 14.0f, deckFall * 2.0f},
        {-deckFall * 0.16f, 0.0f, -deckFall * 1.12f}, 1.0f - deckFall * 0.22f);
    SetOffset(TayamaPartGroup::Bridge,
        {bridgeFall * 2.0f, -bridgeFall * 10.0f, -bridgeFall * 2.0f},
        {bridgeFall * 0.32f, bridgeFall * 0.24f, bridgeFall * 0.82f}, 1.0f - bridgeFall * 0.30f);
    SetOffset(TayamaPartGroup::LeftLiftEngine,
        {-engineFall * 3.0f, -engineFall * 16.0f, engineFall * 2.0f},
        {0.0f, engineFall * 0.35f, engineFall * 0.55f}, 1.0f - engineFall * 0.35f);
    SetOffset(TayamaPartGroup::RightLiftEngine,
        {engineFall * 3.0f, -engineFall * 16.0f, engineFall * 2.0f},
        {0.0f, -engineFall * 0.35f, -engineFall * 0.55f}, 1.0f - engineFall * 0.35f);
    SetOffset(TayamaPartGroup::ArmorPanel,
        {0.0f, -armorFall * 9.0f, armorFall * 4.0f},
        {armorFall * 0.24f, armorFall * 0.45f, 0.0f}, 1.0f - armorFall * 0.32f);
    SetOffset(TayamaPartGroup::Hangar,
        {0.0f, -armorFall * 11.0f, -armorFall * 3.0f},
        {-armorFall * 0.22f, -armorFall * 0.30f, armorFall * 0.20f}, 1.0f - armorFall * 0.34f);

    // 最終爆発後は全グループを消し、60フレームの静かな飛行だけを残す
    if (shooter.m_stage5.phaseTimer >= TayamaCollapseFrames) state.visible.fill(false);
    return state;
}

/**
 * @brief 自機弾を壁面サーチライトへ適用する
 * @param shooter 更新対象
 * @param shot 判定する自機弾
 * @return ライトへ命中した場合true、命中しない場合false
 */
bool SideScrollingShooter::Stage5Module::TryDamageWallSearchlight(SideScrollingShooter& shooter, Shot& shot) {
    if (shooter.m_stage5.phase < Stage5Phase::WallClimbLower ||
        shooter.m_stage5.phase > Stage5Phase::WallClimbUpper) return false;
    const int activeCount = shooter.m_stage5.phase == Stage5Phase::WallClimbLower ? 1 :
        (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle ? 2 : 3);

    // 描画する壁面ライト基部と同じ固定配置へ線分判定する
    for (int index = 0; index < activeCount; ++index) {
        SearchlightState& light = shooter.m_stage5.searchlights[index];
        if (light.destroyed) continue;
        const float sourceX = (static_cast<float>(index) - 1.0f) * 0.72f;
        const float sourceY = 0.72f - static_cast<float>(index) * 0.22f;
        if (!Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            ToWorldX(sourceX), ToWorldY(sourceY), 46.0f, 0.72f)) continue;

        if (!shot.piercing) shot.active = false;
        light.hp -= shot.damage;
        shooter.SpawnExplosion(sourceX, sourceY, 46.0f, light.hp <= 0);
        if (light.hp <= 0) {
            light.hp = 0;
            light.destroyed = true;
            light.phase = SearchlightPhase::Cooldown;
            shooter.m_score += 300;
            PlayCue(shooter, ShooterStages::Stage5::WeakpointDestroyed);
        } else {
            shooter.PlayHitSound();
        }
        return true;
    }
    return false;
}

/**
 * @brief 自機弾をTAYAMAの有効弱点へ適用する
 * @param shooter 更新対象
 * @param shot 判定する自機弾
 * @return TAYAMAへ命中した場合true、命中しない場合false
 */
bool SideScrollingShooter::Stage5Module::TryDamageTayama(SideScrollingShooter& shooter, Shot& shot) {
    if (shooter.m_stage5.phase < Stage5Phase::TayamaFireControl ||
        shooter.m_stage5.phase > Stage5Phase::TayamaCommandCore) return false;
    const Stage5ModelTransform transform = TayamaTransform(shooter);
    const TayamaModelState modelState = TayamaState(shooter);
    constexpr TayamaPartGroup Groups[] = {
        TayamaPartGroup::LeftSearchlight,
        TayamaPartGroup::RightSearchlight,
        TayamaPartGroup::FireControlRadar,
        TayamaPartGroup::LeftLiftEngine,
        TayamaPartGroup::RightLiftEngine,
        TayamaPartGroup::CommandCore
    };
    const Vector3 shotStart {ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz};
    const Vector3 shotEnd {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};

    // 現フェーズの弱点だけを先に判定し、後続フェーズへの先行ダメージを防ぐ
    for (TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        if (!weakpoint.active || weakpoint.destroyed) continue;
        const TayamaPartGroup group = Groups[static_cast<std::size_t>(weakpoint.type)];
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
            transform, shooter.m_stage5.tayamaTransformation, modelState, group);
        if (!bounds.valid || !Hit3DSegment(shotStart.x, shotStart.y, shotStart.z,
            shotEnd.x, shotEnd.y, shotEnd.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;

        if (!shot.piercing) shot.active = false;
        weakpoint.hp -= shot.damage;
        weakpoint.hitFlashFrames = BossPartHitFlashFrames;
        shooter.SpawnExplosion(FromWorldX(bounds.center.x), FromWorldY(bounds.center.y), bounds.center.z,
            weakpoint.hp <= 0);
        if (weakpoint.hp <= 0) {
            weakpoint.hp = 0;
            weakpoint.destroyed = true;
            shooter.m_score += weakpoint.type == TayamaWeakpoint::CommandCore ? 5000 : 750;
            if (weakpoint.type == TayamaWeakpoint::LeftSearchlight) shooter.m_stage5.searchlights[0].destroyed = true;
            if (weakpoint.type == TayamaWeakpoint::RightSearchlight) shooter.m_stage5.searchlights[1].destroyed = true;
            PlayCue(shooter, ShooterStages::Stage5::WeakpointDestroyed);
        } else {
            shooter.PlayHitSound();
        }

        bool phaseComplete = true;
        for (const TayamaWeakpointState& current : shooter.m_stage5.tayamaWeakpoints) {
            if (IsTayamaWeakpointActiveForPhase(current.type, shooter.m_stage5.phase) && !current.destroyed) {
                phaseComplete = false;
                break;
            }
        }
        if (phaseComplete) {
            if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl) {
                StartTayamaPhase(shooter, Stage5Phase::TayamaLiftEngines);
            } else if (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines) {
                StartTayamaPhase(shooter, Stage5Phase::TayamaCommandCore);
            } else {
                static const auto tayamaDeathVoice =
                    VoiceCodec::DecodeForAudioService(VoiceSamples::tayamaDeath);
                if (shooter.m_audio) shooter.m_audio->PlaySE(tayamaDeathVoice);
                shooter.UnlockGallery(GalleryEntry::Tayama);
                StartPhase(shooter, Stage5Phase::TayamaCollapse, false);
            }
        } else {
            UpdateTayamaBossHp(shooter);
        }
        return true;
    }

    // 無効な装甲への命中は小さな着弾だけを出し、HPとスコアを変えない
    for (int group = 0; group < static_cast<int>(TayamaPartGroup::Count); ++group) {
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
            shooter.m_stage5.tayamaTransformation, modelState, static_cast<TayamaPartGroup>(group));
        if (!bounds.valid || !Hit3DSegment(shotStart.x, shotStart.y, shotStart.z,
            shotEnd.x, shotEnd.y, shotEnd.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;
        if (!shot.piercing) shot.active = false;
        shooter.SpawnExplosion(shot.x, shot.y, shot.z);
        shooter.PlayHitSound();
        return true;
    }
    return false;
}

/**
 * @brief 現在フェーズで有効なTAYAMA弱点HP合計をHUDへ反映する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::UpdateTayamaBossHp(SideScrollingShooter& shooter) {
    int currentHp = 0;
    int currentMaxHp = 0;
    for (const TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        if (!IsTayamaWeakpointActiveForPhase(weakpoint.type, shooter.m_stage5.phase)) continue;
        currentHp += (std::max)(0, weakpoint.hp);
        currentMaxHp += weakpoint.maxHp;
    }
    shooter.m_bossHp = currentHp;
    if (shooter.m_displayBossHp <= 0.0f || shooter.m_displayBossHp > static_cast<float>(currentMaxHp)) {
        shooter.m_displayBossHp = static_cast<float>(currentHp);
    }
}

/**
 * @brief TAYAMAの攻略フェーズを開始する
 * @param shooter 更新対象
 * @param phase 開始する攻略状態
 * @param resetCurrentHp 現在フェーズのHPを初期値へ戻す場合true
 * @return なし
 */
void SideScrollingShooter::Stage5Module::StartTayamaPhase(SideScrollingShooter& shooter, Stage5Phase phase, bool resetCurrentHp) {
    shooter.m_stage5.phase = phase;
    shooter.m_stage5.phaseTimer = 0;
    shooter.m_stage5.attackTimer = 0;
    shooter.m_stage5.coreTargetX = shooter.m_playerX;
    shooter.m_stage5.coreTargetY = shooter.m_playerY;
    shooter.m_bossBattle = false;
    shooter.m_stage5.tayamaTransformation = 1.0f;
    for (auto& enemy : shooter.m_enemies) enemy.active = false;
    for (auto& shot : shooter.m_shots) {
        if (shot.enemy) shot.active = false;
    }

    // 前フェーズの破壊結果を維持し、現在フェーズだけを有効化する
    for (TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        weakpoint.active = IsTayamaWeakpointActiveForPhase(weakpoint.type, phase) && !weakpoint.destroyed;
        if (weakpoint.active && resetCurrentHp) weakpoint.hp = weakpoint.maxHp;
        weakpoint.hitFlashFrames = 0;
    }
    if (phase == Stage5Phase::TayamaFireControl) {
        ResetWallSearchlights(shooter, 2);
        SaveCheckpoint(shooter, Stage5Checkpoint::TayamaFireControl);
    } else if (phase == Stage5Phase::TayamaLiftEngines) {
        SaveCheckpoint(shooter, Stage5Checkpoint::TayamaLiftEngines);
    } else {
        SaveCheckpoint(shooter, Stage5Checkpoint::TayamaCommandCore);
        PlayCue(shooter, ShooterStages::Stage5::CoreWarning);
    }
    UpdateTayamaBossHp(shooter);
    shooter.m_displayBossHp = static_cast<float>(shooter.m_bossHp);
    shooter.m_invincible = (std::max)(shooter.m_invincible, 75);
}

/**
 * @brief TAYAMA戦の更新処理
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickTayama(SideScrollingShooter& shooter) {
    for (TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        if (weakpoint.hitFlashFrames > 0) --weakpoint.hitFlashFrames;
    }
    ++shooter.m_stage5.attackTimer;
    shooter.m_stage5.guardSpawnCooldown = (std::max)(0, shooter.m_stage5.guardSpawnCooldown - 1);

    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl) {
        TickSearchlights(shooter, 2, true);
        if (shooter.m_stage5.attackTimer % 120 == 72) {
            for (int lane = -3; lane <= 3; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(shooter, static_cast<float>(lane) * 0.18f, 0.62f, 58.0f,
                    shooter.m_playerX + static_cast<float>(lane) * 0.10f,
                    shooter.m_playerY + static_cast<float>(lane) * 0.09f, PlayerRailZ, 0.60f);
            }
        }
        if (shooter.m_stage5.guardSpawnCooldown == 0 && shooter.m_stage5.phaseTimer > 120) {
            shooter.SpawnEnemy(Stage::ArmoredEnemy, 1.16f, (shooter.m_stage5.phaseTimer / 240) % 2 == 0 ? -0.72f : 0.72f,
                0.58f, 60.0f);
            shooter.m_stage5.guardSpawnCooldown = 300;
        }
        const int sweepCycle = shooter.m_stage5.attackTimer % 210;
        if (sweepCycle == 0) {
            shooter.m_stage5.coreTargetY = shooter.m_playerY;
            PlayCue(shooter, ShooterStages::Stage5::BarrageWarning);
        }
        if (sweepCycle == 36) {
            for (int lane = -4; lane <= 4; ++lane) {
                SpawnEnemyShotAt(shooter, static_cast<float>(lane) * 0.27f, 0.70f, 58.0f,
                    static_cast<float>(lane) * 0.27f, shooter.m_stage5.coreTargetY,
                    PlayerRailZ, 0.76f);
            }
        }
        return;
    }

    if (shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines) {
        const int cycle = shooter.m_stage5.attackTimer % 132;
        if (cycle == 0) {
            shooter.m_stage5.coreTargetX = shooter.m_playerX;
            shooter.m_stage5.coreTargetY = shooter.m_playerY;
            PlayCue(shooter, ShooterStages::Stage5::BarrageWarning);
        }
        if (cycle == 32) {
            for (int engine = 0; engine < 2; ++engine) {
                const TayamaWeakpoint type = engine == 0 ?
                    TayamaWeakpoint::LeftLiftEngine : TayamaWeakpoint::RightLiftEngine;
                if (shooter.m_stage5.tayamaWeakpoints[static_cast<int>(type)].destroyed) continue;
                const float side = engine == 0 ? -1.0f : 1.0f;
                for (int lane = -2; lane <= 2; ++lane) {
                    SpawnEnemyShotAt(shooter, side * 0.72f, -0.42f, 55.0f,
                        shooter.m_stage5.coreTargetX + static_cast<float>(lane) * 0.13f,
                        shooter.m_stage5.coreTargetY, PlayerRailZ, 0.72f);
                }
            }
        }
        if (shooter.m_stage5.guardSpawnCooldown == 0 && shooter.m_stage5.phaseTimer > 150) {
            shooter.SpawnEnemy(Stage::StraightShooterEnemy, 1.16f,
                (shooter.m_stage5.phaseTimer / 360) % 2 == 0 ? -0.58f : 0.58f, 0.54f, 59.0f);
            shooter.m_stage5.guardSpawnCooldown = 360;
        }
        return;
    }

    if (shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore) {
        const int cycle = shooter.m_stage5.attackTimer % 180;
        if (cycle == 0) {
            shooter.m_stage5.coreTargetX = shooter.m_playerX;
            shooter.m_stage5.coreTargetY = shooter.m_playerY;
            PlayCue(shooter, ShooterStages::Stage5::CoreWarning);
        }
        if (cycle == 42 || cycle == 52 || cycle == 62) {
            SpawnEnemyShotAt(shooter, 0.0f, 0.35f, 55.0f,
                shooter.m_stage5.coreTargetX, shooter.m_stage5.coreTargetY, PlayerRailZ, 0.92f);
        }
        if (cycle == 104) {
            for (int ray = 0; ray < 12; ++ray) {
                const float angle = static_cast<float>(ray) * Math::TwoPi / 12.0f;
                SpawnEnemyShotAt(shooter, 0.0f, 0.35f, 55.0f,
                    shooter.m_playerX + std::cos(angle) * 0.72f,
                    shooter.m_playerY + std::sin(angle) * 0.56f, PlayerRailZ, 0.66f);
            }
        }
        if (cycle == 138) {
            for (int lane = -4; lane <= 4; ++lane) {
                SpawnEnemyShotAt(shooter, static_cast<float>(lane) * 0.22f, -0.62f, 54.0f,
                    static_cast<float>(lane) * 0.22f,
                    -0.52f + std::abs(static_cast<float>(lane)) * 0.10f,
                    PlayerRailZ, 0.58f);
            }
        }
    }
}

/**
 * @brief Stage 5専用シーケンスを更新する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::TickStateMachine(SideScrollingShooter& shooter) {
    ++shooter.m_stage5.phaseTimer;

    if (shooter.m_stage5.phase == Stage5Phase::EastsourceIntro) {
        if (shooter.m_stage5.phaseTimer == 58) {
            shooter.SpawnExplosion(0.78f, 0.22f, 61.0f, true);
            constexpr float PanelColor[] = {0.20f, 0.22f, 0.30f, 1.0f};
            for (int i = 0; i < 8; ++i) {
                shooter.SpawnDebrisPiece(5.2f + static_cast<float>(i) * 0.28f, 1.0f + static_cast<float>(i % 3),
                    61.0f, 0.03f + static_cast<float>(i) * 0.004f, 0.02f,
                    -0.04f - static_cast<float>(i % 2) * 0.02f, 0.0f, 0.08f,
                    1, 0.8f, 0.35f, 0.16f, PanelColor, 120, 90, false);
            }
        }
        if (shooter.m_stage5.phaseTimer >= ShooterStages::Stage5::EastsourceIntroFrames) {
            StartPhase(shooter, Stage5Phase::EastsourceBattle);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceBattle) {
        const Enemy& eastsource = shooter.m_enemies[0];
        const int phase = eastsource.bossPhase;
        const int pursuitCycle = eastsource.age % 180;
        if ((phase == BossNormalPhase2 || phase == BossSpecialPhase2) && pursuitCycle < 90) {
            TickSearchlights(shooter, 1, false);
        }
        if (pursuitCycle == 90) ResetWallSearchlights(shooter, 1);
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::EastsourceFall) {
        if (shooter.m_stage5.phaseTimer >= ShooterStages::Stage5::EastsourceFallFrames) {
            shooter.m_enemies[0].active = false;
            StartPhase(shooter, Stage5Phase::WallClimbTransition, false);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbTransition) {
        shooter.m_stage5.tayamaTransformation = Math::Lerp(0.0f, 0.10f,
            SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / WallClimbTransitionFrames));
        if (shooter.m_stage5.phaseTimer >= WallClimbTransitionFrames) {
            StartPhase(shooter, Stage5Phase::WallClimbLower);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbLower) {
        shooter.m_stage5.tayamaTransformation = Math::Lerp(0.10f, 0.34f,
            SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / WallClimbLowerFrames));
        TickSearchlights(shooter, 1, false);
        if (shooter.m_stage5.phaseTimer >= WallClimbLowerFrames) {
            StartPhase(shooter, Stage5Phase::WallClimbMiddle);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbMiddle) {
        shooter.m_stage5.tayamaTransformation = Math::Lerp(0.34f, 0.64f,
            SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / WallClimbMiddleFrames));
        TickSearchlights(shooter, 2, false);
        if (shooter.m_stage5.guardSpawnCooldown-- <= 0) {
            shooter.SpawnEnemy(Stage::ArmoredEnemy, 1.16f,
                (shooter.m_stage5.phaseTimer / 180) % 2 == 0 ? -0.76f : 0.76f, 0.42f, 58.0f);
            shooter.m_stage5.guardSpawnCooldown = 210;
        }
        if (shooter.m_stage5.phaseTimer >= WallClimbMiddleFrames) {
            StartPhase(shooter, Stage5Phase::WallClimbUpper);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::WallClimbUpper) {
        shooter.m_stage5.tayamaTransformation = Math::Lerp(0.64f, 0.90f,
            SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / WallClimbUpperFrames));
        TickSearchlights(shooter, 3, false);
        if (shooter.m_stage5.guardSpawnCooldown-- <= 0) {
            shooter.SpawnEnemy(Stage::StraightShooterEnemy, 1.16f,
                (shooter.m_stage5.phaseTimer / 150) % 2 == 0 ? -0.82f : 0.82f, -0.34f, 60.0f);
            shooter.m_stage5.guardSpawnCooldown = 180;
        }
        if (shooter.m_stage5.phaseTimer >= WallClimbUpperFrames) {
            StartPhase(shooter, Stage5Phase::RooftopArrival, false);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::RooftopArrival) {
        shooter.m_stage5.tayamaTransformation = Math::Lerp(0.90f, 0.96f,
            SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / RooftopArrivalFrames));
        if (shooter.m_stage5.phaseTimer >= RooftopArrivalFrames) {
            StartPhase(shooter, Stage5Phase::CarrierTransformation, false);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::CarrierTransformation) {
        shooter.m_stage5.tayamaTransformation = Math::Lerp(0.96f, 1.0f,
            SmoothStep(static_cast<float>(shooter.m_stage5.phaseTimer) / CarrierTransformationFrames));
        if (shooter.m_stage5.phaseTimer >= CarrierTransformationFrames) {
            StartTayamaPhase(shooter, Stage5Phase::TayamaFireControl);
        }
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaFireControl ||
        shooter.m_stage5.phase == Stage5Phase::TayamaLiftEngines ||
        shooter.m_stage5.phase == Stage5Phase::TayamaCommandCore) {
        TickTayama(shooter);
        return;
    }
    if (shooter.m_stage5.phase == Stage5Phase::TayamaCollapse) {
        // 艦尾から艦首へ連鎖させ、最終フレームまで画面内で輪郭を崩す
        if (shooter.m_stage5.phaseTimer < TayamaCollapseFrames && shooter.m_stage5.phaseTimer % 36 == 0) {
            const int burst = shooter.m_stage5.phaseTimer / 36;
            shooter.SpawnExplosion(-0.85f + static_cast<float>((burst * 7) % 17) * 0.10f,
                -0.34f + static_cast<float>((burst * 5) % 9) * 0.09f, 54.0f, true);
            PlayCue(shooter, ShooterStages::Stage5::ChainExplosion);
        }
        if (shooter.m_stage5.phaseTimer == 450) PlayCue(shooter, ShooterStages::Stage5::FinalExplosion);
        if (shooter.m_stage5.phaseTimer >= TayamaCollapseFrames + Stage5QuietFlightFrames) {
            StartPhase(shooter, Stage5Phase::EndingReady, false);
        }
    }
}

/**
 * @brief 現在のStage 5チェックポイントへ復帰する
 * @param shooter 更新対象
 * @return なし
 */
void SideScrollingShooter::Stage5Module::RestartCheckpoint(SideScrollingShooter& shooter) {
    ++shooter.m_chapterRetryCounts[2];
    shooter.m_shots = {};
    shooter.m_enemies = {};
    shooter.m_items = {};
    shooter.m_explosions = {};
    shooter.m_debris = {};
    shooter.m_score = shooter.m_stage5.checkpointScore;
    shooter.m_kills = shooter.m_stage5.checkpointKills;
    shooter.m_playerX = -0.72f;
    shooter.m_playerY = 0.0f;
    shooter.m_viewMode = ViewMode::Rail3D;
    shooter.m_nextViewMode = ViewMode::Rail3D;
    shooter.m_viewTransitionTimer = 0;
    shooter.m_viewTransitionProgress = 0.0f;
    shooter.m_invincible = 120;
    shooter.m_restartTimer = RestartDisplayFrames;

    if (shooter.m_stage5.checkpoint == Stage5Checkpoint::Eastsource) {
        StartPhase(shooter, Stage5Phase::EastsourceBattle, false);
        return;
    }
    if (shooter.m_stage5.checkpoint == Stage5Checkpoint::WallClimbLower ||
        shooter.m_stage5.checkpoint == Stage5Checkpoint::WallClimbMiddle ||
        shooter.m_stage5.checkpoint == Stage5Checkpoint::WallClimbUpper) {
        const Stage5Phase phase = shooter.m_stage5.checkpoint == Stage5Checkpoint::WallClimbLower ?
            Stage5Phase::WallClimbLower :
            (shooter.m_stage5.checkpoint == Stage5Checkpoint::WallClimbMiddle ?
                Stage5Phase::WallClimbMiddle : Stage5Phase::WallClimbUpper);
        StartPhase(shooter, phase, false);
        return;
    }

    // TAYAMAは前フェーズを破壊済みとし、現在フェーズのHPだけを戻す
    const Stage5Phase phase = shooter.m_stage5.checkpoint == Stage5Checkpoint::TayamaFireControl ?
        Stage5Phase::TayamaFireControl :
        (shooter.m_stage5.checkpoint == Stage5Checkpoint::TayamaLiftEngines ?
            Stage5Phase::TayamaLiftEngines : Stage5Phase::TayamaCommandCore);
    for (TayamaWeakpointState& weakpoint : shooter.m_stage5.tayamaWeakpoints) {
        const bool previousPhase =
            (phase >= Stage5Phase::TayamaLiftEngines &&
                static_cast<int>(weakpoint.type) <= static_cast<int>(TayamaWeakpoint::FireControlRadar)) ||
            (phase >= Stage5Phase::TayamaCommandCore &&
                (weakpoint.type == TayamaWeakpoint::LeftLiftEngine ||
                    weakpoint.type == TayamaWeakpoint::RightLiftEngine));
        weakpoint.destroyed = previousPhase;
        weakpoint.hp = previousPhase ? 0 : weakpoint.maxHp;
    }
    StartTayamaPhase(shooter, phase, true);
}

/**
 * @brief Stage 5用の効果音をクールダウン付きで再生する
 * @param shooter 更新対象
 * @param cue 効果音種別
 * @return なし
 */
void SideScrollingShooter::Stage5Module::PlayCue(SideScrollingShooter& shooter, int cue) {
    if (!shooter.m_audio || shooter.m_stage5.soundCooldown > 0) return;
    switch (cue) {
    case ShooterStages::Stage5::DistantThunder:
        shooter.m_audio->PlayMMLSE("t90 o2 l8 v7 c r g");
        shooter.m_stage5.soundCooldown = 90;
        break;
    case ShooterStages::Stage5::Thunder:
        shooter.m_audio->PlayMMLSE("t180 o2 l32 v13 c>c<g c");
        shooter.m_stage5.soundCooldown = 45;
        break;
    case ShooterStages::Stage5::SearchlightDetect:
        shooter.m_audio->PlayMMLSE("t220 o6 l32 v8 c r c");
        shooter.m_stage5.soundCooldown = 18;
        break;
    case ShooterStages::Stage5::SearchlightLocked:
        shooter.m_audio->PlayMMLSE("t240 o6 l16 v11 c>g");
        shooter.m_stage5.soundCooldown = 24;
        break;
    case ShooterStages::Stage5::BarrageWarning:
        shooter.m_audio->PlayMMLSE("t180 o4 l32 v10 c c c");
        shooter.m_stage5.soundCooldown = 18;
        break;
    case ShooterStages::Stage5::EastsourceEntrance:
        shooter.m_audio->PlayMMLSE(SideScrollingShooterShared::BossWarningSirenMml);
        shooter.m_stage5.soundCooldown = 60;
        break;
    case ShooterStages::Stage5::SignalLost:
        shooter.m_audio->PlayMMLSE("t140 o5 l32 v8 g f e c");
        shooter.m_stage5.soundCooldown = 60;
        break;
    case ShooterStages::Stage5::Transformation:
        shooter.m_audio->PlayMMLSE("t110 o2 l16 v12 c d e g");
        shooter.m_stage5.soundCooldown = 75;
        break;
    case ShooterStages::Stage5::WeakpointDestroyed:
        shooter.m_audio->PlaySE(Audio::SfxrPreset::Explosion);
        shooter.m_stage5.soundCooldown = 20;
        break;
    case ShooterStages::Stage5::CoreWarning:
        shooter.m_audio->PlayMMLSE("t240 o3 l16 v12 c > c < c > c");
        shooter.m_stage5.soundCooldown = 36;
        break;
    case ShooterStages::Stage5::FinalExplosion:
        shooter.m_audio->PlayMMLSE("t80 o1 l2 v15 c g c");
        shooter.m_stage5.soundCooldown = 90;
        break;
    default:
        shooter.m_audio->PlaySE(Audio::SfxrPreset::Explosion);
        shooter.m_stage5.soundCooldown = 16;
        break;
    }
}
