#pragma once

#include "../../SideScrollingShooter.h"

struct Stage4MainWeaponPose;

/**
 * @brief Stage 4の定義と専用描画入口を集約する
 */
class SideScrollingShooter::Stage4Module final {
public:
    Stage4Module() = delete;

    /**
     * @brief Stage 4固有の主砲交換状態を初期化する
     * @param shooter 初期化するゲーム本体
     * @return なし
     */
    static void Reset(SideScrollingShooter& shooter);

    /**
     * @brief Stage 4主砲交換のデバッグ入力を処理する
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void ProcessDebugInput(SideScrollingShooter& shooter);

    /**
     * @brief 必要なら主砲交換を開始して一フレーム進める
     * @param shooter 更新するゲーム本体
     * @param boss Stage 4ボス
     * @return 主砲交換中の場合true、通常行動を続ける場合false
     */
    static bool TickWeaponSwap(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief Stage 4の迫撃砲照準角を目標角へ近づける
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void TickSiegeMortarAim(SideScrollingShooter& shooter);

    /**
     * @brief 現在主砲交換中か判定する
     * @param shooter 判定するゲーム本体
     * @return 交換中の場合true
     */
    static bool IsWeaponSwapActive(const SideScrollingShooter& shooter);

    /**
     * @brief Stage 4ボス被弾後の三段階フェーズ遷移とHP保護を処理する
     * @param shooter 更新するゲーム本体
     * @param boss 被弾したStage 4ボス
     * @return ボスを撃破した場合true
     */
    static bool HandleBossPhaseAfterDamage(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief Stage 4の敵出現とボス弾幕定義を取得する
     * @return Stage 4の不変定義
     */
    static const Stage& Definition(DifficultyType difficulty);

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
    static void FireBossPartBarrage(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 指定球がStage 4ボス本体へ接触したか判定する
     * @param shooter 判定に使用するゲーム本体
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return ボス本体へ接触している場合true、接触していない場合false
     */
    static bool HitsHazard(const SideScrollingShooter& shooter,
        float x, float y, float z, float radius);

    /**
     * @brief Stage 4特殊弾を座標加算前に更新する
     * @param shooter 更新するゲーム本体
     * @param shot 更新する弾
     * @return なし
     */
    static void TickSpecialShotBeforeMove(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief Stage 4特殊弾を座標加算後に更新する
     * @param shooter 更新するゲーム本体
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
     * @brief Stage 4特殊弾が画面外カリング猶予中か判定する
     * @param shot 判定する弾
     * @return カリングを保留する場合true、通常カリングを行う場合false
     */
    static bool IsShotCullProtected(const Shot& shot);

    /**
     * @brief Stage 4敵弾のプレイヤー命中半径を取得する
     * @param shot 判定する敵弾
     * @param railMode レール表示中の場合true
     * @return 表示モードと弾種に対応する命中半径
     */
    static float EnemyShotHitRadius(const Shot& shot, bool railMode);

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

    /**
     * @brief Stage 4特殊弾の専用描画を試みる
     * @param shooter 現在のゲーム状態
     * @param renderer 描画先レンダラー
     * @param camera 現在の描画カメラ
     * @param shot 描画対象の弾
     * @param yaw 表示モードに対応するY軸回転
     * @return 専用描画を行った場合true、共通描画を使用する場合false
     */
    static bool DrawSpecialShot(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw);

private:
    /**
     * @brief 現在主砲から指定主砲への交換を開始する
     * @param shooter 更新するゲーム本体
     * @param boss Stage 4ボス
     * @param incomingWeapon 搬入する主砲
     * @return なし
     */
    static void BeginWeaponSwap(SideScrollingShooter& shooter, Enemy& boss,
        ShooterStages::Stage4::MainWeaponType incomingWeapon);

    /**
     * @brief 現在の交換工程を次へ進める
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void AdvanceWeaponSwap(SideScrollingShooter& shooter);

    /**
     * @brief 指定主砲への交換をデバッグ開始する
     * @param shooter 更新するゲーム本体
     * @param incomingWeapon 搬入する主砲
     * @return なし
     */
    static void StartDebugWeaponSwap(SideScrollingShooter& shooter,
        ShooterStages::Stage4::MainWeaponType incomingWeapon);

    /**
     * @brief Stage 4主砲身から現在形態に対応する砲丸を生成する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 発射元となるStage 4ボス
     * @return なし
     */
    static void SpawnMainCannonball(SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Phase2主砲から重力つき砲丸弾幕を生成する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 発射元となるStage 4ボス
     * @param muzzle 砲口ワールド座標
     * @param pose 発射時の主砲姿勢
     * @return なし
     */
    static void SpawnSiegeMortarBarrage(
        SideScrollingShooter& shooter, const Enemy& boss,
        const Vector3& muzzle, const Stage4MainWeaponPose& pose);

    /**
     * @brief Phase3主砲から超特大砲丸を生成する
     * @param shooter 弾を生成するゲーム本体
     * @param muzzle 砲口ワールド座標
     * @param pose 発射時の主砲姿勢
     * @return なし
     */
    static void SpawnRomanceCannonShot(
        SideScrollingShooter& shooter, const Vector3& muzzle,
        const Stage4MainWeaponPose& pose);

    /**
     * @brief 次回射撃に向けた迫撃砲照準角をランダムに決める
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void ChooseNextSiegeMortarAim(SideScrollingShooter& shooter);

    /**
     * @brief 砲丸弾を固定長プールへ追加する
     * @param shooter 弾を生成するゲーム本体
     * @param muzzle 砲口ワールド座標
     * @param velocity 弾速ワールドベクトル
     * @param sideRadius 2D座標系の当たり判定半径
     * @param explosionRadius 爆破当たり判定半径
     * @param gravity 重力を使う場合true
     * @param detonateAtPlayerZ 3D中に自機Z到達で爆破する場合true
     * @param damage 自機命中時のダメージ
     * @return 生成できた場合true、空きがない場合false
     */
    static bool SpawnCannonballShot(SideScrollingShooter& shooter,
        const Vector3& muzzle, const Vector3& velocity,
        float sideRadius, float explosionRadius,
        bool gravity, bool detonateAtPlayerZ, int damage);

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
