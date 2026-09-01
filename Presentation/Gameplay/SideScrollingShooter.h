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
    /**
     * @brief 全ステージをクリア済みか取得する
     * @return 最終ステージのクリア演出中ならtrue
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
        int barrageIndex = -1;
        int barrageCount = 0;
        PlayerType playerType = Homing;
        bool enemy = false;
        bool special = false;
        bool piercing = false;
        bool grazed = false;
        bool active = false;
    };

    static constexpr int BossPartCapacity = 5;

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
        float actionX = 0.0f;
        float actionY = 0.0f;
        float actionZ = 0.0f;
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
        int bossPhase = 0;
        std::array<int, BossPartCapacity> bossPartHp {};
        const EnemyBehavior* behavior = nullptr;
        bool active = false;
    };

    /** @brief 弾が敵へ命中した位置に表示する短時間の爆発 */
    struct Explosion {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        int age = 0;
        bool active = false;
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
    static constexpr int ExplosionCapacity = 32;
    static constexpr int ExplosionLifetimeFrames = 18;
    static constexpr int AttackWarningFrames = 12;
    static constexpr int DebrisCapacity = 96;
    static constexpr int DebrisLifetimeFrames = 36;
    static constexpr int MeteorCount = 6;
    static constexpr int BoneArchMaxHp = 12000;
    static constexpr float MaxPower = 4.0f;
    static constexpr int ChapterLengthFrames = 500;
    static constexpr int ChapterResultCountUpFrames = 120;
    static constexpr int ChapterResultDisplayFrames = 180;
    static constexpr int RestartDisplayFrames = 180;
    static constexpr float BossStartDistance = 12.0f;
    static constexpr int BossMaxHp = 480;
    static constexpr int ViewTransitionFrames = 90;
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
    static const EnemyBehavior& StraightShooterEnemyBehaviorInstance();
    static const EnemyBehavior& CircleShooterEnemyBehaviorInstance();
    static const EnemyBehavior& EnemyBehaviorForType(int type);
    void TickViewTransition();
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
    void SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy,
        int barrageIndex = -1, int barrageCount = 0);
    /** @brief 弾の命中位置へ爆発エフェクトを生成する */
    void SpawnExplosion(float x, float y, float z);
    /** @brief 機体モデルを構成する部品を飛散エフェクトとして生成する */
    void SpawnEnemyDebris(const Enemy& enemy, int bossPart = -1);
    /** @brief 飛散するモデル部品を固定長プールへ追加する */
    void SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
        float yaw, float spin, int shape, float width, float height, float depth,
        const float color[4]);
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
    bool TryHitBossPart(const Shot& shot, const Enemy& boss, int& part) const;
    void PlayShotSound();
    void PlayHitSound();
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
    static void DrawShape(Renderer& renderer,
        float x, float y, float w, float h, const float color[4]);
    static void DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
        float x, float y, float z, float w, float h, float d, const float color[4], float yaw = 0.0f);
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
    static void DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
        float x, float y, float z, bool visible, float yaw = 0.0f);
    void DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw = 0.0f) const;
    void DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw = 0.0f) const;
    /** @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する */
    static void DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion);
    /** @brief 飛散中の機体部品を描画する */
    static void DrawDebris(Renderer& renderer, const Camera3D& camera, const Debris& debris);
    /** @brief 取得アイテムを描画する */
    static void DrawItemModel(Renderer& renderer, const Camera3D& camera,
        const Item& item, float yaw = 0.0f);
    /** @brief チャプター終了時の戦績を描画する */
    void DrawChapterResult(Renderer& renderer) const;
    /** @brief リスタート中のカウントダウンを描画する */
    void DrawRestart(Renderer& renderer) const;

    std::array<Shot, ShotCapacity> m_shots {};
    std::array<Enemy, EnemyCapacity> m_enemies {};
    std::array<Item, ItemCapacity> m_items {};
    std::array<Explosion, ExplosionCapacity> m_explosions {};
    std::array<Debris, DebrisCapacity> m_debris {};
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
    int m_score = 0;
    int m_kills = 0;
    int m_bossHp = 0;
    float m_displayBossHp = 0.0f;
    int m_bossStoryLine = 0;
    bool m_bossStoryActive = false;
    int m_stageNumber = 1;
    int m_chapterNumber = 1;
    std::array<int, 3> m_chapterRetryCounts {};
    ChapterResult m_chapterResult {};
    float m_chapterStartPower = 0.0f;
    int m_chapterStartScore = 0;
    int m_chapterStartKills = 0;
    int m_chapterResultTimer = 0;
    int m_restartTimer = 0;
    float m_power = 0.0f;
    int m_clearTimer = 0;
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
    ViewMode m_viewMode = ViewMode::Side2D;
    ViewMode m_nextViewMode = ViewMode::Side2D;
    int m_viewTransitionTimer = 0;
    float m_viewTransitionProgress = 0.0f;
};
