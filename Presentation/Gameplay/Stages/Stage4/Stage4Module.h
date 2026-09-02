#pragma once

#include "../../SideScrollingShooter.h"

/**
 * @brief Stage 4の定義と専用描画入口を集約する
 */
class SideScrollingShooter::Stage4Module final {
public:
    Stage4Module() = delete;

    /**
     * @brief Stage 4の敵出現とボス弾幕定義を取得する
     * @return Stage 4の不変定義
     */
    static const Stage& Definition();

    /**
     * @brief Stage 4ボスの専用モデル描画を試みる
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 現在の描画カメラ
     * @param enemy 描画対象のボス
     * @param yaw ボス全体のY軸回転
     * @return 専用モデルを描画済みならtrue、共通ボス描画を使用する場合false
     */
    static bool DrawBossModel(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw);

private:
    class StageDefinitionImpl;
};
