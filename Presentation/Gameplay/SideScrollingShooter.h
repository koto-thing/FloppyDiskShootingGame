#pragma once

#include <array>

#include "../../Domain/ValueObjects/DifficultyType.h"
#include "../../Domain/ValueObjects/PlayerType.h"
#include "../../Engine/Graphics/Camera3D.h"

class AudioService;
class Renderer;
struct EastsourceModelState;
struct Stage5ModelTransform;
struct TayamaModelState;

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
     * @brief 全ステージをクリア済みか取得する
     * @return 最終ステージのミッション終了表示が完了した場合true
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
    class Stage3;
    class Stage4;
    class Stage5;
    class EnemyBehavior;
    class BasicEnemyBehavior;
    class HeavyEnemyBehavior;
    class ArmoredEnemyBehavior;
    class BossEnemyBehavior;
    class Stage2BossEnemyBehavior;
    class StraightShooterEnemyBehavior;
    class CircleShooterEnemyBehavior;
    class SquareShooterEnemyBehavior;

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
        int funnelDustAge = -1;
        float funnelDustX = 0.0f;
        float funnelDustY = 0.0f;
        float funnelDustZ = 0.0f;
        float funnelEngineVx = 0.0f;
        float funnelEngineVy = 0.0f;
        float funnelEngineVz = 0.0f;
        PlayerType playerType = Homing;
        bool enemy = false;
        bool funnel = false;
        bool funnelDelayedEngine = false;
        bool missile = false;
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

    /** @brief Stage2ボス専用の行動状態 */
    enum class Stage2BossAction {
        Idle,
        MainGunCharge,
        MainGunFire,
        MainGunCooldown,
        Dive,
        Underground,
        Warning,
        Charge,
        Recover,
        Separating
    };

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
        int shotInterval = 0;
        int attackWarningFrames = 0;
        float attackWarningTargetX = 0.0f;
        float attackWarningTargetY = 0.0f;
        int bossPhase = BossNormalPhase1;
        std::array<int, BossPartCount> bossPartHp {};
        std::array<int, BossPartCount> bossPartMaxHp {};
        std::array<int, BossPartCount> bossPartHitFlashFrames {};
        Stage2BossAction stage2BossAction = Stage2BossAction::Idle;
        int stage2BossActionAge = 0;
        float landBattleshipOffsetY = 0.0f;
        float landBattleshipOffsetX = 0.0f;
        float landBattleshipOffsetZ = 0.0f;
        float sandSubmarineOffsetY = 0.0f;
        float sandSubmarineOffsetX = 0.0f;
        float sandSubmarineOffsetZ = 0.0f;
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
    };

    /** @brief 撃破された機体モデルから分離して飛散する部品 */
    struct Debris {
        enum class Effect {
            None,
            Stage2Sink,
            Stage2Impact,
            Stage2ImpactPiece
        };

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
        int effectAge = -1;
        Effect effect = Effect::None;
        bool gravity = false;
        bool active = false;
    };

    /** @brief ステージ1を横切る破壊可能な隕石 */
    struct Meteor {
        float travel = 0.0f;
        float scale = 1.0f;
        float yaw = 0.0f;
        float spin = 0.0f;
        int hp = 0;
        bool destroyed = false;
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
    static constexpr int AttackWarningFrames = 12;
    static constexpr int BossPartHitFlashFrames = 12;
    static constexpr int Stage2RailgunCycleFrames = 180;
    static constexpr int Stage2RailgunFireFrame = 60;
    static constexpr int Stage2RailgunVisualFrames = 12;
    static constexpr int Stage2RailgunMirageFrames = 36;
    static_assert(Stage2RailgunFireFrame + Stage2RailgunMirageFrames <= Stage2RailgunCycleFrames);
    static constexpr int DebrisCapacity = 96;
    static constexpr int DebrisLifetimeFrames = 36;
    static constexpr int MeteorCount = 6;
    static constexpr int BoneArchMaxHp = 12000;
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
    /** @brief Stage 5専用の進行状態 */
    enum class Stage5Phase {
        Approach,
        EastsourceIntro,
        EastsourceBattle,
        EastsourceFall,
        WallClimbTransition,
        WallClimbLower,
        WallClimbMiddle,
        WallClimbUpper,
        RooftopArrival,
        CarrierTransformation,
        TayamaFireControl,
        TayamaLiftEngines,
        TayamaCommandCore,
        TayamaCollapse,
        EndingReady
    };

    /** @brief Stage 5の被弾復帰地点 */
    enum class Stage5Checkpoint {
        Chapter1,
        Chapter2,
        Chapter3,
        Eastsource,
        WallClimbLower,
        WallClimbMiddle,
        WallClimbUpper,
        TayamaFireControl,
        TayamaLiftEngines,
        TayamaCommandCore
    };

    /** @brief サーチライトの索敵状態 */
    enum class SearchlightPhase {
        Searching,
        Detecting,
        Locked,
        Firing,
        Cooldown
    };

    /** @brief TAYAMAの破壊可能な弱点 */
    enum class TayamaWeakpoint {
        LeftSearchlight,
        RightSearchlight,
        FireControlRadar,
        LeftLiftEngine,
        RightLiftEngine,
        CommandCore,
        Count
    };

    /** @brief 壁面およびTAYAMAのサーチライト状態 */
    struct SearchlightState {
        SearchlightPhase phase = SearchlightPhase::Searching;
        float beamX = 0.0f;
        float beamY = 0.0f;
        float lockedX = 0.0f;
        float lockedY = 0.0f;
        int detectionFrames = 0;
        int timer = 0;
        int volley = 0;
        int hp = 0;
        bool destroyed = false;
    };

    /** @brief TAYAMA弱点の固定長状態 */
    struct TayamaWeakpointState {
        TayamaWeakpoint type = TayamaWeakpoint::LeftSearchlight;
        int hp = 0;
        int maxHp = 0;
        bool active = false;
        bool destroyed = false;
        int hitFlashFrames = 0;
    };

    static constexpr int Stage5SearchlightCount = 3;
    static constexpr int TayamaWeakpointCount = static_cast<int>(TayamaWeakpoint::Count);
    static constexpr int EastsourceMaxHp = 1200;
    static constexpr int EastsourceNoseHp = 180;
    static constexpr int EastsourceWingHp = 240;
    static constexpr int EastsourceEngineHp = 210;
    static constexpr int WallClimbTransitionFrames = 120;
    static constexpr int WallClimbLowerFrames = 420;
    static constexpr int WallClimbMiddleFrames = 480;
    static constexpr int WallClimbUpperFrames = 540;
    static constexpr int RooftopArrivalFrames = 180;
    static constexpr int CarrierTransformationFrames = 120;
    static constexpr int TayamaCollapseFrames = 540;
    static constexpr int Stage5QuietFlightFrames = 60;
    static constexpr int SearchlightLockFrames = 45;
    static constexpr int SearchlightWarningFrames = 24;
    static constexpr int SearchlightVolleyCount = 3;
    static constexpr int SearchlightVolleyIntervalFrames = 10;
    static constexpr float SearchlightDetectionRadius = 0.27f;

    /**
     * @brief Stage 5の状態遷移が正規経路か判定する
     * @param from 遷移元
     * @param to 遷移先
     * @return 正規経路の場合true
     */
    static constexpr bool IsValidStage5Transition(Stage5Phase from, Stage5Phase to) {
        return (from == Stage5Phase::Approach && to == Stage5Phase::EastsourceIntro) ||
            (from == Stage5Phase::EastsourceIntro && to == Stage5Phase::EastsourceBattle) ||
            (from == Stage5Phase::EastsourceBattle && to == Stage5Phase::EastsourceFall) ||
            (from == Stage5Phase::EastsourceFall && to == Stage5Phase::WallClimbTransition) ||
            (from == Stage5Phase::WallClimbTransition && to == Stage5Phase::WallClimbLower) ||
            (from == Stage5Phase::WallClimbLower && to == Stage5Phase::WallClimbMiddle) ||
            (from == Stage5Phase::WallClimbMiddle && to == Stage5Phase::WallClimbUpper) ||
            (from == Stage5Phase::WallClimbUpper && to == Stage5Phase::RooftopArrival) ||
            (from == Stage5Phase::RooftopArrival && to == Stage5Phase::CarrierTransformation) ||
            (from == Stage5Phase::CarrierTransformation && to == Stage5Phase::TayamaFireControl) ||
            (from == Stage5Phase::TayamaFireControl && to == Stage5Phase::TayamaLiftEngines) ||
            (from == Stage5Phase::TayamaLiftEngines && to == Stage5Phase::TayamaCommandCore) ||
            (from == Stage5Phase::TayamaCommandCore && to == Stage5Phase::TayamaCollapse) ||
            (from == Stage5Phase::TayamaCollapse && to == Stage5Phase::EndingReady);
    }

    /**
     * @brief 指定弱点が現在フェーズで有効か判定する
     * @param weakpoint 判定する弱点
     * @param phase 現在のStage 5状態
     * @return ダメージを受ける場合true
     */
    static constexpr bool IsTayamaWeakpointActiveForPhase(
        TayamaWeakpoint weakpoint, Stage5Phase phase) {
        if (phase == Stage5Phase::TayamaFireControl) {
            return weakpoint == TayamaWeakpoint::LeftSearchlight ||
                weakpoint == TayamaWeakpoint::RightSearchlight ||
                weakpoint == TayamaWeakpoint::FireControlRadar;
        }
        if (phase == Stage5Phase::TayamaLiftEngines) {
            return weakpoint == TayamaWeakpoint::LeftLiftEngine ||
                weakpoint == TayamaWeakpoint::RightLiftEngine;
        }
        return phase == Stage5Phase::TayamaCommandCore &&
            weakpoint == TayamaWeakpoint::CommandCore;
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
     * @brief Stage 5専用状態を初期化する
     * @return なし
     */
    void ResetStage5();
    /**
     * @brief デバッグ用に指定ステージとチャプターから開始する
     * @param stageNumber 開始するステージ番号
     * @param chapterNumber 開始するチャプター番号
     * @param bossBattle ボス戦から開始する場合true
     * @return なし
     */
    void StartDebugCheckpoint(int stageNumber, int chapterNumber, bool bossBattle);
    /**
     * @brief Stage 5の指定状態からデバッグ開始する
     * @param phase 開始する状態
     * @return なし
     */
    void StartDebugStage5Phase(Stage5Phase phase);
    /**
     * @brief 指定難易度のステージ1敵出現シートを取得する
     * @param difficulty 取得する難易度
     * @return 難易度に対応するステージ1敵出現シート
     */
    static const Stage& Stage1EnemySheetInstance(DifficultyType difficulty);
    static const Stage& Stage2EnemySheetInstance(DifficultyType difficulty);
    static const Stage& Stage3Instance();
    static const Stage& Stage4Instance();
    static const Stage& Stage5Instance();
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
    static const EnemyBehavior& Stage2BossEnemyBehaviorInstance();
    static const EnemyBehavior& StraightShooterEnemyBehaviorInstance();
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
    /**
     * @brief Stage 5後半で3D表示が固定されているか取得する
     * @return 3D表示が固定されている場合true
     */
    bool IsStage5ViewLocked() const;
    /**
     * @brief 現在の進行状態で背景スクロールを更新するか取得する
     * @return 背景スクロールを更新する場合true
     */
    bool ShouldAdvanceStageScroll() const;
    /**
     * @brief Stage 5専用シーケンスを更新する
     * @return なし
     */
    void TickStage5();
    /**
     * @brief Stage 5状態を開始する
     * @param phase 開始する状態
     * @param saveCheckpoint 復帰地点として保存する場合true
     * @return なし
     */
    void StartStage5Phase(Stage5Phase phase, bool saveCheckpoint = true);
    /**
     * @brief EASTSOURCE戦を戦闘可能な状態で開始する
     * @return なし
     */
    void StartEastsourceBattle();
    /**
     * @brief EASTSOURCEの移動と攻撃を更新する
     * @param eastsource 更新するEASTSOURCE本体
     * @return なし
     */
    void TickEastsource(Enemy& eastsource);
    /**
     * @brief EASTSOURCE撃破後の信号消失演出へ移行する
     * @param eastsource 撃破されたEASTSOURCE本体
     * @return なし
     */
    void DefeatEastsource(Enemy& eastsource);
    /**
     * @brief 現在のStage 5チェックポイントへ復帰する
     * @return なし
     */
    void RestartStage5Checkpoint();
    /**
     * @brief 現在状態をStage 5チェックポイントとして保存する
     * @param checkpoint 保存する復帰地点
     * @return なし
     */
    void SaveStage5Checkpoint(Stage5Checkpoint checkpoint);
    /**
     * @brief 指定数のサーチライトを更新する
     * @param activeCount 更新するライト数
     * @param tayamaWeakpoints TAYAMA弱点と破壊状態を共有する場合true
     * @return なし
     */
    void TickSearchlights(int activeCount, bool tayamaWeakpoints);
    /**
     * @brief 壁面区画のサーチライトを初期化する
     * @param activeCount 有効にするライト数
     * @return なし
     */
    void ResetWallSearchlights(int activeCount);
    /**
     * @brief サーチライトの保存済み地点へ集中砲火を生成する
     * @param light 発射に使用するライト状態
     * @param lightIndex 発射元ライト番号
     * @return なし
     */
    void FireSearchlightVolley(const SearchlightState& light, int lightIndex);
    /**
     * @brief 指定地点へ向かう敵弾を生成する
     * @param sourceX 発射元ゲーム座標X
     * @param sourceY 発射元ゲーム座標Y
     * @param sourceZ 発射元レール座標Z
     * @param targetX 固定目標ゲーム座標X
     * @param targetY 固定目標ゲーム座標Y
     * @param targetZ 固定目標レール座標Z
     * @param speed ワールド空間の弾速
     * @return なし
     */
    void SpawnEnemyShotAt(float sourceX, float sourceY, float sourceZ,
        float targetX, float targetY, float targetZ, float speed);
    /**
     * @brief TAYAMA戦の更新処理
     * @return なし
     */
    void TickTayama();
    /**
     * @brief TAYAMAの攻略フェーズを開始する
     * @param phase 開始する攻略状態
     * @param resetCurrentHp 現在フェーズのHPを初期値へ戻す場合true
     * @return なし
     */
    void StartTayamaPhase(Stage5Phase phase, bool resetCurrentHp = true);
    /**
     * @brief 自機弾をTAYAMAの有効弱点へ適用する
     * @param shot 判定する自機弾
     * @return TAYAMAへ命中した場合true
     */
    bool TryDamageTayama(Shot& shot);
    /**
     * @brief 自機弾を壁面サーチライトへ適用する
     * @param shot 判定する自機弾
     * @return ライトへ命中した場合true
     */
    bool TryDamageWallSearchlight(Shot& shot);
    /**
     * @brief EASTSOURCEの描画と当たり判定で共有する親Transformを取得する
     * @param eastsource EASTSOURCE本体
     * @return ワールド座標へ変換する親Transform
     */
    Stage5ModelTransform EastsourceTransform(const Enemy& eastsource) const;
    /**
     * @brief EASTSOURCEの部位状態をモデルグループへ変換する
     * @param eastsource EASTSOURCE本体
     * @return 描画と当たり判定へ渡すモデル状態
     */
    EastsourceModelState EastsourceState(const Enemy& eastsource) const;
    /**
     * @brief TAYAMAの描画と当たり判定で共有する親Transformを取得する
     * @return 現在の進行に対応する親Transform
     */
    Stage5ModelTransform TayamaTransform() const;
    /**
     * @brief 現在の弱点と崩壊状態をTAYAMAモデルグループへ変換する
     * @return 描画と当たり判定へ渡すモデル状態
     */
    TayamaModelState TayamaState() const;
    /**
     * @brief 現在フェーズで有効なTAYAMA弱点HP合計をHUDへ反映する
     * @return なし
     */
    void UpdateTayamaBossHp();
    /**
     * @brief Stage 5用の効果音をクールダウン付きで再生する
     * @param cue 効果音種別
     * @return なし
     */
    void PlayStage5Cue(int cue);
    /**
     * @brief Stage2ボス撃破演出の振動音または最終爆発音を再生する
     * @param finalExplosion 最終爆発音を再生する場合true
     * @return なし
     */
    void PlayStage2DefeatSound(bool finalExplosion);
    void InitializeRailObjects();
    void InitializeSideObjects();
    void TickPlayer();
    void TickEnemies();
    void TickShots();
    /** @brief ステージ固有の破壊可能ギミックを更新する */
    void TickStageGimmicks();
    /** @brief ステージ固有の破壊可能ギミックを初期状態へ戻す */
    void ResetStageGimmicks();
    /**
     * @brief 自機弾がステージ固有ギミックへ命中した場合にダメージを適用する
     * @param shot 命中判定対象の自機弾
     * @return ギミックへ命中した場合true
     */
    bool TryDamageStageGimmick(Shot& shot);
    /**
     * @brief 砂漠を横切る骨アーチへ指定球が接触したか判定する
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 骨アーチに接触している場合true
     */
    bool HitsDesertBoneArch(float x, float y, float z, float radius) const;
    /**
     * @brief ステージ1を横切る隕石へ指定球が接触したか判定する
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 隕石に接触している場合true
     */
    bool HitsStage1Meteor(float x, float y, float z, float radius) const;
    /**
     * @brief 指定球が接触しているステージ1隕石の番号を取得する
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return 接触した隕石の番号、接触していない場合-1
     */
    int FindStage1Meteor(float x, float y, float z, float radius) const;
    /**
     * @brief 海面からアーチ状に飛び出すウミヘビへ指定球が接触したか判定する
     * @param x 判定対象のゲーム座標X
     * @param y 判定対象のゲーム座標Y
     * @param z 判定対象のレール座標Z
     * @param radius 判定対象の半径
     * @return ウミヘビに接触している場合true
     */
    bool HitsOceanSeaSerpent(float x, float y, float z, float radius) const;
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
     * @return ボスを撃破した場合true
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
     * @brief 潜砂艦から慣性飛行後に自機へ突進するファンネルを射出する
     * @param x 発射X座標
     * @param y 発射Y座標
     * @param z 発射Z座標
     * @param launchIndex 同時射出内の配置番号
     * @param delayedEngine 短い落下後に補助エンジンを起動する場合true
     * @return なし
     */
    void SpawnStage2Funnel(float x, float y, float z, bool delayedEngine = false);
    /**
     * @brief 潜砂艦の側面ハッチから自機狙いのミサイルを発射する
     * @param x 発射X座標
     * @param y 発射Y座標
     * @param z 発射Z座標
     * @param side 側面ハッチの左右符号
     * @return なし
     */
    void SpawnStage2Missile(float x, float y, float z, float side);
    /**
     * @brief 命中または敵撃破位置へ爆発エフェクトを生成する
     * @param x 2D座標系のX座標
     * @param y 2D座標系のY座標
     * @param z 3Dレール座標系のZ座標
     * @param destruction 敵撃破用の大爆発を生成する場合true
     * @return なし
     */
    void SpawnExplosion(float x, float y, float z, bool destruction = false);
    /** @brief 機体モデルを構成する部品を飛散エフェクトとして生成する */
    void SpawnEnemyDebris(const Enemy& enemy, int bossPart = -1);
    /** @brief 飛散するモデル部品を固定長プールへ追加する */
    void SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
        float yaw, float spin, int shape, float width, float height, float depth,
        const float color[4], int lifetime = DebrisLifetimeFrames,
        int shrinkStartAge = DebrisLifetimeFrames, bool gravity = false,
        Debris::Effect effect = Debris::Effect::None);
    /**
     * @brief 砂漠の骨アーチを破壊して小さな骨を飛散させる
     * @return なし
     */
    void DestroyDesertBoneArch();
    /**
     * @brief 被弾または破壊された隕石から当たり判定を持たない小隕石を飛散させる
     * @param meteor 破片の発生元となる隕石
     * @param count 発生させる小隕石の数
     * @return なし
     */
    void SpawnMeteorDebris(const Meteor& meteor, int count);
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
     * @return 部位へ命中した場合true
     */
    bool TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const;
    bool TryHitStage2BossBody(const Shot& shot, const Enemy& boss) const;
    /**
     * @brief Stage2潜砂艦のPhase1接地基準Y座標を取得する
     * @param boss 座標を求めるStage2ボス
     * @return 切削爪が地中へ収まる潜砂艦のワールドY座標
     */
    float Stage2Phase1SubmarineWorldY(const Enemy& boss) const;
    /**
     * @brief Stage2潜砂艦の現在の親Y座標を取得する
     * @param boss 座標を求めるStage2ボス
     * @return 描画と当たり判定で共有するワールドY座標
     */
    float Stage2SubmarineWorldY(const Enemy& boss) const;
    float Stage2BattleshipWorldY(const Enemy& boss) const;
    void PlayShotSound();
    void PlayHitSound();
    /** @brief Stage2レールガン発射時の雷撃音を再生する */
    void PlayRailgunSound();
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
     * @return 移動区間内で接触する場合true
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
    void ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const;
    void ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const;
    void Render2D(Renderer& renderer) const;
    void Render3D(Renderer& renderer) const;
    /**
     * @brief Stage 5の要塞、照明、崩壊演出を3D空間へ描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @return なし
     */
    void RenderStage5(Renderer& renderer, const Camera3D& camera) const;
    /**
     * @brief Stage 5の雨、稲光、照準表示を画面空間へ描画する
     * @param renderer 描画先レンダラー
     * @return なし
     */
    void DrawStage5Weather(Renderer& renderer) const;
    /**
     * @brief Stage 5専用HUDを描画する
     * @param renderer 描画先レンダラー
     * @return なし
     */
    void DrawStage5Hud(Renderer& renderer) const;
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
    /**
     * @brief プリミティブ球だけで構成した砂漠の巨大骨アーチを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    void DrawDesertBoneArch(Renderer& renderer, const Camera3D& camera, float railWeight) const;
    /**
     * @brief プリミティブ球だけで構成したステージ1の巨大隕石を描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    void DrawStage1Meteor(Renderer& renderer, const Camera3D& camera, float railWeight) const;
    /**
     * @brief 海面から飛び出す巨大ウミヘビを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    void DrawOceanSeaSerpent(Renderer& renderer, const Camera3D& camera, float railWeight) const;
    void DrawBossHud(Renderer& renderer) const;
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
     * @brief 砂面から放物線状に舞う砂埃を描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param position 発生地点のワールド座標
     * @param age 発生からの経過フレーム
     * @param railWeight 3D表示の補間率
     * @return なし
     */
    static void DrawSandDust(Renderer& renderer, const Camera3D& camera,
        const Vector3& position, int age, float railWeight);
    static void DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
        float x, float y, float z, bool visible, float yaw = 0.0f);
    void DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw = 0.0f) const;
    void DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw = 0.0f) const;
    /** @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する */
    static void DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion);
    /** @brief 飛散中の機体部品を描画する */
    static void DrawDebris(Renderer& renderer, const Camera3D& camera,
        const Debris& debris, float railWeight);
    /** @brief 取得アイテムを描画する */
    static void DrawItemModel(Renderer& renderer, const Camera3D& camera,
        const Item& item, float yaw = 0.0f);
    /** @brief チャプター終了時の戦績を描画する */
    void DrawChapterResult(Renderer& renderer) const;
    /** @brief リスタート中のカウントダウンを描画する */
    void DrawRestart(Renderer& renderer) const;
    /** @brief ミッション開始または終了の文字アニメーションを描画する */
    void DrawMissionBanner(Renderer& renderer) const;

    std::array<Shot, ShotCapacity> m_shots {};
    std::array<Enemy, EnemyCapacity> m_enemies {};
    std::array<Item, ItemCapacity> m_items {};
    std::array<Explosion, ExplosionCapacity> m_explosions {};
    std::array<Debris, DebrisCapacity> m_debris {};
    std::array<SearchlightState, Stage5SearchlightCount> m_searchlights {};
    std::array<TayamaWeakpointState, TayamaWeakpointCount> m_tayamaWeakpoints {};
    AudioService* m_audio = nullptr;
    const Stage* m_stage = nullptr;
    PlayerType m_playerType = Homing;
    DifficultyType m_difficulty = Easy;
    float m_playerX = -0.72f;
    float m_playerY = 0.0f;
    float m_scroll = 0.0f;
    float m_tayamaTransformation = 0.0f;
    float m_stage5CheckpointPower = 0.0f;
    float m_stage5CoreTargetX = 0.0f;
    float m_stage5CoreTargetY = 0.0f;
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
    int m_missionStartTimer = 0;
    float m_power = 0.0f;
    int m_clearTimer = 0;
    int m_stage5PhaseTimer = 0;
    int m_stage5CheckpointScore = 0;
    int m_stage5CheckpointKills = 0;
    int m_stage5SoundCooldown = 0;
    int m_stage5AttackTimer = 0;
    int m_stage5GuardSpawnCooldown = 0;
    std::array<Meteor, MeteorCount> m_meteors {};
    int m_boneArchHp = BoneArchMaxHp;
    bool m_boneArchDestroyed = false;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_fire = false;
    bool m_clear = false;
    bool m_bossBattle = false;
    bool m_bossBattlePending = false;
    bool m_chapterResultActive = false;
    bool m_viewToggleRequested = false;
    Stage5Phase m_stage5Phase = Stage5Phase::Approach;
    Stage5Checkpoint m_stage5Checkpoint = Stage5Checkpoint::Chapter1;
    ViewMode m_viewMode = ViewMode::Side2D;
    ViewMode m_nextViewMode = ViewMode::Side2D;
    int m_viewTransitionTimer = 0;
    int m_viewToggleCooldown = 0;
    float m_viewTransitionProgress = 0.0f;
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
