#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"

namespace {
constexpr float PlayerColor[4] = { 0.80f, 0.80f, 0.85f, 1.0f };
constexpr float PlayerAccent[4] = { 0.10f, 0.90f, 0.90f, 1.0f };
constexpr float EnemyColor[4] = { 0.90f, 0.12f, 0.12f, 1.0f };
constexpr float EnemyAccent[4] = { 1.00f, 0.55f, 0.08f, 1.0f };
constexpr float BossColor[4] = { 0.45f, 0.15f, 0.80f, 1.0f };
constexpr float BossAccent[4] = { 1.00f, 0.30f, 0.65f, 1.0f };
constexpr float PlayerShotColor[4] = { 0.15f, 1.00f, 0.25f, 1.0f };
constexpr float EnemyShotColor[4] = { 1.00f, 0.25f, 0.25f, 1.0f };
constexpr float GridColor[4] = { 0.05f, 0.22f, 0.16f, 1.0f };
constexpr float StarColor[4] = { 0.55f, 0.70f, 0.85f, 1.0f };

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
}

void SideScrollingShooter::Initialize(AudioService* audio, PlayerType playerType) {
    m_audio = audio;
    m_playerType = playerType;
    Reset();
}

void SideScrollingShooter::Reset() {
    m_shots = {};
    m_enemies = {};
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
    m_gameOver = false;
    m_clear = false;
    m_bossBattle = false;
}

void SideScrollingShooter::ProcessInput() {
    m_moveLeft = Input::GetKey(KeyCode::LeftArrow) || Input::GetKey(KeyCode::A);
    m_moveRight = Input::GetKey(KeyCode::RightArrow) || Input::GetKey(KeyCode::D);
    m_moveUp = Input::GetKey(KeyCode::UpArrow) || Input::GetKey(KeyCode::W);
    m_moveDown = Input::GetKey(KeyCode::DownArrow) || Input::GetKey(KeyCode::S);
    m_fire = Input::GetKey(KeyCode::Z) || Input::GetKey(KeyCode::Space);

    if ((m_gameOver || m_clear) && Input::GetKeyDown(KeyCode::R)) {
        Reset();
    }
}

void SideScrollingShooter::Tick() {
    if (m_gameOver || m_clear) {
        return;
    }

    ++m_frame;
    if (!m_bossBattle) {
        m_scroll += 0.008f;
    }
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_specialShotCooldown = (std::max)(0, m_specialShotCooldown - 1);
    m_invincible = (std::max)(0, m_invincible - 1);

    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f, -0.88f, 0.35f);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f, -0.72f, 0.72f);

    bool firedPlayerShot = false;
    if (m_fire && m_shotCooldown == 0) {
        /** @brief 機首中央から全機体共通の通常弾を発射する */
        FireNormalShot();
        m_shotCooldown = NormalShotConfig.fireIntervalFrames;
        firedPlayerShot = true;
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
    if (!m_bossBattle && m_scroll >= BossStartDistance) {
        StartBossBattle();
    }

    if (!m_bossBattle && --m_spawnCooldown <= 0) {
        SpawnEnemy();
        m_spawnCooldown = (std::max)(28, 70 - m_kills);
    }

    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        ++enemy.age;
        if (enemy.type == 2) {
            // ボスは画面内へ進入した後、上下に往復する
            if (enemy.x > 0.70f) {
                enemy.x -= 0.008f;
            }
            enemy.y = std::sin(enemy.age * 0.025f) * 0.48f;
        } else {
            enemy.x -= enemy.type == 0 ? 0.010f : 0.007f;
            enemy.y = enemy.baseY + std::sin(enemy.phase + enemy.age * 0.055f) *
                (enemy.type == 0 ? 0.10f : 0.18f);
        }

        const int aimedShotInterval = enemy.type == 2 ? 42 : (enemy.type == 0 ? 105 : 72);
        if (enemy.age % aimedShotInterval == 0) {
            const float dxToPlayer = m_playerX - enemy.x;
            const float dyToPlayer = m_playerY - enemy.y;
            const float length = std::sqrt(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);
            if (length > 0.001f) {
                SpawnShot(enemy.x - 0.06f, enemy.y, dxToPlayer / length * 0.018f,
                    dyToPlayer / length * 0.018f, true);
            }
        }

        // ボスは一定間隔で3方向へ弾を発射する
        if (enemy.type == 2 && enemy.age % 120 == 0) {
            SpawnShot(enemy.x - 0.12f, enemy.y, -0.020f, -0.010f, true);
            SpawnShot(enemy.x - 0.12f, enemy.y, -0.022f, 0.000f, true);
            SpawnShot(enemy.x - 0.12f, enemy.y, -0.020f, 0.010f, true);
        }

        if (enemy.type != 2 && enemy.x < -1.08f) enemy.active = false;
        const float enemyRadius = enemy.type == 2 ? 0.18f : 0.065f;
        if (enemy.active && m_invincible == 0 &&
            Hit(m_playerX, m_playerY, 0.055f, enemy.x, enemy.y, enemyRadius)) {
            if (enemy.type != 2) enemy.active = false;
            DamagePlayer();
        }
    }

    for (auto& shot : m_shots) {
        if (!shot.active) continue;

        /** @brief 追尾弾を最寄りの前方敵へ旋回させる */
        if (!shot.enemy && shot.special && shot.playerType == Homing) {
            UpdateHomingShot(shot);
        }

        shot.x += shot.vx;
        shot.y += shot.vy;
        if (shot.x < -1.1f || shot.x > 1.1f || std::abs(shot.y) > 1.05f) {
            shot.active = false;
            continue;
        }

        if (shot.enemy) {
            if (m_invincible == 0 && Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y, 0.022f)) {
                shot.active = false;
                DamagePlayer();
            }
            continue;
        }

        for (size_t enemyIndex = 0; enemyIndex < m_enemies.size(); ++enemyIndex) {
            auto& enemy = m_enemies[enemyIndex];
            const float enemyRadius = enemy.type == 2 ? 0.18f : 0.065f;
            const unsigned int enemyBit = 1u << enemyIndex;
            if (!enemy.active || (shot.hitEnemyMask & enemyBit) != 0 ||
                !Hit(shot.x, shot.y, shot.hitRadius,
                enemy.x, enemy.y, enemyRadius)) continue;
            shot.hitEnemyMask |= enemyBit;
            if (!shot.piercing) shot.active = false;
            enemy.hp -= shot.damage;
            if (enemy.hp <= 0) {
                enemy.active = false;
                if (enemy.type == 2) {
                    m_bossHp = 0;
                    m_score += 5000;
                    m_clear = true;
                } else {
                    ++m_kills;
                    m_score += enemy.type == 0 ? 100 : 250;
                }
                PlayHitSound();
            } else if (enemy.type == 2) {
                m_bossHp = enemy.hp;
            }
            break;
        }
    }
}

void SideScrollingShooter::SpawnEnemy() {
    for (auto& enemy : m_enemies) {
        if (enemy.active) continue;
        enemy.active = true;
        enemy.x = 1.05f;
        enemy.baseY = -0.60f + static_cast<float>((m_frame * 37) % 120) / 100.0f;
        enemy.y = enemy.baseY;
        enemy.phase = static_cast<float>(m_frame % 31) * 0.2f;
        enemy.type = ((m_kills + m_frame / 60) % 5 == 4) ? 1 : 0;
        enemy.hp = enemy.type == 0 ? 1 : 3;
        enemy.maxHp = enemy.hp;
        enemy.age = 0;
        return;
    }
}

void SideScrollingShooter::StartBossBattle() {
    m_bossBattle = true;
    m_bossHp = BossMaxHp;

    // 通常敵と敵弾を消去してボス戦へ切り替える
    for (auto& enemy : m_enemies) {
        enemy.active = false;
    }
    for (auto& shot : m_shots) {
        if (shot.enemy) shot.active = false;
    }

    Enemy& boss = m_enemies[0];
    boss.active = true;
    boss.x = 1.16f;
    boss.y = 0.0f;
    boss.baseY = 0.0f;
    boss.phase = 0.0f;
    boss.type = 2;
    boss.hp = BossMaxHp;
    boss.maxHp = BossMaxHp;
    boss.age = 0;
    m_invincible = (std::max)(m_invincible, 60);

    if (m_audio) {
        m_audio->PlayMMLSE("t180 o4 l16 v12 c g > c");
    }
}

void SideScrollingShooter::SpawnShot(float x, float y, float vx, float vy, bool enemy) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = x;
        shot.y = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.enemy = enemy;
        shot.active = true;
        return;
    }
}

/** @brief 機首中央から全機体共通の通常弾を生成する */
void SideScrollingShooter::FireNormalShot() {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = m_playerX + NormalShotConfig.spawnOffsetX;
        shot.y = m_playerY;
        shot.vx = NormalShotConfig.speed;
        shot.hitRadius = NormalShotConfig.hitRadius;
        shot.damage = NormalShotConfig.damage;
        shot.active = true;
        return;
    }
}

/** @brief 選択中の機体タイプに対応する特殊弾を生成する */
void SideScrollingShooter::FireSpecialShots() {
    const auto& config = PlayerShotConfigs[static_cast<size_t>(m_playerType)];
    constexpr float DegreesToRadians = 3.1415926535f / 180.0f;

    /** @brief 弾数に応じて左右対称の角度と発射位置を求める */
    for (int i = 0; i < config.projectileCount; ++i) {
        const float centeredIndex = static_cast<float>(i) -
            static_cast<float>(config.projectileCount - 1) * 0.5f;
        const float angleStep = config.projectileCount > 1
            ? config.spreadAngleDegrees / static_cast<float>(config.projectileCount - 1)
            : 0.0f;
        const float angle = centeredIndex * angleStep * DegreesToRadians;
        const float spawnY = m_playerY + centeredIndex * config.spawnOffsetY;

        /** @brief 空きスロットへ機体タイプ固有の属性を設定する */
        for (auto& shot : m_shots) {
            if (shot.active) continue;
            shot = {};
            shot.x = m_playerX + config.spawnOffsetX;
            shot.y = spawnY;
            shot.vx = std::cos(angle) * config.speed;
            shot.vy = std::sin(angle) * config.speed;
            shot.hitRadius = config.hitRadius;
            shot.damage = config.damage;
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

void SideScrollingShooter::DrawShape(Renderer& renderer,
    float x, float y, float w, float h, const float color[4]) {
    // 描画ファサードへ矩形コマンドとして記録する
    renderer.Draw(Rect { { x, y }, { w, h } }, { color[0], color[1], color[2], color[3] });
}

void SideScrollingShooter::Render(Renderer& renderer) const {

    for (int i = 0; i < 18; ++i) {
        float x = WrapNdcX(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f));
        float y = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        DrawShape(renderer, x, y, 0.006f, 0.010f, StarColor);
    }
    for (int i = 0; i < 7; ++i) {
        float x = WrapNdcX(i * 0.34f - m_scroll * 0.55f);
        DrawShape(renderer, x, -0.80f, 0.008f, 1.55f, GridColor);
    }
    for (int i = 0; i < 5; ++i) {
        DrawShape(renderer, 0.0f, -0.80f + i * 0.40f, 2.0f, 0.006f, GridColor);
    }

    if (m_invincible == 0 || (m_invincible / 5) % 2 == 0) {
        DrawShape(renderer, m_playerX, m_playerY, 0.16f, 0.055f, PlayerColor);
        DrawShape(renderer, m_playerX - 0.035f, m_playerY + 0.055f, 0.075f, 0.045f, PlayerAccent);
        DrawShape(renderer, m_playerX - 0.035f, m_playerY - 0.055f, 0.075f, 0.045f, PlayerAccent);
    }

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        if (enemy.type == 2) {
            // ボスは複数の矩形を組み合わせて通常敵と識別できる外見にする
            DrawShape(renderer, enemy.x, enemy.y, 0.28f, 0.23f, BossColor);
            DrawShape(renderer, enemy.x - 0.12f, enemy.y + 0.20f,
                0.17f, 0.075f, BossAccent);
            DrawShape(renderer, enemy.x - 0.12f, enemy.y - 0.20f,
                0.17f, 0.075f, BossAccent);
            DrawShape(renderer, enemy.x - 0.23f, enemy.y,
                0.065f, 0.10f, EnemyAccent);
        } else {
            DrawShape(renderer, enemy.x, enemy.y,
                enemy.type == 0 ? 0.12f : 0.17f,
                enemy.type == 0 ? 0.10f : 0.15f, EnemyColor);
            DrawShape(renderer, enemy.x + 0.025f, enemy.y,
                0.035f, 0.045f, EnemyAccent);
        }
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        if (shot.enemy) {
            DrawShape(renderer, shot.x, shot.y, 0.025f, 0.025f, EnemyShotColor);
            continue;
        }

        /** @brief 機体タイプに応じた大きさでプロシージャル自機弾を描画する */
        Vector2 visualSize { 0.060f, 0.018f };
        int visualType = 3;
        if (shot.special) {
            visualType = static_cast<int>(shot.playerType);
            visualSize = { 0.075f, 0.035f };
            if (shot.playerType == Piercing) visualSize = { 0.145f, 0.032f };
            if (shot.playerType == Spread) visualSize = { 0.042f, 0.042f };
        }
        renderer.DrawPlayerShot({
            { shot.x, shot.y },
            visualSize,
            std::atan2(shot.vy, shot.vx),
            static_cast<float>(m_frame),
            visualType
        });
    }

    char status[80];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / BossStartDistance * 100.0f));
    std::snprintf(status, sizeof(status), "SCORE %06d   LIVES %d   DIST %03d%%",
        m_score, m_lives, progress);
    renderer.DrawText(status, { -0.92f, 0.86f }, 0.018f, { 0.75f, 0.95f, 0.85f, 1.0f });
    renderer.DrawText("MOVE: ARROWS/WASD  SHOT: Z/SPACE", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    if (m_bossBattle && !m_clear) {
        constexpr float BossBarBack[4] = { 0.20f, 0.08f, 0.22f, 1.0f };
        constexpr float BossBarFill[4] = { 0.95f, 0.15f, 0.45f, 1.0f };
        const float hpRate = static_cast<float>(m_bossHp) / BossMaxHp;
        DrawShape(renderer, 0.0f, 0.76f, 0.62f, 0.025f, BossBarBack);
        DrawShape(renderer, -0.62f * (1.0f - hpRate), 0.76f,
            0.62f * hpRate, 0.018f, BossBarFill);
        renderer.DrawText("BOSS", { 0.02f, 0.86f }, 0.014f,
            { 1.0f, 0.45f, 0.65f, 1.0f });
    }
    if (m_gameOver) {
        renderer.DrawText("GAME OVER", { -0.20f, 0.12f }, 0.045f, { 1.0f, 0.2f, 0.2f, 1.0f });
        renderer.DrawText("PRESS R TO RETRY", { -0.22f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    } else if (m_clear) {
        renderer.DrawText("STAGE CLEAR", { -0.23f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
        renderer.DrawText("PRESS R TO REPLAY", { -0.23f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    }
}
