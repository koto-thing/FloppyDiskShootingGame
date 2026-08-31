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
constexpr float SideCameraZ = -16.0f;
constexpr float SideCameraFieldOfView = 38.0f;

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
    m_stageNumber = 1;
    m_chapterNumber = 1;
    if (resetRetryCounts) m_chapterRetryCounts = {};
    m_chapterResult = {};
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
    m_lives = 3;
    m_score = 0;
    m_kills = 0;
    m_bossHp = 0;
    m_bossStoryLine = 0;
    m_bossStoryActive = false;
    m_clearTimer = 0;
    m_gameOver = false;
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

    if ((m_gameOver || m_clear) && Input::GetKeyDown(KeyCode::R)) {
        if (m_gameOver && m_chapterNumber <= static_cast<int>(m_chapterRetryCounts.size())) {
            ++m_chapterRetryCounts[m_chapterNumber - 1];
        }
        Reset(false);
    }
}

void SideScrollingShooter::Tick() {
    TickViewTransition();

    if (m_gameOver) {
        return;
    }
    if (m_clear) {
        if (m_stageNumber < 5 && --m_clearTimer <= 0) StartNextStage();
        return;
    }
    if (m_chapterResultActive) {
        TickChapterResult();
        if (m_chapterResultActive) {
            // 戦闘進行は止めたまま、結果画面の背後と画面上の弾・アイテムだけを動かす
            m_scroll += 0.008f;
            TickPlayer();
            TickShots();
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
    if (!m_bossBattle && !m_chapterResultActive && m_frame >= m_chapterNumber * ChapterLengthFrames) {
        FinishChapter();
    }

    TickPlayer();

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

    // 規定スクロール距離へ到達したら通常区間を終了してボス戦を開始する
    if (!m_bossBattle && m_scroll >= m_stage->BossStartDistance()) {
        StartBossBattle();
    }

    Stage::EnemySpawnRule spawn;
    if (!m_bossBattle && !m_chapterResultActive &&
        m_stage->TrySelectEnemySpawn(m_frame, spawn, m_chapterNumber)) {
        SpawnEnemy(spawn.enemyType, spawn.sideX, spawn.railX, spawn.y, spawn.railZ);
    }

    TickEnemies();
    TickShots();
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

        // ボスは一定間隔で3方向へ弾を発射する
        if (enemy.type == 2 && enemy.age % 120 == 0) {
            const bool railMode = IsRailGameplayActive();
            const int bulletCount = m_stage->BossBulletCount(railMode);
            for (int i = 0; i < bulletCount; ++i) {
                const Stage::BossBullet bullet = m_stage->GetBossBullet(i, railMode);
                SpawnShot(enemy.x + bullet.offsetX, enemy.y + bullet.offsetY,
                    bullet.vx, bullet.vy, true, enemy.z, enemy.behavior->RailAimedShotSpeed());
            }
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
            if (enemy.type != 2) enemy.active = false;
            DamagePlayer();
        }
    }
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
            }
            continue;
        }

        for (auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            if (enemy.behavior == nullptr) {
                enemy.behavior = &EnemyBehaviorForType(enemy.type);
            }
            BossPart hitPart = BossNose;
            if (enemy.type == 2 && TryHitBossPart(shot, enemy, hitPart)) {
                if (!shot.piercing) shot.active = false;
                enemy.bossPartHp[hitPart] -= shot.damage;
                if (enemy.bossPartHp[hitPart] <= 0) {
                    enemy.bossPartHp[hitPart] = 0;
                    // 部位破壊の報酬として、本体へ通常弾10発分の追加ダメージを与える
                    enemy.hp -= 10;
                    PlayHitSound();
                }
                if (enemy.hp <= 0) {
                    enemy.active = false;
                    SpawnPowerItem(enemy.x, enemy.y, enemy.z, 1.00f);
                    m_bossHp = 0;
                    m_score += 5000;
                    m_clear = true;
                    m_clearTimer = 120;
                    PlayHitSound();
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
            if (!shot.piercing) shot.active = false;
            enemy.hp -= shot.damage;
            if (enemy.hp <= 0) {
                enemy.active = false;
                if (enemy.type == 2) {
                    SpawnPowerItem(enemy.x, enemy.y, enemy.z, 1.00f);
                    m_bossHp = 0;
                    m_score += 5000;
                    m_clear = true;
                    m_clearTimer = 120;
                } else {
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

        // 自機が近づいたアイテムだけを弱く追尾させる
        const float dx = m_playerX - item.x;
        const float dy = m_playerY - item.y;
        const bool followsPlayer = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 3.5f,
                ToWorldX(item.x), ToWorldY(item.y), item.z, 0.0f) :
            Hit(m_playerX, m_playerY, 0.45f, item.x, item.y, 0.0f);
        if (followsPlayer) {
            item.x += dx * 0.025f;
            item.y += dy * 0.025f;
            if (IsRailGameplayActive()) {
                item.z += (PlayerRailZ - item.z) * 0.025f;
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
    boss.bossPartHp = { 8, 12, 12, 10, 10 };
    m_invincible = (std::max)(m_invincible, 60);

    if (m_audio) {
        m_audio->PlayMMLSE("t180 o4 l16 v12 c g > c");
    }
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
    m_stage = &StageForNumber(m_stageNumber, m_difficulty);
    m_chapterNumber = 1;
    m_chapterRetryCounts = {};
    m_chapterResult = {};
    m_chapterResultTimer = 0;
    m_scroll = 0.0f;
    m_frame = 0;
    m_spawnCooldown = 35;
    m_shotCooldown = 0;
    m_specialShotCooldown = 0;
    m_bossHp = 0;
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

void SideScrollingShooter::DamagePlayer() {
    --m_lives;
    m_invincible = 120;
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    PlayHitSound();
    if (m_lives <= 0) m_gameOver = true;
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

void SideScrollingShooter::DrawShotModel(Renderer& renderer, const Camera3D& camera, const Shot& shot, float yaw) {
    if (shot.enemy) {
        DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
            0.16f, 0.16f, 0.65f, EnemyShotColor, yaw);
        return;
    }

    // 特殊弾は選択した機体種別ごとの色と形状で描画する
    if (shot.special) {
        switch (shot.playerType) {
        case Homing:
            DrawModelPrimitive(renderer, camera, 3, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                0.20f, 0.20f, 0.85f, HomingShotColor, yaw);
            return;
        case Piercing:
            DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                0.13f, 0.13f, 1.50f, PiercingShotColor, yaw);
            return;
        case Spread:
            DrawModelPrimitive(renderer, camera, 5, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                0.24f, 0.24f, 0.24f, SpreadShotColor, yaw);
            return;
        }
    }
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.16f, 0.16f, 1.15f, PlayerShotColor, yaw);
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
    const float progress = (std::min)(1.0f,
        static_cast<float>(m_chapterResultTimer) / static_cast<float>(ChapterResultCountUpFrames));
    const int annihilationRate = m_chapterResult.enemySpawnCount == 0 ? 0 :
        m_chapterResult.enemyDefeatCount * 100 / m_chapterResult.enemySpawnCount;
    const int graze = static_cast<int>(m_chapterResult.grazeCount * progress);
    const int defeat = static_cast<int>(m_chapterResult.enemyDefeatCount * progress);
    const int retry = static_cast<int>(m_chapterResult.retryCount * progress);
    const int score = static_cast<int>(m_chapterResult.score * progress);
    const int total = static_cast<int>(m_chapterResult.totalScore * progress);
    const int displayedRate = static_cast<int>(annihilationRate * progress);
    char line[64];

    renderer.DrawText("CHAPTER RESULT", TextAlign::Center, 0.028f, { 1.0f, 0.88f, 0.25f, 1.0f }, { 0.0f, 0.40f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "GRAZE        %d", graze);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, 1.0f }, { 0.0f, 0.20f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "ANNIHILATION  %d / %d  %d%%", defeat, m_chapterResult.enemySpawnCount, displayedRate);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, 1.0f }, { 0.0f, 0.08f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "RETRY        %d", retry);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, 1.0f }, { 0.0f, -0.04f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "CHAPTER SCORE  %06d", score);
    renderer.DrawText(line, TextAlign::Center, 0.016f, { 0.85f, 0.95f, 1.0f, 1.0f }, { 0.0f, -0.16f }, CharacterSpacing);
    std::snprintf(line, sizeof(line), "TOTAL SCORE    %06d", total);
    renderer.DrawText(line, TextAlign::Center, 0.020f, { 1.0f, 0.88f, 0.25f, 1.0f }, { 0.0f, -0.31f }, CharacterSpacing);
}

void SideScrollingShooter::DrawBossHud(Renderer& renderer) const {
    if (!m_bossBattle || m_clear) {
        return;
    }

    // 2D/3D共通のボスHP表示をカメラリセット後のUI座標へ描画する
    constexpr float BossBarBack[4] = { 0.20f, 0.08f, 0.22f, 1.0f };
    constexpr float BossBarFill[4] = { 0.95f, 0.15f, 0.45f, 1.0f };
    const float hpRate = static_cast<float>(m_bossHp) / m_stage->BossMaxHp();
    DrawShape(renderer, 0.0f, 0.76f, 0.62f, 0.025f, BossBarBack);
    DrawShape(renderer, -0.62f * (1.0f - hpRate), 0.76f,
        0.62f * hpRate, 0.018f, BossBarFill);
    renderer.DrawText("BOSS", { 0.02f, 0.86f }, 0.014f,
        { 1.0f, 0.45f, 0.65f, 1.0f });
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
    if (IsRailRenderActive()) {
        Render3D(renderer);
        return;
    }
    Render2D(renderer);
}

void SideScrollingShooter::Render2D(Renderer& renderer) const {
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
    renderer.Draw({
        PrimitiveShape::Sprite2D,
        camera.ProjectionMatrix() * camera.ViewMatrix() * backgroundWorld,
        Vector3::One,
        {SideBackgroundColor[0], SideBackgroundColor[1], SideBackgroundColor[2], SideBackgroundColor[3]}
    });

    for (int i = 0; i < 18; ++i) {
        float x = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
        float y = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        DrawModelPrimitive(renderer, camera, 1, ToWorldX(x), ToWorldY(y), SidePlaneZ + 12.0f,
            0.06f, 0.06f, 0.06f, StarColor);
    }
    for (int i = 0; i < 7; ++i) {
        float x = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
        DrawModelPrimitive(renderer, camera, 1, ToWorldX(x), ToWorldY(-0.80f), SidePlaneZ + 7.0f,
            0.04f, 6.8f, 0.08f, GridColor);
    }
    for (int i = 0; i < 5; ++i) {
        DrawModelPrimitive(renderer, camera, 1, 0.0f, ToWorldY(-0.80f + i * 0.40f), SidePlaneZ + 7.0f,
            14.0f, 0.04f, 0.08f, GridColor);
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
    std::snprintf(progressStatus, sizeof(progressStatus), "LIVES %d  DIST %03d%%", m_lives, progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText("MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });

    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawBossStory(renderer);
    if (m_gameOver) {
        renderer.DrawText("GAME OVER", { -0.20f, 0.12f }, 0.045f, { 1.0f, 0.2f, 0.2f, 1.0f });
        renderer.DrawText("PRESS R TO RETRY", { -0.22f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    } else if (m_clear) {
        renderer.DrawText(m_stageNumber == 5 ? "ALL STAGES CLEAR" : "STAGE CLEAR", { -0.32f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
    }
}

void SideScrollingShooter::Render3D(Renderer& renderer) const {
    Camera3D camera;
    ConfigureRailCamera(camera, renderer);
    const float railWeight = RailBlend();
    const float playerYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    const float enemyYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    // 遷移開始直後は2D背景の配置を保ち、画面全体が反転して見える初期ジャンプを避ける
    for (int i = 0; i < 18; ++i) {
        const float sideX = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
        const float sideY = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        const float railX = -13.0f + static_cast<float>((i * 37) % 260) / 10.0f;
        const float railY = -6.0f + static_cast<float>((i * 53) % 120) / 10.0f;
        const float railZ = 8.0f + static_cast<float>((i * 29 + m_frame) % 540) / 9.0f;
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(ToWorldX(sideX), railX, railWeight),
            Math::Lerp(ToWorldY(sideY), railY, railWeight),
            Math::Lerp(SidePlaneZ + 12.0f, railZ, railWeight),
            0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f, 0.06f + railWeight * 0.01f,
            StarColor);
    }
    for (int i = 18; i < 28; ++i) {
        const float x = -13.0f + static_cast<float>((i * 37) % 260) / 10.0f;
        const float y = -6.0f + static_cast<float>((i * 53) % 120) / 10.0f;
        const float z = 8.0f + static_cast<float>((i * 29 + m_frame) % 540) / 9.0f;
        DrawModelPrimitive(renderer, camera, 1, x, y, z, 0.07f, 0.07f, 0.07f, StarColor);
    }
    for (int i = 0; i < 8; ++i) {
        const float x = -10.5f + i * 3.0f;
        const float sideX = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
        DrawModelPrimitive(renderer, camera, 1,
            Math::Lerp(ToWorldX(sideX), x, railWeight),
            Math::Lerp(ToWorldY(-0.80f), -3.3f, railWeight),
            Math::Lerp(SidePlaneZ + 7.0f, 32.0f, railWeight),
            Math::Lerp(0.04f, 0.025f, railWeight),
            Math::Lerp(6.8f, 0.025f, railWeight),
            Math::Lerp(0.08f, 70.0f, railWeight),
            GridColor);
    }
    for (int i = 0; i < 12; ++i) {
        const float z = 6.0f + i * 5.5f - std::fmod(m_scroll * 80.0f, 5.5f);
        DrawModelPrimitive(renderer, camera, 1, 0.0f,
            Math::Lerp(ToWorldY(-0.80f + (i % 5) * 0.40f), -3.3f, railWeight),
            Math::Lerp(SidePlaneZ + 7.0f, z, railWeight),
            Math::Lerp(14.0f, 22.0f, railWeight),
            Math::Lerp(0.04f, 0.025f, railWeight),
            Math::Lerp(0.08f, 0.025f, railWeight),
            GridColor);
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
        DrawEnemyModel(renderer, camera, drawEnemy, enemyYaw);
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot drawShot = shot;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? shot.transitionSideX :
            (exitingRail ? shot.x : ToSideXFromRailZ(shot.z));
        const float sideY = enteringRail ? shot.transitionSideY : shot.y;
        drawShot.x = Math::Lerp(sideX, exitingRail ? shot.transitionSideX : shot.x, railWeight);
        drawShot.y = Math::Lerp(sideY, shot.y, railWeight);
        drawShot.z = Math::Lerp(SidePlaneZ + (shot.enemy ? 1.0f : -0.4f), shot.z, railWeight);
        DrawShotModel(renderer, camera, drawShot, shot.enemy ? enemyYaw : playerYaw);
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
    std::snprintf(progressStatus, sizeof(progressStatus), "LIVES %d  DIST %03d%%", m_lives, progress);
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
    DrawBossStory(renderer);
    if (m_gameOver) {
        renderer.DrawText("GAME OVER", { -0.20f, 0.12f }, 0.045f, { 1.0f, 0.2f, 0.2f, 1.0f });
        renderer.DrawText("PRESS R TO RETRY", { -0.22f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    } else if (m_clear) {
        renderer.DrawText(m_stageNumber == 5 ? "ALL STAGES CLEAR" : "STAGE CLEAR", { -0.32f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
    }
}
