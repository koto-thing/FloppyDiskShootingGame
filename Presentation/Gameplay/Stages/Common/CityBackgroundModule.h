#pragma once

#include "../../SideScrollingShooter.h"

/**
 * @brief Stage 4とStage 5で共有する都市背景を描画する
 */
class SideScrollingShooter::CityBackgroundModule final {
public:
    CityBackgroundModule() = delete;

    /**
     * @brief カメラ設定前に都市の夜空を描画する
     * @param renderer 描画先レンダラー
     * @return なし
     */
    static void DrawSky(Renderer& renderer);

    /**
     * @brief 安定した横視点用の都市背景を描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 横視点カメラ
     * @return なし
     */
    static void DrawBackground2D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief 横視点とレール視点を補間した都市背景を描画する
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 現在の描画カメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawBackground3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

private:
    /**
     * @brief スクロール座標をNDCの横幅へ循環させる
     * @param value 循環前のX座標
     * @return -1.0f以上1.0f未満のX座標
     */
    static float WrapNdcX(float value);

    /**
     * @brief 指定距離を正の循環範囲へ収める
     * @param value 循環前の距離
     * @param length 循環範囲の長さ
     * @return 0以上length未満の距離
     */
    static float WrapDistance(float value, float length);
};
