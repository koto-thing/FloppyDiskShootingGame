#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "BossModelView.h"
#include "Stage5ModelView.h"

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
constexpr float SeaSerpentSideEyeSurfaceOffset = 0.90f;
constexpr float SeaSerpentRailEyeSurfaceOffset = 1.70f;
static_assert(SeaSerpentSideEyeSurfaceOffset > 1.35f * 1.25f * 0.5f);
static_assert(SeaSerpentRailEyeSurfaceOffset > 2.50f * 1.25f * 0.5f);
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
constexpr float SearchlightColor[4] = { 1.00f, 0.82f, 0.20f, 0.24f };
constexpr float SearchlightLockedColor[4] = { 1.00f, 0.08f, 0.08f, 0.50f };
constexpr float StormCloudColor[4] = { 0.05f, 0.07f, 0.13f, 1.0f };
constexpr float SideCameraZ = -16.0f;
constexpr float SideCameraFieldOfView = 38.0f;
constexpr int Stage2NightStartFrame = 500;
constexpr int Stage2NightFrame = 750;
constexpr int Stage3DawnStartFrame = 500;
constexpr int Stage3DawnFrame = 750;
constexpr int MissionBannerGlyphDelayFrames = 4;
constexpr int MissionBannerGlyphPopFrames = 8;
constexpr int Stage1BossRushSegmentFrames = 36;
constexpr int Stage1BossRushSegmentCount = 4;
constexpr int Stage1BossRushFrames = Stage1BossRushSegmentFrames * Stage1BossRushSegmentCount;
constexpr int Stage1BossSettleFrames = 96;
constexpr int Stage1BossEntranceFrames = Stage1BossRushFrames + Stage1BossSettleFrames;
constexpr int Phase3FunnelEngineStartFrame = 26;
constexpr float Phase3FunnelLaunchVelocity = 0.09f;
constexpr float Phase3FunnelGravity = 0.0035f;
constexpr float Phase3FunnelRise = Phase3FunnelEngineStartFrame * Phase3FunnelLaunchVelocity -
    Phase3FunnelGravity * Phase3FunnelEngineStartFrame * (Phase3FunnelEngineStartFrame + 1) * 0.5f;
static_assert(Phase3FunnelRise > 0.63f);
static_assert(Phase3FunnelLaunchVelocity -
    Phase3FunnelGravity * Phase3FunnelEngineStartFrame < 0.0f);
constexpr int Stage2BossApproachFrames = 90;
constexpr int Stage2BossAssemblyFrames = 90;
constexpr int Stage2BossEntranceFrames = Stage2BossApproachFrames + Stage2BossAssemblyFrames;
constexpr int BossNameRevealFrames = 150;
constexpr int EastsourceIntroFrames = 210;
constexpr int EastsourceFallFrames = 180;

enum Stage5Cue {
    Stage5DistantThunder,
    Stage5Thunder,
    Stage5SearchlightDetect,
    Stage5SearchlightLocked,
    Stage5BarrageWarning,
    Stage5EastsourceEntrance,
    Stage5SignalLost,
    Stage5Transformation,
    Stage5WeakpointDestroyed,
    Stage5CoreWarning,
    Stage5ChainExplosion,
    Stage5FinalExplosion
};

/**
 * @brief 値を一フレームの最大移動量以内で目標へ近づける
 * @param current 現在値
 * @param target 目標値
 * @param maxDelta 最大移動量
 * @return 更新後の値
 */
float MoveTowards(float current, float target, float maxDelta) {
    return current < target ? (std::min)(current + maxDelta, target) :
        (std::max)(current - maxDelta, target);
}

/**
 * @brief ステージ1ボス出現演出の高速移動区間を取得する
 * @param age 出現演出の経過フレーム
 * @return 0から3までの高速移動区間
 */
constexpr int Stage1BossRushSegment(int age) {
    if (age <= 0) return 0;
    const int segment = age / Stage1BossRushSegmentFrames;
    return segment < Stage1BossRushSegmentCount ? segment : Stage1BossRushSegmentCount - 1;
}

static_assert(Stage1BossRushSegment(0) == 0);
static_assert(Stage1BossRushSegment(Stage1BossRushFrames - 1) == 3);
static_assert(Stage1BossEntranceFrames == 240);
static_assert(Stage2BossEntranceFrames == 180);

/**
 * @brief 文字の登場経過に対応する拡大率を取得する
 * @param glyphAge 文字が登場してからのフレーム数
 * @return 登場前は0、登場中は縮小する拡大率、登場後は1
 */
constexpr float MissionBannerGlyphScale(int glyphAge) {
    if (glyphAge < 0) return 0.0f;
    if (glyphAge >= MissionBannerGlyphPopFrames) return 1.0f;
    return 1.0f + static_cast<float>(MissionBannerGlyphPopFrames - glyphAge) * 0.16f;
}

static_assert(MissionBannerGlyphScale(-1) == 0.0f);
static_assert(MissionBannerGlyphScale(MissionBannerGlyphPopFrames) == 1.0f);

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
 * @brief ウミヘビの各胴体節が頭から遅れて一周する進行率を取得する
 * @param progress 行動全体の進行率
 * @param segmentCount 胴体節数
 * @param segmentDelay 隣接する胴体節間の遅延
 * @param segmentIndex 頭を0とする胴体節番号
 * @return 水中を0、再入水完了を1とする進行率
 */
constexpr float GetSeaSerpentSegmentProgress(float progress, int segmentCount,
    float segmentDelay, int segmentIndex) {
    const float tailDelay = static_cast<float>(segmentCount - 1) * segmentDelay;
    return Math::Clamp01(progress * (1.0f + tailDelay) - static_cast<float>(segmentIndex) * segmentDelay);
}

static_assert(GetSeaSerpentSegmentProgress(1.0f, 23, 0.05f, 22) >= 1.0f - Math::Epsilon);

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
#include "GameplayRandom.h"
#include "Stage1EnemySheet.h"
#include "Stage1EnemySheetEasy.h"
#include "Stage1EnemySheetHard.h"
#include "Stage1EnemySheetNormal.h"
#include "Stage2EnemySheet.h"
#include "Stage2EnemySheetEasy.h"
#include "Stage2EnemySheetHard.h"
#include "Stage2EnemySheetNormal.h"
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

/**
 * @brief 指定難易度のステージ2敵出現シートを取得する
 * @param difficulty 取得する難易度
 * @return 難易度に対応するステージ2敵出現シート
 */
const SideScrollingShooter::Stage& SideScrollingShooter::Stage2EnemySheetInstance(DifficultyType difficulty) {
    static const Stage2EnemySheetEasy easyStage;
    static const Stage2EnemySheetNormal normalStage;
    static const Stage2EnemySheetHard hardStage;
    switch (difficulty) {
    case Hard: return hardStage;
    case Normal: return normalStage;
    default: return easyStage;
    }
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
    case 2: return Stage2EnemySheetInstance(difficulty);
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

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::Stage2BossEnemyBehaviorInstance() {
    static const Stage2BossEnemyBehavior behavior;
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

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::SquareShooterEnemyBehaviorInstance() {
    static const SquareShooterEnemyBehavior behavior;
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
    case 6:
        return SquareShooterEnemyBehaviorInstance();
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
    m_missionStartTimer = MissionBannerDisplayFrames;
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
    m_bossIntroductionPhase = BossIntroductionPhase::None;
    m_bossIntroductionTimer = 0;
    m_clearTimer = 0;
    m_clear = false;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_playerDestructionTimer = 0;
    m_viewToggleRequested = false;
    m_viewMode = ViewMode::Side2D;
    m_nextViewMode = ViewMode::Side2D;
    m_viewTransitionTimer = 0;
    m_viewToggleCooldown = 0;
    m_viewTransitionProgress = 0.0f;
    ResetStage5();
}

/**
 * @brief Stage 5専用状態を初期化する
 * @return なし
 */
void SideScrollingShooter::ResetStage5() {
    m_stage5Phase = Stage5Phase::Approach;
    m_stage5Checkpoint = Stage5Checkpoint::Chapter1;
    m_stage5PhaseTimer = 0;
    m_stage5CheckpointPower = m_power;
    m_stage5CheckpointScore = m_score;
    m_stage5CheckpointKills = m_kills;
    m_stage5SoundCooldown = 0;
    m_stage5AttackTimer = 0;
    m_stage5GuardSpawnCooldown = 0;
    m_stage5CoreTargetX = 0.0f;
    m_stage5CoreTargetY = 0.0f;
    m_tayamaTransformation = 0.0f;
    m_searchlights = {};
    m_tayamaWeakpoints = {{
        {TayamaWeakpoint::LeftSearchlight, 180, 180, false, false, 0},
        {TayamaWeakpoint::RightSearchlight, 180, 180, false, false, 0},
        {TayamaWeakpoint::FireControlRadar, 260, 260, false, false, 0},
        {TayamaWeakpoint::LeftLiftEngine, 360, 360, false, false, 0},
        {TayamaWeakpoint::RightLiftEngine, 360, 360, false, false, 0},
        {TayamaWeakpoint::CommandCore, 900, 900, false, false, 0}
    }};
}

/**
 * @brief デバッグ用に指定ステージとチャプターから開始する
 * @param stageNumber 開始するステージ番号
 * @param chapterNumber 開始するチャプター番号
 * @param bossBattle ボス戦から開始する場合true
 * @return なし
 */
void SideScrollingShooter::StartDebugCheckpoint(int stageNumber, int chapterNumber, bool bossBattle) {
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    ResetStageGimmicks();

    // 指定範囲をゲーム内の進行範囲へ収める
    m_stageNumber = (std::clamp)(stageNumber, 1, 5);
    m_chapterNumber = (std::clamp)(chapterNumber, 1, 3);
    m_stage = &StageForNumber(m_stageNumber, m_difficulty);
    m_chapterRetryCounts = {};
    m_chapterResult = {};
    m_chapterStartPower = m_power;
    m_chapterStartScore = m_score;
    m_chapterStartKills = m_kills;
    m_chapterResultTimer = 0;
    m_missionStartTimer = MissionBannerDisplayFrames;
    m_frame = bossBattle ? m_stage->ChapterEndFrame(3) : m_stage->ChapterEndFrame(m_chapterNumber - 1);
    m_scroll = static_cast<float>(m_frame) * 0.008f;
    m_spawnCooldown = 35;
    m_shotCooldown = 0;
    m_specialShotCooldown = 0;
    m_invincible = 90;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_bossIntroductionPhase = BossIntroductionPhase::None;
    m_bossIntroductionTimer = 0;
    m_clearTimer = 0;
    m_clear = false;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_playerDestructionTimer = 0;
    m_restartTimer = 0;
    m_viewToggleRequested = false;
    m_viewMode = ViewMode::Side2D;
    m_nextViewMode = ViewMode::Side2D;
    m_viewTransitionTimer = 0;
    m_viewToggleCooldown = 0;
    m_viewTransitionProgress = 0.0f;
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    ResetStage5();

    if (bossBattle) {
        m_chapterNumber = 3;
        if (m_stageNumber == 5) {
            m_missionStartTimer = 0;
            m_viewMode = ViewMode::Rail3D;
            m_nextViewMode = ViewMode::Rail3D;
            StartStage5Phase(Stage5Phase::EastsourceBattle);
        } else {
            StartBossBattle();
        }
        m_bossStoryActive = false;
        m_bossIntroductionPhase = BossIntroductionPhase::None;
        m_bossIntroductionTimer = 0;
    }
}

/**
 * @brief Stage 5の指定状態からデバッグ開始する
 * @param phase 開始する状態
 * @return なし
 */
void SideScrollingShooter::StartDebugStage5Phase(Stage5Phase phase) {
    StartDebugCheckpoint(5, 3, false);
    m_chapterNumber = 3;
    m_frame = m_stage->ChapterEndFrame(3);
    m_scroll = static_cast<float>(m_frame) * 0.008f;
    m_missionStartTimer = 0;
    m_viewMode = ViewMode::Rail3D;
    m_nextViewMode = ViewMode::Rail3D;
    m_viewTransitionTimer = 0;
    m_viewTransitionProgress = 0.0f;

    // 後半地点は到達済みの弱点を破壊状態へ合わせてから開始する
    if (phase == Stage5Phase::TayamaLiftEngines ||
        phase == Stage5Phase::TayamaCommandCore || phase == Stage5Phase::TayamaCollapse) {
        for (int i = 0; i <= static_cast<int>(TayamaWeakpoint::FireControlRadar); ++i) {
            m_tayamaWeakpoints[i].hp = 0;
            m_tayamaWeakpoints[i].destroyed = true;
        }
    }
    if (phase == Stage5Phase::TayamaCommandCore || phase == Stage5Phase::TayamaCollapse) {
        for (int i = static_cast<int>(TayamaWeakpoint::LeftLiftEngine);
            i <= static_cast<int>(TayamaWeakpoint::RightLiftEngine); ++i) {
            m_tayamaWeakpoints[i].hp = 0;
            m_tayamaWeakpoints[i].destroyed = true;
        }
    }
    if (phase == Stage5Phase::TayamaCollapse) {
        TayamaWeakpointState& core =
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)];
        core.hp = 0;
        core.destroyed = true;
    }
    m_tayamaTransformation = phase >= Stage5Phase::TayamaFireControl ? 1.0f : 0.0f;
    StartStage5Phase(phase);
#ifdef _DEBUG
    // デバッグ直行後に各フェーズの演出と当たり判定を確認できる時間を確保する
    m_invincible = (std::max)(m_invincible, 600);
#endif
}

void SideScrollingShooter::ProcessInput() {
    m_moveLeft = Input::GetKey(KeyCode::LeftArrow) || Input::GetKey(KeyCode::A);
    m_moveRight = Input::GetKey(KeyCode::RightArrow) || Input::GetKey(KeyCode::D);
    m_moveUp = Input::GetKey(KeyCode::UpArrow) || Input::GetKey(KeyCode::W);
    m_moveDown = Input::GetKey(KeyCode::DownArrow) || Input::GetKey(KeyCode::S);
    m_fire = Input::GetKey(KeyCode::Z) || Input::GetKey(KeyCode::Space);
    m_viewToggleRequested = Input::GetKeyDown(KeyCode::X) && m_viewToggleCooldown == 0 &&
        !IsStage5ViewLocked() &&
        m_bossIntroductionPhase == BossIntroductionPhase::None;

    // デバッグ用に任意の進行地点へ移動する
    if (Input::GetKeyDown(KeyCode::F1)) StartDebugCheckpoint(1, 1, false);
    if (Input::GetKeyDown(KeyCode::F2)) StartDebugCheckpoint(2, 1, false);
    if (Input::GetKeyDown(KeyCode::F3)) StartDebugCheckpoint(3, 1, false);
    if (Input::GetKeyDown(KeyCode::F4)) StartDebugCheckpoint(4, 1, false);
    if (Input::GetKeyDown(KeyCode::F5)) StartDebugCheckpoint(5, 1, false);
    if (Input::GetKeyDown(KeyCode::Alpha1)) StartDebugCheckpoint(m_stageNumber, 1, false);
    if (Input::GetKeyDown(KeyCode::Alpha2)) StartDebugCheckpoint(m_stageNumber, 2, false);
    if (Input::GetKeyDown(KeyCode::Alpha3)) StartDebugCheckpoint(m_stageNumber, 3, false);
    if (Input::GetKeyDown(KeyCode::B)) {
        if (m_stageNumber == 5) StartDebugStage5Phase(Stage5Phase::EastsourceBattle);
        else StartDebugCheckpoint(m_stageNumber, 3, true);
    }
#ifdef _DEBUG
    if (Input::GetKeyDown(KeyCode::F6)) StartDebugStage5Phase(Stage5Phase::WallClimbLower);
    if (Input::GetKeyDown(KeyCode::F7)) StartDebugStage5Phase(Stage5Phase::WallClimbMiddle);
    if (Input::GetKeyDown(KeyCode::F8)) StartDebugStage5Phase(Stage5Phase::WallClimbUpper);
    if (Input::GetKeyDown(KeyCode::F9)) StartDebugStage5Phase(Stage5Phase::TayamaFireControl);
    if (Input::GetKeyDown(KeyCode::F10)) StartDebugStage5Phase(Stage5Phase::TayamaLiftEngines);
    if (Input::GetKeyDown(KeyCode::F11)) StartDebugStage5Phase(Stage5Phase::TayamaCommandCore);
    if (Input::GetKeyDown(KeyCode::F12)) StartDebugStage5Phase(Stage5Phase::TayamaCollapse);
#endif

    if (m_clear && Input::GetKeyDown(KeyCode::R)) {
        Reset(false);
    }
}

void SideScrollingShooter::Tick() {
    // ミッション開始表示中は背景を維持したまま戦闘進行と操作を止める
    if (m_missionStartTimer > 0) {
        --m_missionStartTimer;
        return;
    }

    const bool completingRailToSideTransition = m_viewTransitionTimer == 1 &&
        m_viewMode == ViewMode::Rail3D && m_nextViewMode == ViewMode::Side2D;
    TickViewTransition();

    // HUD用HPは実HPへ追従させ、ダメージ時の減少を視認できるようにする
    if (m_displayBossHp > static_cast<float>(m_bossHp)) {
        const float difference = m_displayBossHp - static_cast<float>(m_bossHp);
        m_displayBossHp = (std::max)(static_cast<float>(m_bossHp),
            m_displayBossHp - (std::max)(1.0f, difference * 0.16f));
    } else {
        m_displayBossHp = static_cast<float>(m_bossHp);
    }
    if (m_clear) {
        TickExplosions();
        TickDebris();
        --m_clearTimer;
        if (m_stageNumber < 5 && m_clearTimer <= 0) StartNextStage();
        return;
    }
    if (m_playerDestructionTimer > 0) {
        // 自機の破壊演出中は戦闘進行を止め、爆発と飛散物だけを更新する
        TickExplosions();
        TickDebris();
        if (--m_playerDestructionTimer == 0) RestartCurrentChapter();
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
            TickChapterExitEnemies();
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
    if (m_bossIntroductionPhase == BossIntroductionPhase::Entrance ||
        m_bossIntroductionPhase == BossIntroductionPhase::NameReveal) {
        TickBossIntroduction();
        return;
    }
    if (m_bossStoryActive) {
        TickBossStory();
        return;
    }

    // Stage 5後半は絶対フレームではなく専用状態と経過時間で進行する
    if (m_stageNumber == 5 && m_stage5Phase != Stage5Phase::Approach) {
        TickStage5();
    } else if (m_stageNumber == 5 && m_chapterNumber == 3 && m_frame % 150 == 30) {
        // TAYAMA浮上に同期して道路上の小型構造物を決定的な間隔で崩す
        const float side = (m_frame / 150) % 2 == 0 ? -1.0f : 1.0f;
        SpawnExplosion(side * 0.88f, -0.72f, 52.0f, true);
    }

    ++m_frame;
    if (ShouldAdvanceStageScroll()) m_scroll += 0.008f;
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_specialShotCooldown = (std::max)(0, m_specialShotCooldown - 1);
    m_invincible = (std::max)(0, m_invincible - 1);
    m_viewToggleCooldown = (std::max)(0, m_viewToggleCooldown - 1);
    m_stage5SoundCooldown = (std::max)(0, m_stage5SoundCooldown - 1);
    if (m_stageNumber == 5 && m_stage5Phase < Stage5Phase::TayamaCommandCore &&
        m_stage5SoundCooldown == 0) {
        if (m_frame % 397 == 0) PlayStage5Cue(Stage5DistantThunder);
        if (m_chapterNumber >= 2 && m_frame % 241 == 0) PlayStage5Cue(Stage5Thunder);
    }
    if (!m_bossBattle && !m_chapterResultActive &&
        (m_stageNumber != 5 || m_stage5Phase == Stage5Phase::Approach) &&
        m_frame >= m_stage->ChapterEndFrame(m_chapterNumber)) {
        FinishChapter();
    }

    TickPlayer();
    TickStageGimmicks();

    // 3Dから2Dへ確定するフレームだけは、座標変換直後の特殊障害物との誤接触を除外する
    if (!completingRailToSideTransition &&
        ((m_stageNumber == 1 && HitsStage1Meteor(m_playerX, m_playerY, PlayerRailZ, 0.055f)) ||
        (m_stageNumber == 2 && HitsDesertBoneArch(m_playerX, m_playerY, PlayerRailZ, 0.055f)) ||
        (m_stageNumber == 3 && HitsOceanSeaSerpent(m_playerX, m_playerY, PlayerRailZ, 0.055f)))) {
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

    if (!m_bossBattle && !m_chapterResultActive &&
        (m_stageNumber != 5 || m_stage5Phase == Stage5Phase::Approach)) {
        for (int spawnIndex = 0;; ++spawnIndex) {
            Stage::EnemySpawnRule spawn;
            if (!m_stage->TrySelectEnemySpawn(m_frame, spawnIndex, spawn, m_chapterNumber)) {
                break;
            }
            SpawnEnemy(spawn.enemyType, spawn.sideX, spawn.railX, spawn.y, spawn.railZ);
        }
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
    const float minY = IsRailGameplayActive() ? PlayerRailMinY() : Side2DPlayerMinY;
    const float maxY = IsRailGameplayActive() ? 0.9f : Side2DPlayerMaxY;
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f, minX, maxX);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f, minY, maxY);
}

void SideScrollingShooter::TickEnemies() {
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        ++enemy.age;
        for (int& frames : enemy.bossPartHitFlashFrames) {
            if (frames > 0) --frames;
        }
        if (enemy.behavior == nullptr) {
            enemy.behavior = &EnemyBehaviorForType(enemy.type);
        }
        if (enemy.shotInterval <= 0) {
            enemy.shotInterval = enemy.behavior->AimedShotInterval();
        }
        if (enemy.type == 2) {
            m_stage->TickBoss(*this, enemy);
        } else {
            enemy.behavior->Tick(*this, enemy);
        }

        // Phase3レールガンは発射フレームの一瞬だけ固定照準線へ当たり判定を通す
        const int beamCycle = enemy.stage2BossActionAge % Stage2RailgunCycleFrames;
        if (enemy.type == 2 && m_stageNumber == 2 && enemy.phase >= 3.0f &&
            enemy.bossPartHp[BossNose] > 0 && beamCycle == Stage2RailgunFireFrame && m_invincible == 0) {
            constexpr float BossScale = 1.92f;
            const float yaw = IsRailGameplayActive() ? 0.0f : Math::HalfPi;
            const float cosine = std::cos(yaw);
            const float sine = std::sin(yaw);
            const float patrolX = std::sin(static_cast<float>(enemy.age) * 0.018f) * 2.4f * RailBlend();
            const float battleshipX = ToWorldX(enemy.x + enemy.landBattleshipOffsetX) + patrolX;
            const float battleshipY = Stage2BattleshipWorldY(enemy);
            const float battleshipZ = enemy.z + enemy.landBattleshipOffsetZ;
            const float startX = battleshipX - 1.55f * cosine * BossScale;
            const float startY = battleshipY + 2.08f * BossScale;
            const float startZ = battleshipZ + 1.55f * sine * BossScale;
            const float targetX = ToWorldX(enemy.actionX);
            const float targetY = ToWorldY(enemy.actionY);
            const float targetZ = IsRailGameplayActive() ? enemy.actionZ : SidePlaneZ;
            const Vector3 direction = Vector3 {targetX - startX, targetY - startY, targetZ - startZ}.Normalized();
            if (Hit3DSegment(startX, startY, startZ,
                targetX + direction.x * 18.0f, targetY + direction.y * 18.0f,
                targetZ + direction.z * 18.0f, 0.52f,
                ToWorldX(m_playerX), ToWorldY(m_playerY), IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ, 0.38f)) {
                DamagePlayer();
                return;
            }
        }

        // Phase3は上部戦艦の船体だけで骨アーチとの接触を判定する
        if (enemy.type == 2 && m_stageNumber == 2 && enemy.phase >= 3.0f && !m_boneArchDestroyed) {
            constexpr float BodyLocalX = 0.55f;
            constexpr float BodyLocalY = 0.95f;
            constexpr float ModelScale = 1.92f;
            constexpr float BodyRadius = 4.25f;
            const float yaw = IsRailGameplayActive() ? 0.0f : Math::HalfPi;
            const float patrolWorldX = std::sin(static_cast<float>(enemy.age) * 0.018f) *
                2.4f * RailBlend();
            const Vector3 bodyCenter {
                ToWorldX(enemy.x + enemy.landBattleshipOffsetX) + patrolWorldX +
                    std::cos(yaw) * BodyLocalX * ModelScale,
                Stage2BattleshipWorldY(enemy) + BodyLocalY * ModelScale,
                enemy.z + enemy.landBattleshipOffsetZ - std::sin(yaw) * BodyLocalX * ModelScale
            };
            if (HitsDesertBoneArch(FromWorldX(bodyCenter.x), FromWorldY(bodyCenter.y),
                bodyCenter.z, BodyRadius / WorldXScale)) {
                DestroyDesertBoneArch();
                SpawnExplosion(FromWorldX(bodyCenter.x), FromWorldY(bodyCenter.y), bodyCenter.z);
                PlayHitSound();
            }
        }

        // 巨大障害物へ接触した通常敵はスコアやアイテムを発生させず、その場で破壊する
        // ボス(type 2)は対象外
        const float boneHitRadius = IsRailGameplayActive() ? enemy.behavior->CollisionRadius3D(enemy) / WorldXScale :
            enemy.behavior->CollisionRadius(enemy);
        if (enemy.type != 2 && ((m_stageNumber == 1 && HitsStage1Meteor(enemy.x, enemy.y, enemy.z, boneHitRadius)) ||
            (m_stageNumber == 2 && HitsDesertBoneArch(enemy.x, enemy.y, enemy.z, boneHitRadius)) ||
            (m_stageNumber == 3 && HitsOceanSeaSerpent(enemy.x, enemy.y, enemy.z, boneHitRadius)))) {
            SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
            SpawnEnemyDebris(enemy);
            enemy.active = false;
            continue;
        }

        if (enemy.attackWarningFrames > 0) --enemy.attackWarningFrames;
        const int aimedShotInterval = enemy.shotInterval;
        const bool canUseAimedShot = !(enemy.type == 2 && m_stage->IsBossSpecialAttackActive(enemy));
        if (aimedShotInterval > AttackWarningFrames && canUseAimedShot &&
            enemy.age % aimedShotInterval == aimedShotInterval - AttackWarningFrames) {
            // 発射時の追尾を防ぐため、予告した地点を狙い弾の目標として固定する
            enemy.attackWarningTargetX = m_playerX;
            enemy.attackWarningTargetY = m_playerY;
            enemy.attackWarningFrames = AttackWarningFrames;
        }
        if (aimedShotInterval > 0 && enemy.age % aimedShotInterval == 0 && canUseAimedShot) {
            const float dxToPlayer = enemy.attackWarningTargetX - enemy.x;
            const float dyToPlayer = enemy.attackWarningTargetY - enemy.y;
            const float length = std::sqrt(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);
            if (length > 0.001f) {
                const float shotSpeed = enemy.behavior->AimedShotSpeed();
                SpawnShot(enemy.x - 0.06f, enemy.y, dxToPlayer / length * shotSpeed,
                    dyToPlayer / length * shotSpeed, true, enemy.z, enemy.behavior->RailAimedShotSpeed());
            }
        }

        // ボスは通常・特殊フェーズごとの間隔で、未破壊の各部位から弾幕を発射する
        if (enemy.type == 2 &&
            !m_stage->IsBossSpecialAttackActive(enemy) &&
            enemy.age % m_stage->BossAttackInterval(static_cast<BossPhase>(enemy.bossPhase)) == 0) {
            FireBossPartBarrage(enemy);
        }

        if (enemy.type != 2 && !IsRailGameplayActive() && enemy.x < -2.6f) enemy.active = false;
        if (enemy.type != 2 && IsRailGameplayActive() && enemy.z < 2.0f) enemy.active = false;
        if (enemy.type == 2 && !enemy.collisionEnabled) continue;
        const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
        const bool playerHit = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.42f,
                ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.behavior->CollisionRadius3D(enemy)) :
            Hit(m_playerX, m_playerY, 0.055f, enemy.x, enemy.y, enemyRadius);
        if (enemy.active && m_invincible == 0 && playerHit) {
            if (enemy.type != 2) {
                SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
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
    if (m_stageNumber != 1) return;

    // 大小の異なる隕石をそれぞれ移動・回転させる
    for (auto& meteor : m_meteors) {
        if (meteor.destroyed) continue;
        meteor.travel += 0.10f + meteor.scale * 0.06f;
        if (meteor.travel >= 72.0f) meteor.travel -= 72.0f;
        meteor.yaw += meteor.spin;
    }
}

/** @brief ステージ固有の破壊可能ギミックを初期状態へ戻す */
void SideScrollingShooter::ResetStageGimmicks() {
    constexpr float Travel[] = { 0.0f, 11.5f, 24.8f, 37.0f, 50.6f, 63.4f };
    constexpr float Scale[] = { 1.75f, 1.10f, 2.05f, 1.38f, 1.62f, 0.92f };
    constexpr float Spin[] = { 0.035f, -0.052f, 0.028f, -0.041f, 0.046f, -0.061f };
    for (int i = 0; i < MeteorCount; ++i) {
        m_meteors[i] = { Travel[i], Scale[i], static_cast<float>(i) * 0.7f, Spin[i],
            4 + i % 3, false };
    }
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
    const int meteorIndex = m_stageNumber == 1 ?
        FindStage1Meteor(shot.x, shot.y, shot.z, shot.hitRadius) : -1;
    if (meteorIndex >= 0) {
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        Meteor& meteor = m_meteors[meteorIndex];
        meteor.hp -= shot.damage;
        SpawnMeteorDebris(meteor, meteor.hp <= 0 ? 8 : 2);
        if (meteor.hp <= 0) meteor.destroyed = true;
        PlayHitSound();
        return true;
    }
    if (m_stageNumber == 2 && !m_boneArchDestroyed &&
        HitsDesertBoneArch(shot.x, shot.y, shot.z, shot.hitRadius)) {
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
        m_boneArchHp -= shot.damage;
        if (m_boneArchHp <= 0) DestroyDesertBoneArch();
        PlayHitSound();
        return true;
    }
    return false;
}

void SideScrollingShooter::TickShots() {
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        if (shot.funnelDustAge >= 0) ++shot.funnelDustAge;
        const float previousY = shot.y;

        // Phase3ファンネルは短く自由落下してから補助エンジンで自機へ向かう
        if (shot.funnel) ++shot.age;
        if (shot.enemy && shot.funnelDelayedEngine) {
            constexpr int FunnelEngineStartFrame = Phase3FunnelEngineStartFrame;
            constexpr int FunnelLaunchCullGraceFrames = 45;
            constexpr float FunnelGravity = Phase3FunnelGravity;
            static_assert(FunnelEngineStartFrame < FunnelLaunchCullGraceFrames);
            if (shot.age < FunnelEngineStartFrame) {
                shot.vy -= FunnelGravity;
            } else {
                constexpr float FunnelSpeed = 0.72f;
                constexpr float EngineAcceleration = 0.085f;
                if (shot.age == FunnelEngineStartFrame) {
                    // 点火時の自機位置から進行方向を一度だけ固定する
                    const float dx = ToWorldX(m_playerX - shot.x);
                    const float dy = ToWorldY(m_playerY - shot.y);
                    const float dz = IsRailGameplayActive() ? PlayerRailZ - shot.z : 0.0f;
                    const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                    shot.funnelEngineVx = FromWorldX(dx / length * FunnelSpeed);
                    shot.funnelEngineVy = FromWorldY(dy / length * FunnelSpeed);
                    shot.funnelEngineVz = dz / length * FunnelSpeed;
                }
                shot.vx += (shot.funnelEngineVx - shot.vx) * EngineAcceleration;
                shot.vy += (shot.funnelEngineVy - shot.vy) * EngineAcceleration;
                shot.vz += (shot.funnelEngineVz - shot.vz) * EngineAcceleration;
            }
        }
        // Phase2ファンネルは進路を変えず、上方向へ徐々に加速する
        if (shot.enemy && shot.funnel && !shot.funnelDelayedEngine) {
            shot.vy = (std::min)(0.115f, shot.vy + 0.0018f);
        }

        /** @brief 追尾弾を最寄りの前方敵へ旋回させる */
        if (!shot.enemy && shot.special && shot.playerType == Homing) {
            UpdateHomingShot(shot);
        }

        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
        if (shot.funnel && shot.funnelDustAge < 0) {
            const float groundY = FromWorldY(Math::Lerp(-6.0f, -3.65f, RailBlend()));
            if (previousY < groundY && shot.y >= groundY) {
                shot.funnelDustAge = 0;
                shot.funnelDustX = shot.x;
                shot.funnelDustY = groundY;
                shot.funnelDustZ = shot.z;
            }
        }
        if (!IsRailGameplayActive()) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        // ハッチから出た直後は船体外へ抜けるまで通常弾の画面外カリングを猶予する
        constexpr int FunnelLaunchCullGraceFrames = 45;
        const bool funnelLaunching = shot.funnel && shot.age <= FunnelLaunchCullGraceFrames;
        if (!funnelLaunching && !IsRailGameplayActive() &&
            (shot.x < Side2DPlayerMinX - Side2DShotCullMargin ||
                shot.x > Side2DPlayerMaxX + Side2DShotCullMargin ||
                shot.y < Side2DPlayerMinY - Side2DShotCullMargin ||
                shot.y > Side2DPlayerMaxY + Side2DShotCullMargin)) {
            shot.active = false;
            continue;
        }
        // 端から出る円形弾幕が生成直後に欠けないよう、弾のY消滅範囲だけ少し広げる
        if (!funnelLaunching && IsRailGameplayActive() && (shot.z < 0.0f || shot.z > 72.0f ||
            std::abs(shot.x) > 1.2f || std::abs(shot.y) > 1.24f)) {
            shot.active = false;
            continue;
        }

        if (shot.enemy) {
            const bool playerHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.38f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.funnel ? 0.42f : 0.28f) :
                Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y, shot.funnel ? 0.055f : 0.022f);
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

        if (m_stageNumber == 5 &&
            (TryDamageTayama(shot) || TryDamageWallSearchlight(shot))) continue;
        if (TryDamageStageGimmick(shot)) continue;

        for (auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            if (m_chapterResultActive && !enemy.collisionEnabled) continue;
            if (enemy.type == 2 && !enemy.collisionEnabled && m_stageNumber != 2) continue;
            if (enemy.behavior == nullptr) {
                enemy.behavior = &EnemyBehaviorForType(enemy.type);
            }
            BossPart hitPart = BossNose;
            if (enemy.type == 2 && TryHitBossPart(shot, enemy, hitPart)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                if (!shot.piercing) shot.active = false;
                enemy.bossPartHitFlashFrames[hitPart] = BossPartHitFlashFrames;
                enemy.bossPartHp[hitPart] -= shot.damage;
                if (enemy.bossPartHp[hitPart] <= 0) {
                    enemy.bossPartHp[hitPart] = 0;
                    SpawnEnemyDebris(enemy, hitPart);
                    const int partDamage = m_stage->BossPartBreakDamage(hitPart);
                    const bool bossDefeated = DamageBoss(enemy, partDamage);
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
            if (enemy.type == 2 && m_stageNumber == 2 && TryHitStage2BossBody(shot, enemy)) {
                SpawnExplosion(shot.x, shot.y, shot.z);
                if (!shot.piercing) shot.active = false;
                if (DamageBoss(enemy, shot.damage)) DefeatBoss(enemy);
                else m_bossHp = enemy.hp;
                PlayHitSound();
                break;
            }
            // Stage2のcollisionEnabledは潜航中の本体接触だけを無効化し、露出中の上部戦艦は部位判定を維持する
            if (enemy.type == 2 && !enemy.collisionEnabled) continue;
            const float enemyRadius = enemy.behavior->CollisionRadius(enemy);
            const bool enemyHit = IsRailGameplayActive() ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
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
                    SpawnExplosion(enemy.x, enemy.y, enemy.z, true);
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
        if (m_stageNumber == 5) {
            StartStage5Phase(Stage5Phase::EastsourceIntro);
            return;
        }
        m_bossBattlePending = true;
        return;
    }

    ++m_chapterNumber;
    m_chapterResult = {};
    m_chapterStartPower = m_power;
    m_chapterStartScore = m_score;
    m_chapterStartKills = m_kills;
    if (m_stageNumber == 5) {
        SaveStage5Checkpoint(m_chapterNumber == 2 ?
            Stage5Checkpoint::Chapter2 : Stage5Checkpoint::Chapter3);
    }
}

/**
 * @brief チャプター終了中の敵を当たり判定なしで画面外へ退避させる
 * @return なし
 */
void SideScrollingShooter::TickChapterExitEnemies() {
    constexpr float SideExitSpeed = 0.10f;
    constexpr float RailExitSpeed = 1.4f;
    static_assert(SideExitSpeed > 0.0f && RailExitSpeed > 0.0f);

    // 敵AIと射撃を止めたまま進行方向へ高速移動させる
    for (auto& enemy : m_enemies) {
        if (!enemy.active || enemy.type == 2) continue;
        enemy.collisionEnabled = false;
        if (IsRailGameplayActive()) {
            enemy.z -= RailExitSpeed;
            if (enemy.z < 2.0f) enemy.active = false;
        } else {
            enemy.x -= SideExitSpeed;
            enemy.z = ToRailZFromSideX(enemy.x);
            if (enemy.x < -2.6f) enemy.active = false;
        }
    }
}

/**
 * @brief 現在のチャプター戦績を確定して表示を開始する
 */
void SideScrollingShooter::FinishChapter() {
    m_chapterResult.retryCount = m_chapterRetryCounts[m_chapterNumber - 1];
    m_chapterResult.totalScore = CalculateChapterTotalScore(m_chapterResult);
    m_chapterResultTimer = 0;
    m_chapterResultActive = true;

    // 敵弾をその場で爆発へ変換して当たり判定を消す
    for (auto& shot : m_shots) {
        if (!shot.active || !shot.enemy) continue;
        SpawnExplosion(shot.x, shot.y, shot.z);
        shot.active = false;
    }

    // 通常敵は当たり判定を無効化し、結果表示中に画面外へ退避させる
    for (auto& enemy : m_enemies) {
        if (enemy.active && enemy.type != 2) enemy.collisionEnabled = false;
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

    RequestViewMode(m_viewMode == ViewMode::Side2D ? ViewMode::Rail3D : ViewMode::Side2D);
}

/**
 * @brief 入力を偽装せず表示モード変更を要求する
 * @param mode 切り替え先
 * @return なし
 */
void SideScrollingShooter::RequestViewMode(ViewMode mode) {
    if ((m_viewTransitionTimer > 0 && m_nextViewMode == mode) ||
        (m_viewTransitionTimer == 0 && m_viewMode == mode)) {
        return;
    }

    // 遷移開始時に座標系を切り替え、遷移中も切替先のゲームルールで更新する
    m_nextViewMode = mode;
    m_viewTransitionTimer = ViewTransitionFrames;
    m_viewTransitionProgress = 0.0f;

    // 切り替え開始から3秒間の無敵と8秒間の再入力待ちを付与する
    m_invincible = (std::max)(m_invincible, ViewToggleInvincibleFrames);
    m_viewToggleCooldown = ViewToggleCooldownFrames;
    if (mode == ViewMode::Rail3D) {
        // 遷移中はTickPlayerを通らないため、開始時点で機体を地面上へ戻す
        m_playerY = (std::max)(m_playerY, PlayerRailMinY());
        InitializeRailObjects();
    } else {
        InitializeSideObjects();
    }
}

/**
 * @brief Stage 5後半で3D表示が固定されているか取得する
 * @return 3D表示が固定されている場合true
 */
bool SideScrollingShooter::IsStage5ViewLocked() const {
    return m_stageNumber == 5 && m_stage5Phase != Stage5Phase::Approach;
}

/**
 * @brief 現在の進行状態で背景スクロールを更新するか取得する
 * @return 背景スクロールを更新する場合true
 */
bool SideScrollingShooter::ShouldAdvanceStageScroll() const {
    if (m_stageNumber != 5) return true;
    return m_stage5Phase <= Stage5Phase::WallClimbUpper ||
        m_stage5Phase == Stage5Phase::EastsourceFall;
}

void SideScrollingShooter::InitializeRailObjects() {
    // 自機以外は2D横位置を3D奥行きへ移して、レール用の横位置を別に持つ
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        enemy.transitionSideX = enemy.x;
        enemy.transitionSideY = enemy.y;
        enemy.transitionRailZ = enemy.z;
        if (enemy.z <= 0.0f) {
            enemy.z = ToRailZFromSideX(enemy.x);
        }
        if (enemy.type == 2) {
            m_stage->ConfigureBossRailAnchor(enemy);
            continue;
        }
        if (enemy.type == Stage::SquareShooterEnemy) {
            enemy.z = ToRailZFromSideX(enemy.transitionSideX);
            enemy.baseX = enemy.railAnchorX;
            enemy.x = enemy.railAnchorX;
            continue;
        }
        enemy.baseX = enemy.type == 2 ? 0.0f : (std::clamp)(enemy.baseX, -0.76f, 0.76f);
        enemy.x = enemy.baseX;
    }
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        shot.transitionSideX = shot.x;
        shot.transitionSideY = shot.y;
        if (shot.enemy && shot.barrageCount < 0) {
            constexpr int GridSize = 5;
            constexpr float SpreadSpeed = 0.010f;
            constexpr float RowScale = 1.4f;
            const int barrageCount = -shot.barrageCount;
            const int column = barrageCount == GridSize * GridSize ? shot.barrageIndex % GridSize : GridSize / 2;
            const int row = barrageCount == GridSize * GridSize ? shot.barrageIndex / GridSize : shot.barrageIndex;
            const float centerX = (std::clamp)(shot.x, -0.76f, 0.76f);
            shot.x = centerX;
            shot.z = ToRailZFromSideX(centerX);
            shot.vx = static_cast<float>(column - GridSize / 2) * SpreadSpeed;
            shot.vy = static_cast<float>(row - GridSize / 2) * SpreadSpeed * RowScale;
            shot.vz = -0.58f;
            continue;
        }
        if (shot.enemy && shot.barrageCount > 0) {
            constexpr float Radius = 0.22f;
            constexpr float RingShotSpeed = 0.58f;
            constexpr float RingSpreadSpeed = 0.010f;
            const float angle = static_cast<float>(shot.barrageIndex) * Math::TwoPi /
                static_cast<float>(shot.barrageCount);
            const float cx = std::cos(angle);
            const float sy = std::sin(angle);
            const float lineOffsetY = (static_cast<float>(shot.barrageIndex) -
                static_cast<float>(shot.barrageCount - 1) * 0.5f) * 0.075f;
            const float centerX = (std::clamp)(shot.x, -0.76f, 0.76f);
            const float centerY = shot.y - lineOffsetY;
            shot.x = centerX + cx * Radius;
            shot.y = centerY + sy * Radius;
            shot.z = ToRailZFromSideX(centerX);
            shot.vx = cx * RingSpreadSpeed;
            shot.vy = sy * RingSpreadSpeed * 1.4f;
            shot.vz = -RingShotSpeed;
            continue;
        }
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
        enemy.transitionRailZ = enemy.z;
        if (enemy.type == 2) {
            m_stage->ConfigureBossSideAnchor(enemy);
            continue;
        }
        enemy.x = ToSideXFromRailZ(enemy.z);
        enemy.baseX = enemy.x;
        enemy.z = ToRailZFromSideX(enemy.x);
    }
    for (auto& shot : m_shots) {
        if (!shot.active) continue;
        shot.transitionSideX = shot.x;
        shot.transitionSideY = shot.y;
        if (shot.enemy && shot.barrageCount < 0) {
            constexpr int GridSize = 5;
            constexpr float AimedShotSpeed = 0.018f;
            constexpr float LineSpacing = 0.075f;
            const int barrageCount = -shot.barrageCount;
            if (barrageCount == GridSize * GridSize && shot.barrageIndex % GridSize != GridSize / 2) {
                shot.active = false;
                continue;
            }
            const int row = barrageCount == GridSize * GridSize ? shot.barrageIndex / GridSize : shot.barrageIndex;
            shot.x = ToSideXFromRailZ(shot.z);
            shot.y = shot.transitionSideY + static_cast<float>(row - GridSize / 2) * LineSpacing;
            shot.z = ToRailZFromSideX(shot.x);
            shot.vx = -AimedShotSpeed;
            shot.vy = 0.0f;
            shot.vz = 0.0f;
            continue;
        }
        if (shot.enemy && shot.barrageCount > 0) {
            constexpr float Radius = 0.22f;
            constexpr float AimedShotSpeed = 0.018f;
            const float angle = static_cast<float>(shot.barrageIndex) * Math::TwoPi /
                static_cast<float>(shot.barrageCount);
            const float sy = std::sin(angle);
            const float lineOffsetY = (static_cast<float>(shot.barrageIndex) -
                static_cast<float>(shot.barrageCount - 1) * 0.5f) * 0.075f;
            shot.x = ToSideXFromRailZ(shot.z);
            shot.y = shot.y - sy * Radius + lineOffsetY;
            shot.z = ToRailZFromSideX(shot.x);
            shot.vx = -AimedShotSpeed;
            shot.vy = 0.0f;
            shot.vz = 0.0f;
            continue;
        }
        // レール奥行きの移動量を2D画面の横移動量へ変換する
        shot.x = ToSideXFromRailZ(shot.z);
        shot.z = ToRailZFromSideX(shot.x);
        shot.vx = shot.vz / 18.0f;
        shot.vz = 0.0f;
    }
}

void SideScrollingShooter::SpawnEnemy(int enemyType, float sideX, float railX, float y, float railZ) {
    constexpr float SideEnemyEntryX = 2.80f;

    for (auto& enemy : m_enemies) {
        if (enemy.active) continue;
        enemy.active = true;
        m_stage->ConfigureEnemy(*this, enemy, enemyType, m_frame, m_kills, IsRailGameplayActive());
        enemy.railAnchorX = railX;
        // 出現テーブルの座標に関わらず、敵機全体が表示領域外から入る位置に固定する
        enemy.baseX = IsRailGameplayActive() ? railX : (std::max)(sideX, SideEnemyEntryX);
        enemy.x = enemy.baseX;
        enemy.baseY = y;
        enemy.y = y;
        enemy.z = IsRailGameplayActive() ? (std::max)(railZ, EnemyRailFarZ) : ToRailZFromSideX(enemy.x);
        ++m_chapterResult.enemySpawnCount;
        return;
    }
}

void SideScrollingShooter::StartBossBattle() {
    m_bossBattle = true;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_bossIntroductionPhase = BossIntroductionPhase::Entrance;
    m_bossIntroductionTimer = 0;

    // 通常敵と敵弾を消去してボス戦へ切り替える
    for (auto& enemy : m_enemies) {
        enemy.active = false;
    }
    for (auto& shot : m_shots) {
        if (shot.enemy) shot.active = false;
    }

    Enemy& boss = m_enemies[0];
    m_stage->ConfigureBoss(boss, IsRailGameplayActive());
    m_stage->ConfigureBossPartHp(boss);
    boss.bossPartMaxHp = boss.bossPartHp;
    m_bossHp = boss.hp;
    m_displayBossHp = static_cast<float>(m_bossHp);
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
    return FindStage1Meteor(x, y, z, radius) >= 0;
}

/**
 * @brief 指定球が接触しているステージ1隕石の番号を取得する
 * @param x 判定対象のゲーム座標X
 * @param y 判定対象のゲーム座標Y
 * @param z 判定対象のレール座標Z
 * @param radius 判定対象の半径
 * @return 接触した隕石の番号、接触していない場合-1
 */
int SideScrollingShooter::FindStage1Meteor(float x, float y, float z, float radius) const {
    for (int i = 0; i < MeteorCount; ++i) {
        const Meteor& meteor = m_meteors[i];
        if (meteor.destroyed) continue;
        const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
        const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
        if (IsRailGameplayActive()) {
            const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
            const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                railX, railY, 72.0f - meteor.travel, 2.70f * meteor.scale)) {
                return i;
            }
            continue;
        }
        if (Hit(x, y, radius, sideX, sideY, 0.42f * meteor.scale)) return i;
    }
    return -1;
}

/**
 * @brief 海面からアーチ状に飛び出すウミヘビへ指定球が接触したか判定する
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
        const float bodyProgress = GetSeaSerpentSegmentProgress(
            motion.progress, motion.segmentCount, motion.segmentDelay, i);
        const float elevation = std::sin(Math::Pi * bodyProgress) *
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
            const float railY = -3.65f + (elevation < railHeight ? visibleHeight * 0.5f :
                elevation - railHeight * 0.5f);
            if (Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                railX, railY, railZ,
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
            const float sideY = -6.0f + (elevation < sideHeight ? visibleHeight * 0.5f :
                elevation - sideHeight * 0.5f);
            const float dy = (ToWorldY(y) - sideY) / hitHeight;
            if (dx * dx + dy * dy <= 1.0f) return true;
        }
    }
    return false;
}

/**
 * @brief 全ステージをクリア済みか取得する
 * @return TAYAMA崩壊と静かな飛行が完了した場合true
 */
bool SideScrollingShooter::IsAllStagesCleared() const {
    return m_stageNumber == 5 && m_stage5Phase == Stage5Phase::EndingReady;
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
        const int lifetime = explosion.destruction ?
            DestructionExplosionLifetimeFrames : ExplosionLifetimeFrames;
        if (++explosion.age >= lifetime) explosion.active = false;
    }
}

/** @brief 飛散中の機体部品を更新する */
void SideScrollingShooter::TickDebris() {
    constexpr int Stage2FirstSinkEndFrame = 108;
    constexpr int Stage2ResurfaceStartFrame = 148;
    static_assert(Stage2FirstSinkEndFrame < Stage2ResurfaceStartFrame);
    Debris* stage2LowerHull = nullptr;
    for (auto& debris : m_debris) {
        if (debris.active && debris.effect == Debris::Effect::Stage2Sink) {
            stage2LowerHull = &debris;
            break;
        }
    }
    for (auto& debris : m_debris) {
        if (!debris.active) continue;

        // Stage2下部船体は一旦沈み、最後の再浮上後に上部船体へ押し戻される
        if (debris.effect == Debris::Effect::Stage2Sink) {
            if (debris.effectAge >= 0) {
                debris.y -= 0.018f;
                ++debris.effectAge;
            } else if (debris.age < Stage2FirstSinkEndFrame) {
                debris.y -= 0.0075f;
            } else if (debris.age >= Stage2ResurfaceStartFrame) {
                const float groundTopY = Math::Lerp(-6.0f, -3.65f, RailBlend());
                const float visibleTargetY = groundTopY - debris.height * 0.12f;
                debris.y = (std::min)(visibleTargetY, debris.y + 0.026f);
            }
            if (++debris.age >= debris.lifetime) debris.active = false;
            continue;
        }

        // 船底が再浮上した下部船体の上面へ届くまで、上部船体をゆっくり降下させる
        if ((debris.effect == Debris::Effect::Stage2Impact ||
            debris.effect == Debris::Effect::Stage2ImpactPiece) && debris.effectAge < 0) {
            const bool lowerHullHit = stage2LowerHull != nullptr &&
                debris.effect == Debris::Effect::Stage2Impact &&
                stage2LowerHull->age >= Stage2ResurfaceStartFrame &&
                debris.y - debris.height * 0.5f <=
                    stage2LowerHull->y + stage2LowerHull->height * 0.5f;
            const bool collisionStarted = stage2LowerHull != nullptr && stage2LowerHull->effectAge >= 0;
            if (lowerHullHit) {
                // 船体同士が衝突した瞬間に最後の大爆発音を重ねる
                stage2LowerHull->effectAge = 0;
                PlayStage2DefeatSound(true);
            }
            if (!lowerHullHit && !collisionStarted) {
                debris.y -= 0.012f;
                debris.yaw += debris.spin * 0.18f;
                if (++debris.age >= debris.lifetime) debris.active = false;
                continue;
            }
        }

        debris.x += debris.vx;
        debris.y += debris.vy;
        debris.z += debris.vz;
        if (debris.gravity || m_stage->HasDebrisGravity()) debris.vy -= 0.006f;
        debris.yaw += debris.spin;

        // 衝突後は上部船体を大きく飛散させ、主船体の衝突時刻を爆発演出へ渡す
        if (debris.effect == Debris::Effect::Stage2Impact ||
            debris.effect == Debris::Effect::Stage2ImpactPiece) {
            if (debris.effectAge < 0) {
                debris.effectAge = 0;
                debris.vx += debris.effect == Debris::Effect::Stage2Impact ? 0.025f :
                    (debris.x < 0.0f ? -0.055f : 0.055f);
                debris.vy = 0.075f;
                debris.vz += debris.effect == Debris::Effect::Stage2Impact ? 0.0f :
                    (debris.z < 0.0f ? -0.040f : 0.040f);
                debris.spin *= 3.5f;
            }
            const float groundTopY = Math::Lerp(-6.0f, -3.65f, RailBlend());
            const float minimumY = groundTopY + debris.height * 0.5f;
            if (debris.y <= minimumY) {
                debris.y = minimumY;
                debris.vx *= 0.86f;
                debris.vy = 0.0f;
                debris.vz *= 0.86f;
                debris.spin *= 0.78f;
            }
            ++debris.effectAge;
        }
        if (++debris.age >= debris.lifetime) debris.active = false;
    }
}

void SideScrollingShooter::FireBossPartBarrage(const Enemy& boss) {
    if (m_stageNumber == 2) {
        constexpr float ModelScale = 1.92f;
        constexpr Vector3 PartPosition[] = {
            {-2.30f, 2.08f, 0.0f}, {-0.55f, 2.60f, -0.92f},
            {0.65f, 2.60f, 0.0f}, {2.85f, 2.18f, 0.92f}, {0.25f, 1.42f, 0.0f}
        };
        const bool railMode = IsRailGameplayActive();
        const float yaw = railMode ? 0.0f : Math::HalfPi;
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        const float patrolX = boss.phase >= 2.0f ?
            std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * Math::Clamp01(1.0f - yaw / Math::HalfPi) : 0.0f;
        const Vector3 battleshipPosition {
            ToWorldX(boss.x + boss.landBattleshipOffsetX) + patrolX,
            Stage2BattleshipWorldY(boss),
            boss.z + boss.landBattleshipOffsetZ
        };

        // 毎フレーム移動する上部戦艦の描画位置へローカル砲塔座標を合成する
        for (int part = 0; part < BossRightEngine; ++part) {
            if (boss.bossPartHp[part] <= 0) continue;
            const Vector3& local = PartPosition[part];
            const Vector3 world {
                battleshipPosition.x + (local.x * cosine + local.z * sine) * ModelScale,
                battleshipPosition.y + local.y * ModelScale,
                battleshipPosition.z + (-local.x * sine + local.z * cosine) * ModelScale
            };
            const int bulletCount = m_stage->BossPartBulletCount(
                static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), railMode);
            for (int index = 0; index < bulletCount; ++index) {
                const Stage::BossBullet bullet = m_stage->GetBossPartBullet(
                    static_cast<BossPart>(part), static_cast<BossPhase>(boss.bossPhase), index, railMode);
                SpawnShot(FromWorldX(world.x) + bullet.offsetX, FromWorldY(world.y) + bullet.offsetY,
                    bullet.vx, bullet.vy, true, world.z, boss.behavior->RailAimedShotSpeed());
            }
        }
        return;
    }

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
    const int nextPhase = m_stage->BossPhaseForHp(boss.hp, boss.maxHp);
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
    if (m_stageNumber == 5 && m_stage5Phase == Stage5Phase::EastsourceBattle) {
        DefeatEastsource(boss);
        return;
    }
    SpawnExplosion(boss.x, boss.y, boss.z, true);
    SpawnEnemyDebris(boss);
    boss.active = false;
    SpawnPowerItem(boss.x, boss.y, boss.z, 1.00f);
    m_bossHp = 0;
    m_score += 5000;
    m_clear = true;
    // Stage2は上下船体の沈下、再浮上、衝突、大破まで見届けてから次ステージへ進む
    m_clearTimer = m_stageNumber == 2 ? 440 : ClearWaitFrames;
    if (m_stageNumber == 2) PlayStage2DefeatSound(false);
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
    if (!m_bossStoryActive) {
        m_bossIntroductionPhase = BossIntroductionPhase::NameReveal;
        m_bossIntroductionTimer = 0;
    }
}

/**
 * @brief 現在状態をStage 5チェックポイントとして保存する
 * @param checkpoint 保存するチェックポイント
 * @return なし
 */
void SideScrollingShooter::SaveStage5Checkpoint(Stage5Checkpoint checkpoint) {
    m_stage5Checkpoint = checkpoint;
    m_stage5CheckpointPower = m_power;
    m_stage5CheckpointScore = m_score;
    m_stage5CheckpointKills = m_kills;
}

/**
 * @brief Stage 5状態を開始する
 * @param phase 開始する状態
 * @param saveCheckpoint 復帰地点として保存する場合true
 * @return なし
 */
void SideScrollingShooter::StartStage5Phase(Stage5Phase phase, bool saveCheckpoint) {
    m_stage5Phase = phase;
    m_stage5PhaseTimer = 0;
    m_stage5AttackTimer = 0;
    m_stage5CoreTargetX = m_playerX;
    m_stage5CoreTargetY = m_playerY;
    m_stage5GuardSpawnCooldown = 0;

    if (phase != Stage5Phase::Approach) RequestViewMode(ViewMode::Rail3D);

    // TAYAMA攻略は専用初期化へ委譲する
    if (phase == Stage5Phase::TayamaFireControl ||
        phase == Stage5Phase::TayamaLiftEngines ||
        phase == Stage5Phase::TayamaCommandCore) {
        StartTayamaPhase(phase, true);
        return;
    }

    if (phase == Stage5Phase::EastsourceIntro) {
        m_shots = {};
        m_enemies = {};
        m_items = {};
        m_bossBattle = true;
        Enemy& eastsource = m_enemies[0];
        m_stage->ConfigureBoss(eastsource, true);
        m_stage->ConfigureBossPartHp(eastsource);
        eastsource.bossPartMaxHp = eastsource.bossPartHp;
        eastsource.x = 1.45f;
        eastsource.y = 0.30f;
        eastsource.z = 66.0f;
        eastsource.collisionEnabled = false;
        m_bossHp = eastsource.hp;
        m_displayBossHp = static_cast<float>(m_bossHp);
        if (saveCheckpoint) SaveStage5Checkpoint(Stage5Checkpoint::Eastsource);
        PlayStage5Cue(Stage5EastsourceEntrance);
        return;
    }
    if (phase == Stage5Phase::EastsourceBattle) {
        StartEastsourceBattle();
        if (saveCheckpoint) SaveStage5Checkpoint(Stage5Checkpoint::Eastsource);
        return;
    }
    if (phase == Stage5Phase::EastsourceFall) {
        m_bossBattle = false;
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
        PlayStage5Cue(Stage5SignalLost);
        return;
    }

    // 壁面区画へ入るたび、その区画固有のライトを初期化する
    if (phase >= Stage5Phase::WallClimbTransition && phase <= Stage5Phase::CarrierTransformation) {
        m_bossBattle = false;
        for (auto& enemy : m_enemies) enemy.active = false;
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
        int lightCount = 0;
        Stage5Checkpoint checkpoint = Stage5Checkpoint::WallClimbLower;
        if (phase == Stage5Phase::WallClimbLower) {
            lightCount = 1;
            m_tayamaTransformation = 0.10f;
        } else if (phase == Stage5Phase::WallClimbMiddle) {
            lightCount = 2;
            checkpoint = Stage5Checkpoint::WallClimbMiddle;
            m_tayamaTransformation = 0.34f;
        } else if (phase == Stage5Phase::WallClimbUpper) {
            lightCount = 3;
            checkpoint = Stage5Checkpoint::WallClimbUpper;
            m_tayamaTransformation = 0.64f;
        } else if (phase == Stage5Phase::RooftopArrival) {
            m_tayamaTransformation = 0.90f;
        } else if (phase == Stage5Phase::CarrierTransformation) {
            m_tayamaTransformation = 0.96f;
            PlayStage5Cue(Stage5Transformation);
        }
        ResetWallSearchlights(lightCount);
        if (saveCheckpoint && lightCount > 0) SaveStage5Checkpoint(checkpoint);
        m_invincible = (std::max)(m_invincible, 60);
        return;
    }

    if (phase == Stage5Phase::TayamaCollapse) {
        m_bossBattle = false;
        m_bossHp = 0;
        m_displayBossHp = 0.0f;
        for (auto& enemy : m_enemies) enemy.active = false;
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
        m_invincible = TayamaCollapseFrames + Stage5QuietFlightFrames + 60;
        PlayStage5Cue(Stage5ChainExplosion);
        return;
    }

    if (phase == Stage5Phase::EndingReady) {
        m_clear = true;
        m_clearTimer = 0;
    }
}

/**
 * @brief EASTSOURCE戦を戦闘可能な状態で開始する
 * @return なし
 */
void SideScrollingShooter::StartEastsourceBattle() {
    m_bossBattle = true;
    Enemy& eastsource = m_enemies[0];
    if (!eastsource.active || eastsource.type != Stage::BossEnemy) {
        eastsource = {};
        m_stage->ConfigureBoss(eastsource, true);
        m_stage->ConfigureBossPartHp(eastsource);
        eastsource.bossPartMaxHp = eastsource.bossPartHp;
    }
    eastsource.collisionEnabled = true;
    eastsource.age = 0;
    eastsource.motionAge = 0;
    eastsource.bossPhase = BossNormalPhase1;
    eastsource.x = 0.42f;
    eastsource.y = 0.0f;
    eastsource.z = 45.0f;
    eastsource.baseX = eastsource.x;
    eastsource.baseY = eastsource.y;
    eastsource.baseZ = eastsource.z;
    m_bossHp = eastsource.hp;
    m_displayBossHp = static_cast<float>(m_bossHp);
    m_invincible = (std::max)(m_invincible, 60);
    ResetWallSearchlights(1);
}

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
void SideScrollingShooter::SpawnEnemyShotAt(float sourceX, float sourceY, float sourceZ,
    float targetX, float targetY, float targetZ, float speed) {
    const float dx = ToWorldX(targetX - sourceX);
    const float dy = ToWorldY(targetY - sourceY);
    const float dz = targetZ - sourceZ;
    const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
    SpawnShotDirect(sourceX, sourceY, sourceZ,
        FromWorldX(dx / length * speed), FromWorldY(dy / length * speed),
        dz / length * speed, true);
}

/**
 * @brief EASTSOURCEの移動と攻撃を更新する
 * @param eastsource 更新するEASTSOURCE
 * @return なし
 */
void SideScrollingShooter::TickEastsource(Enemy& eastsource) {
    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        const float progress = SmoothStep(static_cast<float>(m_stage5PhaseTimer) / EastsourceIntroFrames);
        eastsource.x = Math::Lerp(1.45f, 0.42f, progress);
        eastsource.y = Math::Lerp(0.30f, 0.0f, progress) + std::sin(progress * Math::Pi * 5.0f) * 0.08f;
        eastsource.z = Math::Lerp(66.0f, 45.0f, progress);
        eastsource.collisionEnabled = false;
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceFall) {
        eastsource.collisionEnabled = false;
        eastsource.x -= 0.004f;
        eastsource.y -= 0.018f + static_cast<float>(m_stage5PhaseTimer) * 0.00008f;
        eastsource.z += 0.08f;
        return;
    }
    if (m_stage5Phase != Stage5Phase::EastsourceBattle) return;

    const bool leftEngine = eastsource.bossPartHp[BossLeftEngine] > 0;
    const bool rightEngine = eastsource.bossPartHp[BossRightEngine] > 0;
    const float engineRate = (static_cast<float>(leftEngine) + static_cast<float>(rightEngine)) * 0.5f;
    const float moveAmplitude = 0.20f + engineRate * 0.38f;
    const float moveSpeed = 0.009f + engineRate * 0.009f;
    eastsource.x = std::sin(static_cast<float>(eastsource.age) * moveSpeed) * moveAmplitude;
    eastsource.y = std::sin(static_cast<float>(eastsource.age) * 0.021f) * 0.42f;
    eastsource.z = 43.0f + std::sin(static_cast<float>(eastsource.age) * 0.013f) * 3.0f;

    const bool nose = eastsource.bossPartHp[BossNose] > 0;
    const int phase = eastsource.bossPhase;
    if (phase == BossNormalPhase2 || phase == BossSpecialPhase2) {
        const int pursuitCycle = eastsource.age % 180;
        const bool fromLeft = (eastsource.age / 180) % 2 == 0;
        const BossPart wingPart = fromLeft ? BossLeftWing : BossRightWing;
        const BossPart enginePart = fromLeft ? BossLeftEngine : BossRightEngine;
        const bool wing = eastsource.bossPartHp[wingPart] > 0;
        const bool engine = eastsource.bossPartHp[enginePart] > 0;
        const float side = fromLeft ? -1.0f : 1.0f;
        const int passEnd = 112 + (engine ? 36 : 52);

        // 索敵中は遠ざかり、予告後だけ画面外から固定方向へ高速再進入する
        if (!wing) {
            eastsource.collisionEnabled = true;
        } else if (pursuitCycle < 82) {
            eastsource.z = Math::Lerp(43.0f, 59.0f,
                SmoothStep(static_cast<float>(pursuitCycle) / 82.0f));
        } else if (pursuitCycle < 112) {
            eastsource.x = side * 1.48f;
            eastsource.z = 59.0f;
            eastsource.collisionEnabled = false;
        } else if (pursuitCycle < passEnd) {
            const float pass = SmoothStep(static_cast<float>(pursuitCycle - 112) / (engine ? 36.0f : 52.0f));
            eastsource.x = Math::Lerp(side * 1.48f, -side * 1.48f, pass);
            eastsource.y = eastsource.attackWarningTargetY;
            eastsource.z = Math::Lerp(31.0f, 19.0f, std::sin(pass * Math::Pi));
            eastsource.collisionEnabled = false;
        } else {
            const float settle = SmoothStep(static_cast<float>(pursuitCycle - passEnd) /
                static_cast<float>(180 - passEnd));
            eastsource.x = Math::Lerp(-side * 1.48f, 0.0f, settle);
            eastsource.z = Math::Lerp(28.0f, 43.0f, settle);
            eastsource.collisionEnabled = settle > 0.55f;
        }
    } else {
        eastsource.collisionEnabled = true;
    }
    if (phase == BossNormalPhase1 || phase == BossSpecialPhase2) {
        const int cycleLength = nose ? 118 : 148;
        const int cycle = eastsource.age % cycleLength;
        const int warningFrames = nose ? 34 : 54;
        if (cycle == 0) {
            const float error = nose ? 0.0f : std::sin(static_cast<float>(eastsource.age) * 0.37f) * 0.24f;
            eastsource.attackWarningTargetX = m_playerX + error;
            eastsource.attackWarningTargetY = m_playerY - error * 0.45f;
            eastsource.attackWarningFrames = warningFrames;
            PlayStage5Cue(Stage5BarrageWarning);
        }
        const int shotCount = nose ? 3 : 1;
        for (int shotIndex = 0; shotIndex < shotCount; ++shotIndex) {
            if (cycle == warningFrames + shotIndex * 10) {
                SpawnEnemyShotAt(eastsource.x, eastsource.y, eastsource.z,
                    eastsource.attackWarningTargetX, eastsource.attackWarningTargetY,
                    PlayerRailZ, 0.72f);
            }
        }
    }

    if (phase == BossSpecialPhase1 || phase == BossSpecialPhase2) {
        const int cycle = eastsource.age % 96;
        if (cycle == 22 && eastsource.bossPartHp[BossLeftWing] > 0) {
            for (int lane = -2; lane <= 2; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(eastsource.x - 0.42f, eastsource.y + 0.12f, eastsource.z,
                    m_playerX + 0.25f, static_cast<float>(lane) * 0.25f, PlayerRailZ, 0.64f);
            }
        }
        if (cycle == 48 && eastsource.bossPartHp[BossRightWing] > 0) {
            for (int lane = -2; lane <= 2; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(eastsource.x + 0.42f, eastsource.y - 0.12f, eastsource.z,
                    m_playerX - 0.25f, static_cast<float>(lane) * 0.25f, PlayerRailZ, 0.64f);
            }
        }
    }

    if (phase == BossNormalPhase2 || phase == BossSpecialPhase2) {
        const int cycle = eastsource.age % 180;
        if (cycle == 92) {
            eastsource.attackWarningTargetX = m_playerX;
            eastsource.attackWarningTargetY = m_playerY;
            eastsource.attackWarningFrames = 30;
        }
        if (cycle >= 120 && cycle < 138) {
            const bool fromLeft = (eastsource.age / 180) % 2 == 0;
            const bool wing = eastsource.bossPartHp[fromLeft ? BossLeftWing : BossRightWing] > 0;
            const bool engine = eastsource.bossPartHp[fromLeft ? BossLeftEngine : BossRightEngine] > 0;
            if (wing && cycle % (engine ? 4 : 7) == 0) {
                const float sourceX = fromLeft ? -1.25f : 1.25f;
                SpawnEnemyShotAt(sourceX, eastsource.attackWarningTargetY, 18.0f,
                    eastsource.attackWarningTargetX, eastsource.attackWarningTargetY,
                    PlayerRailZ, engine ? 0.82f : 0.58f);
            }
        }
    }
}

/**
 * @brief EASTSOURCE撃破後の信号消失演出へ移行する
 * @param eastsource 撃破状態へ移行するEASTSOURCE
 * @return なし
 */
void SideScrollingShooter::DefeatEastsource(Enemy& eastsource) {
    if (m_stage5Phase != Stage5Phase::EastsourceBattle) return;
    eastsource.hp = 0;
    eastsource.collisionEnabled = false;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_score += 5000;
    const BossPart detachedPart = eastsource.bossPartHp[BossLeftWing] > 0 ? BossLeftWing :
        (eastsource.bossPartHp[BossRightWing] > 0 ? BossRightWing :
            (eastsource.bossPartHp[BossLeftEngine] > 0 ? BossLeftEngine : BossRightEngine));
    SpawnEnemyDebris(eastsource, detachedPart);
    eastsource.bossPartHp[detachedPart] = 0;
    StartStage5Phase(Stage5Phase::EastsourceFall, false);
}

/**
 * @brief 壁面区画のサーチライトを初期化する
 * @param activeCount 有効化するサーチライト数
 * @return なし
 */
void SideScrollingShooter::ResetWallSearchlights(int activeCount) {
    m_searchlights = {};
    for (int i = 0; i < Stage5SearchlightCount; ++i) {
        SearchlightState& light = m_searchlights[i];
        light.beamX = (static_cast<float>(i) - 1.0f) * 0.62f;
        light.beamY = i % 2 == 0 ? 0.34f : -0.28f;
        light.hp = i < activeCount ? 90 : 0;
        light.destroyed = i >= activeCount;
        light.timer = i * 27;
    }
}

/**
 * @brief サーチライトの保存済み地点へ集中砲火を生成する
 * @param light 集中砲火に使用するサーチライト状態
 * @param lightIndex 発射元を決めるサーチライト番号
 * @return なし
 */
void SideScrollingShooter::FireSearchlightVolley(const SearchlightState& light, int lightIndex) {
    float sourceX = (static_cast<float>(lightIndex) - 1.0f) * 0.72f;
    float sourceY = 0.72f - static_cast<float>(lightIndex) * 0.22f;
    float sourceZ = 46.0f;
    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        const TayamaPartGroup group = lightIndex == 0 ?
            TayamaPartGroup::LeftSearchlight : TayamaPartGroup::RightSearchlight;
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
            TayamaTransform(), m_tayamaTransformation, TayamaState(), group);
        if (bounds.valid) {
            sourceX = FromWorldX(bounds.center.x);
            sourceY = FromWorldY(bounds.center.y);
            sourceZ = bounds.center.z;
        }
    }
    for (int bullet = -2; bullet <= 2; ++bullet) {
        const float spread = static_cast<float>(bullet) * 0.065f;
        SpawnEnemyShotAt(sourceX, sourceY, sourceZ,
            light.lockedX + spread, light.lockedY + std::abs(spread) * 0.35f,
            PlayerRailZ, 0.78f);
    }
}

/**
 * @brief 指定数のサーチライトを更新する
 * @param activeCount 更新するライト数
 * @param tayamaWeakpoints TAYAMA弱点と破壊状態を共有する場合true
 * @return なし
 */
void SideScrollingShooter::TickSearchlights(int activeCount, bool tayamaWeakpoints) {
    const bool radarDestroyed = tayamaWeakpoints &&
        m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed;
    const int difficultyOffset = m_difficulty == Easy ? 9 : (m_difficulty == Hard ? -9 : 0);
    const int lockFrames = SearchlightLockFrames + difficultyOffset + (radarDestroyed ? 18 : 0);

    for (int i = 0; i < activeCount; ++i) {
        SearchlightState& light = m_searchlights[i];
        if (tayamaWeakpoints) {
            const TayamaWeakpoint type = i == 0 ? TayamaWeakpoint::LeftSearchlight :
                TayamaWeakpoint::RightSearchlight;
            light.destroyed = m_tayamaWeakpoints[static_cast<int>(type)].destroyed;
        }
        if (light.destroyed) continue;

        const float scanTargetX = std::sin(static_cast<float>(m_stage5PhaseTimer + i * 67) *
            (0.018f + static_cast<float>(i) * 0.002f)) * 0.92f;
        const float scanTargetY = std::sin(static_cast<float>(m_stage5PhaseTimer + i * 43) *
            (0.013f + static_cast<float>(i) * 0.003f)) * 0.66f;
        if (light.phase == SearchlightPhase::Searching || light.phase == SearchlightPhase::Detecting) {
            const float trackingLimit = radarDestroyed ? 0.010f : 0.016f;
            light.beamX = MoveTowards(light.beamX, scanTargetX, trackingLimit);
            light.beamY = MoveTowards(light.beamY, scanTargetY, trackingLimit * 0.78f);
        }

        const bool illuminated = Hit(m_playerX, m_playerY, 0.055f,
            light.beamX, light.beamY, SearchlightDetectionRadius);
        if (light.phase == SearchlightPhase::Searching) {
            if (illuminated) {
                light.phase = SearchlightPhase::Detecting;
                light.detectionFrames = 1;
                PlayStage5Cue(Stage5SearchlightDetect);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Detecting) {
            light.detectionFrames = illuminated ? light.detectionFrames + 1 :
                (std::max)(0, light.detectionFrames - 2);
            if (light.detectionFrames == 0) {
                light.phase = SearchlightPhase::Searching;
            } else if (light.detectionFrames >= lockFrames) {
                light.lockedX = m_playerX;
                light.lockedY = m_playerY;
                light.phase = SearchlightPhase::Locked;
                light.timer = SearchlightWarningFrames;
                PlayStage5Cue(Stage5SearchlightLocked);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Locked) {
            if (--light.timer <= 0) {
                light.phase = SearchlightPhase::Firing;
                light.timer = 0;
                light.volley = 0;
                PlayStage5Cue(Stage5BarrageWarning);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Firing) {
            if (light.timer % SearchlightVolleyIntervalFrames == 0 &&
                light.volley < SearchlightVolleyCount) {
                FireSearchlightVolley(light, i);
                ++light.volley;
            }
            ++light.timer;
            if (light.volley >= SearchlightVolleyCount &&
                light.timer >= SearchlightVolleyIntervalFrames * SearchlightVolleyCount) {
                light.phase = SearchlightPhase::Cooldown;
                light.timer = 90 + i * 24 + (m_difficulty == Easy ? 30 : 0);
                light.detectionFrames = 0;
            }
            continue;
        }
        if (--light.timer <= 0) light.phase = SearchlightPhase::Searching;
    }
}

/**
 * @brief EASTSOURCEの描画と当たり判定で共有する親Transformを取得する
 * @param eastsource EASTSOURCE本体
 * @return ワールド座標へ変換する親Transform
 */
Stage5ModelTransform SideScrollingShooter::EastsourceTransform(const Enemy& eastsource) const {
    const float fallRoll = m_stage5Phase == Stage5Phase::EastsourceFall ?
        static_cast<float>(m_stage5PhaseTimer) * 0.035f : 0.0f;
    return {{ToWorldX(eastsource.x), ToWorldY(eastsource.y), eastsource.z},
        {0.0f, Math::Lerp(Math::HalfPi, 0.0f, RailBlend()), fallRoll}, 0.72f};
}

/**
 * @brief EASTSOURCEの部位状態をモデルグループへ変換する
 * @param eastsource EASTSOURCE本体
 * @return 描画と当たり判定へ渡すモデル状態
 */
EastsourceModelState SideScrollingShooter::EastsourceState(const Enemy& eastsource) const {
    EastsourceModelState state;
    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };

    // 既存BossPartの固定順をEASTSOURCEのモデルグループへ一度だけ写す
    for (int part = BossNose; part <= BossRightEngine; ++part) {
        const std::size_t group = static_cast<std::size_t>(Groups[part]);
        state.destroyed[group] = eastsource.bossPartHp[part] <= 0;
        const int flash = eastsource.bossPartHitFlashFrames[part];
        state.hitFlash[group] = flash > 0 && (flash / 2) % 2 != 0;
    }
    return state;
}

/**
 * @brief TAYAMAの描画と当たり判定で共有する親Transformを取得する
 * @return 現在の進行に対応する親Transform
 */
Stage5ModelTransform SideScrollingShooter::TayamaTransform() const {
    float y = 3.0f;
    float z = 57.0f;
    float scale = 1.34f;
    float pitch = 0.0f;
    float roll = 0.0f;

    // 通常チャプターでは遠景の都市構造として見せ、後半ほど接近させる
    if (m_stage5Phase == Stage5Phase::Approach) {
        const int chapterLength = m_stage != nullptr ? m_stage->ChapterFrameLength() : ChapterLengthFrames;
        const float chapterProgress = Math::Clamp01(
            static_cast<float>(m_frame - (m_chapterNumber - 1) * chapterLength) /
            static_cast<float>(chapterLength));
        y = -9.0f + static_cast<float>(m_chapterNumber - 1) * 2.4f + chapterProgress * 1.2f;
        z = 88.0f - static_cast<float>(m_chapterNumber - 1) * 6.0f;
        scale = 0.82f + static_cast<float>(m_chapterNumber - 1) * 0.12f;
    } else if (m_stage5Phase <= Stage5Phase::EastsourceFall) {
        const float approach = m_stage5Phase == Stage5Phase::EastsourceIntro ?
            SmoothStep(Math::Clamp01(static_cast<float>(m_stage5PhaseTimer) / EastsourceIntroFrames)) : 1.0f;
        y = Math::Lerp(-3.0f, -1.5f, approach);
        z = Math::Lerp(76.0f, 59.0f, approach);
        scale = Math::Lerp(1.06f, 1.42f, approach);
    } else if (m_stage5Phase <= Stage5Phase::WallClimbUpper) {
        y = -1.5f + m_tayamaTransformation * 4.0f;
        z = 59.0f;
        scale = 1.42f;
    } else if (m_stage5Phase == Stage5Phase::TayamaCollapse) {
        z = 57.0f + static_cast<float>((std::min)(m_stage5PhaseTimer, 450)) * 0.004f;
        scale = 1.34f + static_cast<float>((std::max)(0, m_stage5PhaseTimer - 330)) * 0.0006f;
    }
    if (m_stage5Phase == Stage5Phase::RooftopArrival) {
        const float arrival = SmoothStep(Math::Clamp01(
            static_cast<float>(m_stage5PhaseTimer) / RooftopArrivalFrames));
        pitch = Math::Lerp(0.0f, 0.58f, arrival);
        y = Math::Lerp(2.1f, 0.0f, arrival);
        z = Math::Lerp(59.0f, 57.0f, arrival);
        scale = Math::Lerp(1.42f, 1.08f, arrival);
    } else if (m_stage5Phase >= Stage5Phase::CarrierTransformation) {
        pitch = 0.58f;
        y = 0.0f;
        scale = m_stage5Phase == Stage5Phase::TayamaCollapse ?
            1.08f + static_cast<float>((std::max)(0, m_stage5PhaseTimer - 330)) * 0.0006f : 1.08f;
    }
    if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        y = Math::Lerp(0.0f, 1.8f, SmoothStep(Math::Clamp01(
            static_cast<float>(m_stage5PhaseTimer) / 180.0f)));
    } else if (m_stage5Phase == Stage5Phase::TayamaCollapse) {
        y = 1.8f - static_cast<float>((std::min)(m_stage5PhaseTimer, 450)) * 0.002f;
    }

    // 揚力エンジンの片側破壊を艦体ロールへ反映するが入力軸は回転させない
    if (m_stage5Phase >= Stage5Phase::TayamaLiftEngines) {
        const bool left = m_tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed;
        const bool right = m_tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed;
        if (left != right) roll = left ? -0.12f : 0.12f;
    }
    return {{0.0f, y, z}, {pitch, 0.0f, roll}, scale};
}

/**
 * @brief 現在の弱点と崩壊状態をTAYAMAモデルグループへ変換する
 * @return 描画と当たり判定へ渡すモデル状態
 */
TayamaModelState SideScrollingShooter::TayamaState() const {
    TayamaModelState state;
    if (m_stage5Phase == Stage5Phase::EndingReady) {
        state.visible.fill(false);
        return state;
    }
    constexpr TayamaPartGroup Groups[] = {
        TayamaPartGroup::LeftSearchlight,
        TayamaPartGroup::RightSearchlight,
        TayamaPartGroup::FireControlRadar,
        TayamaPartGroup::LeftLiftEngine,
        TayamaPartGroup::RightLiftEngine,
        TayamaPartGroup::CommandCore
    };

    // 弱点の破壊と点滅を同名モデルグループへ反映する
    for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        const std::size_t index = static_cast<std::size_t>(weakpoint.type);
        const std::size_t group = static_cast<std::size_t>(Groups[index]);
        state.destroyed[group] = weakpoint.destroyed;
        state.hitFlash[group] = weakpoint.hitFlashFrames > 0 &&
            (weakpoint.hitFlashFrames / 2) % 2 != 0;
    }
    state.visible[static_cast<std::size_t>(TayamaPartGroup::CommandCore)] =
        m_stage5Phase >= Stage5Phase::TayamaCommandCore;
    if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        state.visible[static_cast<std::size_t>(TayamaPartGroup::ArmorPanel)] = false;
    }

    if (m_stage5Phase != Stage5Phase::TayamaCollapse) return state;

    // 大型構造はDebrisへ分解せずグループ単位のTransformで画面内崩壊させる
    const float deckFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 180) / 270.0f);
    const float bridgeFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 300) / 190.0f);
    const float engineFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 60) / 180.0f);
    const float armorFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 210) / 210.0f);
    auto SetOffset = [&](TayamaPartGroup group, const Vector3& position,
        const Vector3& rotation, float scale) {
        state.collapseOffsets[static_cast<std::size_t>(group)] =
            {position, rotation, {scale, scale, scale}};
    };
    SetOffset(TayamaPartGroup::LeftFlightDeck,
        {-deckFall * 12.0f, -deckFall * 13.0f, deckFall * 3.0f},
        {deckFall * 0.18f, 0.0f, deckFall * 1.05f}, 1.0f - deckFall * 0.22f);
    SetOffset(TayamaPartGroup::RightFlightDeck,
        {deckFall * 12.0f, -deckFall * 14.0f, deckFall * 2.0f},
        {-deckFall * 0.16f, 0.0f, -deckFall * 1.12f}, 1.0f - deckFall * 0.22f);
    SetOffset(TayamaPartGroup::Bridge,
        {bridgeFall * 2.0f, -bridgeFall * 10.0f, -bridgeFall * 2.0f},
        {bridgeFall * 0.32f, bridgeFall * 0.24f, bridgeFall * 0.82f}, 1.0f - bridgeFall * 0.30f);
    SetOffset(TayamaPartGroup::LeftLiftEngine,
        {-engineFall * 3.0f, -engineFall * 16.0f, engineFall * 2.0f},
        {0.0f, engineFall * 0.35f, engineFall * 0.55f}, 1.0f - engineFall * 0.35f);
    SetOffset(TayamaPartGroup::RightLiftEngine,
        {engineFall * 3.0f, -engineFall * 16.0f, engineFall * 2.0f},
        {0.0f, -engineFall * 0.35f, -engineFall * 0.55f}, 1.0f - engineFall * 0.35f);
    SetOffset(TayamaPartGroup::ArmorPanel,
        {0.0f, -armorFall * 9.0f, armorFall * 4.0f},
        {armorFall * 0.24f, armorFall * 0.45f, 0.0f}, 1.0f - armorFall * 0.32f);
    SetOffset(TayamaPartGroup::Hangar,
        {0.0f, -armorFall * 11.0f, -armorFall * 3.0f},
        {-armorFall * 0.22f, -armorFall * 0.30f, armorFall * 0.20f}, 1.0f - armorFall * 0.34f);

    // 最終爆発後は全グループを消し、60フレームの静かな飛行だけを残す
    if (m_stage5PhaseTimer >= TayamaCollapseFrames) state.visible.fill(false);
    return state;
}

/**
 * @brief 自機弾を壁面サーチライトへ適用する
 * @param shot 判定する自機弾
 * @return ライトへ命中した場合true
 */
bool SideScrollingShooter::TryDamageWallSearchlight(Shot& shot) {
    if (m_stage5Phase < Stage5Phase::WallClimbLower ||
        m_stage5Phase > Stage5Phase::WallClimbUpper) return false;
    const int activeCount = m_stage5Phase == Stage5Phase::WallClimbLower ? 1 :
        (m_stage5Phase == Stage5Phase::WallClimbMiddle ? 2 : 3);

    // 描画する壁面ライト基部と同じ固定配置へ線分判定する
    for (int index = 0; index < activeCount; ++index) {
        SearchlightState& light = m_searchlights[index];
        if (light.destroyed) continue;
        const float sourceX = (static_cast<float>(index) - 1.0f) * 0.72f;
        const float sourceY = 0.72f - static_cast<float>(index) * 0.22f;
        if (!Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            ToWorldX(sourceX), ToWorldY(sourceY), 46.0f, 0.72f)) continue;

        if (!shot.piercing) shot.active = false;
        light.hp -= shot.damage;
        SpawnExplosion(sourceX, sourceY, 46.0f, light.hp <= 0);
        if (light.hp <= 0) {
            light.hp = 0;
            light.destroyed = true;
            light.phase = SearchlightPhase::Cooldown;
            m_score += 300;
            PlayStage5Cue(Stage5WeakpointDestroyed);
        } else {
            PlayHitSound();
        }
        return true;
    }
    return false;
}

/**
 * @brief 自機弾をTAYAMAの有効弱点へ適用する
 * @param shot 判定する自機弾
 * @return TAYAMAへ命中した場合true
 */
bool SideScrollingShooter::TryDamageTayama(Shot& shot) {
    if (m_stage5Phase < Stage5Phase::TayamaFireControl ||
        m_stage5Phase > Stage5Phase::TayamaCommandCore) return false;
    const Stage5ModelTransform transform = TayamaTransform();
    const TayamaModelState modelState = TayamaState();
    constexpr TayamaPartGroup Groups[] = {
        TayamaPartGroup::LeftSearchlight,
        TayamaPartGroup::RightSearchlight,
        TayamaPartGroup::FireControlRadar,
        TayamaPartGroup::LeftLiftEngine,
        TayamaPartGroup::RightLiftEngine,
        TayamaPartGroup::CommandCore
    };
    const Vector3 shotStart {ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz};
    const Vector3 shotEnd {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};

    // 現フェーズの弱点だけを先に判定し、後続フェーズへの先行ダメージを防ぐ
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (!weakpoint.active || weakpoint.destroyed) continue;
        const TayamaPartGroup group = Groups[static_cast<std::size_t>(weakpoint.type)];
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
            transform, m_tayamaTransformation, modelState, group);
        if (!bounds.valid || !Hit3DSegment(shotStart.x, shotStart.y, shotStart.z,
            shotEnd.x, shotEnd.y, shotEnd.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;

        if (!shot.piercing) shot.active = false;
        weakpoint.hp -= shot.damage;
        weakpoint.hitFlashFrames = BossPartHitFlashFrames;
        SpawnExplosion(FromWorldX(bounds.center.x), FromWorldY(bounds.center.y), bounds.center.z,
            weakpoint.hp <= 0);
        if (weakpoint.hp <= 0) {
            weakpoint.hp = 0;
            weakpoint.destroyed = true;
            m_score += weakpoint.type == TayamaWeakpoint::CommandCore ? 5000 : 750;
            if (weakpoint.type == TayamaWeakpoint::LeftSearchlight) m_searchlights[0].destroyed = true;
            if (weakpoint.type == TayamaWeakpoint::RightSearchlight) m_searchlights[1].destroyed = true;
            PlayStage5Cue(Stage5WeakpointDestroyed);
        } else {
            PlayHitSound();
        }

        bool phaseComplete = true;
        for (const TayamaWeakpointState& current : m_tayamaWeakpoints) {
            if (IsTayamaWeakpointActiveForPhase(current.type, m_stage5Phase) && !current.destroyed) {
                phaseComplete = false;
                break;
            }
        }
        if (phaseComplete) {
            if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
                StartTayamaPhase(Stage5Phase::TayamaLiftEngines);
            } else if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
                StartTayamaPhase(Stage5Phase::TayamaCommandCore);
            } else {
                StartStage5Phase(Stage5Phase::TayamaCollapse, false);
            }
        } else {
            UpdateTayamaBossHp();
        }
        return true;
    }

    // 無効な装甲への命中は小さな着弾だけを出し、HPとスコアを変えない
    for (int group = 0; group < static_cast<int>(TayamaPartGroup::Count); ++group) {
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
            m_tayamaTransformation, modelState, static_cast<TayamaPartGroup>(group));
        if (!bounds.valid || !Hit3DSegment(shotStart.x, shotStart.y, shotStart.z,
            shotEnd.x, shotEnd.y, shotEnd.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;
        if (!shot.piercing) shot.active = false;
        SpawnExplosion(shot.x, shot.y, shot.z);
        PlayHitSound();
        return true;
    }
    return false;
}

/**
 * @brief 現在フェーズで有効なTAYAMA弱点HP合計をHUDへ反映する
 * @return なし
 */
void SideScrollingShooter::UpdateTayamaBossHp() {
    int currentHp = 0;
    int currentMaxHp = 0;
    for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (!IsTayamaWeakpointActiveForPhase(weakpoint.type, m_stage5Phase)) continue;
        currentHp += (std::max)(0, weakpoint.hp);
        currentMaxHp += weakpoint.maxHp;
    }
    m_bossHp = currentHp;
    if (m_displayBossHp <= 0.0f || m_displayBossHp > static_cast<float>(currentMaxHp)) {
        m_displayBossHp = static_cast<float>(currentHp);
    }
}

/**
 * @brief TAYAMAの攻略フェーズを開始する
 * @param phase 開始する攻略状態
 * @param resetCurrentHp 現在フェーズのHPを初期値へ戻す場合true
 * @return なし
 */
void SideScrollingShooter::StartTayamaPhase(Stage5Phase phase, bool resetCurrentHp) {
    m_stage5Phase = phase;
    m_stage5PhaseTimer = 0;
    m_stage5AttackTimer = 0;
    m_stage5CoreTargetX = m_playerX;
    m_stage5CoreTargetY = m_playerY;
    m_bossBattle = false;
    m_tayamaTransformation = 1.0f;
    for (auto& enemy : m_enemies) enemy.active = false;
    for (auto& shot : m_shots) {
        if (shot.enemy) shot.active = false;
    }

    // 前フェーズの破壊結果を維持し、現在フェーズだけを有効化する
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        weakpoint.active = IsTayamaWeakpointActiveForPhase(weakpoint.type, phase) && !weakpoint.destroyed;
        if (weakpoint.active && resetCurrentHp) weakpoint.hp = weakpoint.maxHp;
        weakpoint.hitFlashFrames = 0;
    }
    if (phase == Stage5Phase::TayamaFireControl) {
        ResetWallSearchlights(2);
        SaveStage5Checkpoint(Stage5Checkpoint::TayamaFireControl);
    } else if (phase == Stage5Phase::TayamaLiftEngines) {
        SaveStage5Checkpoint(Stage5Checkpoint::TayamaLiftEngines);
    } else {
        SaveStage5Checkpoint(Stage5Checkpoint::TayamaCommandCore);
        PlayStage5Cue(Stage5CoreWarning);
    }
    UpdateTayamaBossHp();
    m_displayBossHp = static_cast<float>(m_bossHp);
    m_invincible = (std::max)(m_invincible, 75);
}

/**
 * @brief TAYAMA戦の更新処理
 * @return なし
 */
void SideScrollingShooter::TickTayama() {
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (weakpoint.hitFlashFrames > 0) --weakpoint.hitFlashFrames;
    }
    ++m_stage5AttackTimer;
    m_stage5GuardSpawnCooldown = (std::max)(0, m_stage5GuardSpawnCooldown - 1);

    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        TickSearchlights(2, true);
        if (m_stage5AttackTimer % 120 == 72) {
            for (int lane = -3; lane <= 3; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(static_cast<float>(lane) * 0.18f, 0.62f, 58.0f,
                    m_playerX + static_cast<float>(lane) * 0.10f,
                    m_playerY + static_cast<float>(lane) * 0.09f, PlayerRailZ, 0.60f);
            }
        }
        if (m_stage5GuardSpawnCooldown == 0 && m_stage5PhaseTimer > 120) {
            SpawnEnemy(Stage::ArmoredEnemy, 1.16f, (m_stage5PhaseTimer / 240) % 2 == 0 ? -0.72f : 0.72f,
                0.58f, 60.0f);
            m_stage5GuardSpawnCooldown = 300;
        }
        const int sweepCycle = m_stage5AttackTimer % 210;
        if (sweepCycle == 0) {
            m_stage5CoreTargetY = m_playerY;
            PlayStage5Cue(Stage5BarrageWarning);
        }
        if (sweepCycle == 36) {
            for (int lane = -4; lane <= 4; ++lane) {
                SpawnEnemyShotAt(static_cast<float>(lane) * 0.27f, 0.70f, 58.0f,
                    static_cast<float>(lane) * 0.27f, m_stage5CoreTargetY,
                    PlayerRailZ, 0.76f);
            }
        }
        return;
    }

    if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
        const int cycle = m_stage5AttackTimer % 132;
        if (cycle == 0) {
            m_stage5CoreTargetX = m_playerX;
            m_stage5CoreTargetY = m_playerY;
            PlayStage5Cue(Stage5BarrageWarning);
        }
        if (cycle == 32) {
            for (int engine = 0; engine < 2; ++engine) {
                const TayamaWeakpoint type = engine == 0 ?
                    TayamaWeakpoint::LeftLiftEngine : TayamaWeakpoint::RightLiftEngine;
                if (m_tayamaWeakpoints[static_cast<int>(type)].destroyed) continue;
                const float side = engine == 0 ? -1.0f : 1.0f;
                for (int lane = -2; lane <= 2; ++lane) {
                    SpawnEnemyShotAt(side * 0.72f, -0.42f, 55.0f,
                        m_stage5CoreTargetX + static_cast<float>(lane) * 0.13f,
                        m_stage5CoreTargetY, PlayerRailZ, 0.72f);
                }
            }
        }
        if (m_stage5GuardSpawnCooldown == 0 && m_stage5PhaseTimer > 150) {
            SpawnEnemy(Stage::StraightShooterEnemy, 1.16f,
                (m_stage5PhaseTimer / 360) % 2 == 0 ? -0.58f : 0.58f, 0.54f, 59.0f);
            m_stage5GuardSpawnCooldown = 360;
        }
        return;
    }

    if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        const int cycle = m_stage5AttackTimer % 180;
        if (cycle == 0) {
            m_stage5CoreTargetX = m_playerX;
            m_stage5CoreTargetY = m_playerY;
            PlayStage5Cue(Stage5CoreWarning);
        }
        if (cycle == 42 || cycle == 52 || cycle == 62) {
            SpawnEnemyShotAt(0.0f, 0.35f, 55.0f,
                m_stage5CoreTargetX, m_stage5CoreTargetY, PlayerRailZ, 0.92f);
        }
        if (cycle == 104) {
            for (int ray = 0; ray < 12; ++ray) {
                const float angle = static_cast<float>(ray) * Math::TwoPi / 12.0f;
                SpawnEnemyShotAt(0.0f, 0.35f, 55.0f,
                    m_playerX + std::cos(angle) * 0.72f,
                    m_playerY + std::sin(angle) * 0.56f, PlayerRailZ, 0.66f);
            }
        }
        if (cycle == 138) {
            for (int lane = -4; lane <= 4; ++lane) {
                SpawnEnemyShotAt(static_cast<float>(lane) * 0.22f, -0.62f, 54.0f,
                    static_cast<float>(lane) * 0.22f,
                    -0.52f + std::abs(static_cast<float>(lane)) * 0.10f,
                    PlayerRailZ, 0.58f);
            }
        }
    }
}

/**
 * @brief Stage 5専用シーケンスを更新する
 * @return なし
 */
void SideScrollingShooter::TickStage5() {
    ++m_stage5PhaseTimer;

    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        if (m_stage5PhaseTimer == 58) {
            SpawnExplosion(0.78f, 0.22f, 61.0f, true);
            constexpr float PanelColor[] = {0.20f, 0.22f, 0.30f, 1.0f};
            for (int i = 0; i < 8; ++i) {
                SpawnDebrisPiece(5.2f + static_cast<float>(i) * 0.28f, 1.0f + static_cast<float>(i % 3),
                    61.0f, 0.03f + static_cast<float>(i) * 0.004f, 0.02f,
                    -0.04f - static_cast<float>(i % 2) * 0.02f, 0.0f, 0.08f,
                    1, 0.8f, 0.35f, 0.16f, PanelColor, 120, 90, false);
            }
        }
        if (m_stage5PhaseTimer >= EastsourceIntroFrames) {
            StartStage5Phase(Stage5Phase::EastsourceBattle);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceBattle) {
        const Enemy& eastsource = m_enemies[0];
        const int phase = eastsource.bossPhase;
        const int pursuitCycle = eastsource.age % 180;
        if ((phase == BossNormalPhase2 || phase == BossSpecialPhase2) && pursuitCycle < 90) {
            TickSearchlights(1, false);
        }
        if (pursuitCycle == 90) ResetWallSearchlights(1);
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceFall) {
        if (m_stage5PhaseTimer >= EastsourceFallFrames) {
            m_enemies[0].active = false;
            StartStage5Phase(Stage5Phase::WallClimbTransition, false);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbTransition) {
        m_tayamaTransformation = Math::Lerp(0.0f, 0.10f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbTransitionFrames));
        if (m_stage5PhaseTimer >= WallClimbTransitionFrames) {
            StartStage5Phase(Stage5Phase::WallClimbLower);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbLower) {
        m_tayamaTransformation = Math::Lerp(0.10f, 0.34f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbLowerFrames));
        TickSearchlights(1, false);
        if (m_stage5PhaseTimer >= WallClimbLowerFrames) {
            StartStage5Phase(Stage5Phase::WallClimbMiddle);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbMiddle) {
        m_tayamaTransformation = Math::Lerp(0.34f, 0.64f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbMiddleFrames));
        TickSearchlights(2, false);
        if (m_stage5GuardSpawnCooldown-- <= 0) {
            SpawnEnemy(Stage::ArmoredEnemy, 1.16f,
                (m_stage5PhaseTimer / 180) % 2 == 0 ? -0.76f : 0.76f, 0.42f, 58.0f);
            m_stage5GuardSpawnCooldown = 210;
        }
        if (m_stage5PhaseTimer >= WallClimbMiddleFrames) {
            StartStage5Phase(Stage5Phase::WallClimbUpper);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbUpper) {
        m_tayamaTransformation = Math::Lerp(0.64f, 0.90f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbUpperFrames));
        TickSearchlights(3, false);
        if (m_stage5GuardSpawnCooldown-- <= 0) {
            SpawnEnemy(Stage::StraightShooterEnemy, 1.16f,
                (m_stage5PhaseTimer / 150) % 2 == 0 ? -0.82f : 0.82f, -0.34f, 60.0f);
            m_stage5GuardSpawnCooldown = 180;
        }
        if (m_stage5PhaseTimer >= WallClimbUpperFrames) {
            StartStage5Phase(Stage5Phase::RooftopArrival, false);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::RooftopArrival) {
        m_tayamaTransformation = Math::Lerp(0.90f, 0.96f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / RooftopArrivalFrames));
        if (m_stage5PhaseTimer >= RooftopArrivalFrames) {
            StartStage5Phase(Stage5Phase::CarrierTransformation, false);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::CarrierTransformation) {
        m_tayamaTransformation = Math::Lerp(0.96f, 1.0f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / CarrierTransformationFrames));
        if (m_stage5PhaseTimer >= CarrierTransformationFrames) {
            StartTayamaPhase(Stage5Phase::TayamaFireControl);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::TayamaFireControl ||
        m_stage5Phase == Stage5Phase::TayamaLiftEngines ||
        m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        TickTayama();
        return;
    }
    if (m_stage5Phase == Stage5Phase::TayamaCollapse) {
        // 艦尾から艦首へ連鎖させ、最終フレームまで画面内で輪郭を崩す
        if (m_stage5PhaseTimer < TayamaCollapseFrames && m_stage5PhaseTimer % 36 == 0) {
            const int burst = m_stage5PhaseTimer / 36;
            SpawnExplosion(-0.85f + static_cast<float>((burst * 7) % 17) * 0.10f,
                -0.34f + static_cast<float>((burst * 5) % 9) * 0.09f, 54.0f, true);
            PlayStage5Cue(Stage5ChainExplosion);
        }
        if (m_stage5PhaseTimer == 450) PlayStage5Cue(Stage5FinalExplosion);
        if (m_stage5PhaseTimer >= TayamaCollapseFrames + Stage5QuietFlightFrames) {
            StartStage5Phase(Stage5Phase::EndingReady, false);
        }
    }
}

/**
 * @brief 現在のStage 5チェックポイントへ復帰する
 * @return なし
 */
void SideScrollingShooter::RestartStage5Checkpoint() {
    ++m_chapterRetryCounts[2];
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    m_power = m_stage5CheckpointPower;
    m_score = m_stage5CheckpointScore;
    m_kills = m_stage5CheckpointKills;
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    m_viewMode = ViewMode::Rail3D;
    m_nextViewMode = ViewMode::Rail3D;
    m_viewTransitionTimer = 0;
    m_viewTransitionProgress = 0.0f;
    m_invincible = 120;
    m_restartTimer = RestartDisplayFrames;

    if (m_stage5Checkpoint == Stage5Checkpoint::Eastsource) {
        StartStage5Phase(Stage5Phase::EastsourceBattle, false);
        return;
    }
    if (m_stage5Checkpoint == Stage5Checkpoint::WallClimbLower ||
        m_stage5Checkpoint == Stage5Checkpoint::WallClimbMiddle ||
        m_stage5Checkpoint == Stage5Checkpoint::WallClimbUpper) {
        const Stage5Phase phase = m_stage5Checkpoint == Stage5Checkpoint::WallClimbLower ?
            Stage5Phase::WallClimbLower :
            (m_stage5Checkpoint == Stage5Checkpoint::WallClimbMiddle ?
                Stage5Phase::WallClimbMiddle : Stage5Phase::WallClimbUpper);
        StartStage5Phase(phase, false);
        return;
    }

    // TAYAMAは前フェーズを破壊済みとし、現在フェーズのHPだけを戻す
    const Stage5Phase phase = m_stage5Checkpoint == Stage5Checkpoint::TayamaFireControl ?
        Stage5Phase::TayamaFireControl :
        (m_stage5Checkpoint == Stage5Checkpoint::TayamaLiftEngines ?
            Stage5Phase::TayamaLiftEngines : Stage5Phase::TayamaCommandCore);
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        const bool previousPhase =
            (phase >= Stage5Phase::TayamaLiftEngines &&
                static_cast<int>(weakpoint.type) <= static_cast<int>(TayamaWeakpoint::FireControlRadar)) ||
            (phase >= Stage5Phase::TayamaCommandCore &&
                (weakpoint.type == TayamaWeakpoint::LeftLiftEngine ||
                    weakpoint.type == TayamaWeakpoint::RightLiftEngine));
        weakpoint.destroyed = previousPhase;
        weakpoint.hp = previousPhase ? 0 : weakpoint.maxHp;
    }
    StartTayamaPhase(phase, true);
}

/**
 * @brief Stage 5用の効果音をクールダウン付きで再生する
 * @param cue 効果音種別
 * @return なし
 */
void SideScrollingShooter::PlayStage5Cue(int cue) {
    if (!m_audio || m_stage5SoundCooldown > 0) return;
    switch (cue) {
    case Stage5DistantThunder:
        m_audio->PlayMMLSE("t90 o2 l8 v7 c r g");
        m_stage5SoundCooldown = 90;
        break;
    case Stage5Thunder:
        m_audio->PlayMMLSE("t180 o2 l32 v13 c>c<g c");
        m_stage5SoundCooldown = 45;
        break;
    case Stage5SearchlightDetect:
        m_audio->PlayMMLSE("t220 o6 l32 v8 c r c");
        m_stage5SoundCooldown = 18;
        break;
    case Stage5SearchlightLocked:
        m_audio->PlayMMLSE("t240 o6 l16 v11 c>g");
        m_stage5SoundCooldown = 24;
        break;
    case Stage5BarrageWarning:
        m_audio->PlayMMLSE("t180 o4 l32 v10 c c c");
        m_stage5SoundCooldown = 18;
        break;
    case Stage5EastsourceEntrance:
        m_audio->PlayMMLSE("t200 o3 l16 v13 c g > c g");
        m_stage5SoundCooldown = 60;
        break;
    case Stage5SignalLost:
        m_audio->PlayMMLSE("t140 o5 l32 v8 g f e c");
        m_stage5SoundCooldown = 60;
        break;
    case Stage5Transformation:
        m_audio->PlayMMLSE("t110 o2 l16 v12 c d e g");
        m_stage5SoundCooldown = 75;
        break;
    case Stage5WeakpointDestroyed:
        m_audio->PlaySE(Audio::SfxrPreset::Explosion);
        m_stage5SoundCooldown = 20;
        break;
    case Stage5CoreWarning:
        m_audio->PlayMMLSE("t240 o3 l16 v12 c > c < c > c");
        m_stage5SoundCooldown = 36;
        break;
    case Stage5FinalExplosion:
        m_audio->PlayMMLSE("t80 o1 l2 v15 c g c");
        m_stage5SoundCooldown = 90;
        break;
    default:
        m_audio->PlaySE(Audio::SfxrPreset::Explosion);
        m_stage5SoundCooldown = 16;
        break;
    }
}

/**
 * @brief Stage2ボス撃破演出の振動音または最終爆発音を再生する
 * @param finalExplosion 最終爆発音を再生する場合true
 * @return なし
 */
void SideScrollingShooter::PlayStage2DefeatSound(bool finalExplosion) {
    if (!m_audio) return;

    // 低域ノイズの長さと落下量を切り替えてゴゴゴ音とドカーン音を作る
    Audio::SfxrParams sound;
    sound.waveType = Audio::SfxrWaveType::Noise;
    sound.startFrequency = finalExplosion ? 0.24f : 0.10f;
    sound.minFrequency = finalExplosion ? 0.01f : 0.06f;
    sound.slide = finalExplosion ? -0.32f : -0.015f;
    sound.attackTime = finalExplosion ? 0.0f : 0.08f;
    sound.sustainTime = finalExplosion ? 0.42f : 1.10f;
    sound.decayTime = finalExplosion ? 0.90f : 0.72f;
    sound.masterVolume = finalExplosion ? 0.95f : 0.42f;
    m_audio->PlaySE(sound);

    // 最終爆発だけ低い衝撃音を足して爆発の芯を強める
    if (finalExplosion) m_audio->PlayMMLSE("t72 o1 l2 v15 c g c");
}

/**
 * @brief ボス出現と名前表示の時間演出を進行する
 * @return なし
 */
void SideScrollingShooter::TickBossIntroduction() {
    Enemy& boss = m_enemies[0];

    // ステージ1は画面の上下を交互に横断してから端から低速で定位置へ入る
    if (m_bossIntroductionPhase == BossIntroductionPhase::Entrance && m_stageNumber == 1) {
        constexpr float SideX[] = {3.25f, -3.25f, 3.25f, -3.25f, 3.25f};
        constexpr float SideY[] = {1.05f, -1.08f, -0.82f, 1.05f, 0.0f};
        constexpr float RailX[] = {1.45f, -1.45f, 1.45f, -1.45f, 1.45f};
        constexpr float RailY[] = {1.05f, -1.08f, -0.82f, 1.05f, 0.0f};
        if (m_bossIntroductionTimer < Stage1BossRushFrames) {
            const int segment = Stage1BossRushSegment(m_bossIntroductionTimer);
            const float segmentProgress = SmoothStep(static_cast<float>(
                m_bossIntroductionTimer - segment * Stage1BossRushSegmentFrames) /
                static_cast<float>(Stage1BossRushSegmentFrames));
            if (IsRailGameplayActive()) {
                boss.x = Math::Lerp(RailX[segment], RailX[segment + 1], segmentProgress);
                boss.y = Math::Lerp(RailY[segment], RailY[segment + 1], segmentProgress);
                boss.z = 32.0f;
            } else {
                boss.x = Math::Lerp(SideX[segment], SideX[segment + 1], segmentProgress);
                boss.y = Math::Lerp(SideY[segment], SideY[segment + 1], segmentProgress);
                boss.z = ToRailZFromSideX(boss.x);
            }
        } else {
            const float settleProgress = SmoothStep(static_cast<float>(
                m_bossIntroductionTimer - Stage1BossRushFrames) /
                static_cast<float>(Stage1BossSettleFrames));
            if (IsRailGameplayActive()) {
                boss.x = Math::Lerp(RailX[4], 0.0f, settleProgress);
                boss.y = 0.0f;
                boss.z = Math::Lerp(32.0f, 48.0f, settleProgress);
            } else {
                boss.x = Math::Lerp(SideX[4], 1.80f, settleProgress);
                boss.y = 0.0f;
                boss.z = ToRailZFromSideX(boss.x);
            }
        }
    }

    ++m_bossIntroductionTimer;
    const int entranceFrames = m_stageNumber == 1 ? Stage1BossEntranceFrames :
        (m_stageNumber == 2 ? Stage2BossEntranceFrames : 1);
    if (m_bossIntroductionPhase == BossIntroductionPhase::Entrance &&
        m_bossIntroductionTimer >= entranceFrames) {
        // 定位置を既存ステージ定義へ戻して会話へ移行する
        if (IsRailGameplayActive()) m_stage->ConfigureBossRailAnchor(boss);
        else m_stage->ConfigureBossSideAnchor(boss);
        m_bossIntroductionPhase = BossIntroductionPhase::Dialogue;
        m_bossIntroductionTimer = 0;
        m_bossStoryActive = true;
        return;
    }
    if (m_bossIntroductionPhase == BossIntroductionPhase::NameReveal &&
        m_bossIntroductionTimer >= BossNameRevealFrames) {
        // 名前表示完了後に初めて通常の戦闘更新へ戻す
        m_bossIntroductionPhase = BossIntroductionPhase::None;
        m_bossIntroductionTimer = 0;
    }
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
    m_missionStartTimer = MissionBannerDisplayFrames;
    m_scroll = 0.0f;
    m_frame = 0;
    m_spawnCooldown = 35;
    m_shotCooldown = 0;
    m_specialShotCooldown = 0;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_bossIntroductionPhase = BossIntroductionPhase::None;
    m_bossIntroductionTimer = 0;
    m_clear = false;
    m_clearTimer = 0;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_invincible = (std::max)(m_invincible, 90);
    ResetStage5();
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
void SideScrollingShooter::SpawnShotDirect(float x, float y, float z, float vx, float vy, float vz, bool enemy,
    int barrageIndex, int barrageCount) {
    Shot* available = nullptr;
    for (auto& shot : m_shots) {
        if (!shot.active) {
            available = &shot;
            break;
        }
    }

    // Stage 5の予告済み攻撃は満杯時に古い自機弾を置換して欠落を防ぐ
    if (!available && enemy && m_stageNumber == 5) {
        for (auto& shot : m_shots) {
            if (!shot.enemy) {
                available = &shot;
                break;
            }
        }
    }
    if (!available) return;

    Shot& shot = *available;
    shot = {};
    shot.x = x;
    shot.y = y;
    shot.z = z;
    shot.transitionSideX = x;
    shot.transitionSideY = y;
    shot.vx = vx;
    shot.vy = vy;
    shot.vz = vz;
    shot.barrageIndex = barrageIndex;
    shot.barrageCount = barrageCount;
    shot.enemy = enemy;
    shot.active = true;
}

void SideScrollingShooter::SpawnStage2Funnel(float x, float y, float z, bool delayedEngine) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = 0.0f;
        // Phase3は砂面を越えてから短く落下する高さまで、点火前の射出初速を与える
        shot.vy = delayedEngine ? Phase3FunnelLaunchVelocity : 0.018f;
        shot.vz = 0.0f;
        shot.hitRadius = 0.055f;
        shot.damage = 2;
        shot.enemy = true;
        shot.funnel = true;
        shot.funnelDelayedEngine = delayedEngine;
        shot.active = true;
        return;
    }
}

void SideScrollingShooter::SpawnStage2Missile(float x, float y, float z, float side) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.z = z;
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        const float dx = ToWorldX(m_playerX - x);
        const float dy = ToWorldY(m_playerY - y);
        const float dz = IsRailGameplayActive() ? PlayerRailZ - z : 0.0f;
        const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
        constexpr float MissileSpeed = 0.68f;
        shot.vx = FromWorldX(dx / length * MissileSpeed) + side * 0.006f;
        shot.vy = FromWorldY(dy / length * MissileSpeed);
        shot.vz = dz / length * MissileSpeed;
        shot.hitRadius = 0.045f;
        shot.damage = 2;
        shot.enemy = true;
        shot.missile = true;
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
    if (m_stageNumber == 5 &&
        (m_stage5Phase == Stage5Phase::TayamaCollapse ||
            m_stage5Phase == Stage5Phase::EndingReady)) {
        return;
    }
    if (m_playerDestructionTimer > 0) return;

    // 敵撃破と同じ破壊爆発を自機位置へ生成してから復帰を待つ
    SpawnExplosion(m_playerX, m_playerY, PlayerRailZ, true);
    PlayHitSound();
    m_playerDestructionTimer = PlayerDestructionWaitFrames;
}

/**
 * @brief 現在のチャプターを開始時状態へ戻す
 * @return なし
 */
void SideScrollingShooter::RestartCurrentChapter() {
    if (m_stageNumber == 5 && m_stage5Phase != Stage5Phase::Approach) {
        RestartStage5Checkpoint();
        return;
    }
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
    m_bossIntroductionPhase = BossIntroductionPhase::None;
    m_bossIntroductionTimer = 0;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_playerDestructionTimer = 0;
    m_restartTimer = RestartDisplayFrames;
}

/**
 * @brief Stage2潜砂艦のPhase1接地基準Y座標を取得する
 * @param boss 座標を求めるStage2ボス
 * @return 切削爪が地中へ収まる潜砂艦のワールドY座標
 */
float SideScrollingShooter::Stage2Phase1SubmarineWorldY(const Enemy& boss) const {
    constexpr float SideGroundTopY = -6.0f;
    constexpr float RailGroundTopY = -3.65f;
    constexpr float CutterTopLocalY = -0.99f;
    constexpr float CutterEmbedDepth = 0.55f;
    constexpr float ModelScale = 1.92f;
    constexpr float Phase1BobWorldAmplitude = 0.10f * WorldYScale;
    static_assert(CutterEmbedDepth > Phase1BobWorldAmplitude);

    // 2Dと3Dの地面上面を補間し、上下動の最高点でも切削爪を地中へ残す
    const float groundTopY = Math::Lerp(SideGroundTopY, RailGroundTopY, RailBlend());
    return groundTopY - CutterTopLocalY * ModelScale - CutterEmbedDepth + ToWorldY(boss.y);
}

/**
 * @brief Stage2潜砂艦の現在の親Y座標を取得する
 * @param boss 座標を求めるStage2ボス
 * @return 描画と当たり判定で共有するワールドY座標
 */
float SideScrollingShooter::Stage2SubmarineWorldY(const Enemy& boss) const {
    const float assembledY = boss.phase < 2.0f ? Stage2Phase1SubmarineWorldY(boss) :
        ToWorldY(boss.y) - 1.45f + boss.sandSubmarineOffsetY;
    if (m_stageNumber != 2 || m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return assembledY;
    }

    // 前半で地中から砂面直下まで掘り進み、後半で上部戦艦との合体位置まで浮上する
    const float approach = SmoothStep(static_cast<float>(m_bossIntroductionTimer) /
        static_cast<float>(Stage2BossApproachFrames));
    const float assembly = SmoothStep(static_cast<float>(m_bossIntroductionTimer - Stage2BossApproachFrames) /
        static_cast<float>(Stage2BossAssemblyFrames));
    return assembledY + Math::Lerp(-7.0f, -2.4f, approach) * (1.0f - assembly);
}

/**
 * @brief Stage2上部戦艦の親Y座標を取得する
 * @param boss 座標を求めるStage2ボス
 * @return 描画と当たり判定で共有するワールドY座標
 */
float SideScrollingShooter::Stage2BattleshipWorldY(const Enemy& boss) const {
    constexpr float SideGroundTopY = -6.0f;
    constexpr float RailGroundTopY = -3.65f;
    constexpr float HullBottomLocalY = 0.02f;
    constexpr float HoverHeight = 0.18f;
    constexpr float ModelScale = 1.92f;
    constexpr float AssembledUnitOffsetY = 0.77f;
    if (boss.phase >= 2.0f && boss.phase < 3.0f) {
        // Phase2開始時の高度から地表付近まで滑らかに下降する
        constexpr float DescentFrames = 120.0f;
        const float t = Math::Clamp01(static_cast<float>(boss.stage2BossActionAge) / DescentFrames);
        const float smooth = t * t * (3.0f - 2.0f * t);
        const float startY = Stage2Phase1SubmarineWorldY(boss) + AssembledUnitOffsetY;
        const float targetY = Math::Lerp(SideGroundTopY, RailGroundTopY, RailBlend()) +
            HoverHeight - HullBottomLocalY * ModelScale;
        return Math::Lerp(startY, targetY, smooth);
    }
    const float assembledY = boss.phase < 2.0f ?
        Stage2Phase1SubmarineWorldY(boss) + AssembledUnitOffsetY :
        ToWorldY(boss.y) + 1.32f + boss.landBattleshipOffsetY;
    if (m_stageNumber != 2 || m_bossIntroductionPhase != BossIntroductionPhase::Entrance) {
        return assembledY;
    }

    // 前半で上空から降下し、後半で潜砂艦との合体位置へ接近する
    const float approach = SmoothStep(static_cast<float>(m_bossIntroductionTimer) /
        static_cast<float>(Stage2BossApproachFrames));
    const float assembly = SmoothStep(static_cast<float>(m_bossIntroductionTimer - Stage2BossApproachFrames) /
        static_cast<float>(Stage2BossAssemblyFrames));
    return assembledY + Math::Lerp(13.0f, 4.2f, approach) * (1.0f - assembly);
}

/**
 * @brief Stage2上部戦艦の船体へ弾が命中したか判定する
 * @param shot 判定する自機弾
 * @param boss 判定するStage2ボス
 * @return 船体へ命中した場合true
 */
bool SideScrollingShooter::TryHitStage2BossBody(const Shot& shot, const Enemy& boss) const {
    constexpr float BodyLocalX = 0.55f;
    constexpr float BodyLocalY = 0.95f;
    constexpr float ModelScale = 1.92f;
    constexpr float BodyRadius = 4.25f;
    const bool railMode = IsRailGameplayActive();
    const float yaw = railMode ? 0.0f : Math::HalfPi;
    const float patrolX = boss.phase >= 2.0f ?
        std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * RailBlend() : 0.0f;
    const Vector3 center {
        ToWorldX(boss.x + boss.landBattleshipOffsetX) + patrolX + std::cos(yaw) * BodyLocalX * ModelScale,
        Stage2BattleshipWorldY(boss) + BodyLocalY * ModelScale,
        boss.z + boss.landBattleshipOffsetZ - std::sin(yaw) * BodyLocalX * ModelScale
    };
    return railMode ?
        Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            center.x, center.y, center.z, BodyRadius) :
        Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(center.x), FromWorldY(center.y),
            BodyRadius / WorldXScale);
}

bool SideScrollingShooter::TryHitBossPart(const Shot& shot, const Enemy& boss, BossPart& part) const {
    if (m_stageNumber == 5) {
        constexpr EastsourcePartGroup Groups[] = {
            EastsourcePartGroup::Nose,
            EastsourcePartGroup::LeftWing,
            EastsourcePartGroup::RightWing,
            EastsourcePartGroup::LeftEngine,
            EastsourcePartGroup::RightEngine
        };
        const Stage5ModelTransform transform = EastsourceTransform(boss);
        const EastsourceModelState state = EastsourceState(boss);

        // 描画と同じ26パーツから集約した各グループ境界へ線分判定する
        for (int index = BossNose; index <= BossRightEngine; ++index) {
            if (boss.bossPartHp[index] <= 0) continue;
            const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
                transform, state, Groups[index]);
            if (!bounds.valid || !Hit3DSegment(
                ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;
            part = static_cast<BossPart>(index);
            return true;
        }
        return false;
    }

    if (m_stageNumber == 2) {
        constexpr float ModelScale = 1.92f;
        constexpr Vector3 PartPosition[] = {
            {-2.30f, 2.08f, 0.0f}, {-0.55f, 2.60f, -0.92f},
            {0.65f, 2.60f, 0.0f}, {2.85f, 2.18f, 0.92f}, {0.25f, 1.42f, 0.0f}
        };
        constexpr float PartRadius[] = {2.20f, 1.05f, 1.05f, 1.05f, 1.35f};
        bool weaponsDestroyed = true;
        for (int i = 0; i < BossPartCount; ++i) {
            if (i != BossRightEngine && boss.bossPartHp[i] > 0) weaponsDestroyed = false;
        }
        const bool railMode = IsRailGameplayActive();
        const float battleshipYaw = railMode ? 0.0f : Math::HalfPi;
        const bool separated = boss.phase >= 2.0f;
        const float battleshipPatrolX = separated ?
            std::sin(static_cast<float>(boss.age) * 0.018f) * 2.4f * RailBlend() : 0.0f;

        // 武装全破壊までは装甲内の接続コアを命中対象にしない
        for (int i = 0; i <= BossRightEngine; ++i) {
            if (boss.bossPartHp[i] <= 0 || (i == BossRightEngine && !weaponsDestroyed)) continue;
            const Vector3& local = PartPosition[i];
            const bool submarinePart = i == BossRightEngine;
            const float yaw = battleshipYaw + (submarinePart && separated ? Math::HalfPi : 0.0f);
            const float cosine = std::cos(yaw);
            const float sine = std::sin(yaw);
            const float unitOffsetX = submarinePart ? ToWorldX(boss.sandSubmarineOffsetX) :
                ToWorldX(boss.landBattleshipOffsetX) + battleshipPatrolX;
            const float worldY = submarinePart ?
                Stage2SubmarineWorldY(boss) :
                Stage2BattleshipWorldY(boss);
            const float unitOffsetZ = submarinePart ? boss.sandSubmarineOffsetZ : boss.landBattleshipOffsetZ;
            const Vector3 world {
                ToWorldX(boss.x) + unitOffsetX + (local.x * cosine + local.z * sine) * ModelScale,
                worldY + local.y * ModelScale,
                boss.z + unitOffsetZ + (-local.x * sine + local.z * cosine) * ModelScale
            };
            const bool hit = railMode ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    world.x, world.y, world.z, PartRadius[i]) :
                Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(world.x), FromWorldY(world.y),
                    PartRadius[i] / WorldXScale);
            if (!hit) continue;
            part = static_cast<BossPart>(i);
            return true;
        }

        // 側面のオレンジ色の小窓十二基を、描画と同じローカル座標で個別判定する
        for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
            const int partIndex = BossFunnelHatch0 + hatch;
            if (boss.bossPartHp[partIndex] <= 0) continue;
            const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
            const float localZ = (hatch < 6 ? -1.0f : 1.0f) * 1.80f;
            const float submarineYaw = battleshipYaw + (separated ? Math::HalfPi : 0.0f);
            const float cosine = std::cos(submarineYaw);
            const float sine = std::sin(submarineYaw);
            const Vector3 world {
                ToWorldX(boss.x + boss.sandSubmarineOffsetX) +
                    (localX * cosine + localZ * sine) * ModelScale,
                Stage2SubmarineWorldY(boss) - 0.22f * ModelScale,
                boss.z + boss.sandSubmarineOffsetZ +
                    (-localX * sine + localZ * cosine) * ModelScale
            };
            const bool hit = railMode ?
                Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
                    world.x, world.y, world.z, 0.58f) :
                Hit(shot.x, shot.y, shot.hitRadius, FromWorldX(world.x), FromWorldY(world.y),
                    0.58f / WorldXScale);
            if (!hit) continue;
            part = static_cast<BossPart>(partIndex);
            return true;
        }
        return false;
    }

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
            if (!Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
                ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
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

/**
 * @brief 鋭い放電音と短い雷鳴を重ねたStage2レールガンSEを再生する
 * @return なし
 */
void SideScrollingShooter::PlayRailgunSound() {
    if (!m_audio) return;

    // 高周波の亀裂音を急降下させて雷の鋭い立ち上がりを作る
    Audio::SfxrParams crack;
    crack.waveType = Audio::SfxrWaveType::Sawtooth;
    crack.startFrequency = 0.92f;
    crack.minFrequency = 0.06f;
    crack.slide = -0.72f;
    crack.sustainTime = 0.025f;
    crack.decayTime = 0.16f;
    crack.masterVolume = 0.75f;
    m_audio->PlaySE(crack);

    // 短いノイズを重ねて雷撃の破裂感を足す
    Audio::SfxrParams thunder;
    thunder.waveType = Audio::SfxrWaveType::Noise;
    thunder.startFrequency = 0.68f;
    thunder.minFrequency = 0.04f;
    thunder.slide = -0.58f;
    thunder.sustainTime = 0.035f;
    thunder.decayTime = 0.22f;
    thunder.masterVolume = 0.82f;
    m_audio->PlaySE(thunder);
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

bool SideScrollingShooter::Hit3DSegment(float startX, float startY, float startZ,
    float endX, float endY, float endZ, float movingRadius,
    float targetX, float targetY, float targetZ, float targetRadius) {
    const float dx = endX - startX;
    const float dy = endY - startY;
    const float dz = endZ - startZ;
    const float lengthSquared = dx * dx + dy * dy + dz * dz;
    const float toTargetX = targetX - startX;
    const float toTargetY = targetY - startY;
    const float toTargetZ = targetZ - startZ;
    const float progress = lengthSquared > 0.000001f ? (std::clamp)(
        (toTargetX * dx + toTargetY * dy + toTargetZ * dz) / lengthSquared, 0.0f, 1.0f) : 0.0f;
    return Hit3D(startX + dx * progress, startY + dy * progress, startZ + dz * progress, movingRadius,
        targetX, targetY, targetZ, targetRadius);
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
 * @brief 3Dレール視点で機体底面が地面上面に接するY座標下限を取得する
 * @return ゲーム座標系のY座標下限
 */
float SideScrollingShooter::PlayerRailMinY() const {
    constexpr float PlayerHalfHeight = 0.16f;
    const float groundTopY = m_stageNumber == 1 ? -3.275f : -3.65f;
    return FromWorldY(groundTopY + PlayerHalfHeight);
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
    Vector3 railPosition{playerPosition.x * 0.18f, playerPosition.y * 0.12f + 1.0f, PlayerRailZ - 15.5f};
    Vector3 railTarget{playerPosition.x * 0.28f, playerPosition.y * 0.18f, PlayerRailZ + 22.0f};

    // 壁面上昇だけ視線を上へ向け、屋上で水平へ戻す
    if (m_stageNumber == 5 && m_stage5Phase >= Stage5Phase::WallClimbTransition) {
        float climbWeight = 0.0f;
        if (m_stage5Phase == Stage5Phase::WallClimbTransition) {
            climbWeight = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer) /
                static_cast<float>(WallClimbTransitionFrames));
        } else if (m_stage5Phase == Stage5Phase::WallClimbLower) {
            climbWeight = 0.42f;
        } else if (m_stage5Phase == Stage5Phase::WallClimbMiddle) {
            climbWeight = 0.68f;
        } else if (m_stage5Phase == Stage5Phase::WallClimbUpper) {
            climbWeight = 0.92f;
        } else if (m_stage5Phase == Stage5Phase::RooftopArrival) {
            climbWeight = 1.0f - SmoothStep(Math::Clamp01(
                static_cast<float>(m_stage5PhaseTimer) / RooftopArrivalFrames));
        }
        railPosition.y -= climbWeight * 1.8f;
        railTarget.y += climbWeight * 13.0f;
        railTarget.z += climbWeight * 8.0f;
    }
    if (m_stageNumber == 5 && m_stage5Phase == Stage5Phase::TayamaCollapse) {
        const float pullBack = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 60) / 390.0f);
        railPosition.z -= pullBack * 8.0f;
        railPosition.y += pullBack * 2.0f;
        railTarget.z += pullBack * 7.0f;
    }
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(38.0f + (8.0f * railWeight)));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(m_stageNumber == 5 ? 220.0f : 120.0f);
    camera.SetPosition(Vector3::Lerp(sidePosition, railPosition, railWeight));
    camera.LookAt(Vector3::Lerp(sideTarget, railTarget, railWeight));
}

void SideScrollingShooter::DrawShape(Renderer& renderer,
    float x, float y, float w, float h, const float color[4]) {
    // 描画ファサードへ矩形コマンドとして記録する
    renderer.Draw(Rect { { x, y }, { w, h } }, { color[0], color[1], color[2], color[3] });
}

/**
 * @brief 2D画面上の敵攻撃予告を十字フラッシュとして描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawAttackWarnings2D(Renderer& renderer) const {
    constexpr float FlashColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.attackWarningFrames <= 0) continue;

        // 残り時間に合わせて拡大する十字を、攻撃を行う敵機へ重ねる
        const float progress = Math::Clamp01(
            1.0f - static_cast<float>(enemy.attackWarningFrames) / AttackWarningFrames);
        const float armLength = 0.035f + progress * 0.055f;
        DrawShape(renderer, enemy.x, enemy.y, armLength, 0.008f, FlashColor);
        DrawShape(renderer, enemy.x, enemy.y, 0.008f, armLength, FlashColor);
    }
}

/**
 * @brief 3D空間内の敵攻撃予告を発光マーカーとして描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param railWeight 横視点からレール視点への補間率
 * @return なし
 */
void SideScrollingShooter::DrawAttackWarnings3D(Renderer& renderer, const Camera3D& camera, float railWeight) const {
    constexpr float FlashColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
    for (const auto& enemy : m_enemies) {
        if (!enemy.active || enemy.attackWarningFrames <= 0) continue;

        // 2Dと3Dのカメラ遷移中も、予告を敵機の発射位置へ追従させる
        const float progress = Math::Clamp01(
            1.0f - static_cast<float>(enemy.attackWarningFrames) / AttackWarningFrames);
        const float size = 0.20f + progress * 0.35f;
        if (m_stageNumber == 5 && enemy.type == Stage::BossEnemy &&
            m_stage5Phase >= Stage5Phase::EastsourceIntro &&
            m_stage5Phase <= Stage5Phase::EastsourceBattle) {
            // 固定した照準地点をプレイヤー面へ表示して発射後の追尾と誤認させない
            DrawModelPrimitive(renderer, camera, 1,
                ToWorldX(enemy.attackWarningTargetX), ToWorldY(enemy.attackWarningTargetY),
                PlayerRailZ + 0.3f, size, size, size, FlashColor);
            continue;
        }
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? enemy.transitionSideX :
            (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        const float x = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
        const float y = Math::Lerp(sideY, exitingRail ? enemy.transitionSideY : enemy.y, railWeight);
        const float railZ = exitingRail ? enemy.transitionRailZ : enemy.z;
        const float z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), railZ, railWeight);
        DrawModelPrimitive(renderer, camera, 1, ToWorldX(x), ToWorldY(y), z - 0.3f,
            size, size, size, FlashColor);
    }
}

void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    float x, float y, float z, float w, float h, float d, const float color[4], float yaw, float pitch) {
    // プリミティブ形状を実3DカメラのViewProjectionへ乗せて描画する
    const Matrix4x4 world = Matrix4x4::Translation({x, y, z}) *
        Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) * Matrix4x4::Scale({w, h, d});
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
 * @brief XYZ回転を維持して3Dプリミティブを描画する
 * @param renderer 描画先
 * @param camera 使用するカメラ
 * @param shape PrimitiveShapeの列挙値
 * @param position ワールド座標
 * @param scale 寸法
 * @param rotation XYZ回転
 * @param color 色
 * @return なし
 */
void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    const Vector3& position, const Vector3& scale, const Vector3& rotation, const float color[4]) {
    const Matrix4x4 world = Matrix4x4::Translation(position) *
        Matrix4x4::RotationY(rotation.y) * Matrix4x4::RotationX(rotation.x) *
        Matrix4x4::RotationZ(rotation.z) * Matrix4x4::Scale(scale);
    DrawModelPrimitive(renderer, camera, shape, world, color);
}

/**
 * @brief 合成済みワールド行列で3Dプリミティブを描画する
 * @param renderer 描画先
 * @param camera 使用するカメラ
 * @param shape PrimitiveShapeの列挙値
 * @param world 合成済みワールド行列
 * @param color 色
 * @return なし
 */
void SideScrollingShooter::DrawModelPrimitive(Renderer& renderer, const Camera3D& camera, int shape,
    const Matrix4x4& world, const float color[4]) {
    renderer.Draw({
        static_cast<PrimitiveShape>(shape),
        camera.ProjectionMatrix() * camera.ViewMatrix() * world,
        Vector3::One,
        {color[0], color[1], color[2], color[3]},
        0.0f
    });
}

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
void SideScrollingShooter::DrawBlobShadow(Renderer& renderer, const Camera3D& camera,
    float x, float z, float groundTopY, float width, float depth, float opacity) {
    constexpr float ShadowHeight = 0.025f;
    const float shadowColor[4] = {0.015f, 0.020f, 0.025f, opacity};

    // 既存の円柱を薄く潰して八角形のBlobとし、Z-fightingを避ける
    DrawModelPrimitive(renderer, camera, 2, x, groundTopY + ShadowHeight * 0.5f, z,
        width * 2.0f, ShadowHeight, depth * 2.0f, shadowColor);
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
    for (int i = 0; i < MeteorCount; ++i) {
        const Meteor& meteor = m_meteors[i];
        if (meteor.destroyed) continue;
        const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
        const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
        const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
        const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
        const float x = Math::Lerp(ToWorldX(sideX), railX, railWeight);
        const float y = Math::Lerp(ToWorldY(sideY), railY, railWeight);
        const float z = Math::Lerp(SidePlaneZ + 1.2f, 72.0f - meteor.travel, railWeight);
        const float width = Math::Lerp(2.20f, 5.30f, railWeight) * meteor.scale;
        const float height = Math::Lerp(2.55f, 5.30f, railWeight) * meteor.scale;
        const float depth = Math::Lerp(1.45f, 5.30f, railWeight) * meteor.scale;

        // 本体とクレーターを同じ回転角で動かし、視点遷移中も形状を維持する
        DrawModelPrimitive(renderer, camera, 5, x, y, z, width, height, depth, MeteorColor, meteor.yaw);
        for (int crater = 0; crater < 3; ++crater) {
            const float angle = meteor.yaw + static_cast<float>(crater) * Math::TwoPi / 3.0f;
            const float offsetX = std::cos(angle) * Math::Lerp(0.52f, 1.28f, railWeight) * meteor.scale;
            const float offsetY = std::sin(angle * 1.7f) * Math::Lerp(0.40f, 1.16f, railWeight) * meteor.scale;
            DrawModelPrimitive(renderer, camera, 5, x + offsetX, y + offsetY,
                z - std::cos(angle) * Math::Lerp(0.38f, 1.75f, railWeight) * meteor.scale,
                Math::Lerp(0.38f, 0.92f, railWeight) * meteor.scale,
                Math::Lerp(0.42f, 0.92f, railWeight) * meteor.scale,
                Math::Lerp(0.20f, 0.38f, railWeight) * meteor.scale, MeteorCraterColor, meteor.yaw);
        }
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
        const float bodyProgress = GetSeaSerpentSegmentProgress(
            motion.progress, motion.segmentCount, motion.segmentDelay, i);
        const float elevation = std::sin(Math::Pi * bodyProgress) *
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
        const float sideY = -6.0f + (elevation < sideHeight ? sideVisibleHeight * 0.5f :
            elevation - sideHeight * 0.5f);
        const float railY = -3.65f + (elevation < railSize ? railVisibleHeight * 0.5f :
            elevation - railSize * 0.5f);
        const float y = Math::Lerp(sideY, railY, railWeight);
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
    const float headProgress = GetSeaSerpentSegmentProgress(
        motion.progress, motion.segmentCount, motion.segmentDelay, 0);
    const float headElevation = std::sin(Math::Pi * headProgress) * motion.elevation;
    const float headSideX = motion.sideOriginX + motion.direction * headProgress * motion.travel;
    const float headRailX = motion.railOriginX + std::sin(headProgress * 4.0f) * 6.0f;
    const float headRailZ = motion.railOriginZ + motion.railDirection * headProgress * motion.railTravel;
    const float headX = Math::Lerp(headSideX, headRailX, railWeight);
    const float headSideVisibleHeight = Math::Clamp01(headElevation / (1.35f * headScale)) * 1.35f * headScale;
    const float headRailVisibleHeight = Math::Clamp01(headElevation / (2.50f * headScale)) * 2.50f * headScale;
    const float headSideY = -6.0f + (headElevation < 1.35f * headScale ? headSideVisibleHeight * 0.5f :
        headElevation - 1.35f * headScale * 0.5f);
    const float headRailY = -3.65f + (headElevation < 2.50f * headScale ? headRailVisibleHeight * 0.5f :
        headElevation - 2.50f * headScale * 0.5f);
    const float headY = Math::Lerp(headSideY, headRailY, railWeight);
    const float headZ = Math::Lerp(SidePlaneZ + 13.1f, headRailZ, railWeight);

    // 頭が海面を出入りする短い時間だけ、水滴を初速と重力による放物線で飛ばす
    const float emergeProgress = std::asin((std::min)(1.0f, 1.35f * headScale / motion.elevation)) /
        Math::HalfPi;
    const float reentryProgress = 1.0f - emergeProgress;
    const float splashCenter = headProgress < 0.5f ? emergeProgress : reentryProgress;
    const float splashTime = Math::Clamp01((headProgress - (splashCenter - emergeProgress)) /
        (emergeProgress * 2.0f));
    const bool splashActive = std::abs(headProgress - splashCenter) <= emergeProgress;
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
        // 目をカメラ側の頭部表面より前へ置き、移動で視線が斜めになっても胴体へ埋まらないようにする
        const Vector3 headPosition {headX, headY, headZ};
        const Vector3 eyePosition = headPosition +
            camera.Right() * (Math::Lerp(0.36f, 0.82f, railWeight) * motion.scale) +
            camera.Up() * (Math::Lerp(0.30f, 0.72f, railWeight) * motion.scale) +
            (camera.Position() - headPosition).Normalized() *
                (Math::Lerp(SeaSerpentSideEyeSurfaceOffset, SeaSerpentRailEyeSurfaceOffset, railWeight) * motion.scale);
        DrawModelPrimitive(renderer, camera, 5, eyePosition.x, eyePosition.y, eyePosition.z,
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

void SideScrollingShooter::DrawEnemyModel(Renderer& renderer, const Camera3D& camera,
    const Enemy& enemy, float yaw) const {
    const float x = ToWorldX(enemy.x);
    const float y = ToWorldY(enemy.y);
    const float z = enemy.z;
    auto DrawDamageSmoke = [&](BossPart part, const Vector3& position, float size) {
        const int maxHp = enemy.bossPartMaxHp[part];
        if (maxHp <= 0 || enemy.bossPartHp[part] <= 0 || enemy.bossPartHp[part] * 100 > maxHp * 35) return;
        const Matrix4x4 world = Matrix4x4::Translation(position) * Matrix4x4::Scale({size, size * 1.7f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
            static_cast<float>(enemy.age) / 30.0f + static_cast<float>(part) * 0.37f, 1});
    };
    if (enemy.type == 2 && m_stageNumber == 5) {
        const Stage5ModelTransform transform = EastsourceTransform(enemy);
        const EastsourceModelState state = EastsourceState(enemy);

        // 参照ブランチdrawBoss1の26パーツとXYZ回転を変更せず描画する
        EastsourceModelView::VisitParts(transform, state,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color, EastsourcePartGroup) {
                const float partColor[] = {color.r, color.g, color.b, color.a};
                DrawModelPrimitive(renderer, camera, static_cast<int>(shape), world, partColor);
            });

        constexpr EastsourcePartGroup Groups[] = {
            EastsourcePartGroup::Nose,
            EastsourcePartGroup::LeftWing,
            EastsourcePartGroup::RightWing,
            EastsourcePartGroup::LeftEngine,
            EastsourcePartGroup::RightEngine
        };
        for (int part = BossNose; part <= BossRightEngine; ++part) {
            const Stage5GroupBounds bounds = EastsourceModelView::GroupBounds(
                transform, state, Groups[part]);
            if (bounds.valid) DrawDamageSmoke(static_cast<BossPart>(part), bounds.center, 0.82f);
        }
        return;
    }
    if (enemy.type == 2 && m_stageNumber == 2) {
        // 上下ユニットへ別Transformを渡し、合体状態を描画する
        constexpr float BossScale = 1.92f;
        const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
        const int railgunCycle = enemy.stage2BossActionAge % Stage2RailgunCycleFrames;
        const bool railgunLocked = railgunCycle < Stage2RailgunFireFrame + Stage2RailgunVisualFrames;
        const Vector3 aimTarget {
            ToWorldX(railgunLocked ? enemy.actionX : enemy.turretAimX),
            ToWorldY(railgunLocked ? enemy.actionY : enemy.turretAimY),
            railgunLocked ? Math::Lerp(SidePlaneZ, enemy.actionZ, railWeight) :
                Math::Lerp(SidePlaneZ, enemy.turretAimZ, railWeight)
        };
        bool weaponsDestroyed = true;
        for (int i = 0; i < BossPartCount; ++i) {
            if (i != BossRightEngine && enemy.bossPartHp[i] > 0) weaponsDestroyed = false;
        }
        const bool separated = enemy.phase >= 2.0f;
        const float battleshipPatrolX = enemy.phase >= 2.0f ?
            std::sin(static_cast<float>(enemy.age) * 0.018f) * 2.4f * railWeight : 0.0f;
        const BossModelTransform submarine {{x + ToWorldX(enemy.sandSubmarineOffsetX),
            Stage2SubmarineWorldY(enemy),
            z + enemy.sandSubmarineOffsetZ}, aimTarget, yaw + (separated ? Math::HalfPi : 0.0f), BossScale};
        BossModelTransform battleship {{x + ToWorldX(enemy.landBattleshipOffsetX) + battleshipPatrolX,
            Stage2BattleshipWorldY(enemy),
            z + enemy.landBattleshipOffsetZ}, aimTarget, yaw, BossScale,
            enemy.phase >= 3.0f && enemy.stage2BossAction != Stage2BossAction::Separating, true};
        battleship.secondaryAimTarget = {ToWorldX(enemy.turretAimX), ToWorldY(enemy.turretAimY),
            Math::Lerp(SidePlaneZ, enemy.turretAimZ, railWeight)};
        BossModelDamageState damage {
            enemy.bossPartHp[BossNose] > 0,
            {enemy.bossPartHp[BossLeftWing] > 0, enemy.bossPartHp[BossRightWing] > 0,
                enemy.bossPartHp[BossLeftEngine] > 0},
            enemy.bossPartHp[BossRightEngine] > 0,
            weaponsDestroyed,
            enemy.bossPartHitFlashFrames[BossNose] > 0 && (enemy.bossPartHitFlashFrames[BossNose] / 2) % 2 != 0,
            {enemy.bossPartHitFlashFrames[BossLeftWing] > 0 && (enemy.bossPartHitFlashFrames[BossLeftWing] / 2) % 2 != 0,
                enemy.bossPartHitFlashFrames[BossRightWing] > 0 && (enemy.bossPartHitFlashFrames[BossRightWing] / 2) % 2 != 0,
                enemy.bossPartHitFlashFrames[BossLeftEngine] > 0 && (enemy.bossPartHitFlashFrames[BossLeftEngine] / 2) % 2 != 0},
            enemy.bossPartHitFlashFrames[BossRightEngine] > 0 && (enemy.bossPartHitFlashFrames[BossRightEngine] / 2) % 2 != 0
        };
        for (int i = 0; i < BossFunnelHatchCount; ++i) {
            damage.funnelHatches[i] = enemy.bossPartHp[BossFunnelHatch0 + i] > 0;
            const int frames = enemy.bossPartHitFlashFrames[BossFunnelHatch0 + i];
            damage.funnelHatchesHit[i] = frames > 0 && (frames / 2) % 2 != 0;
        }
        auto DrawBossPart = [&](int shape, const Vector3& position, const Vector3& scale,
            const float color[4], float partYaw, float partPitch) {
            DrawModelPrimitive(renderer, camera, shape, position.x, position.y, position.z,
                scale.x, scale.y, scale.z, color, partYaw, partPitch);
        };
        SandSubmarineView::Draw(submarine, DrawBossPart, damage);
        LandBattleshipView::Draw(battleship, DrawBossPart, damage);

        const bool introducing = m_bossIntroductionPhase == BossIntroductionPhase::Entrance;

        // 登場中とPhase2以降は潜砂艦の移動位置へファンネル出現時と同じ砂埃を連続発生させる
        if (separated || introducing) {
            constexpr int DustLifetimeFrames = 28;
            constexpr float SideGroundTopY = -6.0f;
            constexpr float RailGroundTopY = -3.65f;
            constexpr float EdgeLocalX[] = {-4.2f, 0.0f, 4.2f};
            constexpr float HullHalfWidth = 1.58f;
            const float groundTopY = Math::Lerp(SideGroundTopY, RailGroundTopY, railWeight);
            const float cosine = std::cos(submarine.yaw);
            const float sine = std::sin(submarine.yaw);
            const int effectAge = introducing ? m_bossIntroductionTimer : enemy.age;
            for (int edge = 0; edge < 6; ++edge) {
                const float localX = EdgeLocalX[edge / 2];
                const float localZ = (edge % 2 == 0 ? -1.0f : 1.0f) * HullHalfWidth * railWeight;
                const int dustAge = (effectAge + edge * DustLifetimeFrames / 6) % DustLifetimeFrames;
                DrawSandDust(renderer, camera, {
                    submarine.position.x + (localX * cosine + localZ * sine) * submarine.scale,
                    groundTopY,
                    submarine.position.z + (-localX * sine + localZ * cosine) * submarine.scale
                }, dustAge, railWeight);
            }
        }

        // 合体接近の後半は補助エンジンを止め、炎が潜砂艦を貫通しないようにする
        const bool introductionEngineVisible = introducing &&
            m_bossIntroductionTimer < Stage2BossApproachFrames + Stage2BossAssemblyFrames / 2;
        if (separated || introductionEngineVisible) {
            constexpr float EngineLocalX[] = {-1.80f, -1.80f, 1.70f, 1.70f};
            constexpr float EngineLocalZ[] = {-0.78f, 0.78f, -0.78f, 0.78f};
            constexpr int EngineFlameEffectType = 3;
            const float cosine = std::cos(battleship.yaw);
            const float sine = std::sin(battleship.yaw);
            for (int engine = 0; engine < 4; ++engine) {
                const Vector3 nozzle {
                    battleship.position.x +
                        (EngineLocalX[engine] * cosine + EngineLocalZ[engine] * sine) * battleship.scale,
                    battleship.position.y + 0.02f * battleship.scale,
                    battleship.position.z +
                        (-EngineLocalX[engine] * sine + EngineLocalZ[engine] * cosine) * battleship.scale
                };
                const float flameHalfLength = 0.82f + static_cast<float>(engine % 2) * 0.10f;
                const Matrix4x4 world = Matrix4x4::Translation({
                    nozzle.x, nozzle.y - flameHalfLength, nozzle.z
                }) * Matrix4x4::Scale({0.34f, flameHalfLength, 1.0f});
                renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                    static_cast<float>(introducing ? m_bossIntroductionTimer : enemy.age) / 12.0f +
                        static_cast<float>(engine) * 0.71f,
                    EngineFlameEffectType});
            }
        }

        // 各武装の損傷位置から煙を上げる
        constexpr Vector3 SmokeOffsets[] = {
            {-1.55f, 2.55f, 0.0f}, {-0.55f, 2.70f, -0.92f},
            {0.65f, 2.70f, 0.0f}, {2.85f, 2.28f, 0.92f}
        };
        for (int part = BossNose; part <= BossLeftEngine; ++part) {
            const Vector3& offset = SmokeOffsets[part];
            const float cosine = std::cos(battleship.yaw);
            const float sine = std::sin(battleship.yaw);
            DrawDamageSmoke(static_cast<BossPart>(part), {
                battleship.position.x + (offset.x * cosine + offset.z * sine) * battleship.scale,
                battleship.position.y + offset.y * battleship.scale,
                battleship.position.z + (-offset.x * sine + offset.z * cosine) * battleship.scale
            }, 1.25f);
        }
        const float submarineCosine = std::cos(submarine.yaw);
        const float submarineSine = std::sin(submarine.yaw);
        DrawDamageSmoke(BossRightEngine, {
            submarine.position.x + 0.25f * submarineCosine * submarine.scale,
            submarine.position.y + 1.75f * submarine.scale,
            submarine.position.z - 0.25f * submarineSine * submarine.scale
        }, 1.35f);
        for (int hatch = 0; hatch < BossFunnelHatchCount; ++hatch) {
            const float side = hatch < 6 ? -1.0f : 1.0f;
            const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
            const float cosine = std::cos(submarine.yaw);
            const float sine = std::sin(submarine.yaw);
            DrawDamageSmoke(static_cast<BossPart>(BossFunnelHatch0 + hatch), {
                submarine.position.x + (localX * cosine + side * 1.8f * sine) * submarine.scale,
                submarine.position.y + 0.10f * submarine.scale,
                submarine.position.z + (-localX * sine + side * 1.8f * cosine) * submarine.scale
            }, 0.65f);
        }

        // Phase3では発射直後だけ、専用HLSLで固定照準の軌跡を急速に消す
        const int beamCycle = enemy.stage2BossActionAge % Stage2RailgunCycleFrames;
        const int beamAge = beamCycle - Stage2RailgunFireFrame;
        const bool pointerVisible = beamCycle < Stage2RailgunFireFrame &&
            enemy.stage2BossAction != Stage2BossAction::Separating;
        const bool beamVisible = beamAge >= 0 && beamAge < Stage2RailgunVisualFrames;
        const bool mirageVisible = beamAge >= 0 && beamAge < Stage2RailgunMirageFrames;
        if (enemy.phase >= 3.0f && damage.mainGun && (pointerVisible || beamVisible || mirageVisible)) {
            const float cosine = std::cos(battleship.yaw);
            const float sine = std::sin(battleship.yaw);
            const Vector3 pivot {
                battleship.position.x + (-1.55f * cosine) * battleship.scale,
                battleship.position.y + 2.08f * battleship.scale,
                battleship.position.z + (1.55f * sine) * battleship.scale
            };
            const Vector3 lockedTarget {ToWorldX(enemy.actionX), ToWorldY(enemy.actionY),
                railWeight > 0.5f ? enemy.actionZ : SidePlaneZ};
            const Vector3 delta = lockedTarget - pivot;
            const float horizontal = (std::max)(0.001f, std::sqrt(delta.x * delta.x + delta.z * delta.z));
            const float targetLength = (std::max)(0.001f, std::sqrt(horizontal * horizontal + delta.y * delta.y));
            const Vector3 direction = delta / targetLength;
            const float beamLength = targetLength + 18.0f;
            const Vector3 beamCenter = pivot + direction * (beamLength * 0.5f);
            const float beamYaw = std::atan2(direction.z, -direction.x);
            const float beamPitch = -std::asin(direction.y);
            auto DrawRailgunLayer = [&](float width, float progress, int effectType) {
                const Matrix4x4 world = Matrix4x4::Translation(beamCenter) *
                    Matrix4x4::RotationY(beamYaw) * Matrix4x4::RotationZ(beamPitch) *
                    Matrix4x4::Scale({beamLength * 0.5f, width, 1.0f});
                renderer.DrawRailgun({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                    progress, effectType});
            };
            if (mirageVisible) {
                DrawRailgunLayer(1.75f,
                    static_cast<float>(beamAge) / static_cast<float>(Stage2RailgunMirageFrames), 2);
            }
            if (pointerVisible) {
                DrawRailgunLayer(0.10f,
                    static_cast<float>(beamCycle) / static_cast<float>(Stage2RailgunFireFrame), 1);
            } else if (beamVisible) {
                DrawRailgunLayer(0.85f,
                    static_cast<float>(beamAge) / static_cast<float>(Stage2RailgunVisualFrames), 0);
            }
        }
        return;
    }

    if (enemy.type == 2) {
        // Stage2以外は従来の大型戦闘機モデルを維持する
        constexpr float ModelScale = 0.14f;
        constexpr float Gray[] = {0.50f, 0.50f, 0.50f, 1.0f};
        constexpr float White[] = {0.60f, 0.60f, 0.60f, 1.0f};
        constexpr float Black[] = {0.20f, 0.20f, 0.20f, 1.0f};
        constexpr float HitColor[] = {1.0f, 0.03f, 0.03f, 1.0f};
        auto PartColor = [&](BossPart part, const float color[4]) {
            const int frames = enemy.bossPartHitFlashFrames[part];
            return frames > 0 && (frames / 2) % 2 != 0 ? HitColor : color;
        };
        auto DrawBossPart = [&](int shape, float localX, float localY, float localZ,
            float width, float height, float depth, const float color[4]) {
            const Vector3 offset = RotateYawOffset(localX * ModelScale, localY * ModelScale,
                localZ * ModelScale, yaw);
            DrawModelPrimitive(renderer, camera, shape, x + offset.x, y + offset.y, z + offset.z,
                width * ModelScale, height * ModelScale, depth * ModelScale, color, yaw);
        };

        // 機首と上部メインボディを描画する
        if (enemy.bossPartHp[BossNose] > 0) {
            DrawBossPart(2, 0.0f, 3.0f, -14.0f, 6.0f, 6.0f, 4.0f, PartColor(BossNose, Gray));
            DrawBossPart(2, 0.0f, 2.0f, -17.5f, 2.0f, 2.0f, 3.0f, PartColor(BossNose, Gray));
            DrawBossPart(2, 0.0f, 4.5f, -20.0f, 1.0f, 1.0f, 8.0f, PartColor(BossNose, Black));
        }
        DrawBossPart(2, 0.0f, 2.0f, 0.0f, 18.0f, 18.0f, 16.0f, Gray);
        DrawBossPart(2, 0.0f, 2.0f, -10.0f, 14.0f, 14.0f, 4.0f, Gray);
        DrawBossPart(2, 0.0f, 2.0f, 10.0f, 14.0f, 14.0f, 4.0f, Gray);
        DrawBossPart(1, 0.0f, 12.0f, 2.0f, 4.0f, 4.0f, 4.0f, Gray);
        DrawBossPart(2, 0.0f, 13.0f, -2.0f, 1.0f, 1.0f, 4.0f, Black);

        // 下部ボディと左右主翼を描画する
        DrawBossPart(2, 0.0f, -12.0f, 0.0f, 4.0f, 4.0f, 10.0f, Gray);
        DrawBossPart(2, 0.0f, -15.0f, 1.0f, 2.0f, 2.0f, 8.0f, Gray);
        DrawBossPart(2, 0.0f, -12.0f, -7.0f, 1.0f, 1.0f, 6.0f, Black);
        DrawBossPart(1, 2.0f, -8.0f, 0.0f, 1.0f, 5.0f, 1.0f, Black);
        DrawBossPart(1, -2.0f, -8.0f, 0.0f, 1.0f, 5.0f, 1.0f, Black);
        if (enemy.bossPartHp[BossLeftWing] > 0) {
            DrawBossPart(1, 13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, PartColor(BossLeftWing, White));
            DrawBossPart(1, 21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, PartColor(BossLeftWing, White));
        }
        if (enemy.bossPartHp[BossRightWing] > 0) {
            DrawBossPart(1, -13.0f, 2.0f, 0.0f, 8.0f, 4.0f, 12.0f, PartColor(BossRightWing, White));
            DrawBossPart(1, -21.0f, 2.0f, 0.0f, 12.0f, 2.0f, 10.0f, PartColor(BossRightWing, White));
        }

        // 主エンジンと左右エンジンを描画する
        DrawBossPart(2, 0.0f, 3.0f, 15.0f, 10.0f, 10.0f, 6.0f, Gray);
        DrawBossPart(2, 7.0f, 3.0f, 18.0f, 4.0f, 4.0f, 6.0f, Black);
        DrawBossPart(2, -7.0f, 3.0f, 18.0f, 4.0f, 4.0f, 6.0f, Black);
        DrawBossPart(1, 0.0f, -6.0f, 16.5f, 2.0f, 8.0f, 3.0f, White);
        DrawBossPart(1, 0.0f, 12.0f, 16.5f, 2.0f, 8.0f, 3.0f, White);
        if (enemy.bossPartHp[BossLeftEngine] > 0) {
            DrawBossPart(2, 6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, PartColor(BossLeftEngine, Black));
            DrawBossPart(2, 6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, PartColor(BossLeftEngine, Black));
        }
        if (enemy.bossPartHp[BossRightEngine] > 0) {
            DrawBossPart(2, -6.0f, -6.0f, 10.0f, 4.0f, 4.0f, 10.0f, PartColor(BossRightEngine, Black));
            DrawBossPart(2, -6.0f, -6.0f, 16.0f, 2.0f, 2.0f, 2.0f, PartColor(BossRightEngine, Black));
        }
        // 従来ボスの主要部位位置から煙を上げる
        constexpr float SmokeX[] = {0.0f, 17.0f, -17.0f, 6.0f, -6.0f};
        constexpr float SmokeY[] = {4.5f, 3.0f, 3.0f, -4.0f, -4.0f};
        constexpr float SmokeZ[] = {-17.0f, 0.0f, 0.0f, 13.0f, 13.0f};
        for (int part = BossNose; part <= BossRightEngine; ++part) {
            const Vector3 offset = RotateYawOffset(SmokeX[part] * ModelScale,
                SmokeY[part] * ModelScale, SmokeZ[part] * ModelScale, yaw);
            DrawDamageSmoke(static_cast<BossPart>(part), {x + offset.x, y + offset.y, z + offset.z}, 0.75f);
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

/**
 * @brief 砂面から放物線状に舞う砂埃を描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @param position 発生地点のワールド座標
 * @param age 発生からの経過フレーム
 * @param railWeight 3D表示の補間率
 * @return なし
 */
void SideScrollingShooter::DrawSandDust(Renderer& renderer, const Camera3D& camera,
    const Vector3& position, int age, float railWeight) {
    constexpr int DustLifetimeFrames = 28;
    constexpr int DustParticleCount = 13;
    constexpr float DustColor[] = {0.62f, 0.43f, 0.20f, 1.0f};
    static_assert(DustLifetimeFrames > 0 && DustParticleCount > 0);
    if (age < 0 || age >= DustLifetimeFrames) return;

    // ウミヘビの水しぶきと同じ放物線で、2Dでは横方向、3Dでは円形へ砂粒を広げる
    const float progress = static_cast<float>(age) / static_cast<float>(DustLifetimeFrames);
    const float fade = 1.0f - progress;
    for (int i = 0; i < DustParticleCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) /
            static_cast<float>(DustParticleCount);
        const float launchVelocity = 1.10f + static_cast<float>((i * 5) % 4) * 0.18f;
        const float height = launchVelocity * progress - launchVelocity * progress * progress;
        const float radius = (0.28f + static_cast<float>(i % 3) * 0.10f) * progress;
        const float sideOffsetX = static_cast<float>(i - 6) * 0.12f * progress;
        const float offsetX = Math::Lerp(sideOffsetX, std::cos(angle) * radius, railWeight);
        const float offsetZ = Math::Lerp(0.0f, std::sin(angle) * radius, railWeight);
        const float size = (0.14f + static_cast<float>(i % 4) * 0.035f) * fade;
        const float color[4] = {DustColor[0], DustColor[1], DustColor[2], fade * 0.82f};
        DrawModelPrimitive(renderer, camera, 5, position.x + offsetX,
            position.y + 0.06f + height, position.z + offsetZ,
            size, size * 1.35f, size, color);
    }
}

void SideScrollingShooter::DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw) const {
    if (shot.enemy) {
        if (shot.funnel || shot.missile) {
            // 砂地を抜けた地点へ砂埃を発生させる
            constexpr int DustLifetimeFrames = 28;
            if (shot.funnel && shot.funnelDustAge >= 0 && shot.funnelDustAge < DustLifetimeFrames) {
                const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
                DrawSandDust(renderer, camera, {ToWorldX(shot.funnelDustX),
                    ToWorldY(shot.funnelDustY), shot.funnelDustZ}, shot.funnelDustAge, railWeight);
            }
            constexpr float FunnelBody[] = {0.18f, 0.16f, 0.14f, 1.0f};
            constexpr float FunnelEdge[] = {0.72f, 0.20f, 0.08f, 1.0f};
            const float dx = ToWorldX(shot.vx);
            const float dy = ToWorldY(shot.vy);
            const float dz = shot.vz;
            const float horizontal = (std::max)(0.001f, std::sqrt(dx * dx + dz * dz));
            const float length = (std::max)(0.001f, std::sqrt(horizontal * horizontal + dy * dy));
            const float funnelYaw = std::atan2(dz, -dx);
            const float funnelPitch = -std::asin(dy / length);
            DrawModelPrimitive(renderer, camera, 3, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                0.72f, 0.30f, 0.30f, FunnelBody, funnelYaw, funnelPitch);
            DrawModelPrimitive(renderer, camera, 4, ToWorldX(shot.x) - dx / length * 0.28f,
                ToWorldY(shot.y) - dy / length * 0.28f, shot.z - dz / length * 0.28f,
                0.44f, 0.42f, 0.42f, FunnelEdge, funnelYaw, funnelPitch);
            if (shot.funnel) {
                // 推進方向の後方へ既存の煙シェーダーを置き、ロケット噴出煙として流用する
                const Vector3 smokeCenter {ToWorldX(shot.x) - dx / length * 0.72f,
                    ToWorldY(shot.y) - dy / length * 0.72f, shot.z - dz / length * 0.72f};
                const Matrix4x4 smokeWorld = Matrix4x4::Translation(smokeCenter) *
                    Matrix4x4::Scale({0.42f, 0.62f, 1.0f});
                renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld,
                    static_cast<float>(shot.age) / 9.0f, 1});
            }
            return;
        }
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
    if (explosion.destruction) {
        const float progress = static_cast<float>(explosion.age) / DestructionExplosionLifetimeFrames;
        const float fireProgress = Math::Clamp01(progress * 2.55f);
        const float fireSize = 0.72f + fireProgress * 1.55f;
        const Vector3 center {ToWorldX(explosion.x), ToWorldY(explosion.y), explosion.z};

        // 中心火球とずらした火球を重ね、撃破直後の爆発炎を厚くする
        const Matrix4x4 fireWorld = Matrix4x4::Translation(center) *
            Matrix4x4::Scale({fireSize, fireSize, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * fireWorld, fireProgress});
        constexpr Vector3 FireOffsets[] = {{-0.52f, 0.18f, 0.0f}, {0.46f, 0.34f, 0.0f}};
        for (int i = 0; i < 2; ++i) {
            const float lobeProgress = Math::Clamp01(fireProgress * 1.18f - static_cast<float>(i) * 0.10f);
            const Matrix4x4 lobeWorld = Matrix4x4::Translation(center + FireOffsets[i] * fireSize) *
                Matrix4x4::Scale({fireSize * 0.68f, fireSize * 0.68f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * lobeWorld, lobeProgress});
        }

        // 炎が消える頃から大きな黒煙を残す
        const float smokeSize = 1.15f + progress * 1.85f;
        const Matrix4x4 smokeWorld = Matrix4x4::Translation(center) *
            Matrix4x4::Scale({smokeSize, smokeSize * 1.28f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld, progress, 2});
        return;
    }

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
void SideScrollingShooter::DrawDebris(Renderer& renderer, const Camera3D& camera,
    const Debris& debris, float railWeight) {
    const float scale = debris.age > debris.shrinkStartAge ?
        1.0f - Math::Clamp01(static_cast<float>(debris.age - debris.shrinkStartAge) /
            static_cast<float>(debris.lifetime - debris.shrinkStartAge)) : 1.0f;
    DrawModelPrimitive(renderer, camera, debris.shape, debris.x, debris.y, debris.z,
        debris.width * scale, debris.height * scale, debris.depth * scale, debris.color.data(), debris.yaw);

    // 沈下と再浮上の各区間で砂埃を絶えず重ね、船体幅全体から大量に噴き上げる
    if (debris.effect == Debris::Effect::Stage2Sink) {
        constexpr int FirstSinkEndFrame = 108;
        constexpr int ResurfaceStartFrame = 148;
        const bool movingThroughSand = debris.age < FirstSinkEndFrame ||
            debris.age >= ResurfaceStartFrame || debris.effectAge >= 0;
        const float groundTopY = Math::Lerp(-6.0f, -3.65f, railWeight);
        if (movingThroughSand) {
            for (int wave = 0; wave < 4; ++wave) {
                const int dustAge = (debris.age + wave * 7) % 28;
                for (int side = -2; side <= 2; ++side) {
                    DrawSandDust(renderer, camera,
                        {debris.x + side * debris.width * 0.16f, groundTopY, debris.z}, dustAge, railWeight);
                }
            }
        }
    }

    // 降下中は連続爆発と煙を引き、下部船体との衝突後に大爆発へ切り替える
    if (debris.effect == Debris::Effect::Stage2Impact) {
        constexpr int EffectFrames = 72;
        const Matrix4x4 viewProjection = camera.ProjectionMatrix() * camera.ViewMatrix();
        if (debris.effectAge < 0) {
            for (int i = 0; i < 3; ++i) {
                const int cycleAge = (debris.age + i * 17) % 54;
                const float cycle = static_cast<float>(cycleAge) / 54.0f;
                const float offsetX = static_cast<float>(i - 1) * debris.width * 0.22f;
                const float size = debris.width * (0.18f + cycle * 0.24f);
                const Matrix4x4 fireWorld = Matrix4x4::Translation(
                    {debris.x + offsetX, debris.y + debris.height * 0.18f, debris.z}) *
                    Matrix4x4::Scale({size, size, 1.0f});
                renderer.DrawExplosion({viewProjection * fireWorld, cycle});
            }
            const float smokeCycle = static_cast<float>(debris.age % 72) / 72.0f;
            const float smokeSize = debris.width * (0.34f + smokeCycle * 0.38f);
            const Matrix4x4 smokeWorld = Matrix4x4::Translation(
                {debris.x, debris.y + debris.height * (0.45f + smokeCycle * 0.75f), debris.z}) *
                Matrix4x4::Scale({smokeSize, smokeSize * 1.45f, 1.0f});
            renderer.DrawExplosion({viewProjection * smokeWorld, smokeCycle, 2});
            return;
        }

        const float progress = Math::Clamp01(static_cast<float>(debris.effectAge) / EffectFrames);
        for (int i = 0; i < 5; ++i) {
            const float delayed = Math::Clamp01(progress * 1.55f - static_cast<float>(i) * 0.10f);
            const float offsetX = static_cast<float>(i - 2) * debris.width * 0.16f;
            const float size = debris.width * (0.34f + delayed * 0.42f);
            const Matrix4x4 fireWorld = Matrix4x4::Translation(
                {debris.x + offsetX, debris.y + debris.height * 0.22f, debris.z}) *
                Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({viewProjection * fireWorld, delayed});
        }
        const float smokeSize = debris.width * (0.58f + progress * 0.72f);
        const Matrix4x4 smokeWorld = Matrix4x4::Translation(
            {debris.x, debris.y + debris.height * (0.35f + progress * 0.85f), debris.z}) *
            Matrix4x4::Scale({smokeSize, smokeSize * 1.35f, 1.0f});
        renderer.DrawExplosion({viewProjection * smokeWorld, progress, 2});
        for (int side = -2; side <= 2; ++side) {
            DrawSandDust(renderer, camera, {debris.x + side * debris.width * 0.18f,
                debris.y - debris.height * 0.5f, debris.z}, debris.effectAge, railWeight);
        }
    }
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

/**
 * @brief ミッション開始または終了の文字アニメーションを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawMissionBanner(Renderer& renderer) const {
    if ((!m_clear && m_missionStartTimer <= 0) || (m_clear && m_clearTimer <= 0)) return;

    char startText[24];
    std::snprintf(startText, sizeof(startText), "MISSION %d START", m_stageNumber);
    const std::string_view text = m_clear ? "ARRESTED" : startText;
    const int remainingFrames = m_clear ? (std::max)(0, m_clearTimer) : m_missionStartTimer;
    const int elapsedFrames = (m_clear ? ClearWaitFrames : MissionBannerDisplayFrames) - remainingFrames;
    const float fade = remainingFrames < 20 ? static_cast<float>(remainingFrames) / 20.0f : 1.0f;
    constexpr float BaseSize = 0.050f;
    constexpr float Advance = 0.078f;
    const float firstX = -static_cast<float>(text.size() - 1) * Advance * 0.5f;

    // 各文字を時間差で大きく出し、中央の定位置へ収束させる
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == ' ') continue;
        const float scale = MissionBannerGlyphScale(elapsedFrames - static_cast<int>(i) * MissionBannerGlyphDelayFrames);
        if (scale <= 0.0f) continue;
        const char glyph[] = {text[i], '\0'};
        const float size = BaseSize * scale;
        const Vector2 position {firstX + static_cast<float>(i) * Advance, 0.10f};
        renderer.DrawText(glyph, position + Vector2 {0.012f, -0.014f}, size,
            {0.05f, 0.02f, 0.01f, fade * 0.85f});
        renderer.DrawText(glyph, position, size, {1.0f, 0.78f, 0.12f, fade});
    }
}

void SideScrollingShooter::DrawBossHud(Renderer& renderer) const {
    if (m_stageNumber == 5 && m_stage5Phase != Stage5Phase::Approach) {
        DrawStage5Hud(renderer);
        return;
    }
    if (!m_bossBattle || m_clear || m_bossIntroductionPhase != BossIntroductionPhase::None) {
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
        DrawShape(renderer, -BossBarWidth + BossBarWidth * 2.0f * static_cast<float>(phase) / static_cast<float>(BossPhaseCount),
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
void SideScrollingShooter::SpawnExplosion(float x, float y, float z, bool destruction) {
    for (auto& explosion : m_explosions) {
        if (explosion.active) continue;
        explosion = {x, y, IsRailGameplayActive() ? z : ToRailZFromSideX(x), 0, destruction, true};
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
 * @param lifetime 部品が消滅するまでのフレーム数
 * @param shrinkStartAge 縮小を開始するフレーム
 * @param gravity 重力を適用する場合true
 * @return なし
 */
void SideScrollingShooter::SpawnDebrisPiece(float x, float y, float z, float vx, float vy, float vz,
    float yaw, float spin, int shape, float width, float height, float depth, const float color[4],
    int lifetime, int shrinkStartAge, bool gravity, Debris::Effect effect) {
    for (auto& debris : m_debris) {
        if (debris.active) continue;
        debris = {x, y, z, vx, vy, vz, yaw, spin, width, height, depth,
            {color[0], color[1], color[2], color[3]}, shape, 0, lifetime, shrinkStartAge,
            -1, effect, gravity, true};
        return;
    }
}

/**
 * @brief 砂漠の骨アーチを破壊して小さな骨を飛散させる
 * @return なし
 */
void SideScrollingShooter::DestroyDesertBoneArch() {
    constexpr int BoneCount = 13;
    constexpr int Lifetime = 150;
    constexpr int ShrinkStartAge = 90;
    constexpr float RailCenterY = -3.65f;
    constexpr float RailRadius = 10.0f;
    constexpr float SideZ = SidePlaneZ + 1.2f;
    const float railWeight = RailBlend();
    const float phase = std::fmod(m_scroll * 20.0f, 72.0f);
    const float sideCenterX = 1.90f - std::fmod(m_scroll * 0.50f, 4.30f);
    const float railZ = 72.0f - phase;

    // 描画中の各関節位置から小さな骨を外向きに飛散させる
    for (int i = 0; i < BoneCount; ++i) {
        const float angle = Math::HalfPi * 2.0f * static_cast<float>(i) / static_cast<float>(BoneCount - 1);
        const float x = Math::Lerp(ToWorldX(sideCenterX), std::cos(angle) * RailRadius, railWeight);
        const float y = Math::Lerp(ToWorldY(-1.30f + static_cast<float>(i) * 0.24f),
            RailCenterY + std::sin(angle) * RailRadius, railWeight);
        const float z = Math::Lerp(SideZ, railZ, railWeight);
        const float scatterAngle = angle + static_cast<float>(i % 3 - 1) * 0.35f;
        const float size = 0.30f + static_cast<float>(i % 3) * 0.08f;
        SpawnDebrisPiece(x, y, z, std::cos(scatterAngle) * 0.055f,
            0.055f + static_cast<float>(i % 4) * 0.012f,
            railWeight > 0.5f ? std::sin(scatterAngle) * 0.055f : 0.0f,
            angle, (i % 2 == 0 ? 0.08f : -0.08f) * (1.0f + i * 0.04f),
            5, size, size * 1.35f, size * 0.75f, DesertBoneColor,
            Lifetime, ShrinkStartAge, true);
    }

    m_boneArchHp = 0;
    m_boneArchDestroyed = true;
}

/**
 * @brief 被弾または破壊された隕石から当たり判定を持たない小隕石を飛散させる
 * @param meteor 破片の発生元となる隕石
 * @param count 発生させる小隕石の数
 * @return なし
 */
void SideScrollingShooter::SpawnMeteorDebris(const Meteor& meteor, int count) {
    const float sideX = 1.85f - std::fmod(meteor.travel * 0.0325f, 4.40f);
    const float sideY = 0.55f + std::sin(meteor.travel * 0.105f) * 0.34f;
    const float railX = std::sin(meteor.travel * 0.090f) * 7.0f;
    const float railY = 0.80f + std::sin(meteor.travel * 0.135f) * 2.0f;
    const bool railMode = IsRailGameplayActive();
    const float x = railMode ? railX : ToWorldX(sideX);
    const float y = railMode ? railY : ToWorldY(sideY);
    const float z = railMode ? 72.0f - meteor.travel : SidePlaneZ + 1.2f;

    // 既存のデブリは判定に使われないため、そのまま小隕石の漂流表現として再利用する
    for (int i = 0; i < count; ++i) {
        const float angle = meteor.yaw + static_cast<float>(i) * Math::TwoPi / static_cast<float>(count);
        const float size = (0.20f + static_cast<float>(i % 3) * 0.08f) * meteor.scale;
        SpawnDebrisPiece(x, y, z, std::cos(angle) * 0.035f, std::sin(angle * 1.4f) * 0.030f,
            railMode ? std::sin(angle) * 0.045f : 0.0f, angle, meteor.spin * (1.5f + i * 0.1f),
            5, size, size * 0.85f, size * 0.75f, MeteorColor);
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

    if (m_stageNumber == 5) {
        constexpr EastsourcePartGroup Groups[] = {
            EastsourcePartGroup::Nose,
            EastsourcePartGroup::LeftWing,
            EastsourcePartGroup::RightWing,
            EastsourcePartGroup::LeftEngine,
            EastsourcePartGroup::RightEngine
        };
        const EastsourcePartGroup detached = bossPart >= BossNose && bossPart <= BossRightEngine ?
            Groups[bossPart] : EastsourcePartGroup::Body;
        EastsourceModelState intact;
        const Stage5ModelTransform transform = EastsourceTransform(enemy);

        // 破壊グループの実モデルパーツだけを既存の小型Debrisプールへ送る
        EastsourceModelView::VisitParts(transform, intact,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color,
                EastsourcePartGroup group) {
                if (group != detached) return;
                const Vector3 center = world.TransformPoint(Vector3::Zero);
                const float radius = (std::max)(0.24f,
                    (std::min)(1.2f, Stage5ModelDetail::WorldPartRadius(world) * 0.55f));
                const float direction = center.x < ToWorldX(enemy.x) ? -1.0f : 1.0f;
                const float pieceColor[] = {color.r, color.g, color.b, color.a};
                const int debrisShape = shape == PrimitiveShape::Box ? 1 :
                    (shape == PrimitiveShape::Cylinder ? 2 :
                        (shape == PrimitiveShape::Cone ? 3 :
                            (shape == PrimitiveShape::Prism ? 4 : 5)));
                SpawnDebrisPiece(center.x, center.y, center.z,
                    direction * (0.035f + static_cast<float>(pieceNumber % 3) * 0.008f),
                    0.018f + static_cast<float>(pieceNumber % 2) * 0.012f,
                    -0.025f + static_cast<float>(pieceNumber % 3) * 0.018f,
                    0.0f, direction * 0.10f, debrisShape,
                    radius, radius * 0.65f, radius, pieceColor, 90, 64, false);
                ++pieceNumber;
            });
        return;
    }

    if (m_stageNumber == 2) {
        constexpr float ModelScale = 1.92f;
        constexpr float Hull[] = {0.28f, 0.24f, 0.17f, 1.0f};
        constexpr float Armor[] = {0.36f, 0.31f, 0.22f, 1.0f};
        constexpr float Metal[] = {0.48f, 0.45f, 0.36f, 1.0f};
        constexpr float Dark[] = {0.07f, 0.065f, 0.055f, 1.0f};
        constexpr float Core[] = {0.95f, 0.28f, 0.055f, 1.0f};
        auto AddStage2Piece = [&](int shape, const Vector3& local, const Vector3& size,
            const float color[4], float unitOffsetY) {
            AddPiece(shape, local.x, local.y + unitOffsetY / ModelScale, local.z,
                size.x, size.y, size.z, color, ModelScale);
        };

        // 個別部位は描画と同じ構成の主要プリミティブを飛散させる
        if (bossPart == BossNose) {
            AddStage2Piece(2, {-1.55f, 2.03f, 0.0f}, {1.75f, 1.10f, 1.75f}, Dark, 0.78f);
            AddStage2Piece(4, {-2.30f, 2.08f, 0.0f}, {1.85f, 1.08f, 1.55f}, Armor, 0.78f);
            AddStage2Piece(2, {-4.25f, 2.08f, 0.0f}, {2.95f, 0.54f, 0.54f}, Metal, 0.78f);
            return;
        }
        if (bossPart == BossLeftWing || bossPart == BossRightWing || bossPart == BossLeftEngine) {
            constexpr Vector3 Positions[] = {
                {-0.55f, 2.34f, -0.92f}, {0.65f, 2.34f, 0.0f}, {2.85f, 1.92f, 0.92f}
            };
            const int index = bossPart == BossLeftWing ? 0 :
                (bossPart == BossRightWing ? 1 : 2);
            const Vector3& position = Positions[index];
            AddStage2Piece(2, position, {0.76f, 0.46f, 0.76f}, Dark, 0.78f);
            AddStage2Piece(1, {position.x - 0.20f, position.y + 0.26f, position.z},
                {0.82f, 0.46f, 0.64f}, Armor, 0.78f);
            AddStage2Piece(2, {position.x - 1.10f, position.y + 0.26f, position.z},
                {1.35f, 0.20f, 0.20f}, Metal, 0.78f);
            return;
        }
        if (bossPart == BossRightEngine) {
            AddStage2Piece(2, {0.25f, 1.42f, 0.0f}, {1.35f, 0.22f, 1.35f}, Core, -0.45f);
            return;
        }
        if (bossPart >= BossFunnelHatch0) {
            const int hatch = bossPart - BossFunnelHatch0;
            const float localX = -2.65f + static_cast<float>(hatch % 6) * 1.05f;
            const float localZ = (hatch < 6 ? -1.0f : 1.0f) * 1.80f;
            AddStage2Piece(1, {localX, -0.22f, localZ}, {0.38f, 0.34f, 0.08f}, Core, -0.45f);
            return;
        }

        // 撃破時は下部の沈下と再浮上、上部の緩やかな降下を同じ時間軸で開始する
        const float submarineY = Stage2SubmarineWorldY(enemy);
        const float battleshipY = Stage2BattleshipWorldY(enemy);
        const Vector3 lowerOffset = RotateYawOffset(0.0f, -0.45f, 0.0f, yaw);
        SpawnDebrisPiece(x + lowerOffset.x, submarineY + lowerOffset.y, enemy.z + lowerOffset.z,
            0.0f, 0.0f, 0.0f, yaw, 0.0f, 5, 8.8f * ModelScale, 2.05f * ModelScale,
            3.15f * ModelScale, Hull, 420, 420, false, Debris::Effect::Stage2Sink);
        auto AddFallingUpper = [&](int shape, const Vector3& local, const Vector3& size,
            const float color[4], Debris::Effect effect) {
            const Vector3 offset = RotateYawOffset(local.x * ModelScale, local.y * ModelScale,
                local.z * ModelScale, yaw);
            SpawnDebrisPiece(x + offset.x, battleshipY + offset.y + 3.25f, enemy.z + offset.z,
                0.0f, 0.0f, 0.0f, yaw,
                0.018f + local.x * 0.004f, shape, size.x * ModelScale, size.y * ModelScale,
                size.z * ModelScale, color, 420, 420, true, effect);
        };
        AddFallingUpper(1, {0.20f, 0.95f, 0.0f}, {2.65f, 1.30f, 2.70f}, Hull,
            Debris::Effect::Stage2Impact);
        AddFallingUpper(4, {-1.65f, 0.70f, 0.0f}, {2.70f, 1.08f, 2.55f}, Hull,
            Debris::Effect::Stage2ImpactPiece);
        AddFallingUpper(4, {2.65f, 1.62f, 0.0f}, {1.45f, 0.76f, 1.78f}, Armor,
            Debris::Effect::Stage2ImpactPiece);
        for (int part = 0; part < BossPartCount; ++part) {
            if (enemy.bossPartHp[part] > 0) SpawnEnemyDebris(enemy, part);
        }
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

/**
 * @brief 墨の筆跡を模したボス名演出を画面へ描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawBossNameReveal(Renderer& renderer) const {
    if (m_bossIntroductionPhase != BossIntroductionPhase::NameReveal) return;

    const float age = static_cast<float>(m_bossIntroductionTimer);
    const float inkProgress = SmoothStep(Math::Clamp01(age / 34.0f));
    const float fadeOut = SmoothStep(Math::Clamp01(
        static_cast<float>(BossNameRevealFrames - m_bossIntroductionTimer) / 24.0f));
    const float alpha = inkProgress * fadeOut;
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.0f, 0.0f, 0.0f, 0.52f * alpha});

    // 高さと濃さの異なる筆跡を横へ走らせて、乾いた墨のかすれを作る
    constexpr float StrokeY[] = {-0.22f, -0.14f, -0.07f, 0.01f, 0.09f, 0.16f, 0.22f};
    constexpr float StrokeHeight[] = {0.08f, 0.10f, 0.12f, 0.13f, 0.11f, 0.09f, 0.06f};
    for (int i = 0; i < 7; ++i) {
        const float reveal = SmoothStep(Math::Clamp01(
            inkProgress * 1.30f - static_cast<float>(i) * 0.035f));
        const float width = (1.35f + static_cast<float>((i * 7) % 4) * 0.11f) * reveal;
        const float strokeColor[4] = {
            0.01f, 0.008f, 0.006f, alpha * (0.78f + static_cast<float>(i % 2) * 0.16f)
        };
        DrawShape(renderer, 0.0f, StrokeY[i], width, StrokeHeight[i], strokeColor);
    }

    // 筆の始点と終点へ不揃いな飛沫を置き、矩形だけの帯に見えないよう崩す
    constexpr Vector2 SplatterPositions[] = {
        {-0.82f, 0.27f}, {-0.72f, -0.31f}, {-0.60f, 0.34f},
        {0.66f, 0.31f}, {0.78f, -0.27f}, {0.88f, 0.13f}, {0.57f, -0.36f}
    };
    for (int i = 0; i < 7; ++i) {
        const float radius = (0.018f + static_cast<float>((i * 5) % 4) * 0.009f) * inkProgress;
        renderer.Draw(Circle {SplatterPositions[i], radius},
            {0.01f, 0.008f, 0.006f, alpha * 0.90f});
    }

    // 墨が広がった後にボス名を打ち込み、短い朱色の見得線を添える
    const float nameAlpha = SmoothStep(Math::Clamp01((age - 24.0f) / 18.0f)) * fadeOut;
    const BossStory story = BossStories::ForStage(m_stageNumber);
    renderer.DrawText(story.bossName, TextAlign::Center, 0.064f,
        {0.94f, 0.92f, 0.84f, nameAlpha}, {0.0f, -0.015f}, 0.008f);
    const float redLine[4] = {0.72f, 0.04f, 0.025f, nameAlpha};
    DrawShape(renderer, 0.0f, -0.33f, 0.84f * inkProgress, 0.018f, redLine);
}

/**
 * @brief Stage 5の要塞、照明、崩壊演出を3D空間へ描画する
 * @param renderer 描画先レンダラー
 * @param camera 現在の3Dカメラ
 * @return なし
 */
void SideScrollingShooter::RenderStage5(Renderer& renderer, const Camera3D& camera) const {
    const Stage5ModelTransform transform = TayamaTransform();
    TayamaModelState state = TayamaState();
    const bool lightning = m_stage5Phase < Stage5Phase::TayamaCommandCore &&
        ((m_frame % 241) < 3 || ((m_frame + 73) % 389) < 2);
    if (lightning) {
        for (bool& flash : state.hitFlash) flash = true;
    }

    // 同じ46パーツをビル端点から空母端点まで補間して描画する
    TayamaModelView::VisitParts(transform, m_tayamaTransformation, state,
        [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color, TayamaPartGroup) {
            const float brightness = lightning ? 1.55f : 1.0f;
            const float partColor[] = {
                (std::min)(1.0f, color.r * brightness),
                (std::min)(1.0f, color.g * brightness),
                (std::min)(1.0f, color.b * brightness), color.a
            };
            DrawModelPrimitive(renderer, camera, static_cast<int>(shape), world, partColor);
        });

    // 変形終盤から既存のエンジン炎HLSLを主推進機と生存中の揚力機関へ付ける
    if (m_stage5Phase >= Stage5Phase::CarrierTransformation &&
        m_stage5Phase < Stage5Phase::TayamaCollapse) {
        constexpr TayamaPartGroup EngineGroups[] = {
            TayamaPartGroup::MainThruster,
            TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine
        };
        for (int engine = 0; engine < 3; ++engine) {
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                m_tayamaTransformation, state, EngineGroups[engine]);
            if (!bounds.valid) continue;
            const float width = engine == 0 ? 2.2f : 1.25f;
            const Matrix4x4 flameWorld = Matrix4x4::Translation(
                bounds.center + Vector3 {0.0f, -1.0f - static_cast<float>(engine) * 0.12f, 0.0f}) *
                Matrix4x4::Scale({width, 2.8f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * flameWorld,
                static_cast<float>((m_frame + engine * 7) % 24) / 24.0f, 3});
        }
    }

    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        // 破裂前は赤色警告灯と左右へ押し出される格納庫装甲を段階表示する
        const float warningColor[] = {1.0f, 0.04f, 0.03f,
            (m_stage5PhaseTimer / 5) % 2 == 0 ? 1.0f : 0.28f};
        for (int light = -2; light <= 2; ++light) {
            DrawModelPrimitive(renderer, camera, 1,
                static_cast<float>(light) * 1.35f, 1.5f, 61.5f,
                0.34f, 0.18f, 0.16f, warningColor);
        }
        const float deformation = SmoothStep(Math::Clamp01(
            static_cast<float>(m_stage5PhaseTimer - 18) / 40.0f));
        DrawModelPrimitive(renderer, camera, 1, -1.5f - deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor,
            0.0f, deformation * 0.16f);
        DrawModelPrimitive(renderer, camera, 1, 1.5f + deformation * 2.2f, -0.2f, 61.0f,
            3.0f, 4.2f, 0.35f, TowerFacadeColor, 0.0f, -deformation * 0.16f);
    }

    // 嵐の上層とCOMMAND CORE以降の雲海を少数のCube帯で表現する
    const bool aboveStorm = m_stage5Phase >= Stage5Phase::TayamaCommandCore;
    const int cloudCount = aboveStorm ? 22 : 14;
    for (int i = 0; i < cloudCount; ++i) {
        const float x = -34.0f + static_cast<float>((i * 47) % 680) / 10.0f;
        const float z = 24.0f + static_cast<float>((i * 31 + m_frame / 3) % 760) / 10.0f;
        const float y = aboveStorm ? -5.8f + static_cast<float>(i % 3) * 0.32f :
            12.0f + static_cast<float>(i % 4) * 1.1f;
        const float cloudColor[] = {
            aboveStorm ? 0.32f : StormCloudColor[0],
            aboveStorm ? 0.38f : StormCloudColor[1],
            aboveStorm ? 0.48f : StormCloudColor[2],
            aboveStorm ? 0.82f : 0.72f
        };
        DrawModelPrimitive(renderer, camera, 1, x, y, z,
            8.0f + static_cast<float>(i % 4) * 2.0f, 0.75f, 3.5f, cloudColor);
    }

    // 現フェーズのサーチライト基部と、追尾上限を持つ光軸を同じ座標で描画する
    int activeLights = 0;
    bool tayamaLights = false;
    if (m_stage5Phase == Stage5Phase::WallClimbLower) activeLights = 1;
    if (m_stage5Phase == Stage5Phase::WallClimbMiddle) activeLights = 2;
    if (m_stage5Phase == Stage5Phase::WallClimbUpper) activeLights = 3;
    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        activeLights = 2;
        tayamaLights = true;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceBattle && m_enemies[0].active &&
        m_enemies[0].bossPhase >= BossNormalPhase2 && m_enemies[0].age % 180 < 90) {
        activeLights = 1;
    }
    for (int index = 0; index < activeLights; ++index) {
        const SearchlightState& light = m_searchlights[index];
        if (light.destroyed) continue;
        Vector3 source {
            ToWorldX((static_cast<float>(index) - 1.0f) * 0.72f),
            ToWorldY(0.72f - static_cast<float>(index) * 0.22f),
            tayamaLights ? 57.0f : 46.0f
        };
        if (tayamaLights) {
            const TayamaPartGroup group = index == 0 ?
                TayamaPartGroup::LeftSearchlight : TayamaPartGroup::RightSearchlight;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
                transform, m_tayamaTransformation, state, group);
            if (bounds.valid) source = bounds.center;
        }
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector3 target {
            ToWorldX(locked ? light.lockedX : light.beamX),
            ToWorldY(locked ? light.lockedY : light.beamY), PlayerRailZ
        };
        const Vector3 delta = target - source;
        const float length = (std::max)(0.001f, delta.Length());
        const Vector3 direction = delta / length;
        const float yaw = std::atan2(direction.z, -direction.x);
        const float pitch = -std::asin(direction.y);
        const float* beamColor = locked ? SearchlightLockedColor : SearchlightColor;
        const Matrix4x4 beamWorld = Matrix4x4::Translation(source + direction * (length * 0.5f)) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale({length, locked ? 0.12f : 0.18f, locked ? 0.12f : 0.18f});
        DrawModelPrimitive(renderer, camera, static_cast<int>(PrimitiveShape::Box), beamWorld, beamColor);
        DrawModelPrimitive(renderer, camera, 2, source.x, source.y, source.z,
            0.72f, 0.42f, 0.72f, locked ? SearchlightLockedColor : SatelliteLightColor);
    }

    // 有効弱点へ小さな発光リングを重ねて攻略対象を明示する
    if (m_stage5Phase >= Stage5Phase::TayamaFireControl &&
        m_stage5Phase <= Stage5Phase::TayamaCommandCore) {
        constexpr TayamaPartGroup Groups[] = {
            TayamaPartGroup::LeftSearchlight, TayamaPartGroup::RightSearchlight,
            TayamaPartGroup::FireControlRadar, TayamaPartGroup::LeftLiftEngine,
            TayamaPartGroup::RightLiftEngine, TayamaPartGroup::CommandCore
        };
        for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
            if (!weakpoint.active || weakpoint.destroyed) continue;
            const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
                m_tayamaTransformation, state, Groups[static_cast<std::size_t>(weakpoint.type)]);
            if (!bounds.valid) continue;
            const float size = (std::max)(0.75f, (std::min)(2.2f, bounds.radius * 0.45f));
            const Matrix4x4 world = Matrix4x4::Translation(bounds.center + Vector3 {0.0f, 0.0f, -0.12f}) *
                Matrix4x4::Scale({size, size, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * world,
                static_cast<float>(m_frame % 30) / 30.0f, 0});
        }
    }

    if (m_stage5Phase == Stage5Phase::TayamaCollapse && m_stage5PhaseTimer >= 330 &&
        m_stage5PhaseTimer < TayamaCollapseFrames) {
        // 最終90フレームは内部白光と二重衝撃波で輪郭ごと消滅させる
        const float finalProgress = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 450) / 90.0f);
        const float glow = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 330) / 120.0f);
        const Matrix4x4 glowWorld = Matrix4x4::Translation(transform.position) *
            Matrix4x4::Scale({3.0f + glow * 8.0f, 2.0f + glow * 5.0f, 1.0f});
        renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * glowWorld,
            static_cast<float>(m_frame % 24) / 24.0f, 0});
        if (finalProgress > 0.0f) {
            const Matrix4x4 shockwave = Matrix4x4::Translation(transform.position) *
                Matrix4x4::Scale({4.0f + finalProgress * 24.0f,
                    4.0f + finalProgress * 24.0f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * shockwave,
                finalProgress, 0});
        }
    }
}

/**
 * @brief Stage 5の雨、稲光、照準表示を画面空間へ描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawStage5Weather(Renderer& renderer) const {
    if (m_stageNumber != 5) return;
    float intensity = 0.0f;
    if (m_stage5Phase == Stage5Phase::Approach) {
        intensity = 0.22f + static_cast<float>(m_chapterNumber - 1) * 0.27f;
    } else if (m_stage5Phase <= Stage5Phase::EastsourceFall) {
        intensity = 1.0f;
    } else if (m_stage5Phase <= Stage5Phase::WallClimbUpper) {
        intensity = 0.88f - m_tayamaTransformation * 0.28f;
    } else if (m_stage5Phase <= Stage5Phase::TayamaFireControl) {
        intensity = 0.42f;
    } else if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
        intensity = 0.22f;
    } else if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        intensity = 0.22f * (1.0f - Math::Clamp01(static_cast<float>(m_stage5PhaseTimer) / 180.0f));
    }

    // 最大96本の決定的な横殴り雨を画面空間へ流す
    const int rainCount = intensity > 0.0f ? static_cast<int>(32.0f + intensity * 64.0f) : 0;
    for (int index = 0; index < rainCount; ++index) {
        const int phase = (index * 73 + m_frame * (3 + index % 3)) % 220;
        const int row = (index * 47 + m_frame * (5 + index % 2)) % 210;
        const float x = -1.10f + static_cast<float>(phase) * 0.010f;
        const float y = 1.05f - static_cast<float>(row) * 0.010f;
        const float rainColor[] = {0.50f, 0.72f, 0.90f, 0.16f + intensity * 0.34f};
        DrawShape(renderer, x, y, 0.004f + intensity * 0.002f,
            0.035f + intensity * 0.055f, rainColor);
    }

    // 稲光はTAYAMAの輪郭と警告灯を一瞬だけ強調する
    if (intensity > 0.30f && ((m_frame % 241) < 3 || ((m_frame + 73) % 389) < 2)) {
        const float alpha = (m_frame % 2 == 0 ? 0.30f : 0.16f) * intensity;
        renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.72f, 0.82f, 1.0f, alpha});
    }

    // 甲板掃射、排気レーン、コアレーザーは発射前だけ危険範囲を固定表示する
    if (m_stage5Phase == Stage5Phase::TayamaFireControl && m_stage5AttackTimer % 210 < 36) {
        renderer.Draw(Rect {{0.0f, m_stage5CoreTargetY}, {1.86f, 0.055f}},
            {1.0f, 0.12f, 0.08f, 0.36f});
    }
    if (m_stage5Phase == Stage5Phase::TayamaLiftEngines && m_stage5AttackTimer % 132 < 32) {
        renderer.Draw(Rect {{m_stage5CoreTargetX, m_stage5CoreTargetY}, {0.38f, 0.075f}},
            {1.0f, 0.34f, 0.08f, 0.42f});
        renderer.Draw(Circle {{m_stage5CoreTargetX, m_stage5CoreTargetY}, 0.12f},
            {1.0f, 0.58f, 0.12f, 0.58f});
    }
    if (m_stage5Phase == Stage5Phase::TayamaCommandCore && m_stage5AttackTimer % 180 < 42) {
        const Vector2 target {m_stage5CoreTargetX, m_stage5CoreTargetY};
        renderer.Draw(Circle {target, 0.11f}, {1.0f, 0.08f, 0.04f, 0.62f});
    }

    // 検出円と固定ロック地点を表示し、光軸がロック後に追尾しないことを示す
    const bool eastsourceSearchlight = m_stage5Phase == Stage5Phase::EastsourceBattle &&
        m_enemies[0].active && m_enemies[0].bossPhase >= BossNormalPhase2 &&
        m_enemies[0].age % 180 < 90;
    const bool showSearchlights = eastsourceSearchlight ||
        (m_stage5Phase >= Stage5Phase::WallClimbLower &&
            m_stage5Phase <= Stage5Phase::WallClimbUpper) ||
        m_stage5Phase == Stage5Phase::TayamaFireControl;
    if (!showSearchlights) return;
    for (const SearchlightState& light : m_searchlights) {
        if (light.destroyed || light.phase == SearchlightPhase::Cooldown) continue;
        const bool locked = light.phase == SearchlightPhase::Locked ||
            light.phase == SearchlightPhase::Firing;
        const Vector2 target {locked ? light.lockedX : light.beamX,
            locked ? light.lockedY : light.beamY};
        const bool detecting = light.phase == SearchlightPhase::Detecting;
        const ColorF color = locked ? ColorF {1.0f, 0.08f, 0.08f, 0.86f} :
            (detecting ? ColorF {1.0f, 0.78f, 0.18f, 0.34f} :
                ColorF {0.92f, 0.82f, 0.42f, 0.16f});
        renderer.Draw(Circle {target, locked ? 0.075f : SearchlightDetectionRadius}, color);
        if (locked) {
            renderer.Draw(Rect {{target.x - 0.10f, target.y}, {0.055f, 0.008f}}, color);
            renderer.Draw(Rect {{target.x + 0.10f, target.y}, {0.055f, 0.008f}}, color);
            renderer.Draw(Rect {{target.x, target.y - 0.10f}, {0.008f, 0.055f}}, color);
            renderer.Draw(Rect {{target.x, target.y + 0.10f}, {0.008f, 0.055f}}, color);
        }
    }
}

/**
 * @brief Stage 5専用HUDを描画する
 * @param renderer 描画先レンダラー
 * @return なし
 */
void SideScrollingShooter::DrawStage5Hud(Renderer& renderer) const {
    if (m_stage5Phase == Stage5Phase::Approach ||
        m_stage5Phase == Stage5Phase::TayamaCollapse ||
        m_stage5Phase == Stage5Phase::EndingReady) return;
    constexpr float Back[] = {0.08f, 0.05f, 0.12f, 0.90f};
    constexpr float Fill[] = {0.96f, 0.14f, 0.24f, 1.0f};
    constexpr float Accent[] = {0.16f, 0.82f, 1.0f, 1.0f};
    constexpr float BarWidth = 0.62f;

    if (m_stage5Phase == Stage5Phase::EastsourceBattle) {
        const float hpRate = Math::Clamp01(m_displayBossHp / static_cast<float>(EastsourceMaxHp));
        DrawShape(renderer, 0.0f, 0.76f, BarWidth, 0.025f, Back);
        DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.76f,
            BarWidth * hpRate, 0.018f, Fill);
        for (int phaseDivider = 1; phaseDivider < BossPhaseCount; ++phaseDivider) {
            const float x = -BarWidth + BarWidth * 2.0f *
                static_cast<float>(phaseDivider) / static_cast<float>(BossPhaseCount);
            DrawShape(renderer, x, 0.755f, 0.008f, 0.035f, Accent);
        }
        renderer.DrawText("EASTSOURCE", TextAlign::Center, 0.017f,
            {1.0f, 0.42f, 0.55f, 1.0f}, {0.0f, 0.86f});
        constexpr const char* Labels[] = {"PRECISION", "CROSSFIRE", "PURSUIT", "LAST CONTRACT"};
        const int phase = m_enemies[0].active ? m_enemies[0].bossPhase : 0;
        renderer.DrawText(Labels[(std::clamp)(phase, 0, 3)], {-BarWidth, 0.81f}, 0.012f,
            {1.0f, 0.82f, 0.30f, 1.0f});
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        renderer.DrawText("HOSTILE SIGNAL APPROACHING", TextAlign::Center, 0.018f,
            {1.0f, 0.34f, 0.32f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceFall) {
        renderer.DrawText("SIGNAL LOST", TextAlign::Center, 0.030f,
            {1.0f, 0.18f, 0.18f, 1.0f}, {0.0f, 0.12f});
        return;
    }
    if (m_stage5Phase >= Stage5Phase::WallClimbTransition &&
        m_stage5Phase <= Stage5Phase::WallClimbUpper) {
        const char* section = m_stage5Phase <= Stage5Phase::WallClimbLower ?
            "WALL CLIMB: LOWER" : (m_stage5Phase == Stage5Phase::WallClimbMiddle ?
                "WALL CLIMB: MIDDLE" : "WALL CLIMB: UPPER");
        renderer.DrawText(section, TextAlign::Center, 0.017f,
            {0.85f, 0.94f, 1.0f, 1.0f}, {0.0f, 0.84f});
        char status[40];
        int remaining = 0;
        for (const SearchlightState& light : m_searchlights) if (!light.destroyed) ++remaining;
        std::snprintf(status, sizeof(status), "SEARCHLIGHTS ACTIVE %d", remaining);
        renderer.DrawText(status, TextAlign::Center, 0.012f,
            {1.0f, 0.74f, 0.20f, 1.0f}, {0.0f, 0.78f});
        return;
    }
    if (m_stage5Phase == Stage5Phase::RooftopArrival ||
        m_stage5Phase == Stage5Phase::CarrierTransformation) {
        char status[48];
        std::snprintf(status, sizeof(status), "MOBILE FORTRESS TAYAMA  %03d%%",
            static_cast<int>(m_tayamaTransformation * 100.0f));
        renderer.DrawText(status, TextAlign::Center, 0.018f,
            {0.30f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.82f});
        return;
    }

    // TAYAMA戦は現在フェーズの有効弱点HP合計だけを表示する
    int maxHp = 0;
    for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (IsTayamaWeakpointActiveForPhase(weakpoint.type, m_stage5Phase)) {
            maxHp += weakpoint.maxHp;
        }
    }
    const float hpRate = maxHp > 0 ? Math::Clamp01(m_displayBossHp / static_cast<float>(maxHp)) : 0.0f;
    DrawShape(renderer, 0.0f, 0.74f, BarWidth, 0.025f, Back);
    DrawShape(renderer, BarWidth * (1.0f - hpRate), 0.74f,
        BarWidth * hpRate, 0.018f, Accent);
    renderer.DrawText("MOBILE FORTRESS", TextAlign::Center, 0.011f,
        {0.65f, 0.82f, 0.90f, 1.0f}, {0.0f, 0.88f});
    renderer.DrawText("TAYAMA", TextAlign::Center, 0.022f,
        {0.20f, 0.88f, 1.0f, 1.0f}, {0.0f, 0.83f});
    const char* phase = m_stage5Phase == Stage5Phase::TayamaFireControl ? "PHASE: FIRE CONTROL" :
        (m_stage5Phase == Stage5Phase::TayamaLiftEngines ?
            "PHASE: LIFT ENGINES" : "PHASE: COMMAND CORE");
    renderer.DrawText(phase, {-BarWidth, 0.79f}, 0.012f,
        {1.0f, 0.82f, 0.30f, 1.0f});
    char components[96];
    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        std::snprintf(components, sizeof(components), "L-LIGHT[%c]  R-LIGHT[%c]  RADAR[%c]",
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftSearchlight)].destroyed ? 'X' : ' ',
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightSearchlight)].destroyed ? 'X' : ' ',
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed ? 'X' : ' ');
    } else if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
        std::snprintf(components, sizeof(components), "L-ENGINE[%c]  R-ENGINE[%c]",
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed ? 'X' : ' ',
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed ? 'X' : ' ');
    } else {
        std::snprintf(components, sizeof(components), "COMMAND CORE[%c]",
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)].destroyed ? 'X' : ' ');
    }
    renderer.DrawText(components, TextAlign::Center, 0.010f,
        {0.72f, 0.86f, 0.92f, 1.0f}, {0.0f, 0.68f});
}

void SideScrollingShooter::Render(Renderer& renderer) const {
    // 安定した2D表示では全オブジェクトを同じ奥行きへ固定する
    if (!IsRailRenderActive()) {
        Render2D(renderer);
    } else {
        Render3D(renderer);
    }
    DrawBossNameReveal(renderer);
    DrawMissionBanner(renderer);
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
        DrawDebris(renderer, camera, sideDebris, 0.0f);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        Item sideItem = item;
        sideItem.z = SidePlaneZ - 0.2f;
        DrawItemModel(renderer, camera, sideItem, Math::HalfPi);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        SidePlaneZ, m_playerDestructionTimer == 0 &&
        (m_invincible == 0 || (m_invincible / 5) % 2 == 0), Math::HalfPi);

    renderer.ResetCamera();
    DrawStage5Weather(renderer);
    DrawAttackWarnings2D(renderer);

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
    renderer.DrawText(IsStage5ViewLocked() ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  3D MODE LOCKED" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });

    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
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
            const float railZ = 8.0f +
                WrapDistance(static_cast<float>(i * 43) - m_scroll * 28.0f, 110.0f);
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

    if (isTower) RenderStage5(renderer, camera);

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        if (m_stageNumber == 5 && m_stage5Phase == Stage5Phase::EastsourceIntro &&
            m_stage5PhaseTimer < 58 && enemy.type == Stage::BossEnemy) continue;
        Enemy drawEnemy = enemy;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? enemy.transitionSideX :
            (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        drawEnemy.x = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
        drawEnemy.y = Math::Lerp(sideY, exitingRail ? enemy.transitionSideY : enemy.y, railWeight);
        const float railZ = exitingRail ? enemy.transitionRailZ : enemy.z;
        drawEnemy.z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), railZ, railWeight);
        if (enemy.type != 2) {
            const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
            const float minimumRailY = FromWorldY(groundTopY + 0.32f);
            drawEnemy.y = Math::Lerp(drawEnemy.y, (std::max)(drawEnemy.y, minimumRailY), railWeight);
        }

        // レール3Dへ入るほど機体直下の影を表示する
        if (railWeight > 0.01f) {
            const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
            const bool isBoss = enemy.type == 2;
            DrawBlobShadow(renderer, camera, ToWorldX(drawEnemy.x), drawEnemy.z, groundTopY,
                isBoss ? 2.4f : 0.72f, isBoss ? 2.0f : 0.58f,
                railWeight * (isBoss ? 0.34f : 0.26f));
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
        DrawDebris(renderer, camera, debris, railWeight);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        DrawItemModel(renderer, camera, item, playerYaw);
    }
    const bool playerVisible = m_playerDestructionTimer == 0 &&
        (m_invincible == 0 || (m_invincible / 5) % 2 == 0);
    if (railWeight > 0.01f && playerVisible) {
        const float groundTopY = (isDesert || isOcean || isCity) ? -3.65f : -3.275f;
        DrawBlobShadow(renderer, camera, ToWorldX(m_playerX),
            Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight), groundTopY,
            1.05f, 0.82f, railWeight * 0.30f);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight),
        playerVisible, playerYaw);
    DrawAttackWarnings3D(renderer, camera, railWeight);

    renderer.ResetCamera();
    DrawStage5Weather(renderer);

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
    renderer.DrawText(IsStage5ViewLocked() ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  3D MODE LOCKED" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    if (m_viewTransitionTimer > 0) {
        renderer.DrawText("CAMERA SHIFT", { -0.16f, -0.02f }, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f });
    }
    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
}
