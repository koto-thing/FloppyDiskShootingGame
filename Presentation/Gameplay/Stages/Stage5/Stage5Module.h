#pragma once

#include "../../SideScrollingShooter.h"

struct EastsourceModelState;
struct Stage5ModelTransform;
struct TayamaModelState;

/** @brief Stage 5固有の進行、戦闘、描画を提供する静的モジュール */
class SideScrollingShooter::Stage5Module final {
public:
    Stage5Module() = delete;

    /**
     * @brief Stage 5の不変ステージ定義を取得する
     * @return Stage 5定義
     */
    static const Stage& Definition();

    /**
     * @brief Stage 5状態を初期値へ戻す
     * @param shooter 更新対象
     * @return なし
     */
    static void Reset(SideScrollingShooter& shooter);

    /**
     * @brief Stage 5専用デバッグ入力を処理する
     * @param shooter 更新対象
     * @return なし
     */
    static void ProcessDebugInput(SideScrollingShooter& shooter);

    /**
     * @brief Stage 5ボス戦をデバッグ開始状態へ設定する
     * @param shooter 更新対象
     * @return Stage 5専用開始処理を完了した場合true、falseは返さない
     */
    static bool StartDebugBoss(SideScrollingShooter& shooter);

    /**
     * @brief Stage 5の指定状態からデバッグ開始する
     * @param shooter 更新対象
     * @param phase 開始する状態
     * @return なし
     */
    static void StartDebugPhase(SideScrollingShooter& shooter, Stage5Phase phase);

    /**
     * @brief m_frame加算前のStage 5スクリプトを更新する
     * @param shooter 更新対象
     * @return なし
     */
    static void TickBeforeFrame(SideScrollingShooter& shooter);

    /**
     * @brief m_frame加算後のStage 5環境音を更新する
     * @param shooter 更新対象
     * @return なし
     */
    static void TickAfterFrame(SideScrollingShooter& shooter);

    /**
     * @brief チャプター結果終了後のStage 5専用遷移を処理する
     * @param shooter 更新対象
     * @return 専用遷移を完了して共通処理を中断する場合true、共通処理を続ける場合false
     */
    static bool HandleChapterResult(SideScrollingShooter& shooter);

    /**
     * @brief 新しいチャプターの復帰地点を保存する
     * @param shooter 更新対象
     * @return なし
     */
    static void OnChapterStarted(SideScrollingShooter& shooter);

    /**
     * @brief Stage 5後半で表示モード切り替えをロックするか判定する
     * @param shooter 判定対象
     * @return 表示モードを固定する場合true、切り替えを許可する場合false
     */
    static bool IsViewLocked(const SideScrollingShooter& shooter);

    /**
     * @brief 現在のStage 5状態で背景スクロールを進めるか判定する
     * @param shooter 判定対象
     * @return 背景スクロールを進める場合true、進行を止める場合false
     */
    static bool ShouldAdvanceStageScroll(const SideScrollingShooter& shooter);

    /**
     * @brief 現在のStage 5状態で通常チャプター進行を行うか判定する
     * @param shooter 判定対象
     * @return チャプター終了判定と通常敵生成を行う場合true、Stage 5側が進行を所有する場合false
     */
    static bool UsesChapterTimeline(const SideScrollingShooter& shooter);

    /**
     * @brief 現在のStage 5状態でプレイヤー被弾を無効にするか判定する
     * @param shooter 判定対象
     * @return 被弾を無効にする場合true、共通ダメージを適用する場合false
     */
    static bool IsPlayerDamageIgnored(const SideScrollingShooter& shooter);

    /**
     * @brief Stage 5終幕まで完了したか判定する
     * @param shooter 判定対象
     * @return 全ゲームクリアとして扱う場合true、進行中の場合false
     */
    static bool IsGameCleared(const SideScrollingShooter& shooter);

    /**
     * @brief 現在のStage 5チェックポイントへ復帰する
     * @param shooter 更新対象
     * @return なし
     */
    static void RestartCheckpoint(SideScrollingShooter& shooter);

    /**
     * @brief EASTSOURCEの移動と攻撃を更新する
     * @param shooter 更新対象
     * @param eastsource EASTSOURCE本体
     * @return なし
     */
    static void TickBoss(SideScrollingShooter& shooter, Enemy& eastsource);

    /**
     * @brief 自機弾をStage 5固有ターゲットへ適用する
     * @param shooter 更新対象
     * @param shot 判定する自機弾
     * @return Stage 5固有ターゲットへ命中して共通敵判定を省略する場合true、共通敵判定へ進む場合false
     */
    static bool TryDamageStageTarget(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief 自機弾とEASTSOURCE部位の衝突を判定する
     * @param shooter 判定対象
     * @param shot 判定する自機弾
     * @param boss EASTSOURCE本体
     * @param part 命中部位の格納先
     * @return EASTSOURCEの専用部位へ命中した場合true、命中しない場合false
     */
    static bool TryHitBossPart(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss, BossPart& part);

    /**
     * @brief EASTSOURCE撃破後の専用遷移を処理する
     * @param shooter 更新対象
     * @param boss 撃破されたボス
     * @return 専用撃破遷移を完了して共通撃破処理を省略する場合true、共通撃破処理を使用する場合false
     */
    static bool HandleBossDefeat(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 敵弾用スロット不足時に自機弾を置換できるか判定する
     * @param enemy 生成対象が敵弾の場合true
     * @return Stage 5の予告済み敵弾として置換を許可する場合true、プールを変更しない場合false
     */
    static bool CanReplacePlayerShot(bool enemy);

    /**
     * @brief Stage 5の壁面上昇と崩壊に合わせてレールカメラを補正する
     * @param shooter 判定対象
     * @param railPosition 補正するカメラ位置
     * @param railTarget 補正する注視点
     * @return なし
     */
    static void ApplyCameraCorrection(const SideScrollingShooter& shooter,
        Vector3& railPosition, Vector3& railTarget);

    /**
     * @brief Stage 5用カメラFar Clipを取得する
     * @return Stage 5のFar Clip
     */
    static constexpr float CameraFarClip() { return 220.0f; }

    /**
     * @brief 現在の演出状態で敵を描画するか判定する
     * @param shooter 判定対象
     * @param enemy 描画候補
     * @return 描画する場合true、格納庫内に隠す場合false
     */
    static bool ShouldDrawEnemy(const SideScrollingShooter& shooter, const Enemy& enemy);

    /**
     * @brief Stage 5専用3Dワールドを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @return なし
     */
    static void DrawStageWorld3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief Stage 5専用2D画面エフェクトを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @return なし
     */
    static void DrawOverlay2D(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief Stage 5専用3D画面エフェクトを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @return なし
     */
    static void DrawOverlay3D(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief Stage 5専用HUDを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @return 専用HUDが共通ボスHUDを置き換える場合true、共通ボスHUDを描画する場合false
     */
    static bool DrawHud(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief EASTSOURCE専用攻撃予告を描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @param enemy 攻撃予告を持つ敵
     * @param size 共通処理で算出した予告サイズ
     * @return 専用予告を描画して共通予告を省略する場合true、共通予告を使用する場合false
     */
    static bool DrawSpecialAttackWarning3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float size);

    /**
     * @brief EASTSOURCE専用モデルを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @param enemy 描画する敵
     * @return 専用モデルを描画して共通ボスモデルを省略する場合true、共通ボスモデルを使用する場合false
     */
    static bool DrawBossModel(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& enemy);

    /**
     * @brief EASTSOURCEの破壊部位を専用デブリへ変換する
     * @param shooter 更新対象
     * @param enemy デブリ生成元
     * @param bossPart 破壊部位
     * @return 専用デブリを生成して共通デブリ生成を省略する場合true、共通デブリ生成を使用する場合false
     */
    static bool SpawnBossDebris(SideScrollingShooter& shooter,
        const Enemy& enemy, int bossPart = -1);

private:
    class StageDefinitionImpl;

    static float MoveTowards(float current, float target, float maxDelta);
    static void SaveCheckpoint(SideScrollingShooter& shooter, Stage5Checkpoint checkpoint);
    static void StartPhase(SideScrollingShooter& shooter,
        Stage5Phase phase, bool saveCheckpoint = true);
    static void StartEastsourceBattle(SideScrollingShooter& shooter);
    static void TickEastsource(SideScrollingShooter& shooter, Enemy& eastsource);
    static void DefeatEastsource(SideScrollingShooter& shooter, Enemy& eastsource);
    static void ResetWallSearchlights(SideScrollingShooter& shooter, int activeCount);
    static void FireSearchlightVolley(SideScrollingShooter& shooter,
        const SearchlightState& light, int lightIndex);
    static void TickSearchlights(SideScrollingShooter& shooter,
        int activeCount, bool tayamaWeakpoints);
    static void SpawnEnemyShotAt(SideScrollingShooter& shooter,
        float sourceX, float sourceY, float sourceZ,
        float targetX, float targetY, float targetZ, float speed);
    static Stage5ModelTransform EastsourceTransform(
        const SideScrollingShooter& shooter, const Enemy& eastsource);
    static EastsourceModelState EastsourceState(const Enemy& eastsource);
    static Stage5ModelTransform TayamaTransform(const SideScrollingShooter& shooter);
    static TayamaModelState TayamaState(const SideScrollingShooter& shooter);
    static bool TryDamageWallSearchlight(SideScrollingShooter& shooter, Shot& shot);
    static bool TryDamageTayama(SideScrollingShooter& shooter, Shot& shot);
    static void UpdateTayamaBossHp(SideScrollingShooter& shooter);
    static void StartTayamaPhase(SideScrollingShooter& shooter,
        Stage5Phase phase, bool resetCurrentHp = true);
    static void TickTayama(SideScrollingShooter& shooter);
    static void TickStateMachine(SideScrollingShooter& shooter);
    static void PlayCue(SideScrollingShooter& shooter, int cue);
    static void DrawWeather(const SideScrollingShooter& shooter, Renderer& renderer);
    static void DrawStage5Hud(const SideScrollingShooter& shooter, Renderer& renderer);
};
