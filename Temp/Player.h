#pragma once
#include <windows.h>
#include <vector>
#include "Bullet.h"
#include "../Infrastructure/ExternalServices/InputSystem.h"

/**
 * @brief 自機 (Arwing) クラス (スターフォックス風 3D)
 */
class Player {
public:
    float x;
    float y;
    float z;            // Z座標 (通常 0.0f 付近に固定)
    float speed;
    int score;
    int lives;
    int shield;         // スターフォックス風のシールドゲージ (0〜100)
    int invincibleFrames;
    int shootCooldown;
    float rollAngle;    // Q/Eキー、または左右移動時の翼の傾き
    float size;         // 当たり判定の球半径

    Player() 
        : x(0.0f), y(-5.0f), z(0.0f), speed(1.8f), 
          score(0), lives(3), shield(100), invincibleFrames(0), shootCooldown(0), rollAngle(0.0f), size(3.5f) {
    }

    void Reset() {
        x = 0.0f;
        y = -5.0f;
        z = 0.0f;
        shield = 100;
        invincibleFrames = 90; // リスポーン無敵
        rollAngle = 0.0f;
    }

    void Update(std::vector<Bullet>& bullets) {
        if (shootCooldown > 0) shootCooldown--;
        if (invincibleFrames > 0) invincibleFrames--;

        float dx = 0.0f;
        float dy = 0.0f;

        // 左右上下移動
        if (InputSystem::IsKeyPressed(VK_LEFT))  dx -= 1.0f;
        if (InputSystem::IsKeyPressed(VK_RIGHT)) dx += 1.0f;
        if (InputSystem::IsKeyPressed(VK_UP))    dy += 1.0f;  // 3D空間なのでUPはYプラス方向
        if (InputSystem::IsKeyPressed(VK_DOWN))  dy -= 1.0f;

        // 移動時のロール傾き演出 (スターフォックスらしさ)
        if (dx < 0.0f) {
            rollAngle = rollAngle * 0.85f - 0.08f; // 左ローリング
        } else if (dx > 0.0f) {
            rollAngle = rollAngle * 0.85f + 0.08f; // 右ローリング
        } else {
            rollAngle = rollAngle * 0.8f; // 元に戻る
        }
        // 限界ロール角
        if (rollAngle < -0.6f) rollAngle = -0.6f;
        if (rollAngle > 0.6f) rollAngle = 0.6f;

        if (dx != 0.0f && dy != 0.0f) {
            dx *= 0.7071f;
            dy *= 0.7071f;
        }

        x += dx * speed;
        y += dy * speed;

        // 3Dフライト可能エリア制限 (カメラの視野内に収まるように制限)
        if (x < -60.0f) x = -60.0f;
        if (x > 60.0f) x = 60.0f;
        if (y < -35.0f) y = -35.0f;
        if (y > 35.0f) y = 35.0f;

        // ショット発射 (Zキー)
        if (InputSystem::IsKeyPressed('Z') && shootCooldown == 0) {
            Fire(bullets);
            shootCooldown = 6;
        }
    }

private:
    void Fire(std::vector<Bullet>& bullets) {
        // 自機の左右の翼から奥に向かって発射 (ダブルレーザー)
        int fired = 0;
        for (auto& b : bullets) {
            if (!b.active) {
                b.active = true;
                b.isEnemyBullet = false;
                b.z = z + 5.0f;
                b.vx = (fired == 0) ? -0.1f : 0.1f; // 少し外側に広がる
                b.vy = 0.0f;
                b.vz = 8.5f; // 奥へ高速直進
                b.size = 0.6f;
                b.color[0] = 0.1f; b.color[1] = 0.9f; b.color[2] = 0.1f; b.color[3] = 1.0f; // 緑色のレーザー

                if (fired == 0) {
                    b.x = x - 6.0f;
                    b.y = y;
                    fired++;
                } else if (fired == 1) {
                    b.x = x + 6.0f;
                    b.y = y;
                    fired++;
                    break;
                }
            }
        }
    }
};
