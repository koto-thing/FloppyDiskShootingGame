#include "GameManager.h"
#include "../Infrastructure/InputSystem.h"
#include <windows.h>

GameManager::GameManager() : m_state{} {
}

GameManager::~GameManager() {
}

void GameManager::Initialize() {
    m_state = {};
    m_state.player.active = true;
    m_state.player.pos = { 400.0f, 500.0f };
    m_state.player.size = 32.0f;

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_state.enemies[i].active = true;
        float col = (float)(i % 5);
        float row = (float)(i / 5);
        m_state.enemies[i].pos = { 150.0f + col * 120.0f, 100.0f + row * 60.0f };
        m_state.enemies[i].size = 24.0f;
        m_state.enemies[i].velocity = { 1.5f, 0.0f };
    }
}

void GameManager::ProcessInput() {
    float speed = 5.0f;

    // WASD or 矢印キーで移動
    if (InputSystem::IsKeyPressed(VK_LEFT)  || InputSystem::IsKeyPressed('A')) m_state.player.pos.x -= speed;
    if (InputSystem::IsKeyPressed(VK_RIGHT) || InputSystem::IsKeyPressed('D')) m_state.player.pos.x += speed;
    if (InputSystem::IsKeyPressed(VK_UP)    || InputSystem::IsKeyPressed('W')) m_state.player.pos.y -= speed;
    if (InputSystem::IsKeyPressed(VK_DOWN)  || InputSystem::IsKeyPressed('S')) m_state.player.pos.y += speed;

    // ウィンドウ内でプレイヤーが移動するように座標をクランプ
    if (m_state.player.pos.x < 20.0f) m_state.player.pos.x = 20.0f;
    if (m_state.player.pos.x > 780.0f) m_state.player.pos.x = 780.0f;
    if (m_state.player.pos.y < 20.0f) m_state.player.pos.y = 20.0f;
    if (m_state.player.pos.y > 580.0f) m_state.player.pos.y = 580.0f;

    // スペースキーで弾を発射
    if (GetAsyncKeyState(VK_SPACE) & 0x1) {
        // オブジェクトプールから非アクティブな弾を探して発射
        for (int i = 0; i < MAX_BULLETS; ++i) {
            if (!m_state.playerBullets[i].active) {
                m_state.playerBullets[i].active = true;
                m_state.playerBullets[i].pos = m_state.player.pos;
                m_state.playerBullets[i].size = 12.0f;
                m_state.playerBullets[i].velocity = { 0.0f, -10.0f };
                break;
            }
        }
    }
}

void GameManager::Update() {
    // 弾をうごかして画面外に出たら非アクティブにする
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (m_state.playerBullets[i].active) {
            m_state.playerBullets[i].pos.y += m_state.playerBullets[i].velocity.y;
            if (m_state.playerBullets[i].pos.y < -10.0f) {
                m_state.playerBullets[i].active = false;
            }
        }
    }

    // 敵の移動と方向転換
    bool changeDirection = false;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (m_state.enemies[i].active) {
            m_state.enemies[i].pos.x += m_state.enemies[i].velocity.x;
            if (m_state.enemies[i].pos.x < 30.0f || m_state.enemies[i].pos.x > 770.0f) {
                changeDirection = true;
            }
        }
    }

    if (changeDirection) {
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (m_state.enemies[i].active) {
                m_state.enemies[i].velocity.x *= -1.0f;
                m_state.enemies[i].pos.y += 20.0f; // 下へ移動
            }
        }
    }

    // バレットと敵の当たり判定
    for (int b = 0; b < MAX_BULLETS; ++b) {
        if (!m_state.playerBullets[b].active) continue;

        for (int e = 0; e < MAX_ENEMIES; ++e) {
            if (!m_state.enemies[e].active) continue;

            float dx = m_state.playerBullets[b].pos.x - m_state.enemies[e].pos.x;
            float dy = m_state.playerBullets[b].pos.y - m_state.enemies[e].pos.y;
            float distSq = dx * dx + dy * dy;
            float limit = (m_state.playerBullets[b].size + m_state.enemies[e].size) * 0.5f;

            if (distSq < limit * limit) {
                m_state.playerBullets[b].active = false;
                m_state.enemies[e].active = false;
                m_state.score += 100;
                break;
            }
        }
    }

    // Player vs Enemy collision
    for (int e = 0; e < MAX_ENEMIES; ++e) {
        if (!m_state.enemies[e].active) continue;

        float dx = m_state.player.pos.x - m_state.enemies[e].pos.x;
        float dy = m_state.player.pos.y - m_state.enemies[e].pos.y;
        float distSq = dx * dx + dy * dy;
        float limit = (m_state.player.size + m_state.enemies[e].size) * 0.5f;

        if (distSq < limit * limit) {
            // Reset game state on hit
            m_state.player.pos = { 400.0f, 500.0f };
            m_state.score = 0;
            // Re-initialize enemies
            for (int i = 0; i < MAX_ENEMIES; ++i) {
                m_state.enemies[i].active = true;
                float col = (float)(i % 5);
                float row = (float)(i / 5);
                m_state.enemies[i].pos = { 150.0f + col * 120.0f, 100.0f + row * 60.0f };
                m_state.enemies[i].size = 24.0f;
                m_state.enemies[i].velocity = { 1.5f, 0.0f };
            }
            break;
        }
    }
}
