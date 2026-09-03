#pragma once

#include <array>
#include <cstdint>

#include "../../Domain/ValueObjects/DifficultyType.h"
#include "../../Domain/ValueObjects/GalleryEntry.h"
#include "../../Domain/ValueObjects/PlayerType.h"
#include "../../Engine/Graphics/Camera3D.h"
#include "Stages/Stage1/Stage1State.h"
#include "Stages/Stage2/Stage2State.h"
#include "Stages/Stage4/Stage4State.h"
#include "Stages/Stage3/Stage3State.h"
#include "Stages/Stage5/Stage5State.h"

class AudioService;
class Renderer;
enum class PrimitiveShape;

/**
 * @brief 固定長プールで動作する横スクロールシューティングのゲーム本体
 */
class SideScrollingShooter {
public:
    /** @brief 自機弾の挙動を調整するパラメータ */
    struct PlayerShotParameters {
        int fireIntervalFrames;
        int projectileCount;
        float speed;
        float spreadAngleDegrees;
        float spawnOffsetX;
        float spawnOffsetY;
        float hitRadius;
        int damage;
        float homingStrength;
        bool piercing;
    };

    /** @brief 機体タイプ別の自機弾パラメータ */
    inline static constexpr std::array<PlayerShotParameters, 3> PlayerShotConfigs {{
        // HOMING
        { 10, 2, 0.038f, 5.0f, 0.08f, 0.20f, 0.025f, 1, 0.075f, false },
        // PIERCING
        { 14, 2, 0.052f, 0.0f, 0.10f, 0.20f, 0.032f, 2, 0.000f, true },
        // SPREAD
        { 12, 5, 0.043f, 24.0f, 0.17f, 0.00f, 0.022f, 1, 0.000f, false },
    }};

    /** @brief 全機体共通の通常弾パラメータ */
    inline static constexpr PlayerShotParameters NormalShotConfig {
        6, 1, 0.055f, 0.0f, 0.19f, 0.0f, 0.020f, 1, 0.0f, false
    };

    /**
     * @brief 指定難易度と機体タイプでゲームを初期化する
     * @param audio 効果音を再生するサービス
     * @param playerType 使用する自機タイプ
     * @param difficulty 使用する敵出現難易度
     */
    void Initialize(AudioService* audio, PlayerType playerType, DifficultyType difficulty);
    void ProcessInput();
    void Tick();
    void Render(Renderer& renderer) const;
    /**
     * @brief ゲーム画面の揺れを開始する
     * @param intensity 揺れの最大振幅
     * @param durationFrames 揺れを継続するフレーム数
     * @return なし
     */
    void ShakeScreen(float intensity = 0.3f, int durationFrames = 18);
    /**
     * @brief 全ステージをクリア済みか取得する
     * @return 最終ステージのミッション終了表示が完了した場合true、進行中の場合false
     */
    bool IsAllStagesCleared() const;
    /**
     * @brief 現在の合計スコアを取得する
     * @return 現在の合計スコア
     */
    int Score() const;

private:
    class Stage;
    class Stage1EnemySheet;
    class Stage1EnemySheetEasy;
    class Stage1EnemySheetNormal;
    class Stage1EnemySheetHard;
    class Stage2EnemySheet;
    class Stage2EnemySheetEasy;
    class Stage2EnemySheetNormal;
    class Stage2EnemySheetHard;
    class Stage3EnemySheet;
    class Stage3EnemySheetEasy;
    class Stage3EnemySheetNormal;
    class Stage3EnemySheetHard;
    class Stage4EnemySheet;
    class EnemyBehavior;
    class BasicEnemyBehavior;
    class HeavyEnemyBehavior;
    class ArmoredEnemyBehavior;
    class BossEnemyBehavior;
    class StraightShooterEnemyBehavior;
    class DiveRusherEnemyBehavior;
    class CircleShooterEnemyBehavior;
    class SquareShooterEnemyBehavior;
    class StageDispatch;
    class Stage1Module;
    class Stage2Module;
    class Stage3Module;
    class Stage4Module;
    class Stage5Module;
    class CityBackgroundModule;

    struct Shot {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float transitionSideX = 0.0f;
        float transitionSideY = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        float hitRadius = 0.025f;
        int damage = 1;
        int barrageIndex = -1;
        int barrageCount = 0;
        int age = 0;
        ShooterStages::Stage2::ShotState stage2 {};
        ShooterStages::Stage4::ShotState stage4 {};
        PlayerType playerType = Homing;
        bool enemy = false;
        bool special = false;
        bool piercing = false;
        bool grazed = false;
        bool active = false;
    };

    /** @brief ボス機体で個別に破壊できる部位 */
    enum BossPart {
        BossNose,
        BossLeftWing,
        BossRightWing,
        BossLeftEngine,
        BossRightEngine,
        BossFunnelHatch0,
        BossFunnelHatch1,
        BossFunnelHatch2,
        BossFunnelHatch3,
        BossFunnelHatch4,
        BossFunnelHatch5,
        BossFunnelHatch6,
        BossFunnelHatch7,
        BossFunnelHatch8,
        BossFunnelHatch9,
        BossFunnelHatch10,
        BossFunnelHatch11,
        BossPartCount
    };
    static constexpr int BossFunnelHatchCount = 12;
    static_assert(BossPartCount == 5 + BossFunnelHatchCount);

    /** @brief ボス戦の攻撃フェーズ */
    enum BossPhase {
        BossNormalPhase1,
        BossSpecialPhase1,
        BossNormalPhase2,
        BossSpecialPhase2,
        BossPhaseCount
    };
    static_assert(BossPhaseCount == 4);

    /**
     * @brief 本体HPから現在の攻撃フェーズを取得する
     * @param hp 現在の本体HP
     * @param maxHp 本体の最大HP
     * @return 通常、特殊、通常、特殊の順で進むフェーズ番号
     */
    static constexpr int BossPhaseForHp(int hp, int maxHp) {
        if (maxHp <= 0) return BossNormalPhase1;
        const int clampedHp = hp < 0 ? 0 : (hp > maxHp ? maxHp : hp);
        const int phase = (maxHp - clampedHp) * BossPhaseCount / maxHp;
        return phase < BossPhaseCount ? phase : BossPhaseCount - 1;
    }
    static_assert((480 - 480) * BossPhaseCount / 480 == BossNormalPhase1);
    static_assert((480 - 360) * BossPhaseCount / 480 == BossSpecialPhase1);
    static_assert((480 - 240) * BossPhaseCount / 480 == BossNormalPhase2);
    static_assert((480 - 1) * BossPhaseCount / 480 == BossSpecialPhase2);

    struct Enemy {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float transitionSideX = 0.0f;
        float transitionSideY = 0.0f;
        float transitionRailZ = 0.0f;
        float baseX = 0.0f;
        float baseY = 0.0f;
        float baseZ = 0.0f;
        float railAnchorX = 0.0f;
        float railAnchorY = 0.0f;
        float railAnchorZ = 0.0f;
        float actionX = 0.0f;
        float actionY = 0.0f;
        float actionZ = 0.0f;
        float turretAimX = 0.0f;
        float turretAimY = 0.0f;
        float turretAimZ = 0.0f;
        float phase = 0.0f;
        int hp = 0;
        int maxHp = 0;
        int type = 0;
        int age = 0;
        int motionAge = 0;
        int recoilAge = 0;
        int shotInterval = 0;
        int attackWarningFrames = 0;
        float attackWarningTargetX = 0.0f;
        float attackWarningTargetY = 0.0f;
        int bossPhase = BossNormalPhase1;
        std::array<int, BossPartCount> bossPartHp {};
        std::array<int, BossPartCount> bossPartMaxHp {};
        std::array<int, BossPartCount> bossPartHitFlashFrames {};
        bool collisionEnabled = true;
        const EnemyBehavior* behavior = nullptr;
        bool active = false;
    };

    /** @brief 弾が敵へ命中した位置に表示する短時間の爆発 */
    struct Explosion {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        int age = 0;
        bool destruction = false;
        bool active = false;
        int effectType = 0;
        float hitRadius = 0.0f;
        bool damagedPlayer = false;
    };

    /** @brief 撃破された機体モデルから分離して飛散する部品 */
    struct Debris {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        float yaw = 0.0f;
        float spin = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float depth = 0.0f;
        std::array<float, 4> color {};
        int shape = 1;
        int age = 0;
        int lifetime = 36;
        int shrinkStartAge = 36;
        ShooterStages::Stage2::DebrisState stage2 {};
        bool gravity = false;
        bool active = false;
    };

    enum class ItemType {
        Power,
        Score
    };

    /** @brief 敵撃破時に出現する取得アイテム */
    struct Item {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float power = 0.25f;
        int score = 0;
        ItemType type = ItemType::Power;
        bool active = false;
    };

    /** @brief チャプター終了時に表示する戦績 */
    struct ChapterResult {
        int grazeCount = 0;
        int enemySpawnCount = 0;
        int enemyDefeatCount = 0;
        int retryCount = 0;
        int score = 0;
        int totalScore = 0;
    };

    static constexpr int ShotCapacity = 64;
    static constexpr int EnemyCapacity = 12;
    static constexpr int ItemCapacity = 48;
    static constexpr int ExplosionCapacity = ShotCapacity + 32;
    static_assert(ExplosionCapacity >= ShotCapacity);
    static constexpr int ExplosionLifetimeFrames = 18;
    static constexpr int DestructionExplosionLifetimeFrames = 48;
    static constexpr int MortarExplosionLifetimeFrames = 64;
    static constexpr int AttackWarningFrames = 12;
    static constexpr int BossPartHitFlashFrames = 12;
    static constexpr int DebrisCapacity = 96;
    static constexpr int DebrisLifetimeFrames = 36;
    static constexpr float MaxPower = 4.0f;
    static constexpr int ChapterLengthFrames = 500;
    static constexpr int ChapterResultCountUpFrames = 120;
    static constexpr int ChapterResultDisplayFrames = 180;
    static constexpr int RestartDisplayFrames = 180;
    static constexpr int PlayerDestructionWaitFrames = 120;
    static_assert(PlayerDestructionWaitFrames == 2 * 60);
    static constexpr int MissionBannerDisplayFrames = 120;
    static constexpr int ClearWaitFrames = 180;
    static constexpr float BossStartDistance = 12.0f;
    static constexpr int BossMaxHp = 480;
    static constexpr int ViewTransitionFrames = 90;
    static constexpr int ViewToggleInvincibleFrames = 180;
    static constexpr int ViewToggleCooldownFrames = 480;
    static_assert(ViewToggleInvincibleFrames == 3 * 60);
    static_assert(ViewToggleCooldownFrames == 8 * 60);
    static constexpr float WorldXScale = 7.0f;
    static constexpr float WorldYScale = 4.4f;
    /** @brief 2D画面のプレイ領域左端に対応する自機中心のX座標 */
    static constexpr float Side2DPlayerMinX = -2.00f;
    /** @brief 2D画面のプレイ領域右端に対応する自機中心のX座標 */
    static constexpr float Side2DPlayerMaxX = 2.00f;
    /** @brief フッター直上のプレイ領域下端に対応する自機中心のY座標 */
    static constexpr float Side2DPlayerMinY = -1.38f;
    /** @brief ヘッダー直下のプレイ領域上端に対応する自機中心のY座標 */
    static constexpr float Side2DPlayerMaxY = 1.20f;
    /** @brief 自機弾が画面外へ抜けるまで保持する余白 */
    static constexpr float Side2DShotCullMargin = 0.25f;
    static constexpr float PlayerRailZ = 8.0f;
    static constexpr float SidePlaneZ = 10.0f;
    static constexpr float EnemyRailFarZ = 60.0f;

    enum class ViewMode {
        Side2D,
        Rail3D
    };

public:
    using Stage5Phase = ShooterStages::Stage5::Phase;
    using Stage5Checkpoint = ShooterStages::Stage5::Checkpoint;
    using SearchlightPhase = ShooterStages::Stage5::SearchlightPhase;
    using TayamaWeakpoint = ShooterStages::Stage5::TayamaWeakpoint;
    using SearchlightState = ShooterStages::Stage5::SearchlightState;
    using TayamaWeakpointState = ShooterStages::Stage5::TayamaWeakpointState;

    static constexpr int Stage5SearchlightCount = ShooterStages::Stage5::SearchlightCount;
    static constexpr int TayamaWeakpointCount = ShooterStages::Stage5::TayamaWeakpointCount;
    static constexpr int EastsourceMaxHp = ShooterStages::Stage5::EastsourceMaxHp;
    static constexpr int EastsourceNoseHp = ShooterStages::Stage5::EastsourceNoseHp;
    static constexpr int EastsourceWingHp = ShooterStages::Stage5::EastsourceWingHp;
    static constexpr int EastsourceEngineHp = ShooterStages::Stage5::EastsourceEngineHp;
    static constexpr int WallClimbTransitionFrames = ShooterStages::Stage5::WallClimbTransitionFrames;
    static constexpr int WallClimbLowerFrames = ShooterStages::Stage5::WallClimbLowerFrames;
    static constexpr int WallClimbMiddleFrames = ShooterStages::Stage5::WallClimbMiddleFrames;
    static constexpr int WallClimbUpperFrames = ShooterStages::Stage5::WallClimbUpperFrames;
    static constexpr int RooftopArrivalFrames = ShooterStages::Stage5::RooftopArrivalFrames;
    static constexpr int CarrierTransformationFrames = ShooterStages::Stage5::CarrierTransformationFrames;
    static constexpr int TayamaCollapseFrames = ShooterStages::Stage5::TayamaCollapseFrames;
    static constexpr int Stage5QuietFlightFrames = ShooterStages::Stage5::QuietFlightFrames;
    static constexpr int SearchlightLockFrames = ShooterStages::Stage5::SearchlightLockFrames;
    static constexpr int SearchlightWarningFrames = ShooterStages::Stage5::SearchlightWarningFrames;
    static constexpr int SearchlightVolleyCount = ShooterStages::Stage5::SearchlightVolleyCount;
    static constexpr int SearchlightVolleyIntervalFrames = ShooterStages::Stage5::SearchlightVolleyIntervalFrames;
    static constexpr float SearchlightDetectionRadius = ShooterStages::Stage5::SearchlightDetectionRadius;

    /**
     * @brief Stage 5の状態遷移が正規経路か判定する
     * @param from 遷移元
     * @param to 遷移先
     * @return 正規経路の場合true、許可しない遷移の場合false
     */
    static constexpr bool IsValidStage5Transition(Stage5Phase from, Stage5Phase to) {
        return ShooterStages::Stage5::IsValidTransition(from, to);
    }

    /**
     * @brief 指定弱点が現在フェーズで有効か判定する
     * @param weakpoint 判定する弱点
     * @param phase 現在のStage 5状態
     * @return ダメージを受ける場合true、無効な弱点の場合false
     */
    static constexpr bool IsTayamaWeakpointActiveForPhase(
        TayamaWeakpoint weakpoint, Stage5Phase phase) {
        return ShooterStages::Stage5::IsWeakpointActiveForPhase(weakpoint, phase);
    }

private:
    /** @brief ボス戦開始前の演出状態 */
    enum class BossIntroductionPhase {
        None,
        Entrance,
        Dialogue,
        NameReveal
    };

    void Reset(bool resetRetryCounts = true);
    /**
     * @brief デバッグ用に指定ステージとチャプターから開始する
     * @param stageNumber 開始するステージ番号
     * @param chapterNumber 開始するチャプター番号
     * @param bossBattle ボス戦から開始する場合true
     * @return なし
     */
    void StartDebugCheckpoint(int stageNumber, int chapterNumber, bool bossBattle);
    /**
     * @brief 指定番号のステージ定義を取得する
     * @param stageNumber 取得するステージ番号
     * @return 指定番号に対応するステージ定義
     */
    static const Stage& StageForNumber(int stageNumber, DifficultyType difficulty);
    static const EnemyBehavior& BasicEnemyBehaviorInstance();
    static const EnemyBehavior& HeavyEnemyBehaviorInstance();
    static const EnemyBehavior& ArmoredEnemyBehaviorInstance();
    static const EnemyBehavior& BossEnemyBehaviorInstance();
    static const EnemyBehavior& StraightShooterEnemyBehaviorInstance();
    static const EnemyBehavior& DiveRusherEnemyBehaviorInstance();
    static const EnemyBehavior& CircleShooterEnemyBehaviorInstance();
    static const EnemyBehavior& SquareShooterEnemyBehaviorInstance();
    static const EnemyBehavior& EnemyBehaviorForType(int type);
    void TickViewTransition();
    /**
     * @brief 入力を偽装せず表示モード変更を要求する
     * @param mode 切り替え先
     * @return なし
     */
    void RequestViewMode(ViewMode mode);
    void InitializeRailObjects();
    void InitializeSideObjects();
    void TickPlayer();
    /**
     * @brief 入力中の通常弾・特殊弾発射を更新する
     * @return なし
     */
    void TickPlayerWeapons();
    void TickEnemies();
    void TickShots();
    /** @brief 生存中の爆発エフェクトを更新する */
    void TickExplosions();
    /** @brief 飛散中の機体部品を更新する */
    void TickDebris();
    /** @brief 取得アイテムを更新して自機との取得判定を行う */
    void TickItems();
    /** @brief ボス戦前会話を進行する */
    void TickBossStory();
    /** @brief ボス出現と名前表示の時間演出を進行する */
    void TickBossIntroduction();
    /** @brief チャプター終了演出を更新する */
    void TickChapterResult();
    /**
     * @brief チャプター終了中の敵を当たり判定なしで画面外へ退避させる
     * @return なし
     */
    void TickChapterExitEnemies();
    /** @brief 現在のチャプター戦績を確定して表示を開始する */
    void FinishChapter();
    void SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ);
    /**
     * @brief 未解放の展示だけを永続データへ追加する
     * @param entry 解放する展示
     * @return なし
     */
    void UnlockGallery(GalleryEntry entry);
    /** @brief 指定座標にPowerアイテムを生成する */
    void SpawnPowerItem(float x, float y, float z, float value);
    /** @brief 指定座標にScoreアイテムを生成する */
    void SpawnScoreItem(float x, float y, float z, int value);
    void StartBossBattle();
    /**
     * @brief ボス部位から現在フェーズに対応する弾幕を発射する
     * @param boss 弾幕を発射するボス
     */
    void FireBossPartBarrage(const Enemy& boss);
    /**
     * @brief ボス本体へダメージを与え、攻撃フェーズを更新する
     * @param boss ダメージ対象のボス
     * @param damage 与えるダメージ
     * @return ボスを撃破した場合true、生存している場合false
     */
    bool DamageBoss(Enemy& boss, int damage);
    /**
     * @brief ボス撃破後の報酬とクリア状態を設定する
     * @param boss 撃破したボス
     */
    void DefeatBoss(Enemy& boss);
    /** @brief 次のステージの戦闘状態を初期化する */
    void StartNextStage();
    void SpawnShot(float x, float y, float vx, float vy, bool enemy,
        float z = -1.0f, float railSpeed = -1.0f, int damage = 1);
    /**
     * @brief XYZ速度を指定して固定長プールへ弾を生成する
     * @param x 発射元ゲーム座標X
     * @param y 発射元ゲーム座標Y
     * @param z 発射元レール座標Z
     * @param vx ゲーム座標X方向速度
     * @param vy ゲーム座標Y方向速度
     * @param vz レール座標Z方向速度
     * @param enemy 敵弾の場合true
     * @param barrageIndex 弾幕内の弾番号
     * @param barrageCount 弾幕の総弾数
     * @return なし
     */
    void SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy,
        int barrageIndex = -1, int barrageCount = 0);
    /**
     * @brief 命中または敵撃破位置へ爆発エフェクトを生成する
     * @param x 2D座標系のX座標
     * @param y 2D座標系のY座標
     * @param z 3Dレール座標系のZ座標
     * @param destruction 敵撃破用の大爆発を生成する場合true
     * @return なし
     */
    void SpawnExplosion(float x, float y, float z, bool destruction = false);
    /**
     * @brief 迫撃砲着弾用の大爆破エフェクトを生成する
     * @param x 2D座標系のX座標
     * @param y 2D座標系のY座標
     * @param z 3Dレール座標系のZ座標
     * @return なし
     */
    void SpawnMortarExplosion(float x, float y, float z);
    /** @brief 機体モデルを構成する部品を飛散エフェクトとして生成する */
    void SpawnEnemyDebris(const Enemy& enemy, int bossPart = -1);
    /** @brief 飛散するモデル部品を固定長プールへ追加する */
    Debris* SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
        float yaw, float spin, int shape, float width, float height, float depth,
        const float color[4], int lifetime = DebrisLifetimeFrames,
        int shrinkStartAge = DebrisLifetimeFrames, bool gravity = false);
    void FireSpecialShots();
    void UpdateHomingShot(Shot& shot);
    void DamagePlayer();
    /** @brief 現在のチャプターを開始時状態へ戻す */
    void RestartCurrentChapter();
    /**
     * @brief 自機弾が未破壊のボス部位へ命中したか判定する
     * @param shot 判定対象の自機弾
     * @param boss 判定対象のボス
     * @param part 命中した部位の格納先
     * @return 部位へ命中した場合true、命中しない場合false
     */
    bool TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const;
    /**
     * @brief 共通または移行中のボス部位判定を行う
     * @param shot 判定対象の自機弾
     * @param boss 判定対象のボス
     * @param part 命中した部位の格納先
     * @return 部位へ命中した場合true、命中しない場合false
     */
    bool TryHitDefaultBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const;
    void PlayShotSound();
    void PlayHitSound();
    /** @brief ミサイル噴射開始音を再生する @return なし */
    void PlayMissileLaunchSound();
    /** @brief ボスマシンガンの単発音を再生する @return なし */
    void PlayBossMachineGunSound();
    static bool Hit(float ax, float ay, float ar, float bx, float by, float br);
    static bool Hit3D(float ax, float ay, float az, float ar, float bx, float by, float bz, float br);
    /**
     * @brief 線分上を移動する球が対象球へ接触したか判定する
     * @param startX 移動前のワールド座標X
     * @param startY 移動前のワールド座標Y
     * @param startZ 移動前のワールド座標Z
     * @param endX 移動後のワールド座標X
     * @param endY 移動後のワールド座標Y
     * @param endZ 移動後のワールド座標Z
     * @param movingRadius 移動する球の半径
     * @param targetX 対象球のワールド座標X
     * @param targetY 対象球のワールド座標Y
     * @param targetZ 対象球のワールド座標Z
     * @param targetRadius 対象球の半径
     * @return 移動区間内で接触する場合true、接触しない場合false
     */
    static bool Hit3DSegment(float startX, float startY, float startZ,
        float endX, float endY, float endZ, float movingRadius,
        float targetX, float targetY, float targetZ, float targetRadius);
    static float SmoothStep(float value);
    static float ToWorldX(float x);
    static float ToWorldY(float y);
    static float FromWorldX(float x);
    static float FromWorldY(float y);
    static float ToRailZFromSideX(float x);
    static float ToSideXFromRailZ(float z);
    /**
     * @brief 3Dレール視点で機体底面が地面上面に接するY座標下限を取得する
     * @return ゲーム座標系のY座標下限
     */
    float PlayerRailMinY() const;
    /** @brief 現在のPowerから弾強化段階を取得する */
    int PowerLevel() const;
    /** @brief チャプターの総合スコアを算出する */
    static int CalculateChapterTotalScore(const ChapterResult& result);
    float RailBlend() const;
    bool IsRailGameplayActive() const;
    bool IsRailRenderActive() const;
    /**
     * @brief 現在2Dと3Dの表示を切り替えられるか判定する
     * @return 切り替え可能な場合true
     */
    bool CanToggleView() const;
    void ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const;
    void ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const;
    /**
     * @brief 現在フレームの画面揺れオフセットを取得する
     * @return カメラへ加算するXYオフセット
     */
    Vector2 ScreenShakeOffset() const;
    void Render2D(Renderer& renderer) const;
    void Render3D(Renderer& renderer) const;
    /**
     * @brief 2D画面上の敵攻撃予告を十字フラッシュとして描画する
     * @param renderer 描画先レンダラー
     * @return なし
     */
    void DrawAttackWarnings2D(Renderer& renderer) const;
    /**
     * @brief 3D空間内の敵攻撃予告を発光マーカーとして描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    void DrawAttackWarnings3D(Renderer& renderer, const Camera3D& camera, float railWeight) const;
    void DrawBossHud(Renderer& renderer) const;
    /**
     * @brief 2Dと3Dの表示切り替えクールダウンを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param playerZ 描画中の自機のワールド座標Z
     * @return なし
     */
    void DrawViewToggleCooldownHud(
        Renderer& renderer, const Camera3D& camera, float playerZ) const;
    /** @brief ボス戦前会話を画面へ描画する */
    void DrawBossStory(Renderer& renderer) const;
    /** @brief 墨の筆跡を模したボス名演出を画面へ描画する */
    void DrawBossNameReveal(Renderer& renderer) const;
    static void DrawShape(Renderer& renderer,
        float x, float y, float w, float h, const float color[4]);
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
        float x, float y, float z, float w, float h, float d, const float color[4],
        float yaw = 0.0f, float pitch = 0.0f);
    /**
     * @brief PrimitiveShapeを変換せず3Dプリミティブとして描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param shape 描画形状
     * @param x 中心X座標
     * @param y 中心Y座標
     * @param z 中心Z座標
     * @param w X寸法
     * @param h Y寸法
     * @param d Z寸法
     * @param color RGBA色
     * @param yaw Y軸回転
     * @param pitch Z軸回転
     * @return なし
     */
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera,
        PrimitiveShape shape, float x, float y, float z, float w, float h, float d,
        const float color[4], float yaw = 0.0f, float pitch = 0.0f);
    /**
     * @brief XYZ回転を維持して3Dプリミティブを描画する
     * @param renderer 描画先
     * @param camera 使用するカメラ
     * @param shape 形状
     * @param position ワールド座標
     * @param scale 寸法
     * @param rotation XYZ回転
     * @param color 色
     * @return なし
     */
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
        const Vector3& position, const Vector3& scale, const Vector3& rotation, const float color[4]);
    /**
     * @brief 合成済みワールド行列で3Dプリミティブを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param shape 形状
     * @param world 合成済みワールド行列
     * @param color 色
     * @return なし
     */
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
        const Matrix4x4& world, const float color[4]);
    /**
     * @brief 機体直下の地面へ軽量なBlob Shadowを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param x 影の中心X座標
     * @param z 影の中心Z座標
     * @param groundTopY 地面上面Y座標
     * @param width 影の半幅
     * @param depth 影の半奥行き
     * @param opacity 影の不透明度
     * @return なし
     */
    static void DrawBlobShadow(Renderer& renderer, const Camera3D& camera,
        float x, float z, float groundTopY, float width, float depth, float opacity);
    /**
     * @brief 自機と表示切り替え可能時の機首発光を描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param x 自機中心のワールド座標X
     * @param y 自機中心のワールド座標Y
     * @param z 自機中心のワールド座標Z
     * @param visible 自機を描画する場合true
     * @param yaw 自機のY軸回転角度
     * @return なし
     */
    void DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
        float x, float y, float z, bool visible, float yaw = 0.0f) const;
    void DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw = 0.0f) const;
    void DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw = 0.0f) const;
    /** @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する */
    static void DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion);
    /** @brief 飛散中の機体部品を描画する */
    void DrawDebris(Renderer& renderer, const Camera3D& camera,
        const Debris& debris, float railWeight) const;
    /** @brief 取得アイテムを描画する */
    static void DrawItemModel(Renderer& renderer, const Camera3D& camera,
        const Item& item, float yaw = 0.0f);
    /** @brief チャプター終了時の戦績を描画する */
    void DrawChapterResult(Renderer& renderer) const;
    /** @brief リスタート中のカウントダウンを描画する */
    void DrawRestart(Renderer& renderer) const;
    /**
     * @brief 武装強化時の点滅メッセージを自機上へ描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param playerZ 描画中の自機のワールド座標Z
     * @return なし
     */
    void DrawPowerUp(Renderer& renderer, const Camera3D& camera, float playerZ) const;
    /** @brief ミッション開始または終了の文字アニメーションを描画する */
    void DrawMissionBanner(Renderer& renderer) const;

    std::array<Shot, ShotCapacity> m_shots {};
    std::array<Enemy, EnemyCapacity> m_enemies {};
    std::array<Item, ItemCapacity> m_items {};
    std::array<Explosion, ExplosionCapacity> m_explosions {};
    std::array<Debris, DebrisCapacity> m_debris {};
    ShooterStages::Stage1::State m_stage1 {};
    ShooterStages::Stage2::State m_stage2 {};
    ShooterStages::Stage3::State m_stage3 {};
    ShooterStages::Stage4::State m_stage4 {};
    ShooterStages::Stage5::State m_stage5 {};
    AudioService* m_audio = nullptr;
    const Stage* m_stage = nullptr;
    PlayerType m_playerType = Homing;
    DifficultyType m_difficulty = Easy;
    std::uint32_t m_galleryUnlocks = DefaultGalleryUnlocks;
    float m_playerX = -0.72f;
    float m_playerY = 0.0f;
    float m_scroll = 0.0f;
    int m_frame = 0;
    int m_spawnCooldown = 0;
    int m_shotCooldown = 0;
    int m_specialShotCooldown = 0;
    int m_invincible = 0;
    int m_score = 0;
    int m_kills = 0;
    int m_bossHp = 0;
    float m_displayBossHp = 0.0f;
    int m_bossStoryLine = 0;
    bool m_bossStoryActive = false;
    BossIntroductionPhase m_bossIntroductionPhase = BossIntroductionPhase::None;
    int m_bossIntroductionTimer = 0;
    int m_stageNumber = 1;
    int m_chapterNumber = 1;
    std::array<int, 3> m_chapterRetryCounts {};
    ChapterResult m_chapterResult {};
    float m_chapterStartPower = 0.0f;
    int m_chapterStartScore = 0;
    int m_chapterStartKills = 0;
    int m_chapterResultTimer = 0;
    int m_playerDestructionTimer = 0;
    int m_restartTimer = 0;
    int m_powerUpTimer = 0;
    int m_missionStartTimer = 0;
    float m_power = 0.0f;
    int m_clearTimer = 0;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_slowMove = false;
    bool m_fire = false;
    bool m_clear = false;
    bool m_bossBattle = false;
    bool m_bossBattlePending = false;
    bool m_chapterResultActive = false;
    bool m_viewToggleRequested = false;
    ViewMode m_viewMode = ViewMode::Side2D;
    ViewMode m_nextViewMode = ViewMode::Side2D;
    int m_viewTransitionTimer = 0;
    int m_viewToggleCooldown = 0;
    float m_viewTransitionProgress = 0.0f;
    float m_screenShakeIntensity = 0.0f;
    int m_screenShakeFrames = 0;
    int m_screenShakeDurationFrames = 0;
};

static_assert(SideScrollingShooter::IsValidStage5Transition(
    SideScrollingShooter::Stage5Phase::Approach,
    SideScrollingShooter::Stage5Phase::EastsourceIntro));
static_assert(SideScrollingShooter::IsValidStage5Transition(
    SideScrollingShooter::Stage5Phase::TayamaCommandCore,
    SideScrollingShooter::Stage5Phase::TayamaCollapse));
static_assert(!SideScrollingShooter::IsValidStage5Transition(
    SideScrollingShooter::Stage5Phase::EastsourceBattle,
    SideScrollingShooter::Stage5Phase::EndingReady));
static_assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
    SideScrollingShooter::TayamaWeakpoint::FireControlRadar,
    SideScrollingShooter::Stage5Phase::TayamaFireControl));
static_assert(!SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
    SideScrollingShooter::TayamaWeakpoint::CommandCore,
    SideScrollingShooter::Stage5Phase::TayamaLiftEngines));
