#pragma once

#include "../../SideScrollingShooter.h"

/**
 * @brief Stage 3の定義、海背景、ウミヘビを集約する
 */
class SideScrollingShooter::Stage3Module final {
public:
    Stage3Module() = delete;

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
};
