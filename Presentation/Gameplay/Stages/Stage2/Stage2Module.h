#pragma once

#include "../../SideScrollingShooter.h"

/** @brief Stage 2固有の進行、戦闘、描画を提供する静的モジュール */
class SideScrollingShooter::Stage2Module final {
public:
    Stage2Module() = delete;

    /**
     * @brief 指定難易度のStage 2定義を取得する
     * @param difficulty 取得する難易度
     * @return 難易度に対応するStage 2定義
     */
    static const Stage& Definition(DifficultyType difficulty);

    /**
     * @brief Stage 2固有状態を初期化する
     * @param shooter 初期化するゲーム本体
     * @return なし
     */
    static void Reset(SideScrollingShooter& shooter);

    /**
     * @brief Stage 2ボス専用Behaviorを取得する
     * @return Stage 2ボス専用Behavior
     */
    static const EnemyBehavior& BossBehaviorInstance();

    /**
     * @brief Stage 2ボスを合体状態の初期位置へ配置する
     * @param boss 初期化するボス
     * @param railMode レール表示中の場合true
     * @param stageIndex ステージ番号
     * @return なし
     */
    static void ConfigureBossSpawn(Enemy& boss, bool railMode, int stageIndex);

    /**
     * @brief HP割合に応じたStage 2ボスの状態と攻撃を更新する
     * @param shooter 更新するゲーム本体
     * @param boss 更新するボス
     * @return なし
     */
    static void TickBoss(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief Behavior更新後のレールガン被弾と骨アーチ接触を処理する
     * @param shooter 更新するゲーム本体
     * @param boss 判定するStage 2ボス
     * @return プレイヤー被弾で敵更新全体を中断する場合true、後続処理へ進む場合false
     */
    static bool HandleBossInteractionAfterTick(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief Stage 2ボスが特殊行動中か判定する
     * @param shooter 判定に使用するゲーム本体
     * @param boss 判定するStage 2ボス
     * @return 通常弾幕を停止する特殊行動中の場合true、通常行動中の場合false
     */
    static bool IsBossSpecialAttackActive(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief 指定球が砂漠の骨アーチへ接触したか判定する
     * @param shooter 判定に使用するゲーム本体
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 骨アーチへ接触している場合true、接触していない場合false
     */
    static bool HitsHazard(const SideScrollingShooter& shooter,
        float x, float y, float z, float radius);

    /**
     * @brief 自機弾が骨アーチへ命中した場合にダメージを適用する
     * @param shooter ダメージと演出を適用するゲーム本体
     * @param shot 命中判定対象の弾
     * @return 骨アーチへの命中処理を完了した場合true、共通敵判定へ進める場合false
     */
    static bool TryDamageStageTarget(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief 自機弾とStage 2ボス部位の衝突を判定する
     * @param shooter 判定に使用するゲーム本体
     * @param shot 判定する自機弾
     * @param boss 判定するStage 2ボス
     * @param part 命中部位の格納先
     * @return 専用部位へ命中した場合true、命中していない場合false
     */
    static bool TryHitBossPart(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss, BossPart& part);

    /**
     * @brief 自機弾とStage 2上部戦艦の船体との衝突を判定する
     * @param shooter 判定に使用するゲーム本体
     * @param shot 判定する自機弾
     * @param boss 判定するStage 2ボス
     * @return 上部戦艦へ命中した場合true、命中していない場合false
     */
    static bool TryHitBossBody(const SideScrollingShooter& shooter,
        const Shot& shot, const Enemy& boss);

    /**
     * @brief Stage 2ボスの生存部位から現在フェーズの弾幕を発射する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 発射元となるStage 2ボス
     * @return なし
     */
    static void FireBossPartBarrage(SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Stage 2特殊弾を座標加算前に更新する
     * @param shooter 更新するゲーム本体
     * @param shot 更新する弾
     * @return なし
     */
    static void TickSpecialShotBeforeMove(SideScrollingShooter& shooter, Shot& shot);

    /**
     * @brief Stage 2特殊弾を座標加算後に更新する
     * @param shooter 更新するゲーム本体
     * @param shot 更新する弾
     * @param previousY 座標加算前のゲーム座標Y
     * @return なし
     */
    static void TickSpecialShotAfterMove(
        SideScrollingShooter& shooter, Shot& shot, float previousY);

    /**
     * @brief Stage 2特殊弾が画面外カリング猶予中か判定する
     * @param shot 判定する弾
     * @return カリングを保留する場合true、通常カリングを行う場合false
     */
    static bool IsShotCullProtected(const Shot& shot);

    /**
     * @brief Stage 2敵弾のプレイヤー命中半径を取得する
     * @param shot 判定する敵弾
     * @param railMode レール表示中の場合true
     * @return 表示モードと弾種に対応する命中半径
     */
    static float EnemyShotHitRadius(const Shot& shot, bool railMode);

    /**
     * @brief Stage 2特殊デブリを更新する
     * @param shooter 更新するゲーム本体
     * @param debris 更新するデブリ
     * @return 専用処理だけで更新を完了した場合true、共通更新も必要な場合false
     */
    static bool TickSpecialDebris(SideScrollingShooter& shooter, Debris& debris);

    /**
     * @brief Stage 2ボス撃破後の報酬と船体崩壊演出を開始する
     * @param shooter 更新するゲーム本体
     * @param boss 撃破されたStage 2ボス
     * @return 専用撃破処理を完了した場合true、既に無効なボスの場合false
     */
    static bool HandleBossDefeat(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief カメラ設定前にStage 2の昼夜の空を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @return なし
     */
    static void DrawSky(const SideScrollingShooter& shooter, Renderer& renderer);

    /**
     * @brief Stage 2の横視点砂漠背景を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の横視点カメラ
     * @return なし
     */
    static void DrawBackground2D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera);

    /**
     * @brief Stage 2の視点補間済み砂漠背景を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在のレール視点カメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawBackground3D(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

    /**
     * @brief Stage 2ボス専用モデルとレールガンを描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param boss 描画する敵
     * @param yaw モデルのY軸回転
     * @return Stage 2ボスを専用描画した場合true、共通敵描画へ進める場合false
     */
    static bool DrawBossModel(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Enemy& boss, float yaw);

    /**
     * @brief Stage 2特殊弾を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param shot 描画する弾
     * @param yaw 横視点からレール視点へのモデル回転
     * @return 特殊弾を専用描画した場合true、共通弾描画へ進める場合false
     */
    static bool DrawSpecialShot(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw);

    /**
     * @brief Stage 2特殊デブリと専用演出を描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param debris 描画するデブリ
     * @param railWeight 横視点からレール視点への補間率
     * @return 特殊デブリを専用描画した場合true、共通デブリ描画へ進める場合false
     */
    static bool DrawSpecialDebris(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, const Debris& debris, float railWeight);

    /**
     * @brief Stage 2ボスの破壊部位または撃破時船体を専用デブリへ変換する
     * @param shooter デブリを生成するゲーム本体
     * @param boss デブリ生成元のStage 2ボス
     * @param bossPart 破壊部位、撃破時の残存船体は-1
     * @return Stage 2ボスとして専用デブリを生成した場合true、対象外の場合false
     */
    static bool SpawnBossDebris(SideScrollingShooter& shooter,
        const Enemy& boss, int bossPart = -1);

    /**
     * @brief Stage 2ボス登場演出の総フレーム数を取得する
     * @return 降下と合体を合わせたフレーム数
     */
    static int BossIntroductionFrames();

private:
    class BossBehavior;

    static constexpr int BossApproachFrames = 90;
    static constexpr int BossAssemblyFrames = 90;
    static constexpr int RailgunCycleFrames = 180;
    static constexpr int RailgunFireFrame = 60;
    static constexpr int RailgunVisualFrames = 12;
    static constexpr int RailgunMirageFrames = 36;
    static constexpr int FirstSinkEndFrame = 108;
    static constexpr int ResurfaceStartFrame = 148;
    static constexpr int DustLifetimeFrames = 28;

    static_assert(BossApproachFrames + BossAssemblyFrames == 180);
    static_assert(RailgunFireFrame + RailgunMirageFrames <= RailgunCycleFrames);
    static_assert(FirstSinkEndFrame < ResurfaceStartFrame);

    /**
     * @brief Stage 2ボス行動を切り替えて経過時間を加算前へ戻す
     * @param shooter 更新するゲーム本体
     * @param action 次の行動
     * @return なし
     */
    static void ChangeBossAction(
        SideScrollingShooter& shooter, ShooterStages::Stage2::BossAction action);

    /**
     * @brief 合体状態の重量感ある移動とミサイル発射を更新する
     * @param shooter 更新するゲーム本体
     * @param boss 更新するStage 2ボス
     * @return なし
     */
    static void TickBossPhase1(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 潜砂艦追従とファンネル射出を更新する
     * @param shooter 更新するゲーム本体
     * @param boss 更新するStage 2ボス
     * @return なし
     */
    static void TickBossPhase2(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 分離後のレールガンとファンネル攻撃を更新する
     * @param shooter 更新するゲーム本体
     * @param boss 更新するStage 2ボス
     * @return なし
     */
    static void TickBossPhase3(SideScrollingShooter& shooter, Enemy& boss);

    /**
     * @brief 生存中の側面ハッチからファンネルを射出する
     * @param shooter 弾を生成するゲーム本体
     * @param boss 射出元のStage 2ボス
     * @param sequence 射出順序
     * @param delayedEngine 短い落下後にエンジンを起動する場合true
     * @return なし
     */
    static void LaunchFunnel(SideScrollingShooter& shooter,
        Enemy& boss, int sequence, bool delayedEngine);

    /**
     * @brief Stage 2ファンネルを固定長弾プールへ生成する
     * @param shooter 弾を生成するゲーム本体
     * @param x 発射X座標
     * @param y 発射Y座標
     * @param z 発射Z座標
     * @param delayedEngine 短い落下後にエンジンを起動する場合true
     * @return なし
     */
    static void SpawnFunnel(SideScrollingShooter& shooter,
        float x, float y, float z, bool delayedEngine);

    /**
     * @brief Stage 2ミサイルを固定長弾プールへ生成する
     * @param shooter 弾を生成するゲーム本体
     * @param x 発射X座標
     * @param y 発射Y座標
     * @param z 発射Z座標
     * @param side 側面ハッチの左右符号
     * @return なし
     */
    static void SpawnMissile(SideScrollingShooter& shooter,
        float x, float y, float z, float side);

    /**
     * @brief 骨アーチを破壊して各関節を飛散させる
     * @param shooter 更新するゲーム本体
     * @return なし
     */
    static void DestroyBoneArch(SideScrollingShooter& shooter);

    /**
     * @brief Stage 2潜砂艦のPhase 1接地基準Y座標を取得する
     * @param shooter 座標計算に使用するゲーム本体
     * @param boss 座標を求めるStage 2ボス
     * @return 切削爪が地中へ収まるワールドY座標
     */
    static float Phase1SubmarineWorldY(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Stage 2潜砂艦の現在の親Y座標を取得する
     * @param shooter 座標計算に使用するゲーム本体
     * @param boss 座標を求めるStage 2ボス
     * @return 描画と当たり判定で共有するワールドY座標
     */
    static float SubmarineWorldY(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Stage 2上部戦艦の現在の親Y座標を取得する
     * @param shooter 座標計算に使用するゲーム本体
     * @param boss 座標を求めるStage 2ボス
     * @return 描画と当たり判定で共有するワールドY座標
     */
    static float BattleshipWorldY(
        const SideScrollingShooter& shooter, const Enemy& boss);

    /**
     * @brief Stage 2レールガンの雷撃音を再生する
     * @param shooter 音声サービスを保持するゲーム本体
     * @return なし
     */
    static void PlayRailgunSound(SideScrollingShooter& shooter);

    /**
     * @brief Stage 2撃破演出の振動音または最終爆発音を再生する
     * @param shooter 音声サービスを保持するゲーム本体
     * @param finalExplosion 最終爆発音を再生する場合true
     * @return なし
     */
    static void PlayDefeatSound(SideScrollingShooter& shooter, bool finalExplosion);

    /**
     * @brief 昼から夜への補間率を取得する
     * @param shooter 現在フレームを保持するゲーム本体
     * @return 0.0fから1.0fの昼夜補間率
     */
    static float NightBlend(const SideScrollingShooter& shooter);

    /**
     * @brief スクロール座標をNDC横幅へ循環させる
     * @param value 循環前のX座標
     * @return -1.0f以上1.0f未満のX座標
     */
    static float WrapNdcX(float value);

    /**
     * @brief 距離を0以上の循環範囲へ収める
     * @param value 循環前の距離
     * @param length 循環範囲の長さ
     * @return 0以上length未満の距離
     */
    static float WrapDistance(float value, float length);

    /**
     * @brief 巨大骨アーチを現在の視点補間位置へ描画する
     * @param shooter 描画に使用するゲーム本体
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawBoneArch(const SideScrollingShooter& shooter,
        Renderer& renderer, const Camera3D& camera, float railWeight);

    /**
     * @brief 砂面から放物線状に舞う砂埃を描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param position 発生地点のワールド座標
     * @param age 発生からの経過フレーム
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    static void DrawSandDust(Renderer& renderer, const Camera3D& camera,
        const Vector3& position, int age, float railWeight);
};
