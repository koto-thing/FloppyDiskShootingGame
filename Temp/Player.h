#pragma once
#include <Windows.h>
#include <vector>
#include <cmath>
#include "Bullet.h"
#include "../Infrastructure/ExternalServices/InputService.h"

/**
 * @brief 自機（プレイヤー）クラス (シームレス2D/3D対応)
 */
class Player {
public:
    float x;
    float y;
    float z;            // 3D座標
    float speed;
    float size;         // 衝突判定半径
    int shield;         // シールド残量 (0 - 100)
    int lives;          // 残機
    int score;
    int shootCooldown;
    int invincibleFrames;
    float rollAngle;    // ロール回転角 (スターフォックス風)

    Player() {
        Reset();
        lives = 3;
        score = 0;
    }

    void Reset() {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        speed = 1.0f;
        size = 3.5f;
        shield = 100;
        shootCooldown = 0;
        invincibleFrames = 60; // リセット時無敵
        rollAngle = 0.0f;
    }

    void Update(std::vector<Bullet>& bullets, int dimMode, int nextDimMode, float progress) {
        if (shootCooldown > 0) shootCooldown--;
        if (invincibleFrames > 0) invincibleFrames--;

        float dx = 0.0f;
        float dy = 0.0f;

        // 左右上下入力
        if (InputService::IsKeyPressed(VK_LEFT))  dx -= 1.0f;
        if (InputService::IsKeyPressed(VK_RIGHT)) dx += 1.0f;
        if (InputService::IsKeyPressed(VK_UP))    dy += 1.0f;  
        if (InputService::IsKeyPressed(VK_DOWN))  dy -= 1.0f;

        // 移動時のロール傾き演出 (スターフォックスらしさ)
        if (dx < 0.0f) {
            rollAngle = rollAngle * 0.85f - 0.08f; // 左ローリング
        } else if (dx > 0.0f) {
            rollAngle = rollAngle * 0.85f + 0.08f; // 右ローリング
        } else {
            rollAngle = rollAngle * 0.8f; // 元に戻る
        }
        if (rollAngle < -0.6f) rollAngle = -0.6f;
        if (rollAngle > 0.6f) rollAngle = 0.6f;

        if (dx != 0.0f && dy != 0.0f) {
            dx *= 0.7071f;
            dy *= 0.7071f;
        }

        // 優勢なモードの決定
        int activeMode = (progress < 0.5f) ? dimMode : nextDimMode;

        if (activeMode == 0) {
            // 3Dレールモード：左右=X軸、上下=Y軸 (Z軸は進行スクロールで固定)
            x += dx * speed;
            y += dy * speed;

            // 移動範囲制限
            if (x < -60.0f) x = -60.0f;
            if (x > 60.0f) x = 60.0f;
            if (y < -35.0f) y = -35.0f;
            if (y > 35.0f) y = 35.0f;
        }
        else if (activeMode == 1) {
            // 2D縦スクロールモード：左右=X軸、上下=Z軸 (Z=60が中心、Y軸は0.0f固定)
            x += dx * speed;
            z += dy * speed; // 上下キーでZを動かす！

            // 移動範囲制限 (カメラ視野に合わせる)
            if (x < -50.0f) x = -50.0f;
            if (x > 50.0f) x = 50.0f;
            if (z < 25.0f) z = 25.0f;
            if (z > 95.0f) z = 95.0f;
        }
        else if (activeMode == 2) {
            // 2D横スクロールモード：左右=Z軸 (Z=60中心)、上下=Y軸 (X軸は0.0f固定)
            z += dx * speed; // 左右キーでZを動かす！
            y += dy * speed; // 上下キーでYを動かす！

            // 移動範囲制限
            if (z < 25.0f) z = 25.0f;
            if (z > 95.0f) z = 95.0f;
            if (y < -35.0f) y = -35.0f;
            if (y > 35.0f) y = 35.0f;
        }

        // ショット発射 (Zキー)
        if (InputService::IsKeyPressed('Z') && shootCooldown == 0) {
            Fire(bullets, dimMode, nextDimMode, progress);
            shootCooldown = 6;
        }
    }

private:
    void Fire(std::vector<Bullet>& bullets, int dimMode, int nextDimMode, float progress) {
        int fired = 0;
        for (auto& b : bullets) {
            if (!b.active) {
                b.active = true;
                b.isEnemyBullet = false;
                b.size = 0.6f;
                b.color[0] = 0.1f; b.color[1] = 0.9f; b.color[2] = 0.1f; b.color[3] = 1.0f;

                // 旧モードでの発射位置と初速
                float sx1, sy1, sz1, vx1, vy1, vz1;
                GetFireParams(dimMode, fired, sx1, sy1, sz1, vx1, vy1, vz1);

                // 新モードでの発射位置と初速
                float sx2, sy2, sz2, vx2, vy2, vz2;
                GetFireParams(nextDimMode, fired, sx2, sy2, sz2, vx2, vy2, vz2);

                // 移行度 progress で線形補間
                b.x = sx1 + (sx2 - sx1) * progress;
                b.y = sy1 + (sy2 - sy1) * progress;
                b.z = sz1 + (sz2 - sz1) * progress;
                b.vx = vx1 + (vx2 - vx1) * progress;
                b.vy = vy1 + (vy2 - vy1) * progress;
                b.vz = vz1 + (vz2 - vz1) * progress;

                fired++;
                if (fired >= 2) break;
            }
        }
    }

    void GetFireParams(int mode, int fired, float& sx, float& sy, float& sz, float& vx, float& vy, float& vz) {
        if (mode == 0) {
            // 3Dレールモード：奥方向 (Zプラス) へレーザー発射
            sx = (fired == 0) ? x - 6.0f : x + 6.0f;
            sy = y;
            sz = z + 5.0f;
            vx = (fired == 0) ? -0.1f : 0.1f;
            vy = 0.0f;
            vz = 8.5f;
        }
        else if (mode == 1) {
            // 2D縦スクロールモード：上方向 (Zプラス) へレーザー発射
            sx = (fired == 0) ? x - 4.0f : x + 4.0f;
            sy = y;
            sz = z + 2.0f;
            vx = 0.0f;
            vy = 0.0f;
            vz = 3.5f; // Zプラス方向へ飛ぶ
        }
        else {
            // 2D横スクロールモード：右方向 (Zプラス) へレーザー発射
            sx = x;
            sy = (fired == 0) ? y - 4.0f : y + 4.0f;
            sz = z + 2.0f;
            vx = 0.0f;
            vy = 0.0f;
            vz = 3.5f; // Zプラス方向へ飛ぶ
        }
    }
};
