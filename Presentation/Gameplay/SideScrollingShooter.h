#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "../../Domain/ValueObjects/DifficultyType.h"
#include "../../Domain/ValueObjects/CooperativeInput.h"
#include "../../Domain/ValueObjects/GalleryEntry.h"
#include "../../Domain/ValueObjects/PlayerType.h"
#include "../../Domain/ValueObjects/ResumeCode.h"
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
    friend struct HomingShotTests;
    friend struct OrbitShotTests;
    friend struct CoopGameplayTests;
    friend struct CoopStageTests;
    friend struct ResumeCodeTests;
public:
    /** @brief 現在地点のシングルプレイ再開コードを取得する @return コード、対象外なら空文字列 */
    std::string GetResumeCode() const;
    /** @brief 検証済みコードの開始地点と状態を復元する @param code 入力コード @return 復元成功ならtrue */
    bool RestoreResumeCode(std::string_view code);
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
        { 10, 2, 0.038f, 5.0f, 0.08f, 0.05f, 0.025f, 1, 0.150f, false },
        // PIERCING
        { 18, 2, 0.052f, 0.0f, 0.12f, 0.09f, 0.032f, 1, 0.000f, true },
        // SPREAD
        { 12, 7, 0.043f, 65.0f, 0.17f, 0.00f, 0.022f, 3, 0.000f, false },
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
     * @param playerCount 同時に操作する人数（1または2）
     * @param secondPlayerType 2Pのショットタイプ
     */
    void Initialize(AudioService* audio, PlayerType playerType, DifficultyType difficulty,
        int playerCount = 1, PlayerType secondPlayerType = Homing);
    /** @brief チュートリアル用のゲーム状態を初期化する @param audio 効果音サービス @param playerType 使用機体 @param difficulty 難易度 @return なし */
    void InitializeTutorial(AudioService* audio, PlayerType playerType, DifficultyType difficulty);
    /** @brief 現在のステージBGMを再生する @param force 同じ曲でも再生し直すか @return なし */
    void PlayCurrentStageBgm(bool force = false);
    /** @brief 現在のボスBGMを再生する @param force 同じ曲でも再生し直すか @return なし */
    void PlayCurrentBossBgm(bool force = false);
    /** @brief 実機入力を現在のプレイヤーへ反映する @return なし */
    void ProcessInput();
    /** @brief 同期済みの2人分の操作を適用する @param inputs 固定更新1回分の操作 @return なし */
    void ApplyNetworkInput(const std::array<CooperativeInput, 2>& inputs);
    /** @brief ゲーム状態を固定更新する @return なし */
    void Tick();
    /** @brief ゲーム画面を描画する @param renderer 描画先 @return なし */
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
    /** @brief 全チュートリアル課題を達成したか取得する @return 達成済みの場合true */
    bool IsTutorialComplete() const {
        return m_tutorialStep >= TutorialStepCount && m_clearTimer <= 0;
    }
    /** @brief 現在のチュートリアル課題を飛ばして次へ進む @return なし */
    void NextTutorialStep();

private:
    /** @brief 現在のプレイヤーに操作を適用する @param input 操作状態 @return なし */
    void ApplyPlayerInput(const CooperativeInput& input);
    bool m_networkGame = false;
    CooperativeInput m_networkHostInput {};
    /** @brief チュートリアル専用進行を更新する */
    void TickTutorial();
    /** @brief 現在のチュートリアル課題を準備する */
    void BeginTutorialStep();
    /** @brief チュートリアル案内を描画する @param renderer 描画先 */
    void DrawTutorialHud(Renderer& renderer) const;
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
    class Stage4EnemySheetEasy;
    class Stage4EnemySheetNormal;
    class Stage4EnemySheetHard;
    class Stage5EnemySheet;
    class Stage5EnemySheetEasy;
    class Stage5EnemySheetNormal;
    class Stage5EnemySheetHard;
    class EnemyBehavior;
    class BasicEnemyBehavior;
    class HeavyEnemyBehavior;
    class ArmoredEnemyBehavior;
    class BossEnemyBehavior;
    class StraightShooterEnemyBehavior;
    class DiveRusherEnemyBehavior;
    class CircleShooterEnemyBehavior;
    class SquareShooterEnemyBehavior;
    class MissileShooterEnemyBehavior;
    class LinkedLaserEnemyBehavior;
    class WallSecurityDroneEnemyBehavior;
    class StageDispatch;
    class Stage1Module;
    class Stage2Module;
    class Stage3Module;
    class Stage4Module;
    class Stage5Module;
    class CityBackgroundModule;

    struct Shot {
#if defined(_DEBUG)
        // 描画専用の問い合わせは命中を返さず、全有効部位の判定形状を列挙する
        Renderer* hitboxRenderer = nullptr;
        const Camera3D* hitboxCamera = nullptr;
        float hitboxSideZ = SidePlaneZ;
#endif
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float transitionSideX = 0.0f;
        float transitionSideY = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        float hitRadius = 0.025f;
        float travelDistance = 0.0f;
        int damage = 1;
        int barrageIndex = -1;
        int barrageCount = 0;
        int age = 0;
        int hitCount = 0;
        int homingTarget = -1;
        Vector3 homingOrigin {};
        Vector3 homingLaunchDirection {};
        int homingView = -1;
        int homingTrailCount = 0;
        std::array<Vector3, 24> homingTrail {};
        int owner = 0;
        std::uint16_t bossCollisionIgnoreMask = 0;
        ShooterStages::Stage2::ShotState stage2 {};
        ShooterStages::Stage4::ShotState stage4 {};
        PlayerType playerType = Homing;
        bool enemy = false;
        bool special = false;
        bool piercing = false;
        bool bomb = false;
        bool grazed = false;
        bool firedByBoss = false;
        bool tayamaDragonOrbit = false;
        bool bossCollisionInitialized = false;
        bool active = false;

        /**
         * @brief 自機弾の命中回数を加算して消滅判定を適用する
         * @return なし
         */
        void RegisterHit() { if (!piercing || (!bomb && ++hitCount >= 5)) active = false; }
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
        int recoilType = 0;
        int shotInterval = 0;
        int attackWarningFrames = 0;
        float attackWarningTargetX = 0.0f;
        float attackWarningTargetY = 0.0f;
        int laserLinkId = 0;
        int laserLinkRole = 0;
        int bossPhase = BossNormalPhase1;
        std::array<int, BossPartCount> bossPartHp {};
        std::array<int, BossPartCount> bossPartMaxHp {};
        std::array<int, BossPartCount> bossPartHitFlashFrames {};
        bool collisionEnabled = true;
        bool entersFromTop = false;
        bool entersWallFromTop = false;
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
        unsigned char damagedPlayerMask = 0;
    };

    /** @brief 機体別のミサイル、レーザー、シールド */
    struct Bomb {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        Vector3 direction {};
        PlayerType type = Homing;
        int age = 0;
        bool rail = false;
        bool active = false;
        std::array<Shot, 10> missiles {};
        /** @brief シールドを維持し、一時的な攻撃だけを終了する @return なし */
        void EndAttack() { if (type != Spread) active = false; }
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
        bool damagesPlayer = false;
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
        float vx = 0.0f;
        float vy = 0.0f;
        float power = 0.25f;
        int score = 0;
        int pickupDelay = 0;
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
        bool bombAwarded = false;
    };

    static constexpr int ShotCapacity = 512;
    static constexpr int Stage5ShotCapacity = 1024;
    static constexpr int EnemyCapacity = 12;
    static constexpr int ItemCapacity = 48;
    static constexpr int ExplosionCapacity = ShotCapacity + 32;
    static_assert(ExplosionCapacity >= ShotCapacity);
    static constexpr int ExplosionLifetimeFrames = 18;
    static constexpr int DestructionExplosionLifetimeFrames = 48;
    static constexpr int MortarExplosionLifetimeFrames = 64;
    // 減速後も従来と同程度の距離を追尾できるよう飛行時間を確保する
    static constexpr int BombMissileFrames = 320;
    static constexpr int BombMissileInterval = 6;
    // 十発目を射出し終えてから全弾のブースターを点火する
    static constexpr int BombMissileLaunchFrames = 9 * BombMissileInterval + 1;
    static constexpr int BombLaserFrames = 180;
    static constexpr float BombMissileSpeed = 0.84f;
    static constexpr float BombMissileScale = 0.24f;
    static constexpr float BombMissileRadius = 0.85f * BombMissileScale;
    static constexpr float BombLaserRadius = 1.8f;
    static constexpr float BombLaserLength = 90.0f;
    static constexpr float BombShieldRadius = 1.8f;
    static constexpr int MaxBombCount = 3;
    static constexpr int InitialBombCount = MaxBombCount;
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
    static constexpr float Side2DPlayerMinY = -1.62f;
    /** @brief ヘッダー直下のプレイ領域上端に対応する自機中心のY座標 */
    static constexpr float Side2DPlayerMaxY = 1.53f;
    /** @brief 自機弾が画面外へ抜けるまで保持する余白 */
    static constexpr float Side2DShotCullMargin = 0.25f;
    static constexpr float PlayerRailZ = 8.0f;
    static constexpr float SidePlaneZ = 10.0f;
    static constexpr float EnemyRailFarZ = 60.0f;
    /** @brief 3D時に敵発射体を生成しない自機中心の球半径 */
    static constexpr float EnemyProjectileNoFireDistance3D = 16.0f;

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
     * @brief Stage 5第2部の縦スクロール弾道を使用するか判定する
     * @param stageNumber 現在のステージ番号
     * @param phase 現在のStage 5状態
     * @return 第2部道中の場合true
     */
    static constexpr bool UsesVerticalPlayerShots(int stageNumber, Stage5Phase phase) {
        return stageNumber == 5 && ShooterStages::Stage5::IsPart2RoutePhase(phase);
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

    /**
     * @brief TAYAMAを中心とする円形アリーナ上のXZ座標を取得する
     * @param angle ボス正面を0とする周回角
     * @param sideOffset 固定2D視点内の横移動量
     * @return XにワールドX、YにワールドZを格納した座標
     */
    static Vector2 TayamaOrbitXZ(float angle, float sideOffset) {
        const float radialX = std::sin(angle);
        const float radialZ = -std::cos(angle);
        const float tangentX = std::cos(angle);
        const float tangentZ = std::sin(angle);
        return {
            radialX * ShooterStages::Stage5::TayamaOrbitRadius + tangentX * sideOffset * WorldXScale,
            ShooterStages::Stage5::TayamaArenaCenterZ +
                radialZ * ShooterStages::Stage5::TayamaOrbitRadius + tangentZ * sideOffset * WorldXScale
        };
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
     * @param playBossWarningSound ボス登場警報を再生する場合true
     * @return なし
     */
    void StartDebugCheckpoint(int stageNumber, int chapterNumber, bool bossBattle, bool playBossWarningSound = true);
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
    static const EnemyBehavior& MissileShooterEnemyBehaviorInstance();
    static const EnemyBehavior& LinkedLaserEnemyBehaviorInstance();
    static const EnemyBehavior& WallSecurityDroneEnemyBehaviorInstance();
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
    /** @brief 接続レーザー敵のレーザー接触判定を更新する */
    void TickLinkedEnemyLasers();
    void TickShots();
    /** @brief 弾プールの一時コピーを作らず全弾を初期化する @return なし */
    void ResetShots();
    /**
     * @brief 機体別ボムの発動、移動、持続時間を更新する
     * @return なし
     */
    void TickBomb();
    /** @brief ボムの攻撃範囲を共通の弾判定へ変換する @return 攻撃線分と威力を持つ弾 */
    Shot MakeBombShot() const;
    /** @brief ボムと敵弾の移動軌跡が重なるか判定する @param shot 敵弾 @return 消去対象ならtrue */
    bool BombClearsShot(const Shot& shot);
    /** @brief 二つの弾の掃引範囲が接触するか判定する @param bomb 弾消し側 @param shot 敵弾 @return 接触時true */
    bool BombShotIntersects(const Shot& bomb, const Shot& shot) const;
    /** @brief 自機弾とボムに共通の敵・部位へのダメージを適用する @param shot 自機弾 @return なし */
    void HitPlayerShotTargets(Shot& shot);
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
    /**
     * @brief 難易度に応じたHPへ変換する
     * @param hp Normal基準のHP
     * @param difficulty 使用する難易度
     * @return 難易度倍率を適用したHP
     */
    static constexpr int ScaleHpForDifficulty(int hp, DifficultyType difficulty);
    /** @brief 敵機HPへ難易度倍率を適用する */
    void ApplyDifficultyToEnemyHp(Enemy& enemy) const;
    /** @brief ボス本体と部位HPへ難易度倍率を適用する */
    void ApplyDifficultyToBossHp(Enemy& boss) const;
    /** @brief Stage 5専用弱点HPへ難易度倍率を適用する */
    void ApplyDifficultyToStage5WeakpointHp();
    void SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ);
    /**
     * @brief 未解放の展示だけを永続データへ追加する
     * @param entry 解放する展示
     * @return なし
     */
    void UnlockGallery(GalleryEntry entry);
    /** @brief 指定座標にPowerアイテムを生成する */
    void SpawnPowerItem(float x, float y, float z, float value);
    /**
     * @brief 指定座標にScoreアイテムを生成する
     * @param x 2D座標系のX座標
     * @param y 2D座標系のY座標
     * @param z 3Dレール座標系のZ座標
     * @param value 取得時に加算するScore
     * @param vx 生成直後のX方向速度
     * @param vy 生成直後のY方向速度
     * @param pickupDelay 取得を開始するまでのフレーム数
     * @return なし
     */
    void SpawnScoreItem(float x, float y, float z, int value,
        float vx = 0.0f, float vy = 0.0f, int pickupDelay = 0);
    /**
     * @brief ボス戦開始状態を構築する
     * @param playWarningSound ボス登場警報を再生する場合true
     * @return なし
     */
    void StartBossBattle(bool playWarningSound = true);
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
     * @brief 敵発射体の生成位置が3D時の自機接近禁止範囲外か判定する
     * @param x 発射元ゲーム座標X
     * @param y 発射元ゲーム座標Y
     * @param z 発射元レール座標Z
     * @return 2D時または自機から12ユニットより離れている場合true
     */
    bool CanSpawnEnemyProjectile(float x, float y, float z) const;
    /**
     * @brief 現在のStageで利用できる弾プール容量を取得する
     * @return Stage 5は1024、それ以外は512
     */
    constexpr int ActiveShotCapacity() const {
        return m_stageNumber == 5 ? Stage5ShotCapacity : ShotCapacity;
    }
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
     * @param firedByBoss ボスが発射した弾の場合true
     * @return なし
     */
    void SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy,
        int barrageIndex = -1, int barrageCount = 0, bool firedByBoss = false);
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
     * @param hitRadius 爆破当たり判定半径
     * @return なし
     */
    void SpawnMortarExplosion(float x, float y, float z, float hitRadius = 0.55f);
    /** @brief 機体モデルを構成する部品を飛散エフェクトとして生成する */
    void SpawnEnemyDebris(const Enemy& enemy, int bossPart = -1);
    /** @brief 飛散するモデル部品を固定長プールへ追加する */
    Debris* SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
        float yaw, float spin, int shape, float width, float height, float depth,
        const float color[4], int lifetime = DebrisLifetimeFrames,
        int shrinkStartAge = DebrisLifetimeFrames, bool gravity = false,
        bool damagesPlayer = false);
    void FireSpecialShots();
    void UpdateHomingShot(Shot& shot);
    /** @brief 通常の十字照準のワールド位置を取得する @return 照準位置 */
    Vector3 PlayerAimPoint() const;
    /** @brief 発射と照準予測に共通の通常弾を作る @param snap スナップ補正を適用する場合true @return 通常弾 */
    Shot MakeNormalPlayerShot(bool snap = true) const;
    /** @brief 3D照準の対象を更新する @param advance 枠と弾道の補間を1フレーム進める場合true @return なし */
    void UpdateAimSnap(bool advance = true);
    /** @brief 発射時の弾道を標的へ補正する @param shot 発射する自機弾 @return なし */
    void ApplyAimSnap(Shot& shot) const;
    /**
     * @brief 追尾または画面上の照準距離で攻撃可能な標的を選ぶ
     * @param shot 標的を選ぶ自機弾
     * @param target 選んだ標的のワールド位置
     * @param aimCamera 非nullなら十字照準の近傍だけを選ぶ
     * @return 標的ID、対象なしなら-1
     */
    int FindShotTarget(const Shot& shot, Vector3& target, const Camera3D* aimCamera = nullptr);
    void DamagePlayer();
    /** @brief 現在のチャプターを開始時状態へ戻す */
    void RestartCurrentChapter();
    /**
     * @brief 自機弾が未破壊のボス部位へ命中したか判定する
     * @param shot 判定対象の自機弾
     * @param boss 判定対象のボス
     * @param part 命中した部位の格納先
     * @param aimPosition 非nullなら衝突判定せず指定部位の攻撃可能なワールド中心を取得する
     * @return 部位へ命中した場合true、命中しない場合false
     */
    bool TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part, Vector3* aimPosition = nullptr) const;
    /**
     * @brief 共通または移行中のボス部位判定を行う
     * @param shot 判定対象の自機弾
     * @param boss 判定対象のボス
     * @param part 命中した部位の格納先
     * @param aimPosition 非nullなら衝突判定せず指定部位の攻撃可能なワールド中心を取得する
     * @return 部位へ命中した場合true、命中しない場合false
     */
    bool TryHitDefaultBossPart(const Shot& shot, const Enemy& boss, BossPart& part, Vector3* aimPosition = nullptr) const;
    void PlayShotSound();
    void PlayHitSound();
    /** @brief 敵のエネルギー弾発射音を再生する @return なし */
    void PlayEnemyShotSound();
    /** @brief ミサイル噴射開始音を再生する @return なし */
    void PlayMissileLaunchSound();
    /** @brief ボスマシンガンの単発音を再生する @return なし */
    void PlayBossMachineGunSound();
    static bool Hit(float ax, float ay, float ar, float bx, float by, float br);
    /**
     * @brief 自機弾と円の判定、またはDebug用の判定範囲描画を行う
     * @param shot 判定対象の弾または描画専用の問い合わせ
     * @param x 対象中心のゲーム座標X
     * @param y 対象中心のゲーム座標Y
     * @param radius 対象円のゲーム座標半径
     * @return 命中時true、描画専用時false
     */
    static bool HitShotCircle(const Shot& shot, float x, float y, float radius);
    /**
     * @brief 自機弾の移動線分と球の判定、またはDebug用の判定範囲描画を行う
     * @param shot 判定対象の弾または描画専用の問い合わせ
     * @param x 対象中心のワールド座標X
     * @param y 対象中心のワールド座標Y
     * @param z 対象中心のワールド座標Z
     * @param radius 対象球のワールド半径
     * @return 命中時true、描画専用時false
     */
    static bool HitShotSphere(const Shot& shot, float x, float y, float z, float radius);
    static bool Hit3D(float ax, float ay, float az, float ar, float bx, float by, float bz, float br);
    /**
     * @brief 点と2D線分の最短距離を取得する
     * @param point 判定点
     * @param start 線分開始点
     * @param end 線分終了点
     * @return 最短距離
     */
    static float DistancePointToSegment2D(const Vector2& point, const Vector2& start, const Vector2& end);
    /**
     * @brief 点と3D線分の最短距離を取得する
     * @param point 判定点
     * @param start 線分開始点
     * @param end 線分終了点
     * @return 最短距離
     */
    static float DistancePointToSegment3D(const Vector3& point, const Vector3& start, const Vector3& end);
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
    /**
     * @brief 現在チャプター内の到達率を取得する
     * @return 0から100までの到達率
     */
    int ChapterProgressPercent() const;
    /** @brief チャプターの総合スコアを算出する */
    static int CalculateChapterTotalScore(const ChapterResult& result);
    float RailBlend() const;
    /**
     * @brief TAYAMAとの操作可能な最終戦か判定する
     * @return TAYAMA戦闘中の場合true
     */
    bool IsTayamaBattle() const;
    /**
     * @brief 現在の表示切り替え先がTAYAMA周回3D視点か判定する
     * @return 周回3D視点の場合true
     */
    bool IsTayamaOrbitViewActive() const;
    /**
     * @brief 現在のゲームルール上の自機ワールド座標を取得する
     * @return 自機中心のワールド座標
     */
    Vector3 PlayerWorldPosition() const;
    /**
     * @brief 現在のステージ進行に対応する自機のレール奥行きを取得する
     * @return 自機のレール座標Z
     */
    float PlayerRailDepth() const;
    bool IsRailGameplayActive() const;
    bool IsRailRenderActive() const;
    /**
     * @brief 現在2Dと3Dの表示を切り替えられるか判定する
     * @return 切り替え可能な場合true
     */
    bool CanToggleView() const;
    void ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const;
    void ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const;
    /** @brief 描画と照準に共通のカメラを設定する @param camera 設定先 @param viewport 描画領域 @return なし */
    void ConfigureRailCamera(Camera3D& camera, const Viewport& viewport) const;
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
     * @brief 実際のHPフェーズ境界へボスHPバーの区切りを描画する
     * @param renderer 描画先レンダラー
     * @param y HPバーの中心Y座標
     * @param halfWidth HPバーの半幅
     * @param maxHp ボスの最大HP
     * @param color 区切り線のRGBA色
     * @return なし
     */
    void DrawBossPhaseDividers(Renderer& renderer, float y, float halfWidth,
        int maxHp, const float color[4]) const;
    /**
     * @brief ボス登場中に上下の警告帯を描画する
     * @param renderer 描画先レンダラー
     * @return なし
     */
    void DrawBossWarning(Renderer& renderer) const;
    /**
     * @brief 2Dと3Dの表示切り替えクールダウンを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param playerZ 描画中の自機のワールド座標Z
     * @return なし
     */
    void DrawViewToggleCooldownHud(
        Renderer& renderer, const Camera3D& camera, float playerZ) const;
    /**
     * @brief 3D視点の十字照準とスナップ対象の四角い照準を描画する
     * @param renderer 描画先レンダラー
     * @param camera 射撃方向の投影に使うカメラ
     * @return なし
     */
    void DrawReticle(Renderer& renderer, const Camera3D& camera) const;
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
     * @param pitch 自機のX軸回転角度
     * @param roll 自機のZ軸回転角度
     * @return なし
     */
    void DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
        float x, float y, float z, bool visible,
        float yaw = 0.0f, float pitch = 0.0f, float roll = 0.0f) const;
    void DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw = 0.0f) const;
#if defined(_DEBUG)
    bool m_showHitboxes = true;
    /**
     * @brief ワールド座標の判定楕円体を半透明の赤で描画する
     * @param query 描画先を設定した問い合わせ
     * @param center 判定中心のワールド座標
     * @param radii 各軸の半径
     * @return なし
     */
    static void DrawHitboxEllipsoid(const Shot& query, const Vector3& center, const Vector3& radii);
    /**
     * @brief 敵の接触判定を半透明の赤で描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在のカメラ
     * @param enemy 表示座標へ変換済みの敵
     * @return なし
     */
    void DrawEnemyHitbox(Renderer& renderer, const Camera3D& camera, const Enemy& enemy) const;
#endif
    /**
     * @brief 接続レーザー敵の機体間レーザーを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param railWeight 横視点からレール視点への補間率
     * @return なし
     */
    void DrawLinkedEnemyLasers(Renderer& renderer, const Camera3D& camera, float railWeight) const;
    /**
     * @brief Stage2主砲と同じRailgunシェーダーで線分レーザーを描画する
     * @param renderer 描画先レンダラー
     * @param camera 現在の3Dカメラ
     * @param start 開始ワールド座標
     * @param end 終了ワールド座標
     * @param width レーザー幅
     * @param progress シェーダーへ渡す進行値
     * @param effectType シェーダー効果種別
     * @return なし
     */
    static void DrawRailgunBeamBetween(Renderer& renderer, const Camera3D& camera,
        const Vector3& start, const Vector3& end, float width, float progress, int effectType);
    void DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw = 0.0f) const;
    /** @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する */
    static void DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion);
    /**
     * @brief 機体別のミサイル、極太レーザー、シールドを描画する
     * @param renderer 描画先レンダラー
     * @param camera 描画に使用するカメラ
     * @param bomb 描画対象のボム
     * @return なし
     */
    void DrawBomb(Renderer& renderer, const Camera3D& camera, const Bomb& bomb) const;
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
    /**
     * @brief チュートリアルの操作キーを自機上部へ表示する
     * @param renderer 描画先レンダラー
     * @param camera 自機位置の投影に使うカメラ
     * @param playerZ 描画中の自機Z座標
     * @return なし
     */
    void DrawTutorialControlHint(
        Renderer& renderer, const Camera3D& camera, float playerZ) const;
    /** @brief ミッション開始または終了の文字アニメーションを描画する */
    void DrawMissionBanner(Renderer& renderer) const;
    /** @brief 文字表示領域へ共通の黒いHUD背景を描画する @param renderer 描画先 @return なし */
    void DrawHudBackground(Renderer& renderer) const;

    /** @brief 各プレイヤーが独立して保持する戦闘状態 */
    struct PlayerState {
        PlayerType m_playerType = Homing;
        float m_playerX = -0.72f;
        float m_playerY = 0.0f;
        int m_shotCooldown = 0;
        int m_specialShotCooldown = 0;
        int m_invincible = 0;
        int m_bombCount = InitialBombCount;
        int m_playerDestructionTimer = 0;
        int m_powerUpTimer = 0;
        float m_power = 0.0f;
        bool m_moveLeft = false;
        bool m_moveRight = false;
        bool m_moveUp = false;
        bool m_moveDown = false;
        bool m_slowMove = false;
        bool m_fire = false;
        bool m_bombRequested = false;
        bool m_aimSnapped = false;
        Vector3 m_aimSnapTarget {};
        Vector3 m_aimSnapOffset {};
        float m_aimSnapBlend = 0.0f;
        int m_aimSampleId = -1;
        Vector3 m_aimSamplePosition {};
        Vector3 m_aimTargetVelocity {};
        bool m_crosshairInitialized = false;
        Vector2 m_crosshairPosition {};
        Vector2 m_crosshairVelocity {};
        Bomb m_bomb {};
    };
    std::array<PlayerState, 2> m_players {};
    int m_playerCount = 1;
    mutable int m_activePlayer = 0;
    mutable Viewport m_aimViewport {0, 0, 1280, 720};
    /** @brief 処理対象の自機状態を取得する @return 自機状態 */
    PlayerState& Player() { return m_players[m_activePlayer]; }
    /** @brief 描画対象の自機状態を取得する @return 自機状態 */
    const PlayerState& Player() const { return m_players[m_activePlayer]; }
    /** @brief 各自機へ同じ処理を適用し、呼び出し元の対象へ戻す @param action 自機ごとの処理 @return なし */
    template<class Action> void ForEachPlayer(Action action) const {
        // ponytail: 更新と描画は単一スレッド、並列化時はPlayerStateを引数で渡す
        const int previous = m_activePlayer;
        for (int i = 0; i < m_playerCount; ++i) { m_activePlayer = i; action(); }
        m_activePlayer = previous;
    }

    std::array<Shot, Stage5ShotCapacity> m_shots {};
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
    int m_currentBgmStage = 0;
    bool m_currentBgmIsBoss = false;
    bool m_isRestartingChapter = false;
    const Stage* m_stage = nullptr;
    DifficultyType m_difficulty = Easy;
    std::uint32_t m_galleryUnlocks = DefaultGalleryUnlocks;
    float m_scroll = 0.0f;
    int m_frame = 0;
    int m_spawnCooldown = 0;
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
    int m_restartTimer = 0;
    int m_missionStartTimer = 0;
    int m_clearTimer = 0;
    bool m_tutorialMode = false;
    int m_tutorialStep = 0;
    int m_tutorialStepFrame = 0;
    bool m_tutorialSlowUsed = false;
    static constexpr int TutorialStepCount = 5;
    bool m_clear = false;
    bool m_bossBattle = false;
    bool m_bossBattlePending = false;
    bool m_chapterResultActive = false;
    bool m_viewToggleRequested = false;
    ViewMode m_viewMode = ViewMode::Side2D;
    ViewMode m_nextViewMode = ViewMode::Side2D;
    int m_viewTransitionTimer = 0;
    int m_viewToggleCooldown = 0;
    bool m_grazing = false;
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
static_assert(SideScrollingShooter::UsesVerticalPlayerShots(
    5, SideScrollingShooter::Stage5Phase::WallClimbLower));
static_assert(!SideScrollingShooter::UsesVerticalPlayerShots(
    4, SideScrollingShooter::Stage5Phase::WallClimbLower));
static_assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
    SideScrollingShooter::TayamaWeakpoint::FireControlRadar,
    SideScrollingShooter::Stage5Phase::TayamaFireControl));
static_assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
    SideScrollingShooter::TayamaWeakpoint::CommandCore,
    SideScrollingShooter::Stage5Phase::TayamaFireControl));
