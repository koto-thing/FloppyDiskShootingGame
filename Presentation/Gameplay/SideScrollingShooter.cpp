#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

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

/**
 * @brief 機体のローカル配置をY軸回転してワールド配置へ変換する
 */
Vector3 RotateYawOffset(float x, float y, float z, float yaw) {
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);
    return {x * c + z * s, y, -x * s + z * c};
}
}

void SideScrollingShooter::Initialize(AudioService* audio) {
    m_audio = audio;
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
    m_invincible = 90;
    m_lives = 3;
    m_score = 0;
    m_kills = 0;
    m_bossHp = 0;
    m_gameOver = false;
    m_clear = false;
    m_bossBattle = false;
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
        Reset();
    }
}

void SideScrollingShooter::Tick() {
    const bool wasTransitioning = m_viewTransitionTimer > 0;
    TickViewTransition();

    if (m_gameOver || m_clear) {
        return;
    }
    if (wasTransitioning || m_viewTransitionTimer > 0) {
        return;
    }

    ++m_frame;
    if (!m_bossBattle) {
        m_scroll += 0.008f;
    }
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_invincible = (std::max)(0, m_invincible - 1);

    TickPlayer();

    if (m_fire && m_shotCooldown == 0) {
        SpawnShot(m_playerX + (IsRailGameplayActive() ? 0.0f : 0.12f), m_playerY,
            IsRailGameplayActive() ? 0.0f : 0.045f, 0.0f, false);
        m_shotCooldown = 7;
        PlayShotSound();
    }

    // 規定スクロール距離へ到達したら通常区間を終了してボス戦を開始する
    if (!m_bossBattle && m_scroll >= BossStartDistance) {
        StartBossBattle();
    }

    if (!m_bossBattle && --m_spawnCooldown <= 0) {
        SpawnEnemy();
        m_spawnCooldown = (std::max)(28, 70 - m_kills);
    }

    TickEnemies();
    TickShots();
}

void SideScrollingShooter::TickPlayer() {
    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    //移動範囲制限
    const float minX = IsRailGameplayActive() ? -1.2f : -1.2f;
    const float maxX = IsRailGameplayActive() ? 1.2f : 1.2f;
    const float minY = IsRailGameplayActive() ? -0.9f : -0.9f;
    const float maxY = IsRailGameplayActive() ? 0.9f : 0.9f;
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f, minX, maxX);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f, minY, maxY);
}

void SideScrollingShooter::TickEnemies() {
    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        ++enemy.age;
        if (enemy.type == 2) {
            if (IsRailGameplayActive()) {
                // 3D中のボスは奥で待機しつつ上下左右へ揺れる
                enemy.x = std::sin(enemy.age * 0.018f) * 0.34f;
                enemy.y = std::sin(enemy.age * 0.025f) * 0.36f;
                if (enemy.z <= 0.0f) {
                    enemy.z = 48.0f;
                }
            } else {
                // ボスは画面内へ進入した後、上下に往復する
                if (enemy.x > 0.70f) {
                    enemy.x -= 0.008f;
                }
                enemy.z = ToRailZFromSideX(enemy.x);
                enemy.y = std::sin(enemy.age * 0.025f) * 0.48f;
            }
        } else {
            if (IsRailGameplayActive()) {
                // 3D中の通常敵は奥からカメラ手前へ接近する
                enemy.z -= enemy.type == 0 ? 0.42f : 0.30f;
                enemy.x = enemy.baseX + std::sin(enemy.phase + enemy.age * 0.045f) *
                    (enemy.type == 0 ? 0.16f : 0.24f);
            } else {
                enemy.x -= enemy.type == 0 ? 0.010f : 0.007f;
                enemy.z = ToRailZFromSideX(enemy.x);
            }
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
                    dyToPlayer / length * 0.018f, true, enemy.z);
            }
        }

        // ボスは一定間隔で3方向へ弾を発射する
        if (enemy.type == 2 && enemy.age % 120 == 0) {
            if (IsRailGameplayActive()) {
                SpawnShot(enemy.x, enemy.y, 0.0f, -0.018f, true, enemy.z);
                SpawnShot(enemy.x, enemy.y, 0.0f, 0.000f, true, enemy.z);
                SpawnShot(enemy.x, enemy.y, 0.0f, 0.018f, true, enemy.z);
            } else {
                SpawnShot(enemy.x - 0.12f, enemy.y, -0.020f, -0.010f, true, enemy.z);
                SpawnShot(enemy.x - 0.12f, enemy.y, -0.022f, 0.000f, true, enemy.z);
                SpawnShot(enemy.x - 0.12f, enemy.y, -0.020f, 0.010f, true, enemy.z);
            }
        }

        if (enemy.type != 2 && !IsRailGameplayActive() && enemy.x < -1.08f) enemy.active = false;
        if (enemy.type != 2 && IsRailGameplayActive() && enemy.z < 2.0f) enemy.active = false;
        const float enemyRadius = enemy.type == 2 ? 0.18f : 0.065f;
        const bool playerHit = IsRailGameplayActive() ?
            Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.42f,
                ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.type == 2 ? 1.6f : 0.7f) :
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
        shot.x += shot.vx;
        shot.y += shot.vy;
        shot.z += shot.vz;
        if (!IsRailGameplayActive()) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        if (!IsRailGameplayActive() && (shot.x < -1.1f || shot.x > 1.1f || std::abs(shot.y) > 1.05f)) {
            shot.active = false;
            continue;
        }
        if (IsRailGameplayActive() && (shot.z < 0.0f || shot.z > 72.0f ||
            std::abs(shot.x) > 1.2f || std::abs(shot.y) > 1.0f)) {
            shot.active = false;
            continue;
        }

        if (shot.enemy) {
            const bool playerHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ, 0.38f,
                    ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.28f) :
                Hit(m_playerX, m_playerY, 0.050f, shot.x, shot.y, 0.022f);
            if (m_invincible == 0 && playerHit) {
                shot.active = false;
                DamagePlayer();
            }
            continue;
        }

        for (auto& enemy : m_enemies) {
            const float enemyRadius = enemy.type == 2 ? 0.18f : 0.065f;
            if (!enemy.active) continue;
            const bool enemyHit = IsRailGameplayActive() ?
                Hit3D(ToWorldX(shot.x), ToWorldY(shot.y), shot.z, 0.25f,
                    ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, enemy.type == 2 ? 1.5f : 0.55f) :
                Hit(shot.x, shot.y, 0.025f, enemy.x, enemy.y, enemyRadius);
            if (!enemyHit) continue;
            shot.active = false;
            if (--enemy.hp <= 0) {
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

void SideScrollingShooter::TickViewTransition() {
    if (m_viewTransitionTimer > 0) {
        --m_viewTransitionTimer;
        m_viewTransitionProgress = SmoothStep(
            1.0f - static_cast<float>(m_viewTransitionTimer) / ViewTransitionFrames);
        if (m_viewTransitionTimer == 0) {
            m_viewMode = m_nextViewMode;
            m_viewTransitionProgress = 0.0f;
            if (m_viewMode == ViewMode::Rail3D) {
                InitializeRailObjects();
            } else {
                // 自機以外は3D奥行きを2D横位置へ戻して、奥の物体ほど右側へ配置する
                for (auto& enemy : m_enemies) {
                    if (!enemy.active) continue;
                    enemy.x = ToSideXFromRailZ(enemy.z);
                    enemy.baseX = enemy.x;
                    enemy.z = ToRailZFromSideX(enemy.x);
                }
                for (auto& shot : m_shots) {
                    if (!shot.active) continue;
                    shot.x = ToSideXFromRailZ(shot.z);
                    shot.z = ToRailZFromSideX(shot.x);
                    shot.vz = 0.0f;
                    if (!shot.enemy) {
                        shot.vx = 0.045f;
                    }
                }
            }
        }
        return;
    }

    m_viewTransitionProgress = 0.0f;
    if (!m_viewToggleRequested) {
        return;
    }

    // 表示モードのみを切り替え、ゲーム状態は次フレームから遷移完了まで停止する
    m_nextViewMode = m_viewMode == ViewMode::Side2D ? ViewMode::Rail3D : ViewMode::Side2D;
    m_viewTransitionTimer = ViewTransitionFrames;
    if (m_nextViewMode == ViewMode::Rail3D) {
        InitializeRailObjects();
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
        if (shot.z <= 0.0f) {
            shot.z = ToRailZFromSideX(shot.x);
        }
        shot.x = 0.0f;
        if (shot.enemy) {
            const float dx = ToWorldX(m_playerX) - ToWorldX(shot.x);
            const float dy = ToWorldY(m_playerY) - ToWorldY(shot.y);
            const float dz = PlayerRailZ - shot.z;
            const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
            constexpr float EnemyShotSpeed = 0.62f;
            shot.vx = FromWorldX(dx / length * EnemyShotSpeed);
            shot.vy = FromWorldY(dy / length * EnemyShotSpeed);
            shot.vz = dz / length * EnemyShotSpeed;
        } else {
            shot.vx = 0.0f;
            shot.vy = 0.0f;
            shot.vz = 1.45f;
        }
    }
}

void SideScrollingShooter::SpawnEnemy() {
    for (auto& enemy : m_enemies) {
        if (enemy.active) continue;
        enemy.active = true;
        enemy.baseX = IsRailGameplayActive() ?
            -0.72f + static_cast<float>((m_frame * 53) % 145) / 100.0f : 1.05f;
        enemy.x = enemy.baseX;
        enemy.baseY = IsRailGameplayActive() ?
            -0.52f + static_cast<float>((m_frame * 37) % 105) / 100.0f :
            -0.60f + static_cast<float>((m_frame * 37) % 120) / 100.0f;
        enemy.y = enemy.baseY;
        enemy.z = IsRailGameplayActive() ? EnemyRailFarZ : ToRailZFromSideX(enemy.x);
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
    boss.z = IsRailGameplayActive() ? 48.0f : ToRailZFromSideX(boss.x);
    boss.baseX = 0.0f;
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

void SideScrollingShooter::SpawnShot(float x, float y, float vx, float vy, bool enemy, float z) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot.x = x;
        shot.y = y;
        shot.z = IsRailGameplayActive() ? (z >= 0.0f ? z : PlayerRailZ + 2.0f) :
            ToRailZFromSideX(x);
        shot.transitionSideX = x;
        shot.transitionSideY = y;
        shot.vx = vx;
        shot.vy = vy;
        shot.vz = 0.0f;
        if (IsRailGameplayActive()) {
            if (enemy) {
                const float targetX = m_playerX + vx * 12.0f;
                const float targetY = m_playerY + vy * 12.0f;
                const float dx = ToWorldX(targetX) - ToWorldX(x);
                const float dy = ToWorldY(targetY) - ToWorldY(y);
                const float dz = PlayerRailZ - shot.z;
                const float length = (std::max)(0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                constexpr float EnemyShotSpeed = 0.62f;
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
    return m_viewMode == ViewMode::Rail3D && m_viewTransitionTimer == 0;
}

bool SideScrollingShooter::IsRailRenderActive() const {
    return m_viewMode == ViewMode::Rail3D || m_nextViewMode == ViewMode::Rail3D ||
        m_viewTransitionTimer > 0;
}

void SideScrollingShooter::ConfigureSideCamera(Camera3D& camera, Renderer& renderer) const {
    // 2DモードはXY移動平面を3Dカメラで正面から見て奥行きを持たせる
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Perspective);
    camera.SetFieldOfView(Math::ToRadians(38.0f));
    camera.SetNearClip(0.1f);
    camera.SetFarClip(80.0f);
    camera.SetPosition({0.0f, 0.0f, -11.0f});
    camera.LookAt({0.0f, 0.0f, SidePlaneZ});
}

void SideScrollingShooter::ConfigureRailCamera(Camera3D& camera, Renderer& renderer) const {
    const float railWeight = RailBlend();

    const Vector3 playerPosition{ToWorldX(m_playerX), ToWorldY(m_playerY), PlayerRailZ};
    // 2Dモードと同じカメラ状態から、3Dレールの追従カメラへ補間する
    const Vector3 sidePosition{0.0f, 0.0f, -11.0f};
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
        // ボスは大きめの箱と砲台で構成する
        Vector3 offset = RotateYawOffset(0.0f, 0.0f, 0.0f, yaw);
        DrawModelPrimitive(renderer, camera, 1, x + offset.x, y + offset.y, z + offset.z,
            2.8f, 1.8f, 2.2f, BossColor, yaw);
        offset = RotateYawOffset(-1.55f, 1.35f, 0.0f, yaw);
        DrawModelPrimitive(renderer, camera, 2, x + offset.x, y + offset.y, z + offset.z,
            0.42f, 0.42f, 1.7f, BossAccent, yaw);
        offset = RotateYawOffset(-1.55f, -1.35f, 0.0f, yaw);
        DrawModelPrimitive(renderer, camera, 2, x + offset.x, y + offset.y, z + offset.z,
            0.42f, 0.42f, 1.7f, BossAccent, yaw);
        offset = RotateYawOffset(0.0f, 0.0f, -1.45f, yaw);
        DrawModelPrimitive(renderer, camera, 3, x + offset.x, y + offset.y, z + offset.z,
            0.85f, 0.85f, 1.3f, EnemyAccent, yaw);
        return;
    }

    // 通常敵は奥から来る小型機として描画する
    const float scale = enemy.type == 0 ? 1.0f : 1.28f;
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
    const float* color = shot.enemy ? EnemyShotColor : PlayerShotColor;
    const float length = shot.enemy ? 0.65f : 1.15f;
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        0.16f, 0.16f, length, color, yaw);
}

void SideScrollingShooter::DrawBossHud(Renderer& renderer) const {
    if (!m_bossBattle || m_clear) {
        return;
    }

    // 2D/3D共通のボスHP表示をカメラリセット後のUI座標へ描画する
    constexpr float BossBarBack[4] = { 0.20f, 0.08f, 0.22f, 1.0f };
    constexpr float BossBarFill[4] = { 0.95f, 0.15f, 0.45f, 1.0f };
    const float hpRate = static_cast<float>(m_bossHp) / BossMaxHp;
    DrawShape(renderer, 0.0f, 0.76f, 0.62f, 0.025f, BossBarBack);
    DrawShape(renderer, -0.62f * (1.0f - hpRate), 0.76f,
        0.62f * hpRate, 0.018f, BossBarFill);
    renderer.DrawText("BOSS", { 0.02f, 0.86f }, 0.014f,
        { 1.0f, 0.45f, 0.65f, 1.0f });
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
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        SidePlaneZ, m_invincible == 0 || (m_invincible / 5) % 2 == 0, Math::HalfPi);

    renderer.ResetCamera();

    char status[80];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / BossStartDistance * 100.0f));
    std::snprintf(status, sizeof(status), "SCORE %06d   LIVES %d   DIST %03d%%",
        m_score, m_lives, progress);
    renderer.DrawText(status, { -0.92f, 0.86f }, 0.018f, { 0.75f, 0.95f, 0.85f, 1.0f });
    renderer.DrawText("MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    renderer.DrawText("VIEW: SIDE 2D", { 0.50f, 0.86f }, 0.014f,
        { ModeTextColor[0], ModeTextColor[1], ModeTextColor[2], ModeTextColor[3] });

    DrawBossHud(renderer);
    if (m_gameOver) {
        renderer.DrawText("GAME OVER", { -0.20f, 0.12f }, 0.045f, { 1.0f, 0.2f, 0.2f, 1.0f });
        renderer.DrawText("PRESS R TO RETRY", { -0.22f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    } else if (m_clear) {
        renderer.DrawText("STAGE CLEAR", { -0.23f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
        renderer.DrawText("PRESS R TO REPLAY", { -0.23f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
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
        const float sideX = enteringRail ? enemy.transitionSideX :
            (m_viewTransitionTimer > 0 ? ToSideXFromRailZ(enemy.z) : enemy.x);
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        drawEnemy.x = Math::Lerp(sideX, enemy.x, railWeight);
        drawEnemy.y = Math::Lerp(sideY, enemy.y, railWeight);
        drawEnemy.z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), enemy.z, railWeight);
        DrawEnemyModel(renderer, camera, drawEnemy, enemyYaw);
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot drawShot = shot;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? shot.transitionSideX :
            (m_viewTransitionTimer > 0 ? ToSideXFromRailZ(shot.z) : shot.x);
        const float sideY = enteringRail ? shot.transitionSideY : shot.y;
        drawShot.x = Math::Lerp(sideX, shot.x, railWeight);
        drawShot.y = Math::Lerp(sideY, shot.y, railWeight);
        drawShot.z = Math::Lerp(SidePlaneZ + (shot.enemy ? 1.0f : -0.4f), shot.z, railWeight);
        DrawShotModel(renderer, camera, drawShot, shot.enemy ? enemyYaw : playerYaw);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight),
        m_invincible == 0 || (m_invincible / 5) % 2 == 0, playerYaw);

    renderer.ResetCamera();

    char status[80];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / BossStartDistance * 100.0f));
    std::snprintf(status, sizeof(status), "SCORE %06d   LIVES %d   DIST %03d%%",
        m_score, m_lives, progress);
    renderer.DrawText(status, { -0.92f, 0.86f }, 0.018f, { 0.75f, 0.95f, 0.85f, 1.0f });
    renderer.DrawText("MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    renderer.DrawText("VIEW: RAIL 3D", { 0.50f, 0.86f }, 0.014f,
        { ModeTextColor[0], ModeTextColor[1], ModeTextColor[2], ModeTextColor[3] });
    if (m_viewTransitionTimer > 0) {
        renderer.DrawText("CAMERA SHIFT", { -0.16f, -0.02f }, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f });
    }
    DrawBossHud(renderer);
    if (m_gameOver) {
        renderer.DrawText("GAME OVER", { -0.20f, 0.12f }, 0.045f, { 1.0f, 0.2f, 0.2f, 1.0f });
        renderer.DrawText("PRESS R TO RETRY", { -0.22f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    } else if (m_clear) {
        renderer.DrawText("STAGE CLEAR", { -0.23f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
        renderer.DrawText("PRESS R TO REPLAY", { -0.23f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    }
}
