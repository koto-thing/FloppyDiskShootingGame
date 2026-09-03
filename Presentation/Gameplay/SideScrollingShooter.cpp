#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "../../Infrastructure/ExternalServices/InputService.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"
#include "SideScrollingShooterShared.h"
#include "Stages/Common/StageDispatch.h"
#include "Stages/Stage1/Stage1Module.h"
#include "Voices/VoiceDpcmDecoder.h"

namespace {
using SideScrollingShooterShared::BossNameRevealFrames;
using SideScrollingShooterShared::BossWarningSirenMml;

constexpr float PowerAfterRestart(float power) {
    return power > 1.0f ? power - 1.0f : 0.0f;
}

/**
 * @brief 音声サンプルの60fps換算再生フレーム数を取得する
 * @param sample 変換する音声サンプル
 * @return 端数を切り上げた再生フレーム数
 */
constexpr int VoicePlaybackFrames(const VoiceSamples::ImaAdpcmSample& sample) {
    return static_cast<int>((sample.sampleCount * 60 + sample.sampleRate - 1) / sample.sampleRate);
}

constexpr int VoiceGapFrames = 9;
constexpr int MissionStartSecondVoiceFrame =
    VoicePlaybackFrames(VoiceSamples::mission) + VoiceGapFrames;
constexpr int MissionClearSecondVoiceFrame =
    VoicePlaybackFrames(VoiceSamples::suspect) + VoiceGapFrames;

static_assert(PowerAfterRestart(3.25f) == 2.25f);
static_assert(PowerAfterRestart(0.75f) == 0.0f);

/** @brief 無線風に加工した自機撃破音声を取得する @return 44100Hz PCMデータ */
const std::vector<std::int16_t>& MomijiDeathVoice() {
    static const auto voice = VoiceCodec::DecodeRadioForAudioService(VoiceSamples::momijiDeath);
    return voice;
}

/** @brief ミッション開始前半の音声を取得する @return 44100Hz PCMデータ */
const std::vector<std::int16_t>& MissionVoice() {
    static const auto voice = VoiceCodec::DecodeForAudioService(VoiceSamples::mission);
    return voice;
}

/** @brief ミッション開始後半の音声を取得する @return 44100Hz PCMデータ */
const std::vector<std::int16_t>& StartVoice() {
    static const auto voice = VoiceCodec::DecodeForAudioService(VoiceSamples::start);
    return voice;
}

/** @brief ミッション完了前半の音声を取得する @return 44100Hz PCMデータ */
const std::vector<std::int16_t>& SuspectVoice() {
    static const auto voice = VoiceCodec::DecodeForAudioService(VoiceSamples::suspect);
    return voice;
}

/** @brief ミッション完了後半の音声を取得する @return 44100Hz PCMデータ */
const std::vector<std::int16_t>& ArrestedVoice() {
    static const auto voice = VoiceCodec::DecodeForAudioService(VoiceSamples::arrested);
    return voice;
}
}

#include "SideScrollingShooterEnemies.h"
#include "Stages/Common/StageDefinition.h"

/**
 * @brief 指定番号のステージ定義を取得する
 * @param stageNumber 取得するステージ番号
 * @return 指定番号に対応するステージ定義
 */
const SideScrollingShooter::Stage& SideScrollingShooter::StageForNumber(int stageNumber, DifficultyType difficulty) {
    return StageDispatch::Definition(stageNumber, difficulty);
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

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::DiveRusherEnemyBehaviorInstance() {
    static const DiveRusherEnemyBehavior behavior;
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

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::MissileShooterEnemyBehaviorInstance() {
    static const MissileShooterEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::LinkedLaserEnemyBehaviorInstance() {
    static const LinkedLaserEnemyBehavior behavior;
    return behavior;
}

const SideScrollingShooter::EnemyBehavior& SideScrollingShooter::WallSecurityDroneEnemyBehaviorInstance() {
    static const WallSecurityDroneEnemyBehavior behavior;
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
    case 7:
        return DiveRusherEnemyBehaviorInstance();
    case 8:
        return MissileShooterEnemyBehaviorInstance();
    case 9:
        return LinkedLaserEnemyBehaviorInstance();
    case 10:
        return WallSecurityDroneEnemyBehaviorInstance();
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
    // プレイ開始前に音声をデコードして初回再生時の処理待ちをなくす
    (void)MomijiDeathVoice();
    (void)MissionVoice();
    (void)StartVoice();
    (void)SuspectVoice();
    (void)ArrestedVoice();
    m_audio = audio;
    m_playerType = playerType;
    m_difficulty = difficulty;
    m_galleryUnlocks = SettingsRepository {}.Load().galleryUnlocks;
    Reset(true);
}

/** @brief チュートリアル用のゲーム状態を初期化する @param audio 効果音サービス @param playerType 使用機体 @param difficulty 難易度 @return なし */
void SideScrollingShooter::InitializeTutorial(
    AudioService* audio, PlayerType playerType, DifficultyType difficulty) {
    Initialize(audio, playerType, difficulty);
    m_tutorialMode = true;
    m_missionStartTimer = MissionBannerDisplayFrames;
    m_invincible = 999999;
    m_tutorialStep = 0;
    BeginTutorialStep();
}

void SideScrollingShooter::Reset(bool resetRetryCounts) {
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    m_bomb = {};
    StageDispatch::ResetGimmicks(*this);
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
    m_screenShakeIntensity = 0.0f;
    m_screenShakeFrames = 0;
    m_screenShakeDurationFrames = 0;
    m_clearTimer = 0;
    m_clear = false;
    m_bossBattle = false;
    m_bossBattlePending = false;
    m_chapterResultActive = false;
    m_playerDestructionTimer = 0;
    m_powerUpTimer = 0;
    m_viewToggleRequested = false;
    m_bombRequested = false;
    m_viewMode = ViewMode::Side2D;
    m_nextViewMode = ViewMode::Side2D;
    m_viewTransitionTimer = 0;
    m_viewToggleCooldown = 0;
    m_viewTransitionProgress = 0.0f;
    StageDispatch::ResetScriptState(*this);
}

/**
 * @brief デバッグ用に指定ステージとチャプターから開始する
 * @param stageNumber 開始するステージ番号
 * @param chapterNumber 開始するチャプター番号
 * @param bossBattle ボス戦から開始する場合true
 * @param playBossWarningSound ボス登場警報を再生する場合true
 * @return なし
 */
void SideScrollingShooter::StartDebugCheckpoint(
    int stageNumber,
    int chapterNumber,
    bool bossBattle,
    bool playBossWarningSound) {
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    m_bomb = {};
    StageDispatch::ResetGimmicks(*this);

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
    m_missionStartTimer = bossBattle ? 0 : MissionBannerDisplayFrames;
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
    m_bombRequested = false;
    m_viewMode = ViewMode::Side2D;
    m_nextViewMode = ViewMode::Side2D;
    m_viewTransitionTimer = 0;
    m_viewToggleCooldown = 0;
    m_viewTransitionProgress = 0.0f;
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    m_slowMove = false;
    StageDispatch::ResetScriptState(*this);

    if (bossBattle) {
        m_chapterNumber = 3;
        if (!StageDispatch::StartDebugBoss(*this)) {
            StartBossBattle(playBossWarningSound);
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
    m_slowMove = InputService::IsKeyPressed(VK_SHIFT);
    m_fire = Input::GetKey(KeyCode::Z) || Input::GetKey(KeyCode::Space);
    m_bombRequested = Input::GetKeyDown(KeyCode::C);
    m_viewToggleRequested = Input::GetKeyDown(KeyCode::X) && CanToggleView();

    // デバッグ用に任意の進行地点へ移動する
    if (Input::GetKeyDown(KeyCode::F1)) StartDebugCheckpoint(1, 1, false);
    if (Input::GetKeyDown(KeyCode::F2)) StartDebugCheckpoint(2, 1, false);
    if (Input::GetKeyDown(KeyCode::F3)) StartDebugCheckpoint(3, 1, false);
    if (Input::GetKeyDown(KeyCode::F4)) StartDebugCheckpoint(4, 1, false);
    if (Input::GetKeyDown(KeyCode::F5)) StartDebugCheckpoint(5, 1, false);
    if (Input::GetKeyDown(KeyCode::Alpha1)) StartDebugCheckpoint(m_stageNumber, 1, false);
    if (Input::GetKeyDown(KeyCode::Alpha2)) StartDebugCheckpoint(m_stageNumber, 2, false);
    if (Input::GetKeyDown(KeyCode::Alpha3)) StartDebugCheckpoint(m_stageNumber, 3, false);
    if (Input::GetKeyDown(KeyCode::B) && !StageDispatch::HandleDebugBossInput(*this)) {
        StartDebugCheckpoint(m_stageNumber, 3, true);
    }
    StageDispatch::ProcessDebugInput(*this);

    if (m_clear && Input::GetKeyDown(KeyCode::R)) {
        Reset(false);
    }
}

void SideScrollingShooter::Tick() {
    // 進行停止中も画面演出を終了へ進める
    m_screenShakeFrames = (std::max)(0, m_screenShakeFrames - 1);
    m_powerUpTimer = (std::max)(0, m_powerUpTimer - 1);

    if (m_tutorialMode) {
        TickTutorial();
        return;
    }

    // ミッション開始表示中は背景を維持したまま戦闘進行と操作を止める
    if (m_missionStartTimer > 0) {
        static_assert(MissionStartSecondVoiceFrame + VoicePlaybackFrames(VoiceSamples::start) <=
            MissionBannerDisplayFrames);
        if (!m_tutorialMode && m_audio) {
            const int elapsedFrames = MissionBannerDisplayFrames - m_missionStartTimer;
            if (elapsedFrames == 0) m_audio->PlaySE(MissionVoice());
            else if (elapsedFrames == MissionStartSecondVoiceFrame) m_audio->PlaySE(StartVoice());
        }
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
        static_assert(MissionClearSecondVoiceFrame + VoicePlaybackFrames(VoiceSamples::arrested) <=
            ClearWaitFrames);
        if (!m_tutorialMode && m_audio && m_clearTimer > 0 && m_clearTimer <= ClearWaitFrames) {
            const int elapsedFrames = ClearWaitFrames - m_clearTimer;
            if (elapsedFrames == 0) m_audio->PlaySE(SuspectVoice());
            else if (elapsedFrames == MissionClearSecondVoiceFrame) m_audio->PlaySE(ArrestedVoice());
        }
        StageDispatch::TickBossDefeat(*this);
        TickExplosions();
        TickDebris();
        --m_clearTimer;
        if (m_clearTimer <= 0 && StageDispatch::HasNextStage(*this)) StartNextStage();
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

    StageDispatch::TickBeforeFrame(*this);

    ++m_frame;
    if (StageDispatch::ShouldAdvanceStageScroll(*this)) m_scroll += 0.008f;
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_specialShotCooldown = (std::max)(0, m_specialShotCooldown - 1);
    m_invincible = (std::max)(0, m_invincible - 1);
    m_viewToggleCooldown = (std::max)(0, m_viewToggleCooldown - 1);
    StageDispatch::TickAfterFrame(*this);
    if (!m_bossBattle && !m_chapterResultActive &&
        StageDispatch::UsesChapterTimeline(*this) &&
        m_frame >= m_stage->ChapterEndFrame(m_chapterNumber)) {
        FinishChapter();
    }

    const bool cinematic = StageDispatch::IsCinematic(*this);
    if (!cinematic) TickPlayer();
    StageDispatch::TickWorld(*this);

    // 3Dから2Dへ確定するフレームだけは、座標変換直後の特殊障害物との誤接触を除外する
    if (!completingRailToSideTransition &&
        StageDispatch::HitsHazard(*this, m_playerX, m_playerY, PlayerRailZ, 0.055f)) {
        DamagePlayer();
        return;
    }

    if (!cinematic) TickPlayerWeapons();

    if (!m_bossBattle && !m_chapterResultActive &&
        StageDispatch::UsesChapterTimeline(*this)) {
        for (int spawnIndex = 0;; ++spawnIndex) {
            Stage::EnemySpawnRule spawn;
            if (!m_stage->TrySelectEnemySpawn(m_frame, spawnIndex, spawn, m_chapterNumber)) {
                break;
            }
            SpawnEnemy(spawn.enemyType, spawn.sideX, spawn.railX, spawn.y, spawn.railZ);
        }
    }

    if (!cinematic) TickBomb();
    TickEnemies();
    TickShots();
    TickExplosions();
    TickDebris();
    TickItems();
}

/**
 * @brief ゲーム画面の揺れを開始する
 * @param intensity 揺れの最大振幅
 * @param durationFrames 揺れを継続するフレーム数
 * @return なし
 */
void SideScrollingShooter::ShakeScreen(float intensity, int durationFrames) {
    // 無効な指定は現在の画面揺れを停止する
    if (intensity <= 0.0f || durationFrames <= 0) {
        m_screenShakeIntensity = 0.0f;
        m_screenShakeFrames = 0;
        m_screenShakeDurationFrames = 0;
        return;
    }

    // 呼び出すたびに指定した強さと長さで揺れを開始し直す
    m_screenShakeIntensity = intensity;
    m_screenShakeFrames = durationFrames;
    m_screenShakeDurationFrames = durationFrames;
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
        if (StageDispatch::HandleChapterResult(*this)) return;
        m_bossBattlePending = true;
        return;
    }

    ++m_chapterNumber;
    m_chapterResult = {};
    m_chapterStartPower = m_power;
    m_chapterStartScore = m_score;
    m_chapterStartKills = m_kills;
    StageDispatch::OnChapterStarted(*this);
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
        if (enemy.type == Stage::WallSecurityDroneEnemy) {
            // 外壁警備ドローンは視点切り替え直後から壁面のZ座標を維持する
            enemy.z = WallSecurityDroneEnemyBehavior::WallSurfaceZ();
            enemy.baseX = enemy.railAnchorX;
            enemy.x = enemy.baseX;
            continue;
        }
        if (enemy.type == Stage::SquareShooterEnemy) {
            enemy.z = ToRailZFromSideX(enemy.transitionSideX);
            enemy.baseX = enemy.railAnchorX;
            enemy.x = enemy.railAnchorX;
            continue;
        }
        if (enemy.type == Stage::LinkedLaserEnemy) {
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
        if (!shot.enemy && UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase)) {
            // 第2部の自機弾は3Dへ切り替えても壁面上方向の速度を維持する
            shot.z = PlayerRailZ + 2.0f;
            shot.vz = 0.0f;
            continue;
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
        if (!shot.enemy && UsesVerticalPlayerShots(m_stageNumber, m_stage5.phase)) {
            // 第2部の自機弾は3Dの奥行きを2D横位置へ変換せず上方向を維持する
            shot.z = ToRailZFromSideX(shot.x);
            shot.vz = 0.0f;
            continue;
        }
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

void SideScrollingShooter::StartBossBattle(bool playWarningSound) {
    m_bossBattle = true;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_bossIntroductionPhase = BossIntroductionPhase::Entrance;
    m_bossIntroductionTimer = 0;

    // 通常敵と全弾を消去してボス登場中に自機弾が画面へ残るのを防ぐ
    for (auto& enemy : m_enemies) {
        enemy.active = false;
    }
    m_shots = {};

    Enemy& boss = m_enemies[0];
    m_stage->ConfigureBoss(boss, IsRailGameplayActive());
    m_stage->ConfigureBossPartHp(boss);
    boss.bossPartMaxHp = boss.bossPartHp;
    m_bossHp = boss.hp;
    m_displayBossHp = static_cast<float>(m_bossHp);
    boss.bossPhase = BossNormalPhase1;
    m_invincible = (std::max)(m_invincible, 60);

    if (m_audio && playWarningSound) {
        m_audio->PlayMMLSE(BossWarningSirenMml);
    }
}

/**
 * @brief 全ステージをクリア済みか取得する
 * @return 最終ステージの完了条件を満たした場合true、進行中の場合false
 */
bool SideScrollingShooter::IsAllStagesCleared() const {
    return StageDispatch::IsGameCleared(*this);
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
    const BossStory story = StageDispatch::Story(m_stageNumber);
    if (m_bossStoryLine >= story.lineCount) {
        m_bossStoryActive = false;
        return;
    }

    // Xキーで会話全体を飛ばし、Zキーで次の台詞へ進める
    if (Input::GetKeyDown(KeyCode::X)) m_bossStoryLine = story.lineCount;
    else if (Input::GetKeyDown(KeyCode::Z)) ++m_bossStoryLine;
    else return;

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
    StageDispatch::TickBossIntroduction(*this);

    ++m_bossIntroductionTimer;
    const int entranceFrames = StageDispatch::BossIntroductionFrames(*this);
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

    // スコアと残機を維持したまま、次のステージ用に戦闘オブジェクトを初期化する
    m_shots = {};
    m_enemies = {};
    m_explosions = {};
    m_debris = {};
    m_bomb = {};
    StageDispatch::ResetGimmicks(*this);
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
    StageDispatch::ResetScriptState(*this);
}

/**
 * @brief 被弾効果を再生して現在のチャプターをやり直す
 * @return なし
 */
void SideScrollingShooter::DamagePlayer() {
    if (StageDispatch::IsPlayerDamageIgnored(*this)) return;
    if (m_playerDestructionTimer > 0) return;

    // 敵撃破と同じ破壊爆発を自機位置へ生成してから復帰を待つ
    SpawnExplosion(m_playerX, m_playerY, PlayerRailZ, true);
    PlayHitSound();
    if (m_audio) m_audio->PlaySE(MomijiDeathVoice());
    m_playerDestructionTimer = PlayerDestructionWaitFrames;
}

/** @brief 現在のチュートリアル課題を準備する */
void SideScrollingShooter::BeginTutorialStep() {
    m_shots = {};
    m_enemies = {};
    m_bomb = {};
    for (auto& meteor : m_stage1.meteors) meteor.destroyed = true;
    m_tutorialStepFrame = 0;
    m_tutorialSlowUsed = false;
    m_invincible = m_tutorialStep == 0 ? 0 : 999999;

    // 移動課題には正面を横切る隕石、低速移動課題には狭い上下の隕石を置く
    if (m_tutorialStep == 0) {
        m_playerX = -0.72f;
        m_playerY = 0.0f;
        m_stage1.meteors[0] = {-85.0f, 1.65f, 0.0f, 0.018f, 0.0f, 99, false, 0.85f, true};
        m_stage1.meteors[1] = {-45.0f, 1.65f, 0.7f, -0.018f, 0.0f, 99, false, 0.00f, true};
        m_stage1.meteors[2] = {-85.0f, 1.65f, 1.4f, 0.018f, 0.0f, 99, false, -0.85f, true};
    } else if (m_tutorialStep == 1) {
        m_playerX = -0.72f;
        m_playerY = 0.0f;
        constexpr float SlowMeteorSpeed = 0.85f;
        // 上下3組の隙間を交互にずらし、低速移動で抜ける蛇行通路を作る
        m_stage1.meteors[0] = {-20.0f, 1.65f, 0.0f, 0.018f, 0.0f, 99, false, 0.90f, true, SlowMeteorSpeed};
        m_stage1.meteors[1] = {-20.0f, 1.65f, 0.6f, -0.018f, 0.0f, 99, false, -0.75f, true, SlowMeteorSpeed};
        m_stage1.meteors[2] = {-45.0f, 1.65f, 1.2f, -0.018f, 0.0f, 99, false, 0.75f, true, SlowMeteorSpeed};
        m_stage1.meteors[3] = {-45.0f, 1.65f, 1.8f, 0.018f, 0.0f, 99, false, -0.90f, true, SlowMeteorSpeed};
        m_stage1.meteors[4] = {-70.0f, 1.65f, 2.4f, 0.018f, 0.0f, 99, false, 0.90f, true, SlowMeteorSpeed};
        m_stage1.meteors[5] = {-70.0f, 1.65f, 3.0f, -0.018f, 0.0f, 99, false, -0.75f, true, SlowMeteorSpeed};
    } else if (m_tutorialStep == 2) {
        // ショット課題開始時に攻撃しない標的機を上下中央へ3体同時配置する
        SpawnEnemy(0, 0.62f, 0.0f, 0.58f, 42.0f);
        SpawnEnemy(0, 0.78f, 0.0f, 0.00f, 42.0f);
        SpawnEnemy(0, 0.62f, 0.0f, -0.58f, 42.0f);
        constexpr float TargetX[] = {0.62f, 0.78f, 0.62f};
        int targetIndex = 0;
        for (auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            enemy.hp = enemy.maxHp = 5;
            enemy.actionX = TargetX[targetIndex++];
            enemy.baseX = enemy.actionX;
        }
    }
}

/** @brief 現在のチュートリアル課題を飛ばして次へ進む @return なし */
void SideScrollingShooter::NextTutorialStep() {
    if (!m_tutorialMode || m_missionStartTimer > 0 || m_tutorialStep >= TutorialStepCount) return;
    ++m_tutorialStep;
    if (m_tutorialStep < TutorialStepCount) {
        BeginTutorialStep();
    } else {
        m_clear = true;
        m_clearTimer = ClearWaitFrames;
    }
}

/** @brief チュートリアル専用進行を更新する */
void SideScrollingShooter::TickTutorial() {
    if (IsTutorialComplete()) return;

    // 通常ステージと同じ開始・終了演出中は課題の進行と操作を止める
    if (m_missionStartTimer > 0) {
        --m_missionStartTimer;
        return;
    }
    if (m_tutorialStep >= TutorialStepCount) {
        --m_clearTimer;
        return;
    }

    // 隕石へ接触した場合は撃破演出後に同じ課題を最初からやり直す
    if (m_playerDestructionTimer > 0) {
        TickExplosions();
        TickDebris();
        if (--m_playerDestructionTimer == 0) BeginTutorialStep();
        return;
    }
    ++m_tutorialStepFrame;
    m_scroll += 0.008f;
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_specialShotCooldown = (std::max)(0, m_specialShotCooldown - 1);
    m_viewToggleCooldown = (std::max)(0, m_viewToggleCooldown - 1);
    TickViewTransition();
    TickPlayer();
    if (m_tutorialStep == 1 && m_slowMove &&
        (m_moveLeft || m_moveRight || m_moveUp || m_moveDown)) m_tutorialSlowUsed = true;
    TickPlayerWeapons();

    // ショット課題の標的機は右画面外から指定位置まで進入して停止する
    if (m_tutorialStep == 2) {
        for (auto& enemy : m_enemies) {
            if (!enemy.active || enemy.x <= enemy.actionX) continue;
            enemy.x = (std::max)(enemy.actionX, enemy.x - 0.025f);
            enemy.z = ToRailZFromSideX(enemy.x);
        }
    }

    // ボム課題と視点切り替え課題では、回避不能な縦一列の弾幕を一度だけ生成する
    if ((m_tutorialStep == 3 || m_tutorialStep == 4) && m_tutorialStepFrame == 45) {
        constexpr int BarrageShotCount = 29;
        constexpr float BarrageMinY = Side2DPlayerMinY - 0.10f;
        constexpr float BarrageMaxY = Side2DPlayerMaxY + 0.10f;
        constexpr float BarrageSpeed2D = -0.009f;
        constexpr float BarrageSpeed3D = 0.31f;
        for (int i = 0; i < BarrageShotCount; ++i) {
            const float y = Math::Lerp(BarrageMinY, BarrageMaxY,
                static_cast<float>(i) / static_cast<float>(BarrageShotCount - 1));
            SpawnShot(1.05f, y, BarrageSpeed2D, 0.0f, true,
                EnemyRailFarZ, BarrageSpeed3D);
        }
        PlayEnemyShotSound();
    }

    if (m_tutorialStep <= 1) {
        Stage1Module::TickWorld(*this);
        if (Stage1Module::HitsHazard(*this, m_playerX, m_playerY, PlayerRailZ, 0.055f)) {
            DamagePlayer();
            return;
        }
    }
    TickBomb();
    TickShots();
    TickExplosions();
    TickDebris();

    bool completed = false;
    if (m_tutorialStep == 0) {
        completed = true;
        for (int i = 0; i < 3; ++i) if (!m_stage1.meteors[i].destroyed) completed = false;
    }
    if (m_tutorialStep == 1) {
        completed = m_tutorialSlowUsed;
        for (const auto& meteor : m_stage1.meteors) if (!meteor.destroyed) completed = false;
    }
    if (m_tutorialStep == 2) {
        completed = true;
        for (const auto& enemy : m_enemies) if (enemy.active) completed = false;
    }
    if (m_tutorialStep == 3 && m_tutorialStepFrame > 70) {
        completed = true;
        for (const auto& shot : m_shots) if (shot.active && shot.enemy) completed = false;
    }
    if (m_tutorialStep == 4 && m_tutorialStepFrame > 45) {
        completed = m_viewMode == ViewMode::Rail3D && m_viewTransitionTimer == 0;
        for (const auto& shot : m_shots) if (shot.active && shot.enemy) completed = false;
    }
    if (!completed) return;
    NextTutorialStep();
}

/**
 * @brief 現在のチャプターを開始時状態へ戻す
 * @return なし
 */
void SideScrollingShooter::RestartCurrentChapter() {
    // ボス戦中は警報を再生せずBキーと同じ戦闘開始状態へ戻す
    const bool restartBossBattle = m_bossBattle;
    m_power = PowerAfterRestart(m_power);
    if (restartBossBattle) {
        if (!StageDispatch::HandleDebugBossInput(*this)) {
            StartDebugCheckpoint(m_stageNumber, 3, true, false);
        }
        ++m_chapterRetryCounts[m_chapterNumber - 1];
        m_restartTimer = RestartDisplayFrames;
        return;
    }

    // 通常戦では被弾時点のPowerから1.0だけ失い、0.0未満にはしない
    if (StageDispatch::TryRestartCheckpoint(*this)) return;
    ++m_chapterRetryCounts[m_chapterNumber - 1];
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    m_bomb = {};
    StageDispatch::ResetGimmicks(*this);
    m_chapterResult = {};
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

float SideScrollingShooter::DistancePointToSegment2D(
    const Vector2& point, const Vector2& start, const Vector2& end) {
    const Vector2 segment = end - start;
    const float lengthSquared = segment.LengthSquared();
    if (lengthSquared <= Math::Epsilon) return Vector2::Distance(point, start);
    const float t = Math::Clamp01(Vector2::Dot(point - start, segment) / lengthSquared);
    return Vector2::Distance(point, start + segment * t);
}

float SideScrollingShooter::DistancePointToSegment3D(
    const Vector3& point, const Vector3& start, const Vector3& end) {
    const Vector3 segment = end - start;
    const float lengthSquared = segment.LengthSquared();
    if (lengthSquared <= Math::Epsilon) return Vector3::Distance(point, start);
    const float t = Math::Clamp01(Vector3::Dot(point - start, segment) / lengthSquared);
    return Vector3::Distance(point, start + segment * t);
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
    const float groundTopY = StageDispatch::RailGroundY(*this);
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

/**
 * @brief 現在2Dと3Dの表示を切り替えられるか判定する
 * @return 切り替え可能な場合true
 */
bool SideScrollingShooter::CanToggleView() const {
    return m_viewTransitionTimer == 0 && m_viewToggleCooldown == 0 &&
        m_missionStartTimer == 0 && !StageDispatch::IsViewLocked(*this) &&
        m_bossIntroductionPhase == BossIntroductionPhase::None;
}
