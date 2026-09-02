#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::BossNameRevealFrames;
using SideScrollingShooterShared::Stage2BossApproachFrames;
using SideScrollingShooterShared::Stage2BossAssemblyFrames;
using SideScrollingShooterShared::Stage5DistantThunder;
using SideScrollingShooterShared::Stage5Thunder;

constexpr int Stage1BossRushSegmentFrames = 36;
constexpr int Stage1BossRushSegmentCount = 4;
constexpr int Stage1BossRushFrames = Stage1BossRushSegmentFrames * Stage1BossRushSegmentCount;
constexpr int Stage1BossSettleFrames = 96;
constexpr int Stage1BossEntranceFrames = Stage1BossRushFrames + Stage1BossSettleFrames;

constexpr int Stage2BossEntranceFrames = Stage2BossApproachFrames + Stage2BossAssemblyFrames;

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

}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"
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
