#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <DirectXMath.h>

#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"

namespace {
struct alignas(256) DrawConstants {
    DirectX::XMFLOAT4X4 wvp;
    DirectX::XMFLOAT4 color;
    float time;
    float shape;
    float rotation;
    float padding[41];
};

constexpr float PlayerColor[4] = { 0.80f, 0.80f, 0.85f, 1.0f };
constexpr float PlayerAccent[4] = { 0.10f, 0.90f, 0.90f, 1.0f };
constexpr float EnemyColor[4] = { 0.90f, 0.12f, 0.12f, 1.0f };
constexpr float EnemyAccent[4] = { 1.00f, 0.55f, 0.08f, 1.0f };
constexpr float PlayerShotColor[4] = { 0.15f, 1.00f, 0.25f, 1.0f };
constexpr float EnemyShotColor[4] = { 1.00f, 0.25f, 0.25f, 1.0f };
constexpr float GridColor[4] = { 0.05f, 0.22f, 0.16f, 1.0f };
constexpr float StarColor[4] = { 0.55f, 0.70f, 0.85f, 1.0f };
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
    m_gameOver = false;
    m_clear = false;
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
    m_scroll += 0.008f;
    m_shotCooldown = (std::max)(0, m_shotCooldown - 1);
    m_invincible = (std::max)(0, m_invincible - 1);

    float dx = static_cast<float>(m_moveRight) - static_cast<float>(m_moveLeft);
    float dy = static_cast<float>(m_moveUp) - static_cast<float>(m_moveDown);
    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }
    m_playerX = (std::clamp)(m_playerX + dx * 0.018f, -0.88f, 0.35f);
    m_playerY = (std::clamp)(m_playerY + dy * 0.024f, -0.72f, 0.72f);

    if (m_fire && m_shotCooldown == 0) {
        SpawnShot(m_playerX + 0.12f, m_playerY, 0.045f, 0.0f, false);
        m_shotCooldown = 7;
        PlayShotSound();
    }

    if (--m_spawnCooldown <= 0 && m_kills < 24) {
        SpawnEnemy();
        m_spawnCooldown = (std::max)(28, 70 - m_kills);
    }

    for (auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        ++enemy.age;
        enemy.x -= enemy.type == 0 ? 0.010f : 0.007f;
        enemy.y = enemy.baseY + std::sin(enemy.phase + enemy.age * 0.055f) * (enemy.type == 0 ? 0.10f : 0.18f);

        if (enemy.age % (enemy.type == 0 ? 105 : 72) == 0) {
            const float dxToPlayer = m_playerX - enemy.x;
            const float dyToPlayer = m_playerY - enemy.y;
            const float length = std::sqrt(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);
            if (length > 0.001f) {
                SpawnShot(enemy.x - 0.06f, enemy.y, dxToPlayer / length * 0.018f,
                    dyToPlayer / length * 0.018f, true);
            }
        }

        if (enemy.x < -1.08f) enemy.active = false;
        if (enemy.active && m_invincible == 0 && Hit(m_playerX, m_playerY, 0.055f, enemy.x, enemy.y, 0.065f)) {
            enemy.active = false;
            DamagePlayer();
        }
    }

    for (auto& shot : m_shots) {
        if (!shot.active) continue;
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

        for (auto& enemy : m_enemies) {
            if (!enemy.active || !Hit(shot.x, shot.y, 0.025f, enemy.x, enemy.y, 0.065f)) continue;
            shot.active = false;
            if (--enemy.hp <= 0) {
                enemy.active = false;
                ++m_kills;
                m_score += enemy.type == 0 ? 100 : 250;
                PlayHitSound();
                if (m_kills >= 24) m_clear = true;
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
        enemy.age = 0;
        return;
    }
}

void SideScrollingShooter::SpawnShot(float x, float y, float vx, float vy, bool enemy) {
    for (auto& shot : m_shots) {
        if (shot.active) continue;
        shot = { x, y, vx, vy, enemy, true };
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

void SideScrollingShooter::DrawShape(D3D12RenderingService& renderer, int& index,
    float x, float y, float w, float h, const float color[4]) {
    auto* constants = reinterpret_cast<DrawConstants*>(
        static_cast<char*>(renderer.GetCbvCpuData()) + index * 256);
    const auto matrix = DirectX::XMMatrixScaling(w, h, 1.0f) * DirectX::XMMatrixTranslation(x, y, 0.1f);
    DirectX::XMStoreFloat4x4(&constants->wvp, DirectX::XMMatrixTranspose(matrix));
    constants->color = { color[0], color[1], color[2], color[3] };
    constants->time = 0.0f;
    // ObjectShaderの6番はTriangleStrip用のXYスプライト
    constants->shape = 6.0f;
    constants->rotation = 0.0f;
    auto* commandList = renderer.GetCommandList();
    commandList->SetGraphicsRootConstantBufferView(0,
        renderer.GetConstantBuffer()->GetGPUVirtualAddress() + index * 256);
    commandList->DrawInstanced(4, 1, 0, 0);
    ++index;
}

void SideScrollingShooter::Render(D3D12RenderingService& renderer) const {
    renderer.SetPipelineState(0);
    // 0番台はRenderTextが使用するため図形用の定数バッファ領域を分離する
    int drawIndex = 512;

    for (int i = 0; i < 18; ++i) {
        float x = std::fmod(i * 0.137f - m_scroll * (0.6f + (i % 3) * 0.3f) + 1.0f, 2.0f) - 1.0f;
        float y = -0.82f + static_cast<float>((i * 47) % 164) / 100.0f;
        DrawShape(renderer, drawIndex, x, y, 0.006f, 0.010f, StarColor);
    }
    for (int i = 0; i < 7; ++i) {
        float x = std::fmod(i * 0.34f - m_scroll * 0.55f + 1.0f, 2.0f) - 1.0f;
        DrawShape(renderer, drawIndex, x, -0.80f, 0.008f, 1.55f, GridColor);
    }
    for (int i = 0; i < 5; ++i) {
        DrawShape(renderer, drawIndex, 0.0f, -0.80f + i * 0.40f, 2.0f, 0.006f, GridColor);
    }

    if (m_invincible == 0 || (m_invincible / 5) % 2 == 0) {
        DrawShape(renderer, drawIndex, m_playerX, m_playerY, 0.16f, 0.055f, PlayerColor);
        DrawShape(renderer, drawIndex, m_playerX - 0.035f, m_playerY + 0.055f, 0.075f, 0.045f, PlayerAccent);
        DrawShape(renderer, drawIndex, m_playerX - 0.035f, m_playerY - 0.055f, 0.075f, 0.045f, PlayerAccent);
    }

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        DrawShape(renderer, drawIndex, enemy.x, enemy.y, enemy.type == 0 ? 0.12f : 0.17f,
            enemy.type == 0 ? 0.10f : 0.15f, EnemyColor);
        DrawShape(renderer, drawIndex, enemy.x + 0.025f, enemy.y, 0.035f, 0.045f, EnemyAccent);
    }
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        DrawShape(renderer, drawIndex, shot.x, shot.y, shot.enemy ? 0.025f : 0.060f,
            shot.enemy ? 0.025f : 0.016f, shot.enemy ? EnemyShotColor : PlayerShotColor);
    }

    char status[64];
    std::snprintf(status, sizeof(status), "SCORE %06d   LIVES %d   ENEMY %02d/24", m_score, m_lives, m_kills);
    renderer.RenderText(status, { -0.92f, 0.86f }, 0.018f, { 0.75f, 0.95f, 0.85f, 1.0f });
    renderer.RenderText("MOVE: ARROWS/WASD  SHOT: Z/SPACE", { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    if (m_gameOver) {
        renderer.RenderText("GAME OVER", { -0.20f, 0.12f }, 0.045f, { 1.0f, 0.2f, 0.2f, 1.0f });
        renderer.RenderText("PRESS R TO RETRY", { -0.22f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    } else if (m_clear) {
        renderer.RenderText("STAGE CLEAR", { -0.23f, 0.12f }, 0.045f, { 0.2f, 1.0f, 0.5f, 1.0f });
        renderer.RenderText("PRESS R TO REPLAY", { -0.23f, -0.05f }, 0.020f, { 1, 1, 1, 1 });
    }
}
