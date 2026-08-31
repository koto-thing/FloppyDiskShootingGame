#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"

namespace {
constexpr float ModeTextColor[4] = { 0.55f, 0.85f, 1.0f, 1.0f };
constexpr float PlayerColor[4] = { 0.80f, 0.80f, 0.85f, 1.0f };
constexpr float PlayerAccent[4] = { 0.10f, 0.90f, 0.90f, 1.0f };
constexpr float EnemyColor[4] = { 0.90f, 0.12f, 0.12f, 1.0f };
constexpr float EnemyAccent[4] = { 1.00f, 0.55f, 0.08f, 1.0f };
constexpr float PlayerShotColor[4] = { 0.15f, 1.00f, 0.25f, 1.0f };
constexpr float HomingShotColor[4] = { 0.15f, 0.85f, 1.00f, 1.0f };
constexpr float PiercingShotColor[4] = { 0.72f, 0.20f, 1.00f, 1.0f };
constexpr float SpreadShotColor[4] = { 1.00f, 0.20f, 0.52f, 1.0f };
constexpr float EnemyShotColor[4] = { 1.00f, 0.25f, 0.25f, 1.0f };
constexpr float PowerItemColor[4] = { 1.00f, 0.88f, 0.12f, 1.0f };
constexpr float ScoreItemColor[4] = { 0.25f, 0.90f, 1.00f, 1.0f };
constexpr float SideBackgroundColor[4] = { 0.01f, 0.04f, 0.08f, 1.0f };
constexpr float GridColor[4] = { 0.05f, 0.22f, 0.16f, 1.0f };
constexpr float StarColor[4] = { 0.55f, 0.70f, 0.85f, 1.0f };
constexpr float DesertDaySkyColor[4] = { 0.30f, 0.68f, 0.92f, 1.0f };
constexpr float DesertNightSkyColor[4] = { 0.015f, 0.03f, 0.12f, 1.0f };
constexpr float DesertSandColor[4] = { 0.84f, 0.58f, 0.25f, 1.0f };
constexpr float DesertCactusColor[4] = { 0.08f, 0.34f, 0.16f, 1.0f };
constexpr float DesertBoneColor[4] = { 0.88f, 0.78f, 0.56f, 1.0f };
constexpr float MeteorColor[4] = { 0.30f, 0.22f, 0.18f, 1.0f };
constexpr float MeteorCraterColor[4] = { 0.95f, 0.38f, 0.08f, 1.0f };
constexpr float OceanWaterColor[4] = { 0.04f, 0.34f, 0.60f, 1.0f };
constexpr float OceanWaveColor[4] = { 0.20f, 0.74f, 0.86f, 1.0f };
constexpr float OceanFoamColor[4] = { 0.78f, 0.94f, 0.92f, 1.0f };
constexpr float OceanCloudColor[4] = { 0.90f, 0.95f, 0.96f, 1.0f };
constexpr float OceanSunColor[4] = { 1.00f, 0.82f, 0.20f, 1.0f };
constexpr float SeaSerpentColor[4] = { 0.05f, 0.24f, 0.20f, 1.0f };
constexpr float SeaSerpentBellyColor[4] = { 0.28f, 0.62f, 0.48f, 1.0f };
constexpr float SeaSerpentEyeColor[4] = { 1.00f, 0.84f, 0.16f, 1.0f };
constexpr float CityStreetColor[4] = { 0.025f, 0.05f, 0.11f, 1.0f };
constexpr float CityBuildingColor[4] = { 0.055f, 0.10f, 0.20f, 1.0f };
constexpr float CityWindowCyanColor[4] = { 0.12f, 0.82f, 0.98f, 1.0f };
constexpr float CityWindowMagentaColor[4] = { 0.90f, 0.18f, 0.76f, 1.0f };
constexpr float CityMoonColor[4] = { 0.86f, 0.90f, 0.72f, 1.0f };
constexpr float CityRoadColor[4] = { 0.075f, 0.09f, 0.16f, 1.0f };
constexpr float CityLaneColor[4] = { 0.72f, 0.84f, 0.88f, 1.0f };
constexpr float CityCarBodyColor[4] = { 0.18f, 0.76f, 0.96f, 1.0f };
constexpr float CityCarAccentColor[4] = { 0.98f, 0.24f, 0.70f, 1.0f };
constexpr float TowerFacadeColor[4] = { 0.10f, 0.13f, 0.24f, 1.0f };
constexpr float TowerNeonColor[4] = { 0.90f, 0.08f, 0.42f, 1.0f };
constexpr float TowerRoofColor[4] = { 0.12f, 0.14f, 0.20f, 1.0f };
constexpr float SatelliteBodyColor[4] = { 0.58f, 0.68f, 0.78f, 1.0f };
constexpr float SatellitePanelColor[4] = { 0.16f, 0.48f, 0.88f, 1.0f };
constexpr float SatelliteLightColor[4] = { 0.82f, 0.94f, 1.0f, 1.0f };
constexpr float SideCameraZ = -16.0f;
constexpr float SideCameraFieldOfView = 38.0f;
constexpr int Stage2NightStartFrame = 500;
constexpr int Stage2NightFrame = 750;
constexpr int Stage3DawnStartFrame = 500;
constexpr int Stage3DawnFrame = 750;
constexpr int Stage5WallClimbStartFrame = 750;
constexpr int Stage5WallClimbEndFrame = 1000;

struct SeaSerpentMotion {
    int segmentCount;
    float progress;
    float direction;
    float sideOriginX;
    float railOriginX;
    float railOriginZ;
    float railDirection;
    float railTravel;
    float elevation;
    float travel;
    float segmentSpacing;
    float segmentDelay;
    float scale;
};

/**
 * @brief 現フレームのウミヘビ行動と胴体配置を取得する
 * @param frame ステージ開始からのフレーム数
 * @param motion 行動情報の格納先
 * @return ウミヘビが海面上にいる場合true
 */
bool GetSeaSerpentMotion(int frame, SeaSerpentMotion& motion) {
    constexpr int CycleFrames = 420;
    constexpr int Duration[] = {112, 138, 172};
    constexpr int Segments[] = {14, 16, 23};
    constexpr float Elevation[] = {8.2f, 6.4f, 12.0f};
    constexpr float Travel[] = {12.0f, 17.0f, 14.0f};
    constexpr float Spacing[] = {0.90f, 0.78f, 1.12f};
    constexpr float Delay[] = {0.055f, 0.042f, 0.050f};
    constexpr float Scale[] = {1.70f, 2.00f, 3.20f};
    static_assert(Segments[2] > Segments[0] && Elevation[2] > Elevation[0] && Scale[2] > Scale[0]);
    const int cycle = frame / CycleFrames;
    const int action = cycle % 3;
    const int cycleFrame = frame % CycleFrames;
    const int startFrame = 48 + (cycle * 97) % 170;
    if (cycleFrame < startFrame || cycleFrame >= startFrame + Duration[action]) return false;

    motion.segmentCount = Segments[action];
    motion.progress = static_cast<float>(cycleFrame - startFrame) / static_cast<float>(Duration[action] - 1);
    motion.direction = cycle % 2 == 0 ? 1.0f : -1.0f;
    motion.sideOriginX = -motion.direction * Travel[action] * 0.5f;
    motion.railOriginX = -8.0f + static_cast<float>((cycle * 29) % 17);
    motion.railDirection = cycle % 2 == 0 ? -1.0f : 1.0f;
    motion.railOriginZ = motion.railDirection < 0.0f ? 68.0f : 4.0f;
    motion.railTravel = 64.0f;
    motion.elevation = Elevation[action];
    motion.travel = Travel[action];
    motion.segmentSpacing = Spacing[action];
    motion.segmentDelay = Delay[action];
    motion.scale = Scale[action];
    return true;
}

/**
 * @brief スクロール座標をNDCの横幅へ循環させる
 * @param value 循環前のX座標
 * @return -1.0f以上1.0f未満のX座標
 */
float WrapNdcX(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) {
        wrapped += 2.0f;
    }
    return wrapped - 1.0f;
}

/**
 * @brief 距離を0以上の循環範囲へ収める
 * @param value 循環前の距離
 * @param length 循環範囲の長さ
 * @return 0以上length未満の距離
 */
float WrapDistance(float value, float length) {
    float wrapped = std::fmod(value, length);
    if (wrapped < 0.0f) {
        wrapped += length;
    }
    return wrapped;
}

/**
 * @brief ステージ2の空と夜空の星を画面全体へ描画する
 * @param renderer 描画先
 * @param nightBlend 昼から夜への補間率
 * @return なし
 */
void DrawDesertSky(Renderer& renderer, float nightBlend) {
    const ColorF skyColor {
        Math::Lerp(DesertDaySkyColor[0], DesertNightSkyColor[0], nightBlend),
        Math::Lerp(DesertDaySkyColor[1], DesertNightSkyColor[1], nightBlend),
        Math::Lerp(DesertDaySkyColor[2], DesertNightSkyColor[2], nightBlend), 1.0f
    };
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, skyColor);
}

/**
 * @brief 機体のローカル配置をY軸回転してワールド配置へ変換する
 */
Vector3 RotateYawOffset(float x, float y, float z, float yaw) {
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);
    return {x * c + z * s, y, -x * s + z * c};
}
}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"
#include "Stage1EnemySheet.h"
#include "Stage1EnemySheetEasy.h"
#include "Stage1EnemySheetHard.h"
#include "Stage1EnemySheetNormal.h"
#include "Stage1Story.h"

/**
 * @brief 指定難易度のステージ1敵出現シートを取得する
 * @param difficulty 取得する難易度
 * @return 難易度に対応するステージ1敵出現シート
 */
const SideScrollingShooter::Stage& SideScrollingShooter::Stage1EnemySheetInstance(DifficultyType difficulty) {
    static const Stage1EnemySheetEasy easyStage;
    static const Stage1EnemySheetNormal normalStage;
    static const Stage1EnemySheetHard hardStage;
    switch (difficulty) {
    case Hard: return hardStage;
    case Normal: return normalStage;
    default: return easyStage;
    }
}

const SideScrollingShooter::Stage& SideScrollingShooter::Stage2Instance() {
    static const Stage2 stage;
    return stage;
}

/**
 * @brief ステージ3の定義を取得する
 * @return ステージ3の定義
 */
const SideScrollingShooter::Stage& SideScrollingShooter::Stage3Instance() {
    static const Stage3 stage;
    return stage;
}

/**
 * @brief ステージ4の定義を取得する
 * @return ステージ4の定義
 */
const SideScrollingShooter::Stage& SideScrollingShooter::Stage4Instance() {
    static const Stage4 stage;
    return stage;
}

/**
 * @brief ステージ5の定義を取得する
 * @return ステージ5の定義
 */
const SideScrollingShooter::Stage& SideScrollingShooter::Stage5Instance() {
    static const Stage5 stage;
    return stage;
}

/**
 * @brief 指定番号のステージ定義を取得する
 * @param stageNumber 取得するステージ番号
 * @return 指定番号に対応するステージ定義
 */
const SideScrollingShooter::Stage& SideScrollingShooter::StageForNumber(int stageNumber, DifficultyType difficulty) {
    switch (stageNumber) {
    case 2: return Stage2Instance();
    case 3: return Stage3Instance();
    case 4: return Stage4Instance();
    case 5: return Stage5Instance();
    default: return Stage1EnemySheetInstance(difficulty);
    }
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::BasicEnemyBehaviorInstance() {
    static const BasicEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::HeavyEnemyBehaviorInstance() {
    static const HeavyEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::ArmoredEnemyBehaviorInstance() {
    static const ArmoredEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::BossEnemyBehaviorInstance() {
    static const BossEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::StraightShooterEnemyBehaviorInstance() {
    static const StraightShooterEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::CircleShooterEnemyBehaviorInstance() {
    static const CircleShooterEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::EnemyBehaviorForType(int type) {
    switch (type) {
    case 1:
        return HeavyEnemyBehaviorInstance();
    case 2:
        return BossEnemyBehaviorInstance();
    case 3:
        return StraightShooterEnemyBehaviorInstance();
    case 4:
        return ArmoredEnemyBehaviorInstance();
    case 5:
        return CircleShooterEnemyBehaviorInstance();
    default:
        return BasicEnemyBehaviorInstance();
    }
}

/**
 * @brief 指定難易度と機体タイプでゲームを初期化する
 * @param audio 効果音を再生するサービス
 * @param playerType 使用する自機タイプ
 * @param difficulty 使用する敵出現難易度
 * @return なし
 */
void SideScrollingShooter::Initialize(AudioService* audio, PlayerType playerType, DifficultyType difficulty) {
    m_audio = audio;
    m_playerType = playerType;
    m_difficulty = difficulty;
    Reset(true);
}

void SideScrollingShooter::Reset(bool resetRetryCounts) {
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    ResetStageGimmicks();
    m_stageNumber = 1;
    m_chapterNumber = 1;
    if (resetRetryCounts) m_chapterRetryCounts = {};
    m_chapterResult = {};
    m_chapterStartPower = 0.0f;
    m_chapterStartScore = 0;
    m_chapterStartKills = 0;
    m_chapterResultTimer = 0;
    m_power = 0.0f;
    m_stage = &StageForNumber(m_stageNumber, m_difficulty);
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    m_scroll = 0.0f;
    m_frame = 0;
    m_spawnCooldown = 35;
    m_shotCooldown = 0;
    m_specialShotCooldown = 0;
    m_invincible = 90;
    m_score = 0;
    m_kills = 0;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_clearTimer = 0;
    m_clear = false;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_viewToggleRequested = false;
    m_viewMode = ViewMode::Side2D;
    m_nextViewMode = ViewMode::Side2D;
    m_viewTransitionTimer = 0;
    m_viewTransitionProgress = 0.0f;
}

void SideScrollingShooter::ProcessInput() {
    m_moveLeft = Input::GetKey(KeyCode::LeftArrow) || Input::GetKey(KeyCode::A);
    m_moveRight = Input::GetKey(KeyCode::RightArrow) || Input::GetKey(KeyCode::D);
    m_moveUp = Input::GetKey(KeyCode::UpArrow) || Input::GetKey(KeyCode::W);
    m_moveDown = Input::GetKey(KeyCode::DownArrow) || Input::GetKey(KeyCode::S);
    m_fire = Input::GetKey(KeyCode::Z) || Input::GetKey(KeyCode::Space);
    m_viewToggleRequested = Input::GetKeyDown(KeyCode::X);

    if (m_clear && Input::GetKeyDown(KeyCode::R)) {
        Reset(false);
    }
}

void SideScrollingShooter::Tick() {
    TickViewTransition();

    // HUD用HPは実HPへ追従させ、ダメージ時の減少を視認できるようにする
    if (m_displayBossHp > static_cast<float>(m_bossHp)) {
        const float difference = m_displayBossHp - static_cast<float>(m_bossHp);
        m_displayBossHp = (std::max)(static_cast<float>(m_bossHp),
            m_displayBossHp - (std::max)(1.0f, difference * 0.16f));
    } else {
        m_displayBossHp = static_cast<float>(m_bossHp);
    }
    if (m_viewTransitionTimer > 0) {
        return;
    }
    if (m_clear) {
        TickExplosions();
        TickDebris();
        if (m_stageNumber < 5 && --m_clearTimer <= 0) StartNextStage();
        return;
    }
    if (m_restartTimer > 0) {
        --m_restartTimer;
        // リスタート表示中は距離とチャプター進行を止め、画面上の弾と演出だけを更新する
        TickPlayer();
        TickShots();
        TickExplosions();
        TickItems();
        return;
    }
    if (m_chapterResultActive) {
        TickChapterResult();
        if (m_chapterResultActive) {
            // 戦闘進行は止めたまま、画面上の弾・破壊演出・アイテムを動かす
            TickPlayer();
            TickShots();
            TickExplosions();
            TickDebris();
            TickItems();
        }
        return;
    }
    if (m_bossBattlePending) {
        m_bossBattlePending = false;
        StartBossBattle();
        return;
    }
    if (m_bossStoryActive) {
        TickBossStory();
        return;
    }

    ++m_frame;
    if (!m_bossBattle) {
        m_scroll += 0.008f;
    }
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_specialShotCooldown = (std::max)(0, m_specialShotCooldown - 1);
    m_invincible = (std::max)(0, m_invincible - 1);
    if (!m_bossBattle && !m_chapterResultActive && m_frame >= m_stage->ChapterEndFrame(m_chapterNumber)) {
        FinishChapter();
    }

    TickPlayer();
    TickStageGimmicks();

    // ステージ固有の巨大障害物は無敵時間を無視して自機を破壊する
    if ((m_stageNumber == 1 && HitsStage1Meteor(m_playerX, m_playerY, PlayerRailZ, 0.055f)) ||
        (m_stageNumber == 2 && HitsDesertBoneArch(m_playerX, m_playerY, PlayerRailZ, 0.055f)) ||
        (m_stageNumber == 3 && HitsOceanSeaSerpent(m_playerX, m_playerY, PlayerRailZ, 0.055f))) {
        DamagePlayer();
        return;
    }

    bool firedPlayerShot = false;
    if (m_fire && m_shotCooldown == 0) {
        SpawnShot(m_playerX + (IsRailGameplayActive() ? 0.0f : 0.12f), m_playerY,
            IsRailGameplayActive() ? 0.0f : 0.045f, 0.0f, false, -1.0f, -1.0f, 1 + PowerLevel());
        m_shotCooldown = (std::max)(3, 7 - PowerLevel());
        PlayShotSound();
    }
    if (m_fire && m_specialShotCooldown == 0) {
        /** @brief 選択中の機体タイプに対応する特殊弾を発射する */
        FireSpecialShots();
        const auto& config = PlayerShotConfigs[static_cast<size_t>(m_playerType)];
        m_specialShotCooldown = config.fireIntervalFrames;
        firedPlayerShot = true;
    }
    if (firedPlayerShot) PlayShotSound();

    Stage::EnemySpawnRule spawn;
    if (!m_bossBattle && !m_chapterResultActive &&
        m_stage->TrySelectEnemySpawn(m_frame, spawn, m_chapterNumber)) {
        SpawnEnemy(spawn.enemyType, spawn.sideX, spawn.railX, spawn.y, spawn.railZ);
    }

    TickEnemies();
    TickShots();
    TickExplosions();
    TickDebris();
    TickItems();
}

void SideScrollingShooter::TickPlayer() {
    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    /** @brief 2D画面ではHUDを除くプレイ領域全体を移動可能にする */
    const float minX = IsRailGameplayActive() ? -1.2f : Side2DPlayerMinX;
    const float maxX = IsRailGameplayActive() ? 1.2f : Side2DPlayerMaxX;
    const float minY = IsRailGameplayActive() ? -0.9f : Side2DPlayerMinY;
    const float maxY = IsRailGameplayActive() ? 0.9f : Side2DPlayerMaxY;
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f, minX, maxX);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f, minY, maxY);
}

void SideScrollingShooter::TickEnemies() {
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        ++enemy.age;
        if (enemy.behavior == nullptr) {
            enemy.behavior = &EnemyBehaviorForType(enemy.type);
        }
        if (enemy.shotInterval <= 0) {
            enemy.shotInterval = enemy.behavior->AimedShotInterval();
        }
        enemy.behavior->Tick(*this, enemy);

        // 巨大障害物へ接触した通常敵はスコアやアイテムを発生させず、その場で破壊する。ボス(type 2)は対象外
        const float boneHitRadius = IsRailGameplayActive() ? enemy.behavior->CollisionRadius3D(enemy) / WorldXScale :
            enemy.behavior->CollisionRadius(enemy);
        if (enemy.type != 2 && ((m_stageNumber == 1 && HitsStage1Meteor(enemy.x, enemy.y, enemy.z, boneHitRadius)) ||
            (m_stageNumber == 2 && HitsDesertBoneArch(enemy.x, enemy.y, enemy.z, boneHitRadius)) ||
            (m_stageNumber == 3 && HitsOceanSeaSerpent(enemy.x, enemy.y, enemy.z, boneHitRadius)))) {
            SpawnExplosion(enemy.x, enemy.y, enemy.z);
            SpawnEnemyDebris(enemy);
            enemy.active = false;
            continue;
        }

        const int aimedShotInterval = enemy.shotInterval;
        if (aimedShotInterval > 0 && enemy.age % aimedShotInterval == 0) {
            const float dxToPlayer = m_playerX - enemy.x;
            const float dyToPlayer = m_playerY - enemy.y;
            const float length = std::sqrt(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);
            if (length > 0.001f) {
                const float shotSpeed = enemy.behavior->AimedShotSpeed();
                SpawnShot(enemy.x - 0.06f, enemy.y, dxToPlayer / length * shotSpeed,
                    dyToPlayer / length * shotSpeed, true, enemy.z, enemy.behavior->RailAimedShotSpeed());
            }
        }

        // ボスは通常・特殊フェーズごとの間隔で、未破壊の各部位から弾幕を発射する
        if (enemy.type == 2 && enemy.age % (enemy.bossPhase % 2 == 0 ? 120 : 84) == 0) {
            FireBossPartBarrage(enemy);
        }

        if (enemy.type != 2 && !IsRailGameplayActive() && enemy.x < -1.08f) enemy.active = false;
        if (enemy.type == 3 && !IsRailGameplayActive() && enemy.z < 16.0f) enemy.active = false;
        if (enemy.type != 2 && IsRailGameplayActive() && enemy.z < 2.0f) enemy.active = false;
        const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
        const bool playerHit = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.42f,
                ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.behavior->CollisionRadius3D(enemy)) :
            Hit(m_playerX, m_playerY, 0.055f, enemy.x, enemy.y, enemyRadius);
        if (enemy.active && m_invincible == 0 && playerHit) {
            if (enemy.type != 2) {
                SpawnExplosion(enemy.x, enemy.y, enemy.z);
                SpawnEnemyDebris(enemy);
                enemy.active = false;
            }
            DamagePlayer();
            return;
        }
    }
}

/** @brief ステージ固有の破壊可能ギミックを更新する */
void SideScrollingShooter::TickStageGimmicks() {
    if (m_stageNumber != 1 || m_meteorDestroyed) return;

    // 被弾後は縮小率に合わせて飛来速度も落とす
    m_meteorTravel += 0.16f * m_meteorScale;
    if (m_meteorTravel >= 72.0f) m_meteorTravel -= 72.0f;
}

/** @brief ステージ固有の破壊可能ギミックを初期状態へ戻す */
void SideScrollingShooter::ResetStageGimmicks() {
    m_meteorTravel = 0.0f;
    m_meteorScale = 1.0f;
    m_meteorDestroyed = false;
    m_boneArchHp = BoneArchMaxHp;
    m_boneArchDestroyed = false;
}

/**
 * @brief 自機弾がステージ固有ギミックへ命中した場合にダメージを適用する
 * @param shot 命中判定対象の自機弾
 * @return ギミックへ命中した場合true
 */
bool SideScrollingShooter::TryDamageStageGimmick(Shot& shot) {
    if (shot.enemy) return false;
    if (m_stageNumber == 1 && !m_meteorDestroyed &&
        HitsStage1Meteor(shot.x, shot.y, shot.z, shot.hitRadius)) {
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        m_meteorScale = (std::max)(0.50f, m_meteorScale - 0.10f * static_cast<float>(shot.damage));
        if (m_meteorScale <= 0.50f) m_meteorDestroyed = true;
        PlayHitSound();
        return true;
    }
    if (m_stageNumber == 2 && !m_boneArchDestroyed &&
        HitsDesertBoneArch(shot.x, shot.y, shot.z, shot.hitRadius)) {
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        m_boneArchHp -= shot.damage;
        if (m_boneArchHp <= 0) m_boneArchDestroyed = true;
        PlayHitSound();
        return true;
    }
    return false;
}

void SideScrollingShooter::TickShots() {
    for (auto& shot : m_shots) {
        if (!shot.active) continue;

        /** @brief 追尾弾を最寄りの前方敵へ旋回させる */
        if (!shot.enemy && shot.special && shot.playerType == Homing) {
            UpdateHomingShot(shot);
        }

        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
        if (!IsRailGameplayActive()) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        if (!IsRailGameplayActive() &&
            (shot.x < Side2DPlayerMinX - Side2DShotCullMargin ||
                shot.x > Side2DPlayerMaxX + Side2DShotCullMargin ||
                shot.y < Side2DPlayerMinY - Side2DShotCullMargin ||
                shot.y > Side2DPlayerMaxY + Side2DShotCullMargin)) {
            shot.active = false;
            continue;
        }
        // 端から出る円形弾幕が生成直後に欠けないよう、弾のY消滅範囲だけ少し広げる
        if (IsRailGameplayActive() && (shot.z < 0.0f || shot.z > 72.0f ||
            std::abs(shot.x) > 1.2f || std::abs(shot.y) > 1.24f)) {
            shot.active = false;
            continue;
        }

        if (shot.enemy) {
            const bool playerHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.38f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.28f) :
                Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y, 0.022f);
            const bool grazed = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.80f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.28f) :
                Hit(m_playerX, m_playerY, 0.140f, shot.x, shot.y, 0.022f);
            if (!playerHit && grazed && !shot.grazed) {
                shot.grazed = true;
                ++m_chapterResult.grazeCount;
            }
            if (m_invincible == 0 && playerHit) {
                shot.active = false;
                DamagePlayer();
                return;
            }
            continue;
        }

        if (TryDamageStageGimmick(shot)) continue;

        for (auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            if (enemy.behavior == nullptr) {
                enemy.behavior = &EnemyBehaviorForType(enemy.type);
            }
            BossPart hitPart = BossNose;
            if (enemy.type == 2 && TryHitBossPart(shot, enemy, hitPart)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                if (!shot.piercing) shot.active = false;
                enemy.bossPartHp[hitPart] -= shot.damage;
                if (enemy.bossPartHp[hitPart] <= 0) {
                    enemy.bossPartHp[hitPart] = 0;
                    SpawnEnemyDebris(enemy, hitPart);
                    // 部位破壊の報酬として、本体へ大ダメージを与える
                    const bool bossDefeated = DamageBoss(enemy, 120);
                    PlayHitSound();
                    if (bossDefeated) DefeatBoss(enemy);
                }
                if (enemy.hp <= 0) {
                    DefeatBoss(enemy);
                } else {
                    m_bossHp = enemy.hp;
                }
                break;
            }
            const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
            const bool enemyHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.behavior->ShotHitRadius3D(enemy)) :
                Hit(shot.x, shot.y, shot.hitRadius, enemy.x, enemy.y, enemyRadius);
            if (!enemyHit) continue;
            SpawnExplosion(shot.x, shot.y, shot.z);
            if (!shot.piercing) shot.active = false;
            if (enemy.type == 2) DamageBoss(enemy, shot.damage);
            if (enemy.type != 2) enemy.hp -= shot.damage;
            if (enemy.hp <= 0) {
                if (enemy.type == 2) {
                    DefeatBoss(enemy);
                } else {
                    SpawnEnemyDebris(enemy);
                    enemy.active = false;
                    ++m_kills;
                    const int score = enemy.behavior->Score(enemy);
                    ++m_chapterResult.enemyDefeatCount;
                    SpawnPowerItem(enemy.x + 0.10f, enemy.y, enemy.z, 0.25f);
                    SpawnScoreItem(enemy.x - 0.10f, enemy.y, enemy.z, score);
                }
                PlayHitSound();
            } else if (enemy.type == 2) {
                m_bossHp = enemy.hp;
            }
            break;
        }
    }
}

/**
 * @brief 取得アイテムを更新して自機との取得判定を行う
 */
void SideScrollingShooter::TickItems() {
    for (auto& item : m_items) {
        if (!item.active) continue;

        // 2Dでは左、3Dでは手前へスクロールする
        if (IsRailGameplayActive()) {
            item.z -= 0.28f;
        } else {
            item.x -= 0.012f;
            item.z = ToRailZFromSideX(item.x);
        }

        // 画面外へ出たアイテムを破棄する
        if ((!IsRailGameplayActive() &&
                (item.x < Side2DPlayerMinX || item.x > Side2DPlayerMaxX ||
                    item.y < Side2DPlayerMinY || item.y > Side2DPlayerMaxY)) ||
            (IsRailGameplayActive() &&
                (item.z < 0.0f || item.z > 72.0f || std::abs(item.x) > 1.2f || std::abs(item.y) > 1.24f))) {
            item.active = false;
            continue;
        }

        // 自機が近づいたアイテムだけを強く追尾させる
        const float dx = m_playerX - item.x;
        const float dy = m_playerY - item.y;
        const bool followsPlayer = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 3.5f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.0f) :
            Hit(m_playerX, m_playerY, 0.45f, item.x, item.y, 0.0f);
        if (followsPlayer) {
            item.x += dx * 0.45f;
            item.y += dy * 0.45f;
            if (IsRailGameplayActive()) {
                item.z += (PlayerRailZ - item.z) * 0.45f;
            }
        }
        const bool collected = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.52f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.38f) :
            Hit(m_playerX, m_playerY, 0.075f, item.x, item.y, 0.045f);
        if (!collected) continue;

        if (item.type == ItemType::Power) {
            m_power = (std::min)(MaxPower, m_power + item.power);
        } else {
            m_chapterResult.score += item.score;
            m_score += item.score;
        }
        item.active = false;
    }
}

/**
 * @brief チャプター終了演出を更新する
 */
void SideScrollingShooter::TickChapterResult() {
    if (++m_chapterResultTimer < ChapterResultDisplayFrames) return;

    m_chapterResultActive = false;
    // 次チャプターへ敵弾を持ち越さないよう、結果表示後に消去する
    for (auto& shot : m_shots) {
        if (shot.enemy) shot.active = false;
    }
    if (m_chapterNumber == 3) {
        m_bossBattlePending = true;
        return;
    }

    ++m_chapterNumber;
    m_chapterResult = {};
    m_chapterStartPower = m_power;
    m_chapterStartScore = m_score;
    m_chapterStartKills = m_kills;
}

/**
 * @brief 現在のチャプター戦績を確定して表示を開始する
 */
void SideScrollingShooter::FinishChapter() {
    m_chapterResult.retryCount = m_chapterRetryCounts[m_chapterNumber - 1];
    m_chapterResult.totalScore = CalculateChapterTotalScore(m_chapterResult);
    m_chapterResultTimer = 0;
    m_chapterResultActive = true;

    // 次のチャプターの出現へ戦闘中の敵が持ち越されないようにする
    for (auto& enemy : m_enemies) {
        if (enemy.type != 2) enemy.active = false;
    }
}

void SideScrollingShooter::TickViewTransition() {
    if (m_viewTransitionTimer > 0) {
        --m_viewTransitionTimer;
        m_viewTransitionProgress = SmoothStep(
            1.0f - static_cast<float>(m_viewTransitionTimer) / ViewTransitionFrames);
        if (m_viewTransitionTimer == 0) {
            m_viewMode = m_nextViewMode;
            m_viewTransitionProgress = 0.0f;
        }
        return;
    }

    m_viewTransitionProgress = 0.0f;
    if (!m_viewToggleRequested) {
        return;
    }

    // 遷移開始時に座標系を切り替え、遷移中も切替先のゲームルールで更新する
    m_nextViewMode = m_viewMode == ViewMode::Side2D ? ViewMode::Rail3D : ViewMode::Side2D;
    m_viewTransitionTimer = ViewTransitionFrames;
    if (m_nextViewMode == ViewMode::Rail3D) {
        InitializeRailObjects();
    } else {
        InitializeSideObjects();
    }
}

void SideScrollingShooter::InitializeRailObjects() {
    // 自機以外は2D横位置を3D奥行きへ移して、レール用の横位置を別に持つ
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        enemy.transitionSideX = enemy.x;
        enemy.transitionSideY = enemy.y;
        if (enemy.z <= 0.0f) {
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        enemy.baseX = enemy.type == 2 ? 0.0f : (std::clamp)(enemy.baseX, -0.76f, 0.76f);
        enemy.x = enemy.baseX;
    }
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        shot.transitionSideX = shot.x;
        shot.transitionSideY = shot.y;
        const float sideVx = shot.vx;
        if (shot.z <= 0.0f) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        /** @brief 2D横移動をレール奥行きの移動量へ変換する */
        if (shot.enemy) {
            shot.vz = sideVx * 18.0f;
        }
        else {
            shot.vx = 0.0f;
            shot.vy = 0.0f;
            shot.vz = 1.45f;
        }

    }
}

/**
 * @brief 3Dレール上のオブジェクトを2D横スクロール座標系へ変換する
 */
void SideScrollingShooter::InitializeSideObjects() {
    // 自機以外を2D座標系へ戻し、遷移中も横スクロール用の更新を継続する
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        enemy.transitionSideX = enemy.x;
        enemy.transitionSideY = enemy.y;
        enemy.x = ToSideXFromRailZ(enemy.z);
        enemy.baseX = enemy.x;
        enemy.z = ToRailZFromSideX(enemy.x);
    }
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        // レール奥行きの移動量を2D画面の横移動量へ変換する
        shot.transitionSideX = shot.x;
        shot.transitionSideY = shot.y;
        shot.x = ToSideXFromRailZ(shot.z);
        shot.z = ToRailZFromSideX(shot.x);
        shot.vx = shot.vz / 18.0f;
        shot.vz = 0.0f;
    }
}

void SideScrollingShooter::SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ) {
    for (auto& enemy : m_enemies) {
        if (enemy.active) continue;
        enemy.active = true;
        m_stage->ConfigureEnemy(*this, enemy, enemyType, m_frame, m_kills, IsRailGameplayActive());
        enemy.baseX = IsRailGameplayActive() ? railX : sideX;
        enemy.x = enemy.baseX;
        enemy.baseY = y;
        enemy.y = y;
        enemy.z = IsRailGameplayActive() ? railZ : ToRailZFromSideX(sideX);
        ++m_chapterResult.enemySpawnCount;
        return;
    }
}

void SideScrollingShooter::StartBossBattle() {
    m_bossBattle = true;
    m_bossHp = m_stage->BossMaxHp();
    m_displayBossHp = static_cast<float>(m_bossHp);
    m_bossStoryLine = 0;
    m_bossStoryActive = true;

    // 通常敵と敵弾を消去してボス戦へ切り替える
    for (auto& enemy : m_enemies) {
        enemy.active = false;
    }
    for (auto& shot : m_shots) {
        if (shot.enemy) shot.active = false;
    }

    Enemy& boss = m_enemies[0];
    m_stage->ConfigureBoss(boss, IsRailGameplayActive());
    // 機首、主翼、エンジンは本体とは別HPで管理する
    boss.bossPartHp = { 120, 180, 180, 150, 150 };
    boss.bossPhase = BossNormalPhase1;
    m_invincible = (std::max)(m_invincible, 60);

    if (m_audio) {
        m_audio->PlayMMLSE("t180 o4 l16 v12 c g > c");
    }
}

/**
 * @brief 砂漠を横切る骨アーチへ指定球が接触したか判定する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return 骨アーチに接触している場合true
 */
bool SideScrollingShooter::HitsDesertBoneArch(float x, float y, float z, float radius) const {
    constexpr int BoneCount = 13;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    if (m_boneArchDestroyed) return false;
    const float phase = std::fmod(m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) / static_cast<float>(BoneCount - 1);
        if (IsRailGameplayActive()) {
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                std::cos(angle) * RailRadius, RailCenterY + std::sin(angle) * RailRadius, railZ, 1.35f)) {
                return true;
            }
        } else if (Hit(x, y, radius, sideCenterX,
            -1.30f + static_cast<float>(i) * 0.24f, 0.32f)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief ステージ1を横切る隕石へ指定球が接触したか判定する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return 隕石に接触している場合true
 */
bool SideScrollingShooter::HitsStage1Meteor(float x, float y, float z, float radius) const {
    if (m_meteorDestroyed) return false;
    const float sideX = 1.85f - std::fmod(m_meteorTravel * 0.0325f, 4.40f);
    const float sideY = 0.55f + std::sin(m_meteorTravel * 0.105f) * 0.34f;
    if (IsRailGameplayActive()) {
        const float railX = std::sin(m_meteorTravel * 0.090f) * 7.0f;
        const float railY = 0.80f + std::sin(m_meteorTravel * 0.135f) * 2.0f;
        return Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
            railX, railY, 72.0f - m_meteorTravel, 2.50f * m_meteorScale);
    }
    return Hit(x, y, radius, sideX, sideY, 0.34f * m_meteorScale);
}

/**
 * @brief 海面を横断するウミヘビへ指定球が接触したか判定する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return ウミヘビに接触している場合true
 */
bool SideScrollingShooter::HitsOceanSeaSerpent(float x, float y, float z, float radius) const {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(m_frame, motion)) return false;

    // 描画と同じ胴体球を使い、横視点とレール視点の双方で即死障害物として判定する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const float bodyProgress = (std::max)(0.0f, motion.progress - static_cast<float>(i) * motion.segmentDelay);
        const float elevation = std::sin(Math::HalfPi * 2.0f * bodyProgress) *
            (motion.elevation - static_cast<float>(i) * 0.16f);
        const float sideX = motion.sideOriginX + motion.direction *
            (bodyProgress * motion.travel - static_cast<float>(i) * motion.segmentSpacing);
        const float railX = motion.railOriginX + std::sin(bodyProgress * 4.0f + static_cast<float>(i) * 0.6f) * 6.0f;
        const float railZ = motion.railOriginZ + motion.railDirection *
            (bodyProgress * motion.railTravel - static_cast<float>(i) * 2.6f);
        const float segmentScale = motion.scale * (i == 0 ? 1.25f : 1.0f);
        if (IsRailGameplayActive()) {
            const float railHeight = 2.50f * segmentScale;
            const float visibleHeight = Math::Clamp01(elevation / railHeight) * railHeight;
            if (visibleHeight <= 0.0f) continue;
            const float visibleScale = segmentScale * std::sqrt(visibleHeight / railHeight);
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                railX, -3.65f + visibleHeight * 0.5f, railZ,
                1.25f * visibleScale)) {
                return true;
            }
        } else {
            // 2D描画は自機をワールド座標へ拡大しているため、同じ座標系の胴体楕円で判定する
            const float sideHeight = 1.35f * segmentScale;
            const float visibleHeight = Math::Clamp01(elevation / sideHeight) * sideHeight;
            if (visibleHeight <= 0.0f) continue;
            const float visibleWidth = 1.18f * segmentScale * std::sqrt(visibleHeight / sideHeight);
            const float hitWidth = visibleWidth * 0.5f + radius * WorldXScale;
            const float hitHeight = visibleHeight * 0.5f + radius * WorldYScale;
            const float dx = (ToWorldX(x) - sideX) / hitWidth;
            const float dy = (ToWorldY(y) - (-6.0f + visibleHeight * 0.5f)) / hitHeight;
            if (dx * dx + dy * dy <= 1.0f) return true;
        }
    }
    return false;
}

/**
 * @brief 全ステージをクリア済みか取得する
 * @return 最終ステージのクリア演出中ならtrue
 */
bool SideScrollingShooter::IsAllStagesCleared() const {
    return m_clear && m_stageNumber == 5;
}

/**
 * @brief 現在の合計スコアを取得する
 * @return 現在の合計スコア
 */
int SideScrollingShooter::Score() const {
    return m_score;
}

/** @brief 生存中の爆発エフェクトを更新する */
void SideScrollingShooter::TickExplosions() {
    for (auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        if (++explosion.age >= ExplosionLifetimeFrames) explosion.active = false;
    }
}

/** @brief 飛散中の機体部品を更新する */
void SideScrollingShooter::TickDebris() {
    for (auto& debris : m_debris) {
        if (!debris.active) continue;
        debris.x += debris.vx;
        debris.y += debris.vy;
        debris.z += debris.vz;
        if (m_stage->HasDebrisGravity()) debris.vy -= 0.006f;
        debris.yaw += debris.spin;
        if (++debris.age >= DebrisLifetimeFrames) debris.active = false;
    }
}

void SideScrollingShooter::FireBossPartBarrage(const Enemy& boss) {
    constexpr float ModelScale = 0.14f;
    constexpr float PartX[] = { 0.0f, 17.0f, -17.0f, 6.0f, -6.0f };
    constexpr float PartY[] = { 3.0f, 2.0f, 2.0f, -6.0f, -6.0f };
    constexpr float PartZ[] = { -17.5f, 0.0f, 0.0f, 13.0f, 13.0f };
    const bool railMode = IsRailGameplayActive();

    // 未破壊部位ごとに、ステージ定義の通常または特殊弾幕を発射する
    for (int part = 0; part < BossPartCount; ++part) {
        if (boss.bossPartHp[part] <= 0) continue;
        const int bulletCount = m_stage->BossPartBulletCount(
            static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), railMode);
        for (int index = 0; index < bulletCount; ++index) {
            const Stage::BossBullet bullet = m_stage->GetBossPartBullet(
                static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), index, railMode);
            const float x = railMode ? boss.x + PartX[part] * ModelScale / WorldXScale :
                boss.x + PartZ[part] * ModelScale / WorldXScale;
            const float y = boss.y + PartY[part] * ModelScale / WorldYScale;
            const float z = boss.z + PartZ[part] * ModelScale;
            SpawnShot(x + bullet.offsetX, y + bullet.offsetY, bullet.vx, bullet.vy, true,
                z, boss.behavior->RailAimedShotSpeed());
        }
    }
}

bool SideScrollingShooter::DamageBoss(Enemy& boss, int damage) {
    boss.hp -= damage;
    m_bossHp = (std::max)(0, boss.hp);
    const int nextPhase = BossPhaseForHp(boss.hp, boss.maxHp);
    if (nextPhase != boss.bossPhase) {
        boss.bossPhase = nextPhase;
        // フェーズ切り替え時は画面上の敵弾を消して次の弾幕を読みやすくする
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
    }
    return boss.hp <= 0;
}

void SideScrollingShooter::DefeatBoss(Enemy& boss) {
    if (!boss.active) return;
    SpawnExplosion(boss.x, boss.y, boss.z);
    SpawnEnemyDebris(boss);
    boss.active = false;
    SpawnPowerItem(boss.x, boss.y, boss.z, 1.00f);
    m_bossHp = 0;
    m_score += 5000;
    m_clear = true;
    m_clearTimer = 120;
}

/** @brief ボス戦前会話を進行する */
void SideScrollingShooter::TickBossStory() {
    const BossStory story = BossStories::ForStage(m_stageNumber);
    if (m_bossStoryLine >= story.lineCount) {
        m_bossStoryActive = false;
        return;
    }

    // Zキー入力時だけ次の台詞へ進める
    if (!Input::GetKeyDown(KeyCode::Z)) {
        return;
    }

    ++m_bossStoryLine;
    m_bossStoryActive = m_bossStoryLine < story.lineCount;
}

/** @brief 次のステージの戦闘状態を初期化する */
void SideScrollingShooter::StartNextStage() {
    ++m_stageNumber;
    if (m_stageNumber > 5) {
        m_stageNumber = 5;
        return;
    }

    // スコアと残機を維持したまま、次のステージ用に戦闘オブジェクトを初期化する
    m_shots = {};
    m_enemies = {};
    m_explosions = {};
    m_debris = {};
    ResetStageGimmicks();
    m_stage = &StageForNumber(m_stageNumber, m_difficulty);
    m_chapterNumber = 1;
    m_chapterRetryCounts = {};
    m_chapterResult = {};
    m_chapterStartPower = m_power;
    m_chapterStartScore = m_score;
    m_chapterStartKills = m_kills;
    m_chapterResultTimer = 0;
    m_scroll = 0.0f;
    m_frame = 0;
    m_spawnCooldown = 35;
    m_shotCooldown = 0;
    m_specialShotCooldown = 0;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_clear = false;
    m_clearTimer = 0;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_invincible = (std::max)(m_invincible, 90);
}

void SideScrollingShooter::SpawnShot(float x, float y, float vx, float vy, bool enemy,
    float z, float railSpeed, int damage) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = IsRailGameplayActive() ? (z >= 0.0f ? z : PlayerRailZ + 2.0f) :
            ToRailZFromSideX(x);
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.vz = 0.0f;
        shot.damage = damage;
        if (IsRailGameplayActive()) {
            if (enemy) {
                const float targetX = m_playerX + vx * 12.0f;
                const float targetY = m_playerY + vy * 12.0f;
                const float dx = ToWorldX(targetX) - ToWorldX(x);
                const float dy = ToWorldY(targetY) - ToWorldY(y);
                const float dz = PlayerRailZ - shot.z;
                const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                const float EnemyShotSpeed = railSpeed >= 0.0f ? railSpeed : 0.62f;
                shot.vx = FromWorldX(dx / length * EnemyShotSpeed);
                shot.vy = FromWorldY(dy / length * EnemyShotSpeed);
                shot.vz = dz / length * EnemyShotSpeed;
            } else {
                shot.vx = 0.0f;
                shot.vy = 0.0f;
                shot.vz = 1.45f;
            }
        }
        shot.enemy = enemy;
        shot.active = true;
        return;
    }
}

/**
 * @brief 指定座標にPowerアイテムを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @param value 取得時に加算するPower
 */
void SideScrollingShooter::SpawnPowerItem(float x, float y, float z, float value) {
    for (auto& item : m_items) {
        if (item.active) continue;
        item = {};
        item.x = x;
        item.y = y;
        item.z = IsRailGameplayActive() ? z : ToRailZFromSideX(x);
        item.power = value;
        item.type = ItemType::Power;
        item.active = true;
        return;
    }
}

/**
 * @brief 指定座標にScoreアイテムを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @param value 取得時に加算するScore
 */
void SideScrollingShooter::SpawnScoreItem(float x, float y, float z, int value) {
    for (auto& item : m_items) {
        if (item.active) continue;
        item = {};
        item.x = x;
        item.y = y;
        item.z = IsRailGameplayActive() ? z : ToRailZFromSideX(x);
        item.score = value;
        item.type = ItemType::Score;
        item.active = true;
        return;
    }
}

void SideScrollingShooter::SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.vz = vz;
        shot.enemy = enemy;
        shot.active = true;
        return;
    }
}

/** @brief 選択中の機体タイプに対応する特殊弾を生成する */
void SideScrollingShooter::FireSpecialShots() {
    const auto& config = PlayerShotConfigs[static_cast<size_t>(m_playerType)];
    const int powerLevel = PowerLevel();
    const int projectileCount = m_playerType == Spread ? config.projectileCount + powerLevel * 2 :
        config.projectileCount + powerLevel;
    const int damage = m_playerType == Piercing ? config.damage + powerLevel : config.damage;
    constexpr float DegreesToRadians = 3.1415926535f / 180.0f;

    /** @brief 弾数に応じて左右対称の角度と発射位置を求める */
    for (int i = 0; i < projectileCount; ++i) {
        const float centeredIndex = static_cast<float>(i) -
            static_cast<float>(projectileCount - 1) * 0.5f;
        const float angleStep = projectileCount > 1
            ? config.spreadAngleDegrees / static_cast<float>(projectileCount - 1)
            : 0.0f;
        const float angle = centeredIndex * angleStep * DegreesToRadians;
        const bool railGameplay = IsRailGameplayActive();
        const float spawnY = railGameplay ? m_playerY : m_playerY + centeredIndex * config.spawnOffsetY;
        const float railSpawnOffsetX = config.spawnOffsetY > 0.0f ? config.spawnOffsetY : 0.05f;

        /** @brief 空きスロットへ機体タイプ固有の属性を設定する */
        for (auto& shot : m_shots) {
            if (shot.active) continue;
            shot = {};
            // 3Dレールでは翼の左右から、2Dでは従来どおり機首の上下から発射する
            shot.x = railGameplay ? m_playerX + centeredIndex * railSpawnOffsetX :
                m_playerX + config.spawnOffsetX;
            shot.y = spawnY;
            shot.z = railGameplay ? PlayerRailZ + 2.0f : ToRailZFromSideX(shot.x);
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            if (railGameplay) {
                /** @brief 3Dレールでは特殊弾を奥行き方向へ進ませ、拡散角を横移動へ適用する */
                shot.vx = std::sin(angle) * config.speed;
                shot.vy = 0.0f;
                shot.vz = 1.45f;
            } else {
                /** @brief 2Dでは従来どおり画面右方向を基準に拡散する */
                shot.vx = std::cos(angle) * config.speed;
                shot.vy = std::sin(angle) * config.speed;
                shot.vz = 0.0f;
            }
            shot.hitRadius = config.hitRadius;
            shot.damage = damage;
            shot.playerType = m_playerType;
            shot.special = true;
            shot.piercing = config.piercing;
            shot.active = true;
            break;
        }
    }
}

/** @brief 追尾弾の進行方向を最寄りの前方敵へ近づける */
void SideScrollingShooter::UpdateHomingShot(Shot& shot) {
    if (IsRailGameplayActive()) {
        const Enemy* target = nullptr;
        float nearestDistanceSquared = 10000.0f;

        // 3Dレールでは奥行き方向の前方にいる最寄りの敵を追尾する
        for (const auto& enemy : m_enemies) {
            if (!enemy.active || enemy.z <= shot.z) continue;
            const float dx = ToWorldX(enemy.x - shot.x);
            const float dy = ToWorldY(enemy.y - shot.y);
            const float dz = enemy.z - shot.z;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared < nearestDistanceSquared) {
                nearestDistanceSquared = distanceSquared;
                target = &enemy;
            }
        }
        if (target == nullptr || nearestDistanceSquared <= 0.000001f) return;

        // ワールド空間で旋回量を補間して、レール弾速を維持する
        const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
        const float inverseDistance = 1.0f / std::sqrt(nearestDistanceSquared);
        const float speed = 1.45f;
        float vx = ToWorldX(shot.vx);
        float vy = ToWorldY(shot.vy);
        float vz = shot.vz;
        vx += ((ToWorldX(target->x - shot.x) * inverseDistance * speed) - vx) * config.homingStrength;
        vy += ((ToWorldY(target->y - shot.y) * inverseDistance * speed) - vy) * config.homingStrength;
        vz += (((target->z - shot.z) * inverseDistance * speed) - vz) * config.homingStrength;
        const float velocityLength = std::sqrt(vx * vx + vy * vy + vz * vz);
        if (velocityLength > 0.000001f) {
            shot.vx = FromWorldX(vx / velocityLength * speed);
            shot.vy = FromWorldY(vy / velocityLength * speed);
            shot.vz = vz / velocityLength * speed;
        }
        return;
    }

    const Enemy* target = nullptr;
    float nearestDistanceSquared = 100.0f;

    /** @brief 前方にいる最寄りの敵を追尾対象として検索する */
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.x <= shot.x) continue;
        const float dx = enemy.x - shot.x;
        const float dy = enemy.y - shot.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            target = &enemy;
        }
    }
    if (target == nullptr || nearestDistanceSquared <= 0.000001f) return;

    /** @brief 現在速度と目標方向を補間して速度を一定に保つ */
    const auto& config = PlayerShotConfigs[static_cast<size_t>(Homing)];
    const float inverseDistance = 1.0f / std::sqrt(nearestDistanceSquared);
    const float desiredVx = (target->x - shot.x) * inverseDistance * config.speed;
    const float desiredVy = (target->y - shot.y) * inverseDistance * config.speed;
    shot.vx += (desiredVx - shot.vx) * config.homingStrength;
    shot.vy += (desiredVy - shot.vy) * config.homingStrength;
    const float velocityLength = std::sqrt(shot.vx * shot.vx + shot.vy * shot.vy);
    if (velocityLength > 0.000001f) {
        shot.vx = shot.vx / velocityLength * config.speed;
        shot.vy = shot.vy / velocityLength * config.speed;
    }
}

/**
 * @brief 被弾効果を再生して現在のチャプターをやり直す
 * @return なし
 */
void SideScrollingShooter::DamagePlayer() {
    PlayHitSound();
    RestartCurrentChapter();
}

/**
 * @brief 現在のチャプターを開始時状態へ戻す
 * @return なし
 */
void SideScrollingShooter::RestartCurrentChapter() {
    ++m_chapterRetryCounts[m_chapterNumber - 1];
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    ResetStageGimmicks();
    m_chapterResult = {};
    m_power = m_chapterStartPower;
    m_score = m_chapterStartScore;
    m_kills = m_chapterStartKills;
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    m_frame = m_stage->ChapterEndFrame(m_chapterNumber - 1);
    m_scroll = static_cast<float>(m_frame) * 0.008f;
    m_spawnCooldown = 35;
    m_shotCooldown = 0;
    m_specialShotCooldown = 0;
    m_invincible = 90;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_restartTimer = RestartDisplayFrames;
}

bool SideScrollingShooter::TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const {
    // 既存ボスモデルのローカル座標に対応する、破壊可能部位の中心と当たり判定半径
    constexpr float ModelScale = 0.14f;
    constexpr float PartX[] = { 0.0f, 17.0f, -17.0f, 6.0f, -6.0f };
    constexpr float PartY[] = { 3.0f, 2.0f, 2.0f, -6.0f, -6.0f };
    constexpr float PartZ[] = { -17.5f, 0.0f, 0.0f, 13.0f, 13.0f };
    constexpr float PartRadius[] = { 0.50f, 1.20f, 1.20f, 0.58f, 0.58f };

    for (int i = 0; i < BossPartCount; ++i) {
        if (boss.bossPartHp[i] <= 0) continue;
        if (IsRailGameplayActive()) {
            const float partX = ToWorldX(boss.x) + PartX[i] * ModelScale;
            const float partY = ToWorldY(boss.y) + PartY[i] * ModelScale;
            const float partZ = boss.z + PartZ[i] * ModelScale;
            if (!Hit3D(ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                partX, partY, partZ, PartRadius[i])) {
                continue;
            }
        } else {
            // 2D表示ではY軸回転済みモデルの奥行きを画面X座標へ投影する
            const float partX = boss.x + PartZ[i] * ModelScale / WorldXScale;
            const float partY = boss.y + PartY[i] * ModelScale / WorldYScale;
            if (!Hit(shot.x, shot.y, shot.hitRadius, partX, partY,
                PartRadius[i] / WorldXScale)) {
                continue;
            }
        }
        part = static_cast<BossPart>(i);
        return true;
    }
    return false;
}

void SideScrollingShooter::PlayShotSound() {
    if (m_audio) m_audio->PlayMMLSE("t240 o6 l32 v7 c>c");
}

void SideScrollingShooter::PlayHitSound() {
    if (m_audio) m_audio->PlayMMLSE("t180 o4 l32 v10 g e c");
}

bool SideScrollingShooter::Hit(float ax, float ay, float ar, float bx, float by, float br) {
    const float dx = ax - bx;
    const float dy = ay - by;
    const float radius = ar + br;
    return dx * dx + dy * dy <= radius * radius;
}

bool SideScrollingShooter::Hit3D(
    float ax, float ay, float az, float ar, float bx, float by, float bz, float br) {
    const float dx = ax - bx;
    const float dy = ay - by;
    const float dz = az - bz;
    const float radius = ar + br;
    return dx * dx + dy * dy + dz * dz <= radius * radius;
}

float SideScrollingShooter::SmoothStep(float value) {
    const float t = (std::clamp)(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float SideScrollingShooter::ToWorldX(float x) {
    return x * WorldXScale;
}

float SideScrollingShooter::ToWorldY(float y) {
    return y * WorldYScale;
}

float SideScrollingShooter::FromWorldX(float x) {
    return x / WorldXScale;
}

float SideScrollingShooter::FromWorldY(float y) {
    return y / WorldYScale;
}

/**
 * @brief 2Dモードの横位置を3Dレールの奥行きへ変換する
 */
float SideScrollingShooter::ToRailZFromSideX(float x) {
    return 18.0f + (std::clamp)(x + 1.0f, 0.0f, 2.0f) * 18.0f;
}

/**
 * @brief 3Dレールの奥行きを2Dモードの横位置へ変換する
 */
float SideScrollingShooter::ToSideXFromRailZ(float z) {
    const float sideX = ((z - 18.0f) / 18.0f) - 1.0f;
    return (std::clamp)(sideX, -1.05f, 1.16f);
}

/**
 * @brief 現在のPowerから弾強化段階を取得する
 * @return 0から4までの弾強化段階
 */
int SideScrollingShooter::PowerLevel() const {
    return static_cast<int>(m_power);
}

/**
 * @brief チャプターの総合スコアを算出する
 * @param result 集計済みチャプター戦績
 * @return 撃破スコア、グレイズ、せん滅率、リトライ数を反映した総合スコア
 */
int SideScrollingShooter::CalculateChapterTotalScore(const ChapterResult& result) {
    const int annihilationRate = result.enemySpawnCount == 0 ? 0 :
        result.enemyDefeatCount * 100 / result.enemySpawnCount;
    return (std::max)(0, result.score + result.grazeCount * 100 + annihilationRate * 10 - result.retryCount * 500);
}

float SideScrollingShooter::RailBlend() const {
    float railWeight = m_viewMode == ViewMode::Rail3D ? 1.0f : 0.0f;
    if (m_viewTransitionTimer > 0) {
        const float from = m_viewMode == ViewMode::Rail3D ? 1.0f : 0.0f;
        const float to = m_nextViewMode == ViewMode::Rail3D ? 1.0f : 0.0f;
        railWeight = from + (to - from) * m_viewTransitionProgress;
    }
    return railWeight;
}

bool SideScrollingShooter::IsRailGameplayActive() const {
    return (m_viewTransitionTimer > 0 ? m_nextViewMode : m_viewMode) == ViewMode::Rail3D;
}

bool SideScrollingShooter::IsRailRenderActive() const {
    return m_viewMode == ViewMode::Rail3D || m_nextViewMode == ViewMode::Rail3D ||
        m_viewTransitionTimer > 0;
}

void SideScrollingShooter::ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const {
    // 2DモードはXY移動平面を3Dカメラで正面から見て奥行きを持たせる
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(SideCameraFieldOfView));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(80.0f);
    camera.SetPosition({0.0f, 0.0f, SideCameraZ});
    camera.LookAt({0.0f, 0.0f, SidePlaneZ});
}

void SideScrollingShooter::ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const {
    const float railWeight = RailBlend();

    const Vector3 playerPosition{ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ};
    // 2Dモードと同じカメラ状態から、3Dレールの追従カメラへ補間する
    const Vector3 sidePosition{0.0f, 0.0f, SideCameraZ};
    const Vector3 sideTarget{0.0f, 0.0f, SidePlaneZ};
    const Vector3 railPosition{playerPosition.x * 0.18f, playerPosition.y * 0.12f + 1.0f, PlayerRailZ - 15.5f};
    const Vector3 railTarget{playerPosition.x * 0.28f, playerPosition.y * 0.18f, PlayerRailZ + 22.0f};
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(38.0f + (8.0f * railWeight)));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(120.0f);
    camera.SetPosition(Vector3::Lerp(sidePosition, railPosition, railWeight));
    camera.LookAt(Vector3::Lerp(sideTarget, railTarget, railWeight));
}

void SideScrollingShooter::DrawShape(Renderer& renderer,
    float x, float y, float w, float h, const float color[4]) {
    // 描画ファサードへ矩形コマンドとして記録する
    renderer.Draw(Rect { { x, y }, { w, h } }, { color[0], color[1], color[2], color[3] });
}

void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    float x, float y, float z, float w, float h, float d, const float color[4], float yaw) {
    // プリミティブ形状を実3DカメラのViewProjectionへ乗せて描画する
    const Matrix4x4 world = Matrix4x4::Translation({x, y, z}) *
        Matrix4x4::RotationY(yaw) * Matrix4x4::Scale({w, h, d});
    const PrimitiveShape primitiveShape = shape == 0 ? PrimitiveShape::Plate :
        shape == 1 ? PrimitiveShape::Box :
        shape == 2 ? PrimitiveShape::Cylinder :
        shape == 3 ? PrimitiveShape::Cone :
        shape == 4 ? PrimitiveShape::Prism : PrimitiveShape::Sphere;
    renderer.Draw({
        primitiveShape,
        camera.ProjectionMatrix() * camera.ViewMatrix() * world,
        Vector3::One,
        {color[0], color[1], color[2], color[3]},
        yaw
    });
}

/**
 * @brief プリミティブ球だけで構成した砂漠の巨大骨アーチを描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawDesertBoneArch(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    if (m_boneArchDestroyed) return;

    constexpr int BoneCount = 13;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    constexpr float SideZ = SidePlaneZ + 1.2f;
    const float phase = std::fmod(m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 横視点では骨を縦に並べ、レール視点へ移ると同じ頂点数のアーチへ補間する
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) / static_cast<float>(BoneCount - 1);
        const float x = Math::Lerp(ToWorldX(sideCenterX),
            std::cos(angle) * RailRadius, railWeight);
        const float y = Math::Lerp(ToWorldY(-1.30f + static_cast<float>(i) * 0.24f),
            RailCenterY + std::sin(angle) * RailRadius, railWeight);
        const float z = Math::Lerp(SideZ, railZ, railWeight);
        const float jointScale = i % 3 == 0 ? 1.18f : 1.0f;
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(1.25f, 2.55f, railWeight) * jointScale,
            Math::Lerp(1.40f, 2.55f, railWeight) * jointScale,
            Math::Lerp(0.85f, 2.55f, railWeight) * jointScale, DesertBoneColor);
    }
}

/**
 * @brief プリミティブ球だけで構成したステージ1の巨大隕石を描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawStage1Meteor(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    if (m_meteorDestroyed) return;

    const float sideX = 1.85f - std::fmod(m_meteorTravel * 0.0325f, 4.40f);
    const float sideY = 0.55f + std::sin(m_meteorTravel * 0.105f) * 0.34f;
    const float railX = std::sin(m_meteorTravel * 0.090f) * 7.0f;
    const float railY = 0.80f + std::sin(m_meteorTravel * 0.135f) * 2.0f;
    const float x = Math::Lerp(ToWorldX(sideX), railX, railWeight);
    const float y = Math::Lerp(ToWorldY(sideY), railY, railWeight);
    const float z = Math::Lerp(SidePlaneZ + 1.2f, 72.0f - m_meteorTravel, railWeight);

    // 本体と発光するクレーターを同じ補間座標へ置き、遷移の終端で形状が切り替わらないようにする
    DrawModelPrimitive(renderer, camera, 5, x, y, z,
        Math::Lerp(1.85f, 4.80f, railWeight) * m_meteorScale,
        Math::Lerp(2.20f, 4.80f, railWeight) * m_meteorScale,
        Math::Lerp(1.20f, 4.80f, railWeight) * m_meteorScale, MeteorColor);
    for (int i = 0; i < 3; ++i) {
        const float offsetX = static_cast<float>(i - 1) * Math::Lerp(0.45f, 1.15f, railWeight);
        const float offsetY = (i == 1 ? 0.32f : -0.28f) * Math::Lerp(0.55f, 1.25f, railWeight);
        DrawModelPrimitive(renderer, camera, 5, x + offsetX, y + offsetY,
            z - Math::Lerp(0.62f, 2.32f, railWeight),
            Math::Lerp(0.32f, 0.85f, railWeight) * m_meteorScale,
            Math::Lerp(0.38f, 0.85f, railWeight) * m_meteorScale,
            Math::Lerp(0.16f, 0.35f, railWeight) * m_meteorScale, MeteorCraterColor);
    }
}

/**
 * @brief 海面から飛び出す巨大ウミヘビを描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawOceanSeaSerpent(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    SeaSerpentMotion motion {};
    if (!GetSeaSerpentMotion(m_frame, motion)) return;

    // 通常跳躍、低空横断、超巨大ジャンプを同じ判定用の胴体配置で描画する
    for (int i = 0; i < motion.segmentCount; ++i) {
        const float bodyProgress = (std::max)(0.0f, motion.progress - static_cast<float>(i) * motion.segmentDelay);
        const float elevation = std::sin(Math::HalfPi * 2.0f * bodyProgress) *
            (motion.elevation - static_cast<float>(i) * 0.16f);
        const float sideX = motion.sideOriginX + motion.direction *
            (bodyProgress * motion.travel - static_cast<float>(i) * motion.segmentSpacing);
        const float railX = motion.railOriginX + std::sin(bodyProgress * 4.0f + static_cast<float>(i) * 0.6f) * 6.0f;
        const float railZ = motion.railOriginZ + motion.railDirection *
            (bodyProgress * motion.railTravel - static_cast<float>(i) * 2.6f);
        const float scale = motion.scale * (i == 0 ? 1.25f : 1.0f);
        const float sideWidth = 1.18f * scale;
        const float sideHeight = 1.35f * scale;
        const float sideDepth = 0.72f * scale;
        const float railSize = 2.50f * scale;
        const float sideVisibleHeight = Math::Clamp01(elevation / sideHeight) * sideHeight;
        const float railVisibleHeight = Math::Clamp01(elevation / railSize) * railSize;
        if (sideVisibleHeight <= 0.0f && railVisibleHeight <= 0.0f) continue;
        const float sideVisibleScale = std::sqrt(sideVisibleHeight / sideHeight);
        const float railVisibleScale = std::sqrt(railVisibleHeight / railSize);
        const float x = Math::Lerp(sideX, railX, railWeight);
        const float y = Math::Lerp(-6.0f + sideVisibleHeight * 0.5f, -3.65f + railVisibleHeight * 0.5f, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 13.1f, railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 5, x, y, z,
            Math::Lerp(sideWidth * sideVisibleScale, railSize * railVisibleScale, railWeight),
            Math::Lerp(sideVisibleHeight, railVisibleHeight, railWeight),
            Math::Lerp(sideDepth * sideVisibleScale, railSize * railVisibleScale, railWeight), SeaSerpentColor);
        if (i % 2 == 1 && (sideVisibleHeight >= sideHeight || railVisibleHeight >= railSize)) {
            DrawModelPrimitive(renderer, camera, 5, x, y - Math::Lerp(0.48f, 0.82f, railWeight), z - 0.05f,
                Math::Lerp(0.52f, 1.15f, railWeight), Math::Lerp(0.28f, 0.48f, railWeight),
                Math::Lerp(0.18f, 0.42f, railWeight), SeaSerpentBellyColor);
        }
    }
    const float headScale = motion.scale * 1.25f;
    const float headElevation = std::sin(Math::HalfPi * 2.0f * motion.progress) * motion.elevation;
    const float headSideX = motion.sideOriginX + motion.direction * motion.progress * motion.travel;
    const float headRailX = motion.railOriginX + std::sin(motion.progress * 4.0f) * 6.0f;
    const float headRailZ = motion.railOriginZ + motion.railDirection * motion.progress * motion.railTravel;
    const float headX = Math::Lerp(headSideX, headRailX, railWeight);
    const float headSideVisibleHeight = Math::Clamp01(headElevation / (1.35f * headScale)) * 1.35f * headScale;
    const float headRailVisibleHeight = Math::Clamp01(headElevation / (2.50f * headScale)) * 2.50f * headScale;
    const float headY = Math::Lerp(-6.0f + headSideVisibleHeight * 0.5f,
        -3.65f + headRailVisibleHeight * 0.5f, railWeight);
    const float headZ = Math::Lerp(SidePlaneZ + 13.1f, headRailZ, railWeight);

    // 頭が海面を出入りする短い時間だけ、水滴を初速と重力による放物線で飛ばす
    const float emergeProgress = std::asin((std::min)(1.0f, 1.35f * headScale / motion.elevation)) /
        (Math::HalfPi * 2.0f);
    const float reentryProgress = 0.5f - emergeProgress;
    const float splashCenter = motion.progress < 0.25f ? emergeProgress : reentryProgress;
    const float splashTime = Math::Clamp01((motion.progress - (splashCenter - emergeProgress)) /
        (emergeProgress * 2.0f));
    const bool splashActive = std::abs(motion.progress - splashCenter) <= emergeProgress;
    if (splashActive) {
        for (int i = 0; i < 17; ++i) {
            const float spread = static_cast<float>(i - 8) * 0.22f * motion.scale * splashTime;
            const float launchVelocity = (1.05f + static_cast<float>((i * 5) % 4) * 0.22f) * motion.scale;
            const float gravity = launchVelocity * 2.0f;
            const float dropletHeight = launchVelocity * splashTime - gravity * splashTime * splashTime * 0.5f;
            const float sideSplashX = headSideX - motion.direction * spread;
            const float railSplashX = headRailX - motion.direction * spread;
            DrawModelPrimitive(renderer, camera, 5,
                Math::Lerp(sideSplashX, railSplashX, railWeight),
                Math::Lerp(-5.75f + dropletHeight, -3.45f + dropletHeight, railWeight),
                Math::Lerp(SidePlaneZ + 13.0f, headRailZ - motion.railDirection * spread, railWeight),
                Math::Lerp(0.16f, 0.42f, railWeight) * motion.scale,
                Math::Lerp(0.28f, 0.65f, railWeight) * motion.scale,
                Math::Lerp(0.10f, 0.32f, railWeight) * motion.scale, OceanFoamColor);
        }
    }
    if (headSideVisibleHeight >= 1.35f * headScale || headRailVisibleHeight >= 2.50f * headScale) {
        DrawModelPrimitive(renderer, camera, 5, headX + Math::Lerp(0.36f, 0.82f, railWeight) * motion.scale,
            headY + Math::Lerp(0.30f, 0.72f, railWeight) * motion.scale,
            headZ - Math::Lerp(0.60f, 1.30f, railWeight) * motion.scale,
            Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale, Math::Lerp(0.24f, 0.48f, railWeight) * motion.scale,
            Math::Lerp(0.10f, 0.20f, railWeight) * motion.scale, SeaSerpentEyeColor);
    }
}

void SideScrollingShooter::DrawPlayerModel(Renderer& renderer, const Camera3D& camera,
    float x, float y, float z, bool visible, float yaw) {
    if (!visible) return;

    // 自機は円錐の機首、箱の胴体、三角柱の翼で構成する(仮)
    Vector3 offset = RotateYawOffset(0.0f, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 1, x + offset.x, y + offset.y, z + offset.z,
        0.58f, 0.32f, 1.35f, PlayerColor, yaw);
    offset = RotateYawOffset(0.0f, 0.0f, 0.82f, yaw);
    DrawModelPrimitive(renderer, camera, 3, x + offset.x, y + offset.y, z + offset.z,
        0.42f, 0.42f, 0.78f, PlayerAccent, yaw);
    offset = RotateYawOffset(-0.75f, 0.0f, -0.08f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        1.15f, 0.12f, 0.62f, PlayerAccent, yaw);
    offset = RotateYawOffset(0.75f, 0.0f, -0.08f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        1.15f, 0.12f, 0.62f, PlayerAccent, yaw);
}

void SideScrollingShooter::DrawEnemyModel(Renderer& renderer, const Camera3D& camera, const Enemy& enemy, float yaw) {
    const float x = ToWorldX(enemy.x);
    const float y = ToWorldY(enemy.y);
    const float z = enemy.z;
    if (enemy.type == 2) {
        // 旧ShootingGameで実装済みの大型戦闘機モデルを現行座標系へ縮小して描画する
        constexpr float ModelScale = 0.14f;
        constexpr float Gray[4] = { 0.50f, 0.50f, 0.50f, 1.0f };
        constexpr float White[4] = { 0.60f, 0.60f, 0.60f, 1.0f };
        constexpr float Black[4] = { 0.20f, 0.20f, 0.20f, 1.0f };
        auto DrawBossPart = [&](int shape, float localX, float localY, float localZ,
            float width, float height, float depth, const float color[4]) {
            const Vector3 offset = RotateYawOffset(localX * ModelScale, localY * ModelScale,
                localZ * ModelScale, yaw);
            DrawModelPrimitive(renderer, camera, shape, x + offset.x, y + offset.y, z + offset.z,
                width * ModelScale, height * ModelScale, depth * ModelScale, color, yaw);
        };

        // 機首と上部メインボディ
        if (enemy.bossPartHp[BossNose] > 0) {
            DrawBossPart(2, 0.0f, 3.0f, -14.0f, 6.0f, 6.0f, 4.0f, Gray);
            DrawBossPart(2, 0.0f, 2.0f, -17.5f, 2.0f, 2.0f, 3.0f, Gray);
            DrawBossPart(2, 0.0f, 4.5f, -20.0f, 1.0f, 1.0f, 8.0f, Black);
        }
        DrawBossPart(2, 0.0f, 2.0f, 0.0f, 18.0f, 18.0f, 16.0f, Gray);
        DrawBossPart(2, 0.0f, 2.0f, -10.0f, 14.0f, 14.0f, 4.0f, Gray);
        DrawBossPart(2, 0.0f, 2.0f, 10.0f, 14.0f, 14.0f, 4.0f, Gray);
        DrawBossPart(1, 0.0f, 12.0f, 2.0f, 4.0f, 4.0f, 4.0f, Gray);
        DrawBossPart(2, 0.0f, 13.0f, -2.0f, 1.0f, 1.0f, 4.0f, Black);

        // 下部ボディと左右主翼
        DrawBossPart(2, 0.0f, -12.0f, 0.0f, 4.0f, 4.0f, 10.0f, Gray);
        DrawBossPart(2, 0.0f, -15.0f, 1.0f, 2.0f, 2.0f, 8.0f, Gray);
        DrawBossPart(2, 0.0f, -12.0f, -7.0f, 1.0f, 1.0f, 6.0f, Black);
        DrawBossPart(1, 2.0f, -8.0f, 0.0f, 1.0f, 5.0f, 1.0f, Black);
        DrawBossPart(1, -2.0f, -8.0f, 0.0f, 1.0f, 5.0f, 1.0f, Black);
        if (enemy.bossPartHp[BossLeftWing] > 0) {
            DrawBossPart(1, 13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, White);
            DrawBossPart(1, 21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, White);
        }
        if (enemy.bossPartHp[BossRightWing] > 0) {
            DrawBossPart(1, -13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, White);
            DrawBossPart(1, -21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, White);
        }

        // 主・副エンジン
        DrawBossPart(2, 0.0f, 3.0f, 15.0f, 10.0f, 10.0f, 6.0f, Gray);
        DrawBossPart(2, 7.0f, 3.0f, 18.0f, 4.0f, 4.0f, 6.0f, Black);
        DrawBossPart(2, -7.0f, 3.0f, 18.0f, 4.0f, 4.0f, 6.0f, Black);
        DrawBossPart(1, 0.0f, -6.0f, 16.5f, 2.0f, 8.0f, 3.0f, White);
        DrawBossPart(1, 0.0f, 12.0f, 16.5f, 2.0f, 8.0f, 3.0f, White);
        if (enemy.bossPartHp[BossLeftEngine] > 0) {
            DrawBossPart(2, 6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, Black);
            DrawBossPart(2, 6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, Black);
        }
        if (enemy.bossPartHp[BossRightEngine] > 0) {
            DrawBossPart(2, -6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, Black);
            DrawBossPart(2, -6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, Black);
        }
        return;
    }

    // 通常敵は奥から来る小型機として描画する
    const float scale = enemy.behavior != nullptr ? enemy.behavior->RenderScale() : 1.0f;
    Vector3 offset = RotateYawOffset(0.0f, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 1, x + offset.x, y + offset.y, z + offset.z,
        0.65f * scale, 0.42f * scale, 1.0f * scale, EnemyColor, yaw);
    offset = RotateYawOffset(0.0f, 0.0f, -0.68f * scale, yaw);
    DrawModelPrimitive(renderer, camera, 3, x + offset.x, y + offset.y, z + offset.z,
        0.45f * scale, 0.45f * scale, 0.68f * scale, EnemyAccent, yaw);
    offset = RotateYawOffset(-0.75f * scale, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        0.9f * scale, 0.10f, 0.5f * scale, EnemyAccent, yaw);
    offset = RotateYawOffset(0.75f * scale, 0.0f, 0.0f, yaw);
    DrawModelPrimitive(renderer, camera, 4, x + offset.x, y + offset.y, z + offset.z,
        0.9f * scale, 0.10f, 0.5f * scale, EnemyAccent, yaw);
}

void SideScrollingShooter::DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw) const {
    if (shot.enemy) {
        DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
            0.16f, 0.16f, 0.65f, EnemyShotColor, yaw);
        return;
    }

    // 特殊弾は専用HLSLへ画面座標と進行方向を渡して描画する
    if (shot.special) {
        const Vector3 worldPosition {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};
        Vector2 screenPosition;
        if (!camera.TryWorldToScreen(worldPosition, screenPosition)) return;

        // 弾速ベクトルを画面へ投影して、専用シェーダーの弾頭方向へ反映する
        const Vector3 worldNextPosition {
            ToWorldX(shot.x + shot.vx), ToWorldY(shot.y + shot.vy), shot.z + shot.vz};
        Vector2 nextScreenPosition;
        if (!camera.TryWorldToScreen(worldNextPosition, nextScreenPosition)) nextScreenPosition = screenPosition;
        const Viewport& viewport = camera.GetViewport();
        const Vector2 position {
            (screenPosition.x - static_cast<float>(viewport.x)) / static_cast<float>(viewport.width) * 2.0f - 1.0f,
            1.0f - (screenPosition.y - static_cast<float>(viewport.y)) / static_cast<float>(viewport.height) * 2.0f};
        const Vector2 direction {
            (nextScreenPosition.x - screenPosition.x) / static_cast<float>(viewport.width) * 2.0f,
            (screenPosition.y - nextScreenPosition.y) / static_cast<float>(viewport.height) * 2.0f};
        renderer.DrawPlayerShot({position, {0.040f, 0.020f}, std::atan2(direction.y, direction.x),
            static_cast<float>(m_frame), static_cast<int>(shot.playerType)});
        return;
    }
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.16f, 0.16f, 1.15f, PlayerShotColor, yaw);
}

/**
 * @brief 爆発エフェクトをHLSLへ渡す描画コマンドとして記録する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param explosion 描画対象の爆発
 * @return なし
 */
void SideScrollingShooter::DrawExplosion(Renderer& renderer, const Camera3D& camera, const Explosion& explosion) {
    const float progress = static_cast<float>(explosion.age) / ExplosionLifetimeFrames;
    const float size = 0.36f + progress * 0.72f;
    const Matrix4x4 world = Matrix4x4::Translation({ToWorldX(explosion.x), ToWorldY(explosion.y), explosion.z}) *
        Matrix4x4::Scale({size, size, 1.0f});
    renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world, progress});
}

/**
 * @brief 飛散中の機体部品を描画する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param debris 描画対象の飛散部品
 * @return なし
 */
void SideScrollingShooter::DrawDebris(Renderer& renderer, const Camera3D& camera, const Debris& debris) {
    DrawModelPrimitive(renderer, camera, debris.shape, debris.x, debris.y, debris.z,
        debris.width, debris.height, debris.depth, debris.color.data(), debris.yaw);
}

/**
 * @brief 取得アイテムを描画する
 * @param renderer 描画先レンダラー
 * @param camera 描画に使用するカメラ
 * @param item 描画対象の取得アイテム
 * @param yaw モデルのY軸回転角度
 */
void SideScrollingShooter::DrawItemModel(Renderer& renderer, const Camera3D& camera,
    const Item& item, float yaw) {
    DrawModelPrimitive(renderer, camera, 5, ToWorldX(item.x), ToWorldY(item.y), item.z,
        0.28f, 0.28f, 0.28f, item.type == ItemType::Power ? PowerItemColor : ScoreItemColor, yaw);
}

/**
 * @brief チャプター終了時の戦績を描画する
 * @param renderer 描画先レンダラー
 */
void SideScrollingShooter::DrawChapterResult(Renderer& renderer) const {
    if (!m_chapterResultActive) return;

    constexpr float CharacterSpacing = 0.003f;
    constexpr int FadeFrames = 30;
    const float progress = (std::min)(1.0f,
        static_cast<float>(m_chapterResultTimer) / static_cast<float>(ChapterResultCountUpFrames));
    // 表示開始と終了の両方で文字を滑らかにフェードさせる
    const float alpha = (std::min)(
        SmoothStep(static_cast<float>(m_chapterResultTimer) / FadeFrames),
        SmoothStep(static_cast<float>(ChapterResultDisplayFrames - m_chapterResultTimer) / FadeFrames));
    const int annihilationRate = m_chapterResult.enemySpawnCount == 0 ? 0 :
        m_chapterResult.enemyDefeatCount * 100 / m_chapterResult.enemySpawnCount;
    const int graze = static_cast<int>(m_chapterResult.grazeCount * progress);
    const int defeat = static_cast<int>(m_chapterResult.enemyDefeatCount * progress);
    const int retry = static_cast<int>(m_chapterResult.retryCount * progress);
    const int score = static_cast<int>(m_chapterResult.score * progress);
    const int total = static_cast<int>(m_chapterResult.totalScore * progress);
    const int displayedRate = static_cast<int>(annihilationRate * progress);
    char line[64];

    renderer.DrawText("CHAPTER RESULT", TextAlign::Center, 0.028f, { 1.0f, 0.88f, 0.25f, alpha }, { 0.0f, 0.40f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "GRAZE        %d", graze);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, 0.20f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "ANNIHILATION  %d / %d  %d%%", defeat, m_chapterResult.enemySpawnCount, displayedRate);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, 0.08f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "RETRY        %d", retry);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, -0.04f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "CHAPTER SCORE  %06d", score);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, alpha }, { 0.0f, -0.16f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "TOTAL SCORE    %06d", total);
    renderer.DrawText(line, TextAlign::Center, 0.020f, { 1.0f, 0.88f, 0.25f, alpha }, { 0.0f, -0.31f }, CharacterSpacing);
}

/**
 * @brief リスタート中のカウントダウンを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawRestart(Renderer& renderer) const {
    if (m_restartTimer <= 0) return;

    const int countdown = (m_restartTimer + 59) / 60;
    char text[16];
    std::snprintf(text, sizeof(text), "RESTART %d", countdown);
    renderer.DrawText(text, TextAlign::Center, 0.038f, { 1.0f, 0.88f, 0.25f, 1.0f }, { 0.0f, 0.12f });
}

void SideScrollingShooter::DrawBossHud(Renderer& renderer) const {
    if (!m_bossBattle || m_clear) {
        return;
    }

    // 2D/3D共通のボスHP表示をカメラリセット後のUI座標へ描画する
    constexpr float BossBarBack[4] = { 0.20f, 0.08f, 0.22f, 1.0f };
    constexpr float BossBarFill[4] = { 0.95f, 0.15f, 0.45f, 1.0f };
    constexpr float BossBarDivider[4] = { 1.00f, 0.82f, 0.30f, 0.95f };
    constexpr float BossBarWidth = 0.62f;
    const int bossMaxHp = m_stage->BossMaxHp();
    const float hpRate = bossMaxHp > 0 ? Math::Clamp01(m_displayBossHp / bossMaxHp) : 0.0f;
    DrawShape(renderer, 0.0f, 0.76f, BossBarWidth, 0.025f, BossBarBack);
    DrawShape(renderer, BossBarWidth * (1.0f - hpRate), 0.76f,
        BossBarWidth * hpRate, 0.018f, BossBarFill);
    // 4フェーズの境界をHPバー上に常時表示する
    for (int phase = 1; phase < BossPhaseCount; ++phase) {
        DrawShape(renderer, -BossBarWidth * 0.5f + BossBarWidth * static_cast<float>(phase) / static_cast<float>(BossPhaseCount),
            0.755f, 0.008f, 0.035f, BossBarDivider);
    }
    const BossStory story = BossStories::ForStage(m_stageNumber);
    renderer.DrawText(story.bossName, TextAlign::Center, 0.014f,
        { 1.0f, 0.45f, 0.65f, 1.0f }, { 0.0f, 0.86f });
    const char* phaseLabel = "NORMAL 1";
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.type != 2) continue;
        constexpr const char* PhaseLabels[] = {
            "NORMAL 1", "SPECIAL 1", "NORMAL 2", "SPECIAL 2"
        };
        phaseLabel = PhaseLabels[enemy.bossPhase];
        break;
    }
    renderer.DrawText(phaseLabel, { -BossBarWidth, 0.81f }, 0.012f,
        { 1.0f, 0.82f, 0.30f, 1.0f });
}

/**
 * @brief 弾の命中位置へ爆発エフェクトを生成する
 * @param x 2D座標系のX座標
 * @param y 2D座標系のY座標
 * @param z 3Dレール座標系のZ座標
 * @return なし
 */
void SideScrollingShooter::SpawnExplosion(float x, float y, float z) {
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {x, y, IsRailGameplayActive() ? z : ToRailZFromSideX(x), 0, true};
        return;
    }
}

/**
 * @brief 飛散するモデル部品を固定長プールへ追加する
 * @param x 部品のワールドX座標
 * @param y 部品のワールドY座標
 * @param z 部品のワールドZ座標
 * @param vx 部品のX速度
 * @param vy 部品のY速度
 * @param vz 部品のZ速度
 * @param yaw 部品の初期Y軸回転
 * @param spin 部品のY軸回転速度
 * @param shape 描画するプリミティブ形状
 * @param width 部品の幅
 * @param height 部品の高さ
 * @param depth 部品の奥行き
 * @param color 部品の色
 * @return なし
 */
void SideScrollingShooter::SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
    float yaw, float spin, int shape, float width, float height, float depth, const float color[4]) {
    for (auto& debris : m_debris) {
        if (debris.active) continue;
        debris = {x, y, z, vx, vy, vz, yaw, spin, width, height, depth,
            {color[0], color[1], color[2], color[3]}, shape, 0, true};
        return;
    }
}

/**
 * @brief 機体モデルを構成する部品を飛散エフェクトとして生成する
 * @param enemy 分解する敵機
 * @param bossPart 分解するボス部位、-1は撃破時の残存ボディ
 * @return なし
 */
void SideScrollingShooter::SpawnEnemyDebris(const Enemy& enemy, int bossPart) {
    constexpr float Gray[4] = { 0.50f, 0.50f, 0.50f, 1.0f };
    constexpr float White[4] = { 0.60f, 0.60f, 0.60f, 1.0f };
    constexpr float Black[4] = { 0.20f, 0.20f, 0.20f, 1.0f };
    const float yaw = IsRailGameplayActive() ? 0.0f : Math::HalfPi;
    const float x = ToWorldX(enemy.x);
    const float y = ToWorldY(enemy.y);
    int pieceNumber = 0;
    auto AddPiece = [&](int shape, float localX, float localY, float localZ,
        float width, float height, float depth, const float color[4], float scale = 1.0f) {
        const Vector3 offset = RotateYawOffset(localX * scale, localY * scale, localZ * scale, yaw);
        static constexpr Vector3 SpreadDirections[] = {
            {-0.85f, 0.55f, -0.65f}, {0.90f, -0.40f, -0.75f},
            {-0.70f, -0.80f, 0.85f}, {0.65f, 0.90f, 0.70f},
            {-0.45f, 0.25f, 1.00f}, {0.50f, -0.95f, -0.35f},
            {-1.00f, 0.10f, 0.35f}, {0.95f, 0.35f, -0.15f}
        };
        const Vector3 direction = SpreadDirections[pieceNumber++ % 8];
        const Vector3 velocity = RotateYawOffset(direction.x * 0.040f, direction.y * 0.040f,
            direction.z * 0.040f, yaw);
        SpawnDebrisPiece(x + offset.x, y + offset.y, enemy.z + offset.z,
            velocity.x, velocity.y, velocity.z, yaw, 0.08f + direction.x * 0.050f,
            shape, width * scale, height * scale, depth * scale, color);
    };

    if (enemy.type != 2) {
        const float scale = enemy.behavior != nullptr ? enemy.behavior->RenderScale() : 1.0f;
        AddPiece(1, 0.0f, 0.0f, 0.0f, 0.65f, 0.42f, 1.0f, EnemyColor, scale);
        AddPiece(3, 0.0f, 0.0f, -0.68f, 0.45f, 0.45f, 0.68f, EnemyAccent, scale);
        AddPiece(4, -0.75f, 0.0f, 0.0f, 0.9f, 0.10f, 0.5f, EnemyAccent, scale);
        AddPiece(4, 0.75f, 0.0f, 0.0f, 0.9f, 0.10f, 0.5f, EnemyAccent, scale);
        return;
    }

    constexpr float ModelScale = 0.14f;
    auto AddBossPiece = [&](int shape, float localX, float localY, float localZ,
        float width, float height, float depth, const float color[4]) {
        AddPiece(shape, localX, localY, localZ, width, height, depth, color, ModelScale);
    };
    if (bossPart == BossNose) {
        AddBossPiece(2, 0.0f, 3.0f, -14.0f, 6.0f, 6.0f, 4.0f, Gray);
        AddBossPiece(2, 0.0f, 2.0f, -17.5f, 2.0f, 2.0f, 3.0f, Gray);
        AddBossPiece(2, 0.0f, 4.5f, -20.0f, 1.0f, 1.0f, 8.0f, Black);
        return;
    }
    if (bossPart == BossLeftWing || bossPart == BossRightWing) {
        const float side = bossPart == BossLeftWing ? 1.0f : -1.0f;
        AddBossPiece(1, side * 13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, White);
        AddBossPiece(1, side * 21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, White);
        return;
    }
    if (bossPart == BossLeftEngine || bossPart == BossRightEngine) {
        const float side = bossPart == BossLeftEngine ? 1.0f : -1.0f;
        AddBossPiece(2, side * 6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, Black);
        AddBossPiece(2, side * 6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, Black);
        return;
    }

    // 撃破時は、部位破壊で既に離脱した箇所を除くボディ全体を分解する
    AddBossPiece(2, 0.0f, 2.0f, 0.0f, 18.0f, 18.0f, 16.0f, Gray);
    AddBossPiece(2, 0.0f, 2.0f, -10.0f, 14.0f, 14.0f, 4.0f, Gray);
    AddBossPiece(2, 0.0f, 2.0f, 10.0f, 14.0f, 14.0f, 4.0f, Gray);
    AddBossPiece(2, 0.0f, -12.0f, 0.0f, 4.0f, 4.0f, 10.0f, Gray);
    AddBossPiece(2, 0.0f, -15.0f, 1.0f, 2.0f, 2.0f, 8.0f, Gray);
    AddBossPiece(2, 0.0f, 3.0f, 15.0f, 10.0f, 10.0f, 6.0f, Gray);
    if (enemy.bossPartHp[BossNose] > 0) SpawnEnemyDebris(enemy, BossNose);
    if (enemy.bossPartHp[BossLeftWing] > 0) SpawnEnemyDebris(enemy, BossLeftWing);
    if (enemy.bossPartHp[BossRightWing] > 0) SpawnEnemyDebris(enemy, BossRightWing);
    if (enemy.bossPartHp[BossLeftEngine] > 0) SpawnEnemyDebris(enemy, BossLeftEngine);
    if (enemy.bossPartHp[BossRightEngine] > 0) SpawnEnemyDebris(enemy, BossRightEngine);
}

/**
 * @brief ボス戦前会話を画面へ描画する
 * @param renderer 描画先レンダラー
 */
void SideScrollingShooter::DrawBossStory(Renderer& renderer) const {
    if (!m_bossStoryActive) return;

    const BossStory story = BossStories::ForStage(m_stageNumber);
    if (m_bossStoryLine >= story.lineCount) return;

    // 戦闘画面を少し暗くして会話を前面へ表示する
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.0f, 0.0f, 0.0f, 0.38f});
    const BossStoryLine& line = story.lines[m_bossStoryLine];
    const ColorF nameColor = line.isBoss ? ColorF {1.0f, 0.45f, 0.65f, 1.0f} :
        ColorF {0.35f, 0.90f, 1.0f, 1.0f};
    constexpr float CharacterSpacing = 0.0015f;
    constexpr std::size_t MaxDialogueLineLength = 61;
    const std::string_view text = line.text;
    std::size_t firstLineEnd = text.size();
    if (text.size() > MaxDialogueLineLength) {
        firstLineEnd = text.rfind(' ', MaxDialogueLineLength);
        if (firstLineEnd == std::string_view::npos || firstLineEnd == 0) {
            firstLineEnd = MaxDialogueLineLength;
        }
    }

    // 台詞はボックス幅に合わせて最大二行に折り返す
    renderer.Draw(Rect {{0.0f, -0.48f}, {1.72f, 0.34f}}, {0.03f, 0.08f, 0.14f, 0.92f});
    renderer.DrawText(line.speaker, {-0.78f, -0.37f}, 0.022f, nameColor, CharacterSpacing);
    renderer.DrawText(text.substr(0, firstLineEnd), {-0.78f, -0.51f}, 0.016f,
        ColorF::White(), CharacterSpacing);
    if (firstLineEnd < text.size()) {
        const std::size_t secondLineStart = text[firstLineEnd] == ' ' ? firstLineEnd + 1 : firstLineEnd;
        renderer.DrawText(text.substr(secondLineStart), {-0.78f, -0.56f}, 0.016f,
            ColorF::White(), CharacterSpacing);
    }
    renderer.DrawText("Z: NEXT", {0.61f, -0.60f}, 0.012f,
        {0.65f, 0.75f, 0.82f, 1.0f}, CharacterSpacing);
}

void SideScrollingShooter::Render(Renderer& renderer) const {
    // 安定した2D表示では全オブジェクトを同じ奥行きへ固定する
    if (!IsRailRenderActive()) {
        Render2D(renderer);
        return;
    }
    Render3D(renderer);
}

void SideScrollingShooter::Render2D(Renderer& renderer) const {
    const bool isDesert = m_stageNumber == 2;
    const bool isOcean = m_stageNumber == 3;
    const bool isCity = m_stageNumber == 4 || m_stageNumber == 5;
    const float nightBlend = isDesert ? Math::Clamp01(
        static_cast<float>(m_frame - Stage2NightStartFrame) /
        static_cast<float>(Stage2NightFrame - Stage2NightStartFrame)) :
        (isOcean ? 1.0f - Math::Clamp01(
            static_cast<float>(m_frame - Stage3DawnStartFrame) /
            static_cast<float>(Stage3DawnFrame - Stage3DawnStartFrame)) : (isCity ? 1.0f : 0.0f));
    if (isDesert || isOcean || isCity) DrawDesertSky(renderer, nightBlend);

    Camera3D camera;
    ConfigureSideCamera(camera, renderer);
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    // 透視カメラの表示範囲に合わせて背景面を広げ、画面端まで覆う
    constexpr float BackgroundZ = SidePlaneZ + 20.0f;
    const float backgroundHalfHeight = (BackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float backgroundHalfWidth = backgroundHalfHeight * renderer.AspectRatio();
    const Matrix4x4 backgroundWorld = Matrix4x4::Translation({0.0f, 0.0f, BackgroundZ}) *
        Matrix4x4::Scale({backgroundHalfWidth, backgroundHalfHeight, 1.0f});
    if (!isDesert && !isOcean && !isCity) {
        renderer.Draw({
            PrimitiveShape::Sprite2D,
            camera.ProjectionMatrix() * camera.ViewMatrix() * backgroundWorld,
            Vector3::One,
            {SideBackgroundColor[0], SideBackgroundColor[1], SideBackgroundColor[2], SideBackgroundColor[3]}
        });
    }

    if (isDesert) {
        // 昼はCubeを組み合わせた雲を横方向へ流す
        if (nightBlend < 0.99f) {
            const float cloudColor[4] = {0.88f, 0.91f, 0.88f, 1.0f - nightBlend};
            for (int i = 0; i < 9; ++i) {
                const float x = WrapNdcX(i * 0.47f - m_scroll * 0.08f) * backgroundHalfWidth;
                const float y = backgroundHalfHeight * (0.18f + static_cast<float>((i * 2) % 5) * 0.17f);
                DrawModelPrimitive(renderer, camera, 1, x, y, SidePlaneZ + 11.5f, 1.25f, 0.18f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - 0.72f, y - 0.08f, SidePlaneZ + 11.5f, 0.72f, 0.14f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + 0.76f, y - 0.05f, SidePlaneZ + 11.5f, 0.64f, 0.12f, 0.16f, cloudColor);
            }
        }
        // 夜は偏りのない決定的な配置でCubeの星を表示する
        if (nightBlend > 0.01f) {
            const float starColor[4] = {0.82f, 0.88f, 0.75f, nightBlend};
            for (int i = 0; i < 36; ++i) {
                const float x = (-0.94f + static_cast<float>((i * 73) % 191) / 100.0f) * backgroundHalfWidth;
                const float y = (0.02f + static_cast<float>((i * 37) % 88) / 100.0f) * backgroundHalfHeight;
                const float size = i % 7 == 0 ? 0.075f : 0.045f;
                DrawModelPrimitive(renderer, camera, 1, x, y, SidePlaneZ + 11.8f, size, size, size, starColor);
            }
        }
        // 砂地と角張ったサボテンを横スクロール背景として配置する
        // 上面Y=-6を維持したまま、画面下端より外まで厚みを伸ばす
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
            60.0f, 10.0f, 0.3f, DesertSandColor);
        for (int i = 0; i < 18; ++i) {
            const float x = WrapNdcX(i * 0.41f - m_scroll * 0.32f) * 17.0f;
            // 高さ1.65の幹の底面を砂地上面Y=-6へ合わせる
            constexpr float y = -5.175f;
            constexpr float z = SidePlaneZ + 14.0f;
            DrawModelPrimitive(renderer, camera, 1, x, y, z, 0.32f, 1.65f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.32f, y, z, 0.48f, 0.18f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.52f, y + 0.35f, z, 0.18f, 0.70f, 0.32f, DesertCactusColor);
        }
        DrawDesertBoneArch(renderer, camera, 0.0f);
    } else if (isOcean) {
        // 海面の上端Y=-6を保ち、ドット絵調の波と泡を横スクロールさせる
        const float dayBlend = 1.0f - nightBlend;
        const float waterColor[4] = {
            Math::Lerp(OceanWaterColor[0], OceanWaterColor[0] * 0.30f, nightBlend),
            Math::Lerp(OceanWaterColor[1], OceanWaterColor[1] * 0.36f, nightBlend),
            Math::Lerp(OceanWaterColor[2], OceanWaterColor[2] * 0.48f, nightBlend), 1.0f
        };
        const float waveColor[4] = {
            Math::Lerp(OceanWaveColor[0], OceanWaveColor[0] * 0.38f, nightBlend),
            Math::Lerp(OceanWaveColor[1], OceanWaveColor[1] * 0.42f, nightBlend),
            Math::Lerp(OceanWaveColor[2], OceanWaveColor[2] * 0.55f, nightBlend), 1.0f
        };
        const float foamColor[4] = {OceanFoamColor[0], OceanFoamColor[1], OceanFoamColor[2], 1.0f - nightBlend * 0.35f};
        const float cloudColor[4] = {OceanCloudColor[0], OceanCloudColor[1], OceanCloudColor[2], dayBlend};
        const float sunColor[4] = {OceanSunColor[0], OceanSunColor[1], OceanSunColor[2], dayBlend};
        // 朝に合わせて太陽を昇らせ、Cubeの雲を空の上部へ流す
        if (dayBlend > 0.01f) {
            const float sunX = backgroundHalfWidth * 0.56f;
            const float sunY = backgroundHalfHeight * (0.26f + dayBlend * 0.30f);
            constexpr float skyZ = SidePlaneZ + 11.6f;
            DrawModelPrimitive(renderer, camera, 1, sunX, sunY, skyZ, 1.05f, 1.05f, 0.14f, sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX - 0.62f, sunY, skyZ, 0.28f, 0.28f, 0.15f, sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX + 0.62f, sunY, skyZ, 0.28f, 0.28f, 0.15f, sunColor);
            for (int i = 0; i < 7; ++i) {
                const float x = WrapNdcX(i * 0.53f - m_scroll * 0.06f) * backgroundHalfWidth;
                const float y = backgroundHalfHeight * (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
                DrawModelPrimitive(renderer, camera, 1, x, y, skyZ, 1.15f, 0.18f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - 0.62f, y - 0.07f, skyZ, 0.62f, 0.13f, 0.16f, cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + 0.68f, y - 0.05f, skyZ, 0.54f, 0.12f, 0.16f, cloudColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
            60.0f, 10.0f, 0.3f, waterColor);
        for (int i = 0; i < 24; ++i) {
            const float x = WrapNdcX(i * 0.29f - m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
            const float y = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
            const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
            constexpr float z = SidePlaneZ + 13.6f;
            DrawModelPrimitive(renderer, camera, 1, x, y, z, width, 0.10f, 0.18f, waveColor);
            if (i % 3 == 0) {
                DrawModelPrimitive(renderer, camera, 1, x - width * 0.18f, y + 0.16f, z - 0.02f,
                    width * 0.42f, 0.07f, 0.19f, foamColor);
            }
        }
        DrawOceanSeaSerpent(renderer, camera, 0.0f);
    } else if (isCity) {
        // 常夜の空に決定的な星と、ドット絵調の月を配置する
        constexpr float skyZ = SidePlaneZ + 11.8f;
        for (int i = 0; i < 42; ++i) {
            const float x = (-0.96f + static_cast<float>((i * 71) % 193) / 100.0f) * backgroundHalfWidth;
            const float y = (0.08f + static_cast<float>((i * 43) % 82) / 100.0f) * backgroundHalfHeight;
            const float size = i % 9 == 0 ? 0.085f : 0.045f;
            DrawModelPrimitive(renderer, camera, 1, x, y, skyZ, size, size, 0.12f, StarColor);
        }
        const float moonX = backgroundHalfWidth * 0.58f;
        const float moonY = backgroundHalfHeight * 0.58f;
        DrawModelPrimitive(renderer, camera, 1, moonX, moonY, skyZ, 1.05f, 1.05f, 0.14f, CityMoonColor);
        DrawModelPrimitive(renderer, camera, 1, moonX - 0.38f, moonY + 0.30f, skyZ - 0.01f,
            0.52f, 0.52f, 0.15f, DesertNightSkyColor);
        // ビルは道路上面Y=-6へ接地させ、窓のネオンを前面に重ねる
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -11.0f, SidePlaneZ + 14.0f,
            60.0f, 10.0f, 0.3f, CityStreetColor);
        // 2Dでは道路と車を横スクロールさせ、3Dの右手前ビルは描画しない
        constexpr float roadZ = SidePlaneZ + 13.55f;
        DrawModelPrimitive(renderer, camera, 1, 0.0f, -10.0f, roadZ, 60.0f, 8.0f, 0.22f, CityRoadColor);
        for (int i = 0; i < 9; ++i) {
            const float x = WrapNdcX(i * 0.29f - m_scroll * 1.15f) * (backgroundHalfWidth + 2.0f);
            // 車体の底面を道路上面Y=-6へ接地させる
            constexpr float y = -6.0f + 0.42f * 0.5f;
            const float* bodyColor = i % 2 == 0 ? CityCarBodyColor : CityCarAccentColor;
            DrawModelPrimitive(renderer, camera, 1, x, y, roadZ - 0.16f, 1.75f, 0.42f, 0.12f, bodyColor);
            DrawModelPrimitive(renderer, camera, 1, x, y + 0.30f, roadZ - 0.18f, 0.92f, 0.28f, 0.13f, CityBuildingColor);
            DrawModelPrimitive(renderer, camera, 1, x + 0.72f, y, roadZ - 0.20f, 0.18f, 0.11f, 0.14f, CityLaneColor);
        }
        for (int i = 0; i < 30; ++i) {
            if (i % 2 != 0) continue;
            const float x = WrapNdcX(i * 0.035f - m_scroll * 0.18f) * (backgroundHalfWidth + 2.0f);
            const float width = 3.65f + static_cast<float>((i * 11) % 3) * 0.38f;
            const float height = 3.8f + static_cast<float>((i * 17) % 5) * 1.18f;
            const float y = -6.0f + height * 0.5f;
            constexpr float z = SidePlaneZ + 13.7f;
            DrawModelPrimitive(renderer, camera, 1, x, y, z, width, height, 0.42f, CityBuildingColor);
            for (int row = 0; row < 5; ++row) {
                const float windowY = -5.35f + static_cast<float>(row) * 1.18f;
                if (windowY > -6.0f + height - 0.38f) continue;
                const float* windowColor = (i + row) % 3 == 0 ? CityWindowMagentaColor : CityWindowCyanColor;
                DrawModelPrimitive(renderer, camera, 1, x - width * 0.20f, windowY, z - 0.24f,
                    width * 0.24f, 0.18f, 0.08f, windowColor);
                DrawModelPrimitive(renderer, camera, 1, x + width * 0.20f, windowY, z - 0.24f,
                    width * 0.24f, 0.18f, 0.08f, windowColor);
            }
        }
    } else {
        // 遷移描画と同じ星・グリッド配置を使い、切り替え完了時の交換を防ぐ
        for (int i = 0; i < 28; ++i) {
            float x = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
            float y = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
            DrawModelPrimitive(renderer, camera, 1, x * backgroundHalfWidth, y * backgroundHalfHeight, SidePlaneZ + 12.0f,
                0.06f, 0.06f, 0.06f, StarColor);
        }
        for (int i = 0; i < 8; ++i) {
            float x = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
            DrawModelPrimitive(renderer, camera, 1, x * 20.0f, 0.0f, SidePlaneZ + 7.0f,
                0.04f, 18.0f, 0.08f, GridColor);
        }
        for (int i = 0; i < 12; ++i) {
            DrawModelPrimitive(renderer, camera, 1, 0.0f, (-0.90f + (i % 6) * 0.36f) * 9.0f, SidePlaneZ + 7.0f,
                42.0f, 0.04f, 0.08f, GridColor);
        }
        DrawStage1Meteor(renderer, camera, 0.0f);
    }

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        Enemy sideEnemy = enemy;
        sideEnemy.z = SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f);
        if (enemy.type == 2) {
            DrawEnemyModel(renderer, camera, sideEnemy, Math::HalfPi);
        } else {
            DrawEnemyModel(renderer, camera, sideEnemy, Math::HalfPi);
        }
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot sideShot = shot;
        sideShot.z = SidePlaneZ + (shot.enemy ? 1.0f : -0.4f);
        DrawShotModel(renderer, camera, sideShot, Math::HalfPi);
    }
    for (const auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        Explosion sideExplosion = explosion;
        sideExplosion.z = SidePlaneZ - 0.8f;
        DrawExplosion(renderer, camera, sideExplosion);
    }
    for (const auto& debris : m_debris) {
        if (!debris.active) continue;
        Debris sideDebris = debris;
        sideDebris.z = SidePlaneZ - 0.6f;
        DrawDebris(renderer, camera, sideDebris);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        Item sideItem = item;
        sideItem.z = SidePlaneZ - 0.2f;
        DrawItemModel(renderer, camera, sideItem, Math::HalfPi);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        SidePlaneZ, m_invincible == 0 || (m_invincible / 5) % 2 == 0, Math::HalfPi);

    renderer.ResetCamera();

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / m_stage->BossStartDistance() * 100.0f));
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText("MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });

    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
    if (m_clear) {
        renderer.DrawText(m_stageNumber == 5 ? "ALL STAGES CLEAR" : "STAGE CLEAR", { -0.32f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
    }
}

void SideScrollingShooter::Render3D(Renderer& renderer) const {
    const bool isDesert = m_stageNumber == 2;
    const bool isOcean = m_stageNumber == 3;
    const bool isCity = m_stageNumber == 4 || m_stageNumber == 5;
    const bool isTower = m_stageNumber == 5;
    const float nightBlend = isDesert ? Math::Clamp01(
        static_cast<float>(m_frame - Stage2NightStartFrame) /
        static_cast<float>(Stage2NightFrame - Stage2NightStartFrame)) :
        (isOcean ? 1.0f - Math::Clamp01(
            static_cast<float>(m_frame - Stage3DawnStartFrame) /
            static_cast<float>(Stage3DawnFrame - Stage3DawnStartFrame)) : (isCity ? 1.0f : 0.0f));
    if (isDesert || isOcean || isCity) {
        // カメラ切り替えに影響されない画面背景として空を描画する
        DrawDesertSky(renderer, nightBlend);
    }

    Camera3D camera;
    ConfigureRailCamera(camera, renderer);
    const float railWeight = RailBlend();
    const float playerYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    const float enemyYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    constexpr float SideBackgroundZ = SidePlaneZ + 20.0f;
    const float sideBackgroundHalfHeight = (SideBackgroundZ - SideCameraZ) *
        std::tan(Math::ToRadians(SideCameraFieldOfView) * 0.5f) * 1.01f;
    const float sideBackgroundHalfWidth = sideBackgroundHalfHeight * renderer.AspectRatio();
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    if (isDesert) {
        // 横視点とレール視点で同じ3D砂漠を補間して背景の飛びを防ぐ
        const float sandColor[4] = {
            Math::Lerp(DesertSandColor[0], DesertSandColor[0] * 0.32f, nightBlend),
            Math::Lerp(DesertSandColor[1], DesertSandColor[1] * 0.32f, nightBlend),
            Math::Lerp(DesertSandColor[2], DesertSandColor[2] * 0.45f, nightBlend), 1.0f
        };
        // 昼の雲と夜の星はCubeだけで構成し、視点変更に合わせて奥行きへ展開する
        if (nightBlend < 0.99f) {
            const float cloudColor[4] = {0.88f, 0.91f, 0.88f, 1.0f - nightBlend};
            for (int i = 0; i < 9; ++i) {
                const float sideX = WrapNdcX(i * 0.47f - m_scroll * 0.08f) * sideBackgroundHalfWidth;
                const float sideY = sideBackgroundHalfHeight * (0.18f + static_cast<float>((i * 2) % 5) * 0.17f);
                const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
                const float railY = 5.0f + static_cast<float>((i * 7) % 12);
                const float railZ = 24.0f + static_cast<float>((i * 37) % 88);
                const float x = Math::Lerp(sideX, railX, railWeight);
                const float y = Math::Lerp(sideY, railY, railWeight);
                const float z = Math::Lerp(SidePlaneZ + 11.5f, railZ, railWeight);
                DrawModelPrimitive(renderer, camera, 1, x, y, z,
                    Math::Lerp(1.25f, 2.4f, railWeight), Math::Lerp(0.18f, 0.32f, railWeight),
                    Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - Math::Lerp(0.72f, 1.4f, railWeight),
                    y - Math::Lerp(0.08f, 0.15f, railWeight), z,
                    Math::Lerp(0.72f, 1.3f, railWeight), Math::Lerp(0.14f, 0.24f, railWeight),
                    Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + Math::Lerp(0.76f, 1.5f, railWeight),
                    y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                    Math::Lerp(0.64f, 1.1f, railWeight), Math::Lerp(0.12f, 0.20f, railWeight),
                    Math::Lerp(0.16f, 0.7f, railWeight), cloudColor);
            }
        }
        if (nightBlend > 0.01f) {
            const float starColor[4] = {0.82f, 0.88f, 0.75f, nightBlend};
            for (int i = 0; i < 36; ++i) {
                const float sideX = (-0.94f + static_cast<float>((i * 73) % 191) / 100.0f) * sideBackgroundHalfWidth;
                const float sideY = (0.02f + static_cast<float>((i * 37) % 88) / 100.0f) * sideBackgroundHalfHeight;
                const float railX = -24.0f + static_cast<float>((i * 73) % 480) / 10.0f;
                const float railY = 1.5f + static_cast<float>((i * 37) % 135) / 10.0f;
                const float railZ = 45.0f + static_cast<float>((i * 29) % 48);
                const float x = Math::Lerp(sideX, railX, railWeight);
                const float y = Math::Lerp(sideY, railY, railWeight);
                const float z = Math::Lerp(SidePlaneZ + 11.8f, railZ, railWeight);
                const float sideSize = i % 7 == 0 ? 0.075f : 0.045f;
                const float size = Math::Lerp(sideSize, i % 7 == 0 ? 0.12f : 0.07f, railWeight);
                DrawModelPrimitive(renderer, camera, 1, x, y, z, size, size, size, starColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(-11.0f, -4.0f, railWeight), Math::Lerp(24.0f, 45.0f, railWeight),
            Math::Lerp(60.0f, 140.0f, railWeight), Math::Lerp(10.0f, 0.7f, railWeight),
            Math::Lerp(0.3f, 140.0f, railWeight), sandColor);
        for (int i = 0; i < 18; ++i) {
            const float sideX = WrapNdcX(i * 0.41f - m_scroll * 0.32f) * 17.0f;
            const float railX = -55.0f + static_cast<float>((i * 73) % 1100) / 10.0f;
            constexpr float sideZ = SidePlaneZ + 14.0f;
            const float railZ = 8.0f + std::fmod(static_cast<float>(i * 43) - m_scroll * 28.0f + 110.0f, 110.0f);
            const float x = Math::Lerp(sideX, railX, railWeight);
            const float y = Math::Lerp(-5.175f, -2.825f, railWeight);
            const float z = Math::Lerp(sideZ, railZ, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z, 0.32f, 1.65f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.32f, y, z, 0.48f, 0.18f, 0.32f, DesertCactusColor);
            DrawModelPrimitive(renderer, camera, 1, x - 0.52f, y + 0.35f, z, 0.18f, 0.70f, 0.32f, DesertCactusColor);
        }
        DrawDesertBoneArch(renderer, camera, railWeight);
    } else if (isOcean) {
        // 横視点の海面と同じ波配置をレール空間へ補間し、遷移開始時の飛びを防ぐ
        const float dayBlend = 1.0f - nightBlend;
        const float waterColor[4] = {
            Math::Lerp(OceanWaterColor[0], OceanWaterColor[0] * 0.30f, nightBlend),
            Math::Lerp(OceanWaterColor[1], OceanWaterColor[1] * 0.36f, nightBlend),
            Math::Lerp(OceanWaterColor[2], OceanWaterColor[2] * 0.48f, nightBlend), 1.0f
        };
        const float waveColor[4] = {
            Math::Lerp(OceanWaveColor[0], OceanWaveColor[0] * 0.38f, nightBlend),
            Math::Lerp(OceanWaveColor[1], OceanWaveColor[1] * 0.42f, nightBlend),
            Math::Lerp(OceanWaveColor[2], OceanWaveColor[2] * 0.55f, nightBlend), 1.0f
        };
        const float foamColor[4] = {OceanFoamColor[0], OceanFoamColor[1], OceanFoamColor[2], 1.0f - nightBlend * 0.35f};
        const float cloudColor[4] = {OceanCloudColor[0], OceanCloudColor[1], OceanCloudColor[2], dayBlend};
        const float sunColor[4] = {OceanSunColor[0], OceanSunColor[1], OceanSunColor[2], dayBlend};
        // 太陽と雲は横視点の配置からレール空間へ補間する
        if (dayBlend > 0.01f) {
            const float sideSunX = sideBackgroundHalfWidth * 0.56f;
            const float sideSunY = sideBackgroundHalfHeight * (0.26f + dayBlend * 0.30f);
            const float sunX = Math::Lerp(sideSunX, 32.0f, railWeight);
            const float sunY = Math::Lerp(sideSunY, 14.0f, railWeight);
            const float sunZ = Math::Lerp(SidePlaneZ + 11.6f, 72.0f, railWeight);
            DrawModelPrimitive(renderer, camera, 1, sunX, sunY, sunZ,
                Math::Lerp(1.05f, 2.8f, railWeight), Math::Lerp(1.05f, 2.8f, railWeight),
                Math::Lerp(0.14f, 0.35f, railWeight), sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX - Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
                Math::Lerp(0.28f, 0.70f, railWeight), Math::Lerp(0.28f, 0.70f, railWeight),
                Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
            DrawModelPrimitive(renderer, camera, 1, sunX + Math::Lerp(0.62f, 1.65f, railWeight), sunY, sunZ,
                Math::Lerp(0.28f, 0.70f, railWeight), Math::Lerp(0.28f, 0.70f, railWeight),
                Math::Lerp(0.15f, 0.36f, railWeight), sunColor);
            for (int i = 0; i < 7; ++i) {
                const float sideX = WrapNdcX(i * 0.53f - m_scroll * 0.06f) * sideBackgroundHalfWidth;
                const float sideY = sideBackgroundHalfHeight * (0.32f + static_cast<float>((i * 3) % 4) * 0.13f);
                const float x = Math::Lerp(sideX, -42.0f + static_cast<float>((i * 73) % 840) / 10.0f, railWeight);
                const float y = Math::Lerp(sideY, 8.0f + static_cast<float>((i * 7) % 10), railWeight);
                const float z = Math::Lerp(SidePlaneZ + 11.6f, 32.0f + static_cast<float>((i * 37) % 70), railWeight);
                DrawModelPrimitive(renderer, camera, 1, x, y, z,
                    Math::Lerp(1.15f, 2.2f, railWeight), Math::Lerp(0.18f, 0.30f, railWeight),
                    Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x - Math::Lerp(0.62f, 1.2f, railWeight),
                    y - Math::Lerp(0.07f, 0.12f, railWeight), z,
                    Math::Lerp(0.62f, 1.1f, railWeight), Math::Lerp(0.13f, 0.22f, railWeight),
                    Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
                DrawModelPrimitive(renderer, camera, 1, x + Math::Lerp(0.68f, 1.3f, railWeight),
                    y - Math::Lerp(0.05f, 0.10f, railWeight), z,
                    Math::Lerp(0.54f, 1.0f, railWeight), Math::Lerp(0.12f, 0.20f, railWeight),
                    Math::Lerp(0.16f, 0.65f, railWeight), cloudColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(-11.0f, -4.0f, railWeight), Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
            Math::Lerp(60.0f, 140.0f, railWeight), Math::Lerp(10.0f, 0.7f, railWeight),
            Math::Lerp(0.3f, 140.0f, railWeight), waterColor);
        for (int i = 0; i < 24; ++i) {
            const float sideX = WrapNdcX(i * 0.29f - m_scroll * (0.18f + (i % 3) * 0.05f)) * 18.0f;
            const float sideY = -6.25f - static_cast<float>((i * 37) % 42) / 10.0f;
            const float width = 0.45f + static_cast<float>((i * 17) % 5) * 0.22f;
            const float railX = -50.0f + static_cast<float>((i * 73) % 1000) / 10.0f;
            const float railZ = 8.0f + std::fmod(static_cast<float>(i * 43) - m_scroll * 28.0f + 110.0f, 110.0f);
            const float x = Math::Lerp(sideX, railX, railWeight);
            // 波の底面を海面上面Y=-3.65へ接地させる
            const float y = Math::Lerp(sideY, -3.65f + 0.045f * 0.5f, railWeight);
            const float z = Math::Lerp(SidePlaneZ + 13.6f, railZ, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z,
                Math::Lerp(width, 1.5f, railWeight), Math::Lerp(0.10f, 0.045f, railWeight),
                Math::Lerp(0.18f, 0.70f, railWeight), waveColor);
            if (i % 3 == 0) {
                const float foamY = Math::Lerp(sideY + 0.16f, -3.65f + 0.045f + 0.03f * 0.5f, railWeight);
                DrawModelPrimitive(renderer, camera, 1,
                    x - Math::Lerp(width * 0.18f, 0.25f, railWeight),
                    foamY, z - Math::Lerp(0.02f, 0.08f, railWeight),
                    Math::Lerp(width * 0.42f, 0.65f, railWeight), Math::Lerp(0.07f, 0.03f, railWeight),
                    Math::Lerp(0.19f, 0.72f, railWeight), foamColor);
            }
        }
        DrawOceanSeaSerpent(renderer, camera, railWeight);
    } else if (isCity) {
        // 星・月・ビル群は横視点の配置から都市のレール空間へ補間する
        constexpr float sideSkyZ = SidePlaneZ + 11.8f;
        for (int i = 0; i < 42; ++i) {
            const float sideX = (-0.96f + static_cast<float>((i * 71) % 193) / 100.0f) * sideBackgroundHalfWidth;
            const float sideY = (0.08f + static_cast<float>((i * 43) % 82) / 100.0f) * sideBackgroundHalfHeight;
            const float x = Math::Lerp(sideX, -42.0f + static_cast<float>((i * 71) % 840) / 10.0f, railWeight);
            const float y = Math::Lerp(sideY, 2.0f + static_cast<float>((i * 43) % 150) / 10.0f, railWeight);
            const float z = Math::Lerp(sideSkyZ, 42.0f + static_cast<float>((i * 29) % 60), railWeight);
            const float sideSize = i % 9 == 0 ? 0.085f : 0.045f;
            const float size = Math::Lerp(sideSize, i % 9 == 0 ? 0.15f : 0.08f, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z, size, size, Math::Lerp(0.12f, 0.22f, railWeight), StarColor);
        }
        const float sideMoonX = sideBackgroundHalfWidth * 0.58f;
        const float sideMoonY = sideBackgroundHalfHeight * 0.58f;
        const float moonX = Math::Lerp(sideMoonX, 31.0f, railWeight);
        const float moonY = Math::Lerp(sideMoonY, 15.0f, railWeight);
        const float moonZ = Math::Lerp(sideSkyZ, 88.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 1, moonX, moonY, moonZ,
            Math::Lerp(1.05f, 3.0f, railWeight), Math::Lerp(1.05f, 3.0f, railWeight),
            Math::Lerp(0.14f, 0.40f, railWeight), CityMoonColor);
        DrawModelPrimitive(renderer, camera, 1,
            moonX - Math::Lerp(0.38f, 1.08f, railWeight), moonY + Math::Lerp(0.30f, 0.86f, railWeight),
            moonZ - Math::Lerp(0.01f, 0.05f, railWeight),
            Math::Lerp(0.52f, 1.48f, railWeight), Math::Lerp(0.52f, 1.48f, railWeight),
            Math::Lerp(0.15f, 0.41f, railWeight), DesertNightSkyColor);
        if (isTower) {
            // 中盤から巨大ビルの壁面を流し、レール視点では垂直移動として展開する
            const float wallClimb = m_bossBattle ? 1.0f : SmoothStep(Math::Clamp01(
                static_cast<float>(m_frame - Stage5WallClimbStartFrame) /
                static_cast<float>(Stage5WallClimbEndFrame - Stage5WallClimbStartFrame)));
            // 2Dには壁面を出さず、3Dへの遷移に合わせて視認可能な距離へ展開する
            const float towerBlend = SmoothStep(railWeight);
            const float towerY = Math::Lerp(-3.65f, -10.0f - wallClimb * 30.0f, towerBlend);
            constexpr float towerZ = 62.0f;
            DrawModelPrimitive(renderer, camera, 1, 0.0f, towerY, towerZ,
                90.0f * towerBlend, 100.0f * towerBlend, 2.0f * towerBlend, TowerFacadeColor);
            for (int row = 0; row < 12; ++row) {
                const float y = towerY - (34.0f - static_cast<float>(row) * 6.2f) * towerBlend;
                for (int column = -3; column <= 3; ++column) {
                    const float x = static_cast<float>(column) * 11.2f * towerBlend;
                    const float* windowColor = (row + column) % 3 == 0 ? TowerNeonColor : CityWindowCyanColor;
                    DrawModelPrimitive(renderer, camera, 1, x, y, towerZ - 1.10f * towerBlend,
                        3.8f * towerBlend, 1.1f * towerBlend, 0.12f * towerBlend, windowColor);
                }
            }
            // 壁面を照らすサテライト本体、太陽電池パネル、発光部を描画する
            const float satelliteX = 24.0f * towerBlend;
            const float satelliteY = 24.0f * towerBlend;
            const float satelliteZ = 38.0f;
            DrawModelPrimitive(renderer, camera, 5, satelliteX, satelliteY, satelliteZ,
                1.6f * towerBlend, 1.6f * towerBlend, 1.6f * towerBlend, SatelliteBodyColor);
            DrawModelPrimitive(renderer, camera, 1, satelliteX - 3.2f * towerBlend, satelliteY, satelliteZ,
                4.8f * towerBlend, 0.16f * towerBlend, 1.1f * towerBlend, SatellitePanelColor);
            DrawModelPrimitive(renderer, camera, 1, satelliteX + 3.2f * towerBlend, satelliteY, satelliteZ,
                4.8f * towerBlend, 0.16f * towerBlend, 1.1f * towerBlend, SatellitePanelColor);
            DrawModelPrimitive(renderer, camera, 5, satelliteX - 0.70f * towerBlend, satelliteY - 0.45f * towerBlend,
                satelliteZ - 0.80f * towerBlend, 0.48f * towerBlend, 0.48f * towerBlend, 0.48f * towerBlend,
                SatelliteLightColor);
            if (m_bossBattle) {
                DrawModelPrimitive(renderer, camera, 1, 0.0f,
                    Math::Lerp(-5.60f, -3.55f, railWeight), Math::Lerp(SidePlaneZ + 13.45f, 45.0f, railWeight),
                    Math::Lerp(34.0f, 64.0f, railWeight), Math::Lerp(0.60f, 0.20f, railWeight),
                    Math::Lerp(0.45f, 64.0f, railWeight), TowerRoofColor);
                DrawModelPrimitive(renderer, camera, 1, 0.0f,
                    Math::Lerp(-5.22f, -3.42f, railWeight), Math::Lerp(SidePlaneZ + 13.40f, 45.0f, railWeight),
                    Math::Lerp(9.0f, 18.0f, railWeight), Math::Lerp(0.10f, 0.05f, railWeight),
                    Math::Lerp(0.15f, 18.0f, railWeight), TowerNeonColor);
            }
        }
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(-11.0f, -4.0f, railWeight), Math::Lerp(SidePlaneZ + 14.0f, 45.0f, railWeight),
            Math::Lerp(60.0f, 140.0f, railWeight), Math::Lerp(10.0f, 0.7f, railWeight),
            Math::Lerp(0.3f, 140.0f, railWeight), CityStreetColor);
        // レール3D専用の大通りと流れる車列を描画する
        if (railWeight > 0.01f) {
            const float roadWidth = 24.0f * railWeight;
            DrawModelPrimitive(renderer, camera, 1, 0.0f, -3.60f, 45.0f,
                roadWidth, 0.10f, 140.0f, CityRoadColor);
            for (int lane = -1; lane <= 1; ++lane) {
                for (int segment = 0; segment < 16; ++segment) {
                    const float z = 4.0f + WrapDistance(static_cast<float>(segment) * 10.0f - m_scroll * 150.0f, 160.0f);
                    DrawModelPrimitive(renderer, camera, 1, static_cast<float>(lane) * 6.0f, -3.55f + 0.025f * 0.5f, z,
                        0.20f, 0.025f, 4.2f, CityLaneColor);
                }
            }
            for (int i = 0; i < 10; ++i) {
                const float laneX = -8.5f + static_cast<float>(i % 4) * 5.6f;
                const float speed = i % 2 == 0 ? 175.0f : 105.0f;
                const float z = 8.0f + WrapDistance(static_cast<float>(i) * 19.0f - m_scroll * speed, 120.0f);
                const float* bodyColor = i % 2 == 0 ? CityCarBodyColor : CityCarAccentColor;
                DrawModelPrimitive(renderer, camera, 1, laneX, -3.55f + 0.52f * 0.5f, z, 2.25f, 0.52f, 3.8f, bodyColor);
                DrawModelPrimitive(renderer, camera, 1, laneX, -3.55f + 0.52f + 0.38f * 0.5f, z - 0.15f,
                    1.42f, 0.38f, 1.85f, CityBuildingColor);
                DrawModelPrimitive(renderer, camera, 1, laneX - 0.67f, -3.55f + 0.14f * 0.5f, z - 1.94f,
                    0.24f, 0.14f, 0.10f, CityLaneColor);
                DrawModelPrimitive(renderer, camera, 1, laneX + 0.67f, -3.55f + 0.14f * 0.5f, z - 1.94f,
                    0.24f, 0.14f, 0.10f, CityLaneColor);
            }
        }
        for (int i = 0; i < 30; ++i) {
            const bool leftSide = i % 2 == 0;
            // 右手前ビルは3Dへ入ってから現れ、2Dでは遠景ビルだけを残す
            if (!leftSide && railWeight <= 0.01f) continue;
            const float sideX = WrapNdcX(i * 0.035f - m_scroll * 0.18f) * (sideBackgroundHalfWidth + 2.0f);
            const float sideWidth = 3.65f + static_cast<float>((i * 11) % 3) * 0.38f;
            const float sideHeight = 3.8f + static_cast<float>((i * 17) % 5) * 1.18f;
            const float sideY = -6.0f + sideHeight * 0.5f;
            const float railWidth = 4.2f + static_cast<float>(i % 3) * 0.8f;
            const float railHeight = 9.0f + static_cast<float>((i * 17) % 5) * 2.4f;
            const float x = Math::Lerp(sideX, leftSide ? -18.0f : 18.0f, railWeight);
            const float y = Math::Lerp(sideY, -3.65f + railHeight * 0.5f, railWeight);
            const float z = Math::Lerp(SidePlaneZ + 13.7f,
                10.0f + WrapDistance(static_cast<float>(i * 29) - m_scroll * 36.0f, 100.0f), railWeight);
            const float width = Math::Lerp(sideWidth, railWidth, railWeight);
            const float height = Math::Lerp(sideHeight, railHeight, railWeight);
            const float depth = Math::Lerp(0.42f, 7.0f, railWeight);
            DrawModelPrimitive(renderer, camera, 1, x, y, z, width, height, depth, CityBuildingColor);
            for (int row = 0; row < 5; ++row) {
                const float sideWindowY = -5.35f + static_cast<float>(row) * 1.18f;
                if (sideWindowY > -6.0f + sideHeight - 0.38f) continue;
                const float* windowColor = (i + row) % 3 == 0 ? CityWindowMagentaColor : CityWindowCyanColor;
                const float windowY = Math::Lerp(sideWindowY, -3.65f + 1.2f + static_cast<float>(row) * 2.1f, railWeight);
                const float windowZ = z - Math::Lerp(0.24f, 3.56f, railWeight);
                const float windowWidth = Math::Lerp(sideWidth * 0.24f, 0.75f, railWeight);
                const float windowHeight = Math::Lerp(0.18f, 0.42f, railWeight);
                const float windowDepth = Math::Lerp(0.08f, 0.10f, railWeight);
                DrawModelPrimitive(renderer, camera, 1,
                    x - Math::Lerp(sideWidth * 0.20f, railWidth * 0.22f, railWeight), windowY, windowZ,
                    windowWidth, windowHeight, windowDepth, windowColor);
                DrawModelPrimitive(renderer, camera, 1,
                    x + Math::Lerp(sideWidth * 0.20f, railWidth * 0.22f, railWeight), windowY, windowZ,
                    windowWidth, windowHeight, windowDepth, windowColor);
            }
        }
    } else {
        // 遷移開始直後は2D背景の配置を保ち、画面全体が反転して見える初期ジャンプを避ける
        for (int i = 0; i < 28; ++i) {
        const float sideX = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
        const float sideY = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        const float railX = -13.0f + static_cast<float>((i * 37) % 260) / 10.0f;
        const float railY = -6.0f + static_cast<float>((i * 53) % 120) / 10.0f;
        const float railZ = 8.0f + static_cast<float>((i * 29 + m_frame) % 540) / 9.0f;
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(sideX * sideBackgroundHalfWidth, railX, railWeight),
            Math::Lerp(sideY * sideBackgroundHalfHeight, railY, railWeight),
            Math::Lerp(SidePlaneZ + 12.0f, railZ, railWeight),
            0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f,
            StarColor);
    }
    // 前半で2D縦線を下へ沈めて潰し、後半で3D床グリッドへ展開する
    const float gridLowerWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f));
    const float gridMoveWeight = SmoothStep(Math::Clamp01(railWeight * 2.0f - 1.0f));
    for (int i = 0; i < 8; ++i) {
        const float x = -35.0f + i * 10.0f;
        const float sideX = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(sideX * 20.0f, x, gridMoveWeight),
            Math::Lerp(0.0f, -3.3f, gridLowerWeight),
            Math::Lerp(SidePlaneZ + 7.0f, 32.0f, gridMoveWeight),
            Math::Lerp(0.04f, 0.025f, gridMoveWeight),
            Math::Lerp(18.0f, 0.025f, gridLowerWeight),
            Math::Lerp(0.08f, 140.0f, gridMoveWeight),
            GridColor);
    }
    for (int i = 0; i < 12; ++i) {
        const float z = 6.0f + i * 5.5f - std::fmod(m_scroll * 80.0f, 5.5f);
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp((-0.90f + (i % 6) * 0.36f) * 9.0f, -3.3f, gridLowerWeight),
            Math::Lerp(SidePlaneZ + 7.0f, z, gridMoveWeight),
            Math::Lerp(42.0f, 100.0f, gridMoveWeight),
            Math::Lerp(0.04f, 0.025f, gridMoveWeight),
            Math::Lerp(0.08f, 0.025f, gridMoveWeight),
            GridColor);
    }
    DrawStage1Meteor(renderer, camera, railWeight);
    }

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        Enemy drawEnemy = enemy;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? enemy.transitionSideX :
            (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        drawEnemy.x = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
        drawEnemy.y = Math::Lerp(sideY, enemy.y, railWeight);
        drawEnemy.z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), enemy.z, railWeight);
        if (enemy.type != 2) {
            const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
            const float minimumRailY = FromWorldY(groundTopY + 0.32f);
            drawEnemy.y = Math::Lerp(drawEnemy.y, (std::max)(drawEnemy.y, minimumRailY), railWeight);
        }
        DrawEnemyModel(renderer, camera, drawEnemy, enemyYaw);
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot drawShot = shot;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? shot.transitionSideX : shot.x;
        const float sideY = enteringRail ? shot.transitionSideY : shot.y;
        drawShot.x = Math::Lerp(sideX, exitingRail ? shot.transitionSideX : shot.x, railWeight);
        drawShot.y = Math::Lerp(sideY, shot.y, railWeight);
        drawShot.z = Math::Lerp(SidePlaneZ + (shot.enemy ? 1.0f : -0.4f), shot.z, railWeight);
        DrawShotModel(renderer, camera, drawShot, shot.enemy ? enemyYaw : playerYaw);
    }
    for (const auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        DrawExplosion(renderer, camera, explosion);
    }
    for (const auto& debris : m_debris) {
        if (!debris.active) continue;
        DrawDebris(renderer, camera, debris);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        DrawItemModel(renderer, camera, item, playerYaw);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight),
        m_invincible == 0 || (m_invincible / 5) % 2 == 0, playerYaw);

    renderer.ResetCamera();

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / m_stage->BossStartDistance() * 100.0f));
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText("MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    if (m_viewTransitionTimer > 0) {
        renderer.DrawText("CAMERA SHIFT", { -0.16f, -0.02f }, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f });
    }
    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
    if (m_clear) {
        renderer.DrawText(m_stageNumber == 5 ? "ALL STAGES CLEAR" : "STAGE CLEAR", { -0.32f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
    }
}
