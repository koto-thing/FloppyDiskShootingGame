#pragma once

#include "../../SideScrollingShooter.h"
#include "BossStory.h"

/**
 * @brief ステージ番号による内部振り分けを集約する
 */
class SideScrollingShooter::StageDispatch final {
public:
    StageDispatch() = delete;

    /**
     * @brief 指定番号と難易度のステージ定義を取得する
     * @param stageNumber 取得するステージ番号
     * @param difficulty 取得する難易度
     * @return 指定条件に対応するステージ定義
     */
    static const Stage& Definition(int stageNumber, DifficultyType difficulty);

    /**
     * @brief 指定ステージのボス戦前会話を取得する
     * @param stageNumber 取得するステージ番号
     * @return 指定ステージの台詞とボス名
     */
    static BossStory Story(int stageNumber);

    /**
     * @brief 既存のステージギミック初期化へ転送する
     * @param shooter 初期化するゲーム本体
     * @return なし
     */
    static void ResetGimmicks(SideScrollingShooter& shooter);

    /**
     * @brief 既存のStage 5初期化へ転送する
     * @param shooter 初期化するゲーム本体
     * @return なし
     */
    static void ResetScriptState(SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージのデバッグ専用入力を処理する
     * @param shooter 更新対象
     * @return なし
     */
    static void ProcessDebugInput(SideScrollingShooter& shooter);

    /**
     * @brief Bキーによる現在ステージの専用ボス直行を処理する
     * @param shooter 更新対象
     * @return 専用直行を処理した場合true、共通ボス直行を使用する場合false
     */
    static bool HandleDebugBossInput(SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージの専用ボス開始処理を試みる
     * @param shooter 更新対象
     * @return 専用開始処理を完了した場合true、共通開始処理を使用する場合false
     */
    static bool StartDebugBoss(SideScrollingShooter& shooter);

    /**
     * @brief 共通フレーム番号加算前のステージスクリプトを更新する
     * @param shooter 更新対象
     * @return なし
     */
    static void TickBeforeFrame(SideScrollingShooter& shooter);

    /**
     * @brief 共通クールダウン更新後のステージスクリプトと環境音を更新する
     * @param shooter 更新対象
     * @return なし
     */
    static void TickAfterFrame(SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージが共通チャプター時間軸を使用するか判定する
     * @param shooter 判定対象
     * @return 共通チャプター終了と敵生成を使用する場合true、ステージ側が時間軸を所有する場合false
     */
    static bool UsesChapterTimeline(const SideScrollingShooter& shooter);

    /**
     * @brief チャプター結果終了後のステージ固有遷移を処理する
     * @param shooter 更新対象
     * @return 固有遷移を完了して共通処理を中断する場合true、共通処理を続ける場合false
     */
    static bool HandleChapterResult(SideScrollingShooter& shooter);

    /**
     * @brief 新しいチャプター開始を現在ステージへ通知する
     * @param shooter 更新対象
     * @return なし
     */
    static void OnChapterStarted(SideScrollingShooter& shooter);

    /**
     * @brief ステージ固有チェックポイントへの復帰を試みる
     * @param shooter 更新対象
     * @return 固有復帰を完了した場合true、共通復帰を使用する場合false
     */
    static bool TryRestartCheckpoint(SideScrollingShooter& shooter);

    /**
     * @brief プレイヤー更新後のステージ固有ワールドを更新する
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickWorld(SideScrollingShooter& shooter);

    /**
     * @brief ボスBehavior更新直後のステージ固有接触処理を行う
     * @param shooter 更新対象
     * @param boss 更新済みのボス
     * @return プレイヤー被弾により敵更新全体を中断する場合true、敵更新を続ける場合false
     */
    static bool HandleBossInteractionAfterTick(
        SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 現在ステージのボスが共通弾幕を止める特殊攻撃中か判定する
     * @param shooter 判定対象
     * @param boss 判定するボス
     * @return 共通弾幕を止める場合true、共通弾幕を許可する場合false
     */
    static bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief 指定球が現在ステージの障害物へ接触したか判定する
     * @param shooter 判定に使用するゲーム本体
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 障害物へ接触している場合true、接触していない場合false
     */
    static bool HitsHazard(const SideScrollingShooter& shooter,
        float x, float y, float z, float radius);

    /**
     * @brief 自機弾を現在ステージの固有ターゲットへ適用する
     * @param shooter ダメージを適用するゲーム本体
     * @param shot 判定対象の自機弾
     * @return 固有ターゲットへの命中処理を完了した場合true、共通敵判定へ進める場合false
     */
    static bool TryDamageStageTarget(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief 自機弾と現在ステージのボス部位との専用判定を試みる
     * @param shooter 判定対象
     * @param shot 判定する自機弾
     * @param boss 判定するボス
     * @param part 命中部位の格納先
     * @return 現在ステージのボス部位へ命中した場合true、命中しない場合false
     */
    static bool TryHitBossPart(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss, BossPart& part);

    /**
     * @brief 自機弾と現在ステージの専用ボス船体との判定を試みる
     * @param shooter 判定対象
     * @param shot 判定する自機弾
     * @param boss 判定するボス
     * @return 専用船体へ命中した場合true、命中せず共通判定へ進む場合false
     */
    static bool TryHitBossBody(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss);

    /**
     * @brief ダメージ対象外のボス船体が自機弾を遮るか判定する
     * @param shooter 現在のゲーム状態
     * @param shot 判定する自機弾
     * @param boss 判定するボス
     * @return 船体が弾を遮る場合true
     */
    static bool BlocksPlayerShot(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss);

    /**
     * @brief 接触判定が無効なボスへ部位・船体弾判定を継続するか判定する
     * @param shooter 判定対象
     * @return 専用露出部位への判定を継続する場合true、無効なボス判定を省略する場合false
     */
    static bool CanHitBossWhileCollisionDisabled(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージの生存ボス部位から弾幕を発射する
     * @param shooter 更新対象
     * @param boss 発射元ボス
     * @return なし
     */
    static void FireBossPartBarrage(
        SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief ステージ固有弾を座標加算前に更新する
     * @param shooter 更新対象
     * @param shot 更新する弾
     * @return なし
     */
    static void TickSpecialShotBeforeMove(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief ステージ固有弾を座標加算後に更新する
     * @param shooter 更新対象
     * @param shot 更新する弾
     * @param previousX 座標加算前のゲーム座標X
     * @param previousY 座標加算前のゲーム座標Y
     * @param previousZ 座標加算前のワールド座標Z
     * @return なし
     */
    static void TickSpecialShotAfterMove(
        SideScrollingShooter& shooter, Shot& shot,
        float previousX, float previousY, float previousZ);

    /**
     * @brief ステージ固有弾が画面外カリング猶予中か判定する
     * @param shooter 判定対象
     * @param shot 判定する弾
     * @return カリングを保留する場合true、共通カリングを行う場合false
     */
    static bool IsShotCullProtected(
        const SideScrollingShooter& shooter, const Shot& shot);

    /**
     * @brief 現在ステージの敵弾命中半径を取得する
     * @param shooter 判定対象
     * @param shot 判定する敵弾
     * @return 現在表示モードに対応する命中半径
     */
    static float EnemyShotHitRadius(
        const SideScrollingShooter& shooter, const Shot& shot);

    /**
     * @brief 現在ステージの敵弾がプレイヤーへ命中可能か判定する
     * @param shooter 判定対象
     * @param shot 判定する敵弾
     * @return 命中可能な場合true
     */
    static bool CanEnemyShotDamagePlayer(
        const SideScrollingShooter& shooter, const Shot& shot);

    /**
     * @brief 現在ステージの特殊デブリ更新を試みる
     * @param shooter 更新対象
     * @param debris 更新するデブリ
     * @return 固有処理だけで更新を完了した場合true、共通更新も必要な場合false
     */
    static bool TickSpecialDebris(SideScrollingShooter& shooter, Debris& debris);

    /**
     * @brief 現在ステージの専用ボス撃破処理を試みる
     * @param shooter 更新対象
     * @param boss 撃破されたボス
     * @return 専用撃破処理を完了した場合true、共通撃破処理を使用する場合false
     */
    static bool HandleBossDefeat(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 敵弾用スロット不足時に自機弾を置換できるか判定する
     * @param shooter 判定対象
     * @param enemy 生成対象が敵弾の場合true
     * @return 現在ステージが置換を許可する場合true、プールを変更しない場合false
     */
    static bool CanReplacePlayerShot(const SideScrollingShooter& shooter, bool enemy);

    /**
     * @brief 現在ステージの専用ボスデブリ生成を試みる
     * @param shooter 更新対象
     * @param enemy デブリ生成元
     * @param bossPart 破壊部位
     * @return 専用デブリを生成した場合true、共通デブリ生成を使用する場合false
     */
    static bool SpawnBossDebris(SideScrollingShooter& shooter,
        const Enemy& enemy, int bossPart = -1);

    /**
     * @brief カメラ設定前に現在ステージの画面全体の空を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @return なし
     */
    static void DrawSky(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief 現在ステージの横視点背景を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の横視点カメラ
     * @return なし
     */
    static void DrawBackground2D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief 現在ステージのレール視点背景を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在のレール視点カメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawBackground3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

    /**
     * @brief 現在ステージの専用ボスモデル描画を試みる
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の描画カメラ
     * @param enemy 描画対象のボス
     * @param yaw ボス全体のY軸回転
     * @return 専用モデルを描画済みならtrue、共通ボスモデルを使用する場合false
     */
    static bool DrawBossModel(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw);

    /**
     * @brief 現在ステージに合わせてレールカメラを補正する
     * @param shooter 判定対象
     * @param railPosition 補正するカメラ位置
     * @param railTarget 補正する注視点
     * @return なし
     */
    static void ApplyCameraCorrection(const SideScrollingShooter& shooter,
        Vector3& railPosition, Vector3& railTarget);

    /**
     * @brief 現在ステージの2DカメラY座標を取得する
     * @param shooter 判定対象
     * @return ワールド座標系のカメラY座標
     */
    static float SideCameraY(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージの2D自機Y移動範囲を取得する
     * @param shooter 判定対象
     * @return Xを下限、Yを上限とするゲーム座標
     */
    static Vector2 SidePlayerYRange(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージのレールカメラFar Clipを取得する
     * @param shooter 判定対象
     * @return Far Clip
     */
    static float CameraFarClip(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージのレール視点地面上面Yを取得する
     * @param shooter 判定対象
     * @return ワールド座標系の地面上面Y
     */
    static float RailGroundY(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージのレール視点自機Y座標上限を取得する
     * @param shooter 判定対象
     * @return ゲーム座標系のY座標上限
     */
    static float RailPlayerMaxY(const SideScrollingShooter& shooter);

    /**
     * @brief 現在の演出状態で指定敵を描画するか判定する
     * @param shooter 判定対象
     * @param enemy 描画候補
     * @return 描画する場合true、現在の演出では描画しない場合false
     */
    static bool ShouldDrawEnemy(const SideScrollingShooter& shooter, const Enemy& enemy);

    /**
     * @brief 現在ステージの専用3Dワールドを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @return なし
     */
    static void DrawStageWorld3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief 現在ステージの2D画面エフェクトを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @return なし
     */
    static void DrawOverlay2D(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief 現在ステージの3D画面エフェクトを描画する
     * @param shooter 描画対象
     * @param renderer 描画先
     * @return なし
     */
    static void DrawOverlay3D(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief 現在ステージの専用HUD描画を試みる
     * @param shooter 描画対象
     * @param renderer 描画先
     * @return 専用HUDが共通ボスHUDを置き換えた場合true、共通ボスHUDを描画する場合false
     */
    static bool DrawHud(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief 現在ステージの専用攻撃予告描画を試みる
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @param enemy 攻撃予告を持つ敵
     * @param size 共通処理で算出した予告サイズ
     * @return 専用予告を描画した場合true、共通予告を使用する場合false
     */
    static bool DrawSpecialAttackWarning3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float size);

    /**
     * @brief 現在ステージの専用弾モデル描画を試みる
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @param shot 描画する弾
     * @param yaw モデルのY軸回転
     * @return 専用モデルを描画した場合true、共通モデルを使用する場合false
     */
    static bool DrawSpecialShot(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw);

    /**
     * @brief 現在ステージの専用デブリ描画を試みる
     * @param shooter 描画対象
     * @param renderer 描画先
     * @param camera 現在の3Dカメラ
     * @param debris 描画するデブリ
     * @param railWeight 横視点からレール視点への補間率
     * @return 専用描画を完了した場合true、共通モデルを使用する場合false
     */
    static bool DrawSpecialDebris(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera,
        const Debris& debris, float railWeight);

    /**
     * @brief 現在ステージで表示モード切り替えをロックするか判定する
     * @param shooter 判定対象
     * @return 切り替えをロックする場合true、切り替えを許可する場合false
     */
    static bool IsViewLocked(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージで背景スクロールを進めるか判定する
     * @param shooter 判定対象
     * @return スクロールを進める場合true、進行を停止する場合false
     */
    static bool ShouldAdvanceStageScroll(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージでプレイヤー被弾を無効にするか判定する
     * @param shooter 判定対象
     * @return 被弾を無効にする場合true、共通ダメージを適用する場合false
     */
    static bool IsPlayerDamageIgnored(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージでゲーム全体をクリア済みか判定する
     * @param shooter 判定対象
     * @return ゲーム全体をクリア済みの場合true、継続中の場合false
     */
    static bool IsGameCleared(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージの次に進行可能なステージがあるか判定する
     * @param shooter 判定対象
     * @return 次ステージがある場合true、最終ステージの場合false
     */
    static bool HasNextStage(const SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージのボス登場位置を更新する
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickBossIntroduction(SideScrollingShooter& shooter);

    /**
     * @brief 現在ステージのボス登場演出フレーム数を取得する
     * @param shooter 判定に使用するゲーム本体
     * @return 登場演出フレーム数
     */
    static int BossIntroductionFrames(const SideScrollingShooter& shooter);
};
