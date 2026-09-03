#pragma once

#include "../../SideScrollingShooter.h"

/**
 * @brief Stage 1固有の進行、当たり判定、描画を提供する
 */
class SideScrollingShooter::Stage1Module final {
public:
    Stage1Module() = delete;

    /**
     * @brief 指定難易度のStage 1定義を取得する
     * @param difficulty 取得する難易度
     * @return 難易度に対応するStage 1定義
     */
    static const Stage& Definition(DifficultyType difficulty);

    /**
     * @brief Stage 1固有状態を初期化する
     * @param shooter 初期化するゲーム本体
     * @return なし
     */
    static void Reset(SideScrollingShooter& shooter);

    /**
     * @brief Stage 1の隕石を移動・回転させる
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickWorld(SideScrollingShooter& shooter);

    /**
     * @brief Stage 1ボスに接触した隕石を破壊する
     * @param shooter 更新するゲーム本体
     * @param boss 判定するStage 1ボス
     * @return 敵更新を続けるため常にfalse
     */
    static bool HandleBossInteractionAfterTick(
        SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 指定球がStage 1の隕石へ接触したか判定する
     * @param shooter 判定に使用するゲーム本体
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 隕石へ接触している場合true、接触していない場合false
     */
    static bool HitsHazard(const SideScrollingShooter& shooter,
        float x, float y, float z, float radius);

    /**
     * @brief 自機弾がStage 1の隕石へ命中した場合にダメージを適用する
     * @param shooter ダメージと演出を適用するゲーム本体
     * @param shot 命中判定対象の弾
     * @return 隕石へ命中して処理を完了した場合true、共通敵判定へ進める場合false
     */
    static bool TryDamageTarget(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief 横視点の宇宙背景、グリッド、隕石を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の横視点カメラ
     * @return なし
     */
    static void DrawBackground2D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief 横視点からレール視点へ補間した宇宙背景、グリッド、隕石を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在のレール視点カメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawBackground3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

    /**
     * @brief Stage 1ボス登場中の高速往復位置を更新する
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickBossIntroduction(SideScrollingShooter& shooter);

    /**
     * @brief Stage 1ボス登場演出の総フレーム数を取得する
     * @return 高速往復と定位置への移動を合わせたフレーム数
     */
    static int BossIntroductionFrames();

    /**
     * @brief Stage 1ボスを撃破演出へ移行する
     * @param shooter 更新するゲーム本体
     * @param boss 撃破されたボス
     * @return 専用撃破処理を完了した場合true
     */
    static bool HandleBossDefeat(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief Stage 1ボス撃破後の暴走移動と段階破壊を更新する
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickBossDefeat(SideScrollingShooter& shooter);

private:
    /**
     * @brief 指定球が接触しているStage 1隕石の番号を取得する
     * @param shooter 判定に使用するゲーム本体
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 接触した隕石の番号、接触していない場合-1
     */
    static int FindMeteor(const SideScrollingShooter& shooter,
        float x, float y, float z, float radius);

    /**
     * @brief 被弾または破壊された隕石から小隕石を飛散させる
     * @param shooter デブリを生成するゲーム本体
     * @param meteor 破片の発生元となる隕石
     * @param count 発生させる小隕石の数
     * @return なし
     */
    static void SpawnMeteorDebris(SideScrollingShooter& shooter,
        const ShooterStages::Stage1::Meteor& meteor, int count);

    /**
     * @brief プリミティブ球だけで構成したStage 1の巨大隕石を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawMeteors(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);
};
