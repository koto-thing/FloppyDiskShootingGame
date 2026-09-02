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
     * @brief 自機弾とStage 4ボスの破壊可能砲部位との衝突を判定する
     * @param shooter 判定に使用するゲーム本体
     * @param shot 判定する自機弾
     * @param boss 判定するStage 4ボス
     * @param part 命中部位の格納先
     * @return 破壊可能砲部位へ命中した場合true、命中していない場合false
     */
    static bool TryHitBossPart(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss, BossPart& part);

    /**
     * @brief Stage 4ボスの生存砲部位から自機方向へ弾を発射する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 発射元となるStage 4ボス
     * @return なし
     */
    static void FireBossPartBarrage(SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Stage 4ボスの破壊済み砲部位を飛散部品へ変換する
     * @param shooter デブリを生成するゲーム本体
     * @param boss デブリ生成元のStage 4ボス
     * @param bossPart 破壊部位
     * @return Stage 4ボス部位として専用デブリを生成した場合true、対象外の場合false
     */
    static bool SpawnBossDebris(SideScrollingShooter& shooter,
        const Enemy& boss, int bossPart);

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
    /**
     * @brief Stage 4ボスモデルのY軸回転を取得する
     * @param shooter 現在のゲーム状態
     * @return 描画と判定で共有するY軸回転
     */
    static float ModelYaw(const SideScrollingShooter& shooter);

    /**
     * @brief Stage 4ボスのローカル座標をワールド座標へ変換する
     * @param shooter 現在のゲーム状態
     * @param boss 座標変換元のボス
     * @param local ローカル座標
     * @return ワールド座標
     */
    static Vector3 LocalToWorld(
        const SideScrollingShooter& shooter, const Enemy& boss, const Vector3& local);

    /**
     * @brief Stage 4ボスの副砲部位番号を配列番号へ変換する
     * @param part 判定する部位
     * @return 副砲番号、対象外は-1
     */
    static int SecondaryGunIndex(BossPart part);

    /**
     * @brief Stage 4ボスの破壊可能砲部位のローカル座標を取得する
     * @param part 取得する部位
     * @return 砲部位のローカル座標
     */
    static Vector3 BossPartLocalPosition(BossPart part);
};
