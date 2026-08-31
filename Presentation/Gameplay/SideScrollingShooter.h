#pragma once

#include <array>

#include "../../Domain/ValueObjects/DifficultyType.h"
#include "../../Domain/ValueObjects/PlayerType.h"
#include "../../Engine/Graphics/Camera3D.h"

class AudioService;
class Renderer;

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

private:
    class Stage;
    class Stage1EnemySheet;
    class Stage1EnemySheetEasy;
    class Stage1EnemySheetNormal;
    class Stage1EnemySheetHard;
    class Stage2;
    class Stage3;
    class Stage4;
    class Stage5;
    class EnemyBehavior;
    class BasicEnemyBehavior;
    class HeavyEnemyBehavior;
    class ArmoredEnemyBehavior;
    class BossEnemyBehavior;
    class StraightShooterEnemyBehavior;
    class CircleShooterEnemyBehavior;

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
        BossPartCount
    };
    static_assert(BossPartCount == 5);

    struct Enemy {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float transitionSideX = 0.0f;
        float transitionSideY = 0.0f;
        float baseX = 0.0f;
        float baseY = 0.0f;
        float phase = 0.0f;
        int hp = 0;
        int maxHp = 0;
        int type = 0;
        int age = 0;
        int shotInterval = 0;
        std::array<int, BossPartCount> bossPartHp {};
        const EnemyBehavior* behavior = nullptr;
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
    static constexpr float MaxPower = 4.0f;
    static constexpr int ChapterLengthFrames = 500;
    static constexpr int ChapterResultCountUpFrames = 120;
    static constexpr int ChapterResultDisplayFrames = 180;
    static constexpr float BossStartDistance = 12.0f;
    static constexpr int BossMaxHp = 480;
    static constexpr int ViewTransitionFrames = 90;
    static constexpr float WorldXScale = 7.0f;
    static constexpr float WorldYScale = 4.4f;
    /** @brief 2D画面のプレイ領域左端に対応する自機中心のX座標 */
    static constexpr float Side2DPlayerMinX = -1.82f;
    /** @brief 2D画面のプレイ領域右端に対応する自機中心のX座標 */
    static constexpr float Side2DPlayerMaxX = 1.82f;
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

    void Reset(bool resetRetryCounts = true);
    /**
     * @brief 指定難易度のステージ1敵出現シートを取得する
     * @param difficulty 取得する難易度
     * @return 難易度に対応するステージ1敵出現シート
     */
    static const Stage& Stage1EnemySheetInstance(DifficultyType difficulty);
    static const Stage& Stage2Instance();
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
    static const EnemyBehavior& StraightShooterEnemyBehaviorInstance();
    static const EnemyBehavior& CircleShooterEnemyBehaviorInstance();
    static const EnemyBehavior& EnemyBehaviorForType(int type);
    void TickViewTransition();
    void InitializeRailObjects();
    void InitializeSideObjects();
    void TickPlayer();
    void TickEnemies();
    void TickShots();
    /** @brief 取得アイテムを更新して自機との取得判定を行う */
    void TickItems();
    /** @brief ボス戦前会話を進行する */
    void TickBossStory();
    /** @brief チャプター終了演出を更新する */
    void TickChapterResult();
    /** @brief 現在のチャプター戦績を確定して表示を開始する */
    void FinishChapter();
    void SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ);
    /** @brief 指定座標にPowerアイテムを生成する */
    void SpawnPowerItem(float x, float y, float z, float value);
    /** @brief 指定座標にScoreアイテムを生成する */
    void SpawnScoreItem(float x, float y, float z, int value);
    void StartBossBattle();
    /** @brief 次のステージの戦闘状態を初期化する */
    void StartNextStage();
    void SpawnShot(float x, float y, float vx, float vy, bool enemy,
        float z = -1.0f, float railSpeed = -1.0f, int damage = 1);
    void SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy);
    void FireSpecialShots();
    void UpdateHomingShot(Shot& shot);
    void DamagePlayer();
    /**
     * @brief 自機弾が未破壊のボス部位へ命中したか判定する
     * @param shot 判定対象の自機弾
     * @param boss 判定対象のボス
     * @param part 命中した部位の格納先
     * @return 部位へ命中した場合true
     */
    bool TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const;
    void PlayShotSound();
    void PlayHitSound();
    static bool Hit(float ax, float ay, float ar, float bx, float by, float br);
    static bool Hit3D(float ax, float ay, float az, float ar, float bx, float by, float bz, float br);
    static float SmoothStep(float value);
    static float ToWorldX(float x);
    static float ToWorldY(float y);
    static float FromWorldX(float x);
    static float FromWorldY(float y);
    static float ToRailZFromSideX(float x);
    static float ToSideXFromRailZ(float z);
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
    void DrawBossHud(Renderer& renderer) const;
    /** @brief ボス戦前会話を画面へ描画する */
    void DrawBossStory(Renderer& renderer) const;
    static void DrawShape(Renderer& renderer,
        float x, float y, float w, float h, const float color[4]);
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
        float x, float y, float z, float w, float h, float d, const float color[4], float yaw = 0.0f);
    static void DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
        float x, float y, float z, bool visible, float yaw = 0.0f);
    static void DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw = 0.0f);
    static void DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw = 0.0f);
    /** @brief 取得アイテムを描画する */
    static void DrawItemModel(Renderer& renderer, const Camera3D& camera,
        const Item& item, float yaw = 0.0f);
    /** @brief チャプター終了時の戦績を描画する */
    void DrawChapterResult(Renderer& renderer) const;

    std::array<Shot, ShotCapacity> m_shots {};
    std::array<Enemy, EnemyCapacity> m_enemies {};
    std::array<Item, ItemCapacity> m_items {};
    AudioService* m_audio = nullptr;
    const Stage* m_stage = nullptr;
    PlayerType m_playerType = Homing;
    DifficultyType m_difficulty = Easy;
    float m_playerX = -0.72f;
    float m_playerY = 0.0f;
    float m_scroll = 0.0f;
    int m_frame = 0;
    int m_spawnCooldown = 0;
    int m_shotCooldown = 0;
    int m_specialShotCooldown = 0;
    int m_invincible = 0;
    int m_lives = 3;
    int m_score = 0;
    int m_kills = 0;
    int m_bossHp = 0;
    int m_bossStoryLine = 0;
    bool m_bossStoryActive = false;
    int m_stageNumber = 1;
    int m_chapterNumber = 1;
    std::array<int, 3> m_chapterRetryCounts {};
    ChapterResult m_chapterResult {};
    int m_chapterResultTimer = 0;
    float m_power = 0.0f;
    int m_clearTimer = 0;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_fire = false;
    bool m_gameOver = false;
    bool m_clear = false;
    bool m_bossBattle = false;
    bool m_bossBattlePending = false;
    bool m_chapterResultActive = false;
    bool m_viewToggleRequested = false;
    ViewMode m_viewMode = ViewMode::Side2D;
    ViewMode m_nextViewMode = ViewMode::Side2D;
    int m_viewTransitionTimer = 0;
    float m_viewTransitionProgress = 0.0f;
};
