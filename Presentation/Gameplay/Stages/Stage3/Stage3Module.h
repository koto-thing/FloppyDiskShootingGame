#pragma once

#include "../../SideScrollingShooter.h"
#include "../Common/BossModelTransform.h"

/**
 * @brief Stage 3の定義、海背景、ウミヘビを集約する
 */
class SideScrollingShooter::Stage3Module final {
public:
    static constexpr int Phase2SurvivalFrames = 30 * 60;
    static constexpr int Phase3SurvivalFrames = 60 * 60;

    Stage3Module() = delete;

    /**
     * @brief Phase2耐久戦の残り時間からボスHPを取得する
     * @param startHp Phase2開始時HP
     * @param remainingFrames 耐久戦の残りフレーム数
     * @return 残り時間に比例して減少するHP
     */
    static constexpr int Phase2HpForRemainingFrames(int startHp, int remainingFrames) {
        if (startHp <= 0 || remainingFrames <= 0) return 0;
        if (remainingFrames >= Phase2SurvivalFrames) return startHp;
        return (startHp * remainingFrames + Phase2SurvivalFrames - 1) /
            Phase2SurvivalFrames;
    }

    /**
     * @brief Phase3耐久戦の残り時間からボスHPを取得する
     * @param startHp Phase3開始時HP
     * @param remainingFrames 耐久戦の残りフレーム数
     * @return 残り時間に比例して減少するHP
     */
    static constexpr int Phase3HpForRemainingFrames(int startHp, int remainingFrames) {
        if (startHp <= 0 || remainingFrames <= 0) return 0;
        if (remainingFrames >= Phase3SurvivalFrames) return startHp;
        return (startHp * remainingFrames + Phase3SurvivalFrames - 1) / Phase3SurvivalFrames;
    }

    /**
     * @brief Stage3固有状態を初期化する
     * @param shooter 初期化するゲーム本体
     * @return なし
     */
    static void Reset(SideScrollingShooter& shooter);

    /**
     * @brief Stage 3の敵出現とボス弾幕定義を取得する
     * @return Stage 3の不変定義
     */
    static const Stage& Definition();

    /**
     * @brief カメラ設定前に夜明けで変化する空を描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @return なし
     */
    static void DrawSky(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief 安定した横視点用の海背景とウミヘビを描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 横視点カメラ
     * @return なし
     */
    static void DrawBackground2D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief 横視点とレール視点を補間した海背景とウミヘビを描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 現在の描画カメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawBackground3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

    /**
     * @brief 指定球が現在のウミヘビへ接触したか判定する
     * @param shooter 現在のゲーム状態
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return ウミヘビへ接触した場合true、接触していない場合false
     */
    static bool HitsHazard(const SideScrollingShooter& shooter,
        float x, float y, float z, float radius);

    /**
     * @brief Stage3ボスを導入演出開始位置へ配置する
     * @param boss 初期化するボス
     * @param railMode レール表示中の場合true
     * @param stageIndex ステージ番号
     * @return なし
     */
    static void ConfigureBossSpawn(Enemy& boss, bool railMode, int stageIndex);

    /**
     * @brief Stage3ボスのPhase1区画進行とPhase2耐久戦を更新する
     * @param shooter 更新するゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    static void TickBoss(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief Stage3ボスの区画移動中またはPhase1完了後か判定する
     * @param shooter 判定するゲーム本体
     * @param boss 判定するボス
     * @return 通常砲撃を止める場合true
     */
    static bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief 現在区画の上部砲台と自機弾の衝突を判定する
     * @param shooter 判定するゲーム本体
     * @param shot 判定する自機弾
     * @param boss 判定するボス
     * @param part 命中した部位番号の格納先
     * @return 現在区画の砲台へ命中した場合true
     */
    static bool TryHitBossPart(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss, BossPart& part);

    /**
     * @brief Phase1の飛行戦艦船体が自機弾を遮るか判定する
     * @param shooter 現在のゲーム状態
     * @param shot 判定する自機弾
     * @param boss 判定するボス
     * @return 船体へ命中した場合true
     */
    static bool BlocksPlayerShot(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss);

    /**
     * @brief 自機弾をPhase3反射ファンネルへ適用する
     * @param shooter 更新するゲーム本体
     * @param shot 判定する自機弾
     * @return 命中処理を完了した場合true
     */
    static bool TryDamageStageTarget(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief Phase1上部砲台またはPhase2ゴンドラ武装から弾幕を発射する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 発射元ボス
     * @return なし
     */
    static void FireBossPartBarrage(SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Stage3小型ミサイルを短時間だけ自機へ旋回させる
     * @param shooter 更新するゲーム本体
     * @param shot 更新する敵弾
     * @return なし
     */
    static void TickSpecialShotBeforeMove(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief Stage3小型ミサイルの生成直後カリングを猶予するか判定する
     * @param shot 判定する弾
     * @return カリングを猶予する場合true
     */
    static bool IsShotCullProtected(const Shot& shot);

    /**
     * @brief Stage3敵弾のプレイヤー命中半径を取得する
     * @param shot 判定する敵弾
     * @param railMode レール表示中の場合true
     * @return ゲーム座標またはワールド座標の命中半径
     */
    static float EnemyShotHitRadius(const Shot& shot, bool railMode);

    /**
     * @brief Stage3反射弾がプレイヤーへ向かう区間か判定する
     * @param shot 判定する敵弾
     * @return プレイヤーへ命中可能な場合true
     */
    static bool CanEnemyShotDamagePlayer(const Shot& shot);

    /**
     * @brief Phase3反射弾を専用モデルで描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先
     * @param camera 現在のカメラ
     * @param shot 描画する弾
     * @param yaw 弾のY軸回転
     * @return 専用描画を行った場合true
     */
    static bool DrawSpecialShot(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw);

    /**
     * @brief 破壊した上部砲台を専用デブリへ変換する
     * @param shooter デブリを生成するゲーム本体
     * @param boss デブリ生成元ボス
     * @param bossPart 破壊した部位番号
     * @return Stage3ボス部位を処理した場合true
     */
    static bool SpawnBossDebris(
        SideScrollingShooter& shooter, const Enemy& boss, int bossPart);

    /**
     * @brief 超巨大飛行戦艦、武装、Phase2バリアを描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param boss 描画するボス
     * @param yaw 共通処理が算出したモデル回転
     * @return Stage3ボスを描画した場合true
     */
    static bool DrawBossModel(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& boss, float yaw);

    /**
     * @brief 導入演出、Phase1区画、Phase2移動に合わせてレールカメラを補正する
     * @param shooter 判定するゲーム本体
     * @param railPosition 補正するカメラ位置
     * @param railTarget 補正する注視点
     * @return なし
     */
    static void ApplyCameraCorrection(const SideScrollingShooter& shooter,
        Vector3& railPosition, Vector3& railTarget);

    /**
     * @brief Phase2バリア内へ合わせた2DカメラY座標を取得する
     * @param shooter 判定対象
     * @return ワールド座標系のカメラY座標
     */
    static float SideCameraY(const SideScrollingShooter& shooter);

    /**
     * @brief Phase2バリア内の2D自機Y移動範囲を取得する
     * @param shooter 判定対象
     * @return Xを下限、Yを上限とするゲーム座標
     */
    static Vector2 SidePlayerYRange(const SideScrollingShooter& shooter);

    /**
     * @brief Stage3の海面、甲板、Phase2バリア下面の高度を取得する
     * @param shooter 判定するゲーム本体
     * @return プレイヤー下限と影に使用するワールドY座標
     */
    static float RailGroundY(const SideScrollingShooter& shooter);

    /**
     * @brief Phase2バリア上面からレール視点自機Y座標上限を取得する
     * @param shooter 判定対象
     * @return ゲーム座標系のY座標上限
     */
    static float RailPlayerMaxY(const SideScrollingShooter& shooter);

    /**
     * @brief 導入演出の出現時刻に達したボスだけを描画する
     * @param shooter 判定するゲーム本体
     * @param enemy 描画候補
     * @return 描画する場合true
     */
    static bool ShouldDrawEnemy(const SideScrollingShooter& shooter, const Enemy& enemy);

    /**
     * @brief Stage3で視点切り替えを禁止するか判定する
     * @param shooter 判定するゲーム本体
     * @return 常にfalse
     */
    static bool IsViewLocked(const SideScrollingShooter& shooter);

    /**
     * @brief 操作可能なStage3ボス出現演出を更新する
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickBossIntroduction(SideScrollingShooter& shooter);

    /**
     * @brief Stage3ボス出現演出の長さを取得する
     * @return 演出フレーム数
     */
    static int BossIntroductionFrames();

private:
    class StageDefinitionImpl;
    struct SeaSerpentMotion;
    struct SeaSerpentSegment;

    /**
     * @brief スクロール座標をNDCの横幅へ循環させる
     * @param value 循環前のX座標
     * @return -1.0f以上1.0f未満のX座標
     */
    static float WrapNdcX(float value);

    /**
     * @brief 現在フレームの夜空から昼空への補間率を取得する
     * @param frame Stage 3開始からの経過フレーム
     * @return 夜空を1、昼空を0とする補間率
     */
    static float NightBlend(int frame);

    /**
     * @brief 現フレームのウミヘビ行動と胴体配置を取得する
     * @param frame Stage 3開始からの経過フレーム
     * @param motion 行動情報の格納先
     * @return ウミヘビが海面上にいる場合true、水中にいる場合false
     */
    static bool GetSeaSerpentMotion(int frame, SeaSerpentMotion& motion);

    /**
     * @brief ウミヘビの各胴体節が頭から遅れて一周する進行率を取得する
     * @param progress 行動全体の進行率
     * @param segmentCount 胴体節数
     * @param segmentDelay 隣接する胴体節間の遅延
     * @param segmentIndex 頭を0とする胴体節番号
     * @return 水中を0、再入水完了を1とする進行率
     */
    static constexpr float GetSeaSerpentSegmentProgress(float progress,
        int segmentCount, float segmentDelay, int segmentIndex);

    /**
     * @brief 描画と当たり判定で共有する胴体節の配置を取得する
     * @param motion 現在のウミヘビ行動
     * @param segmentIndex 頭を0とする胴体節番号
     * @return 横視点とレール視点の胴体節配置
     */
    static SeaSerpentSegment GetSeaSerpentSegment(
        const SeaSerpentMotion& motion, int segmentIndex);

    /**
     * @brief 海面から飛び出すウミヘビと水しぶきを描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 現在の描画カメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawSeaSerpent(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

    /**
     * @brief 導入演出を含むStage3ボスの描画Transformを取得する
     * @param shooter 現在のゲーム状態
     * @param boss 対象ボス
     * @return 飛行戦艦の親Transform
     */
    static BossModelTransform BossTransform(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief 現在区画の砲台から自機狙いまたはランダム方向へ機銃弾を発射する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 発射元ボス
     * @param aimed 自機狙いならtrue、ランダム散射ならfalse
     * @return なし
     */
    static void FireBossMachineGun(
        SideScrollingShooter& shooter, const Enemy& boss, bool aimed);

    /**
     * @brief Phase1区画切り替えを含む連続進行量を取得する
     * @param boss 対象ボス
     * @return 艦尾区画を0とする連続進行量
     */
    static float Phase1SectionProgress(const Enemy& boss);

    /**
     * @brief Phase1進行に合わせて戦艦を手前へ送るZ移動量を取得する
     * @param boss 対象ボス
     * @return 戦艦へ加算するワールドZ移動量
     */
    static float Phase1AdvanceZ(const Enemy& boss);

    /**
     * @brief 現在区画へ補間したカメラ注視Zを取得する
     * @param boss 対象ボス
     * @return ワールドZ座標
     */
    static float Phase1FocusZ(const Enemy& boss);

    /**
     * @brief ボス戦開始後の海面下降量を取得する
     * @param shooter 現在のゲーム状態
     * @return ワールドY方向の下降量
     */
    static float BossSeaDrop(const SideScrollingShooter& shooter);
};
