#pragma once
#include <vector>
#include <cmath>
#include "Bullet.h"

/**
 * @brief 敵機クラス (スターフォックス風 3D)
 */
class Enemy {
public:
    float x;
    float y;
    float z;            // 3D奥座標
    float vx;
    float vy;
    float vz;           // Z方向の移動速度
    int hp;
    int maxHp;
    float size;         // 3D衝突判定球の半径
    float scaleX, scaleY, scaleZ; // 3D描画サイズ
    float color[4];
    bool active;
    int patternTimer;
    int type;           // 0: 雑魚A (サイン波突撃), 1: 雑魚B (旋回狙撃), 2: ボス (超巨大3D戦艦)
    float rotAngle;     // 自転アニメーション用

    Enemy() 
        : x(0), y(0), z(0), vx(0), vy(0), vz(0), hp(1), maxHp(1), size(4.0f),
          scaleX(5.0f), scaleY(5.0f), scaleZ(5.0f), active(false), patternTimer(0), type(0), rotAngle(0.0f) {
        color[0] = 0.8f; color[1] = 0.2f; color[2] = 0.2f; color[3] = 1.0f;
    }

    void Spawn(float startX, float startY, float startZ, int enemyType) {
        x = startX;
        y = startY;
        z = startZ;
        type = enemyType;
        active = true;
        patternTimer = 0;
        rotAngle = 0.0f;

        if (type == 0) {
            hp = maxHp = 3;
            vx = 0.0f;
            vy = 0.0f;
            vz = -3.2f; // 手前に突進
            size = 4.0f;
            scaleX = scaleY = scaleZ = 5.0f;
            color[0] = 0.9f; color[1] = 0.4f; color[2] = 0.1f; color[3] = 1.0f; // オレンジピラミッド
        } else if (type == 1) {
            hp = maxHp = 6;
            vx = 0.4f;
            vy = 0.2f;
            vz = -2.0f; // 少しゆっくり迫る
            size = 5.0f;
            scaleX = scaleY = scaleZ = 6.0f;
            color[0] = 0.7f; color[1] = 0.2f; color[2] = 0.9f; color[3] = 1.0f; // 紫
        } else if (type == 2) {
            // ボス！超巨大3D戦艦
            hp = maxHp = 300;
            vx = 0.8f;
            vy = 0.4f;
            vz = 0.0f; // 奥に滞在
            size = 35.0f;
            scaleX = 40.0f;
            scaleY = 12.0f;
            scaleZ = 20.0f;
            color[0] = 0.3f; color[1] = 0.5f; color[2] = 0.8f; color[3] = 1.0f; // 青白い巨大戦艦
        }
    }

    void Update(std::vector<Bullet>& bullets, float playerX, float playerY, float playerZ) {
        if (!active) return;

        patternTimer++;
        rotAngle += 0.03f;

        // 移動パターン
        if (type == 0) {
            // サイン波でらせんを描きながら手前に進む
            z += vz;
            x = x + sin(patternTimer * 0.1f) * 0.8f;
            y = y + cos(patternTimer * 0.1f) * 0.5f;

            if (z < -10.0f) active = false; // カメラの後ろに抜けたら消滅
        }
        else if (type == 1) {
            // プレイヤーをゆるやかに追跡しつつ迫る
            z += vz;
            x += (playerX - x) * 0.015f + vx;
            y += (playerY - y) * 0.015f + vy;

            if (z < -10.0f) active = false;
        }
        else if (type == 2) {
            // ボスは奥(z = 320.0f 付近)で左右上下に遊泳移動
            x += vx;
            y += vy;
            z = 320.0f + sin(patternTimer * 0.02f) * 30.0f;

            if (x < -80.0f || x > 80.0f) vx = -vx;
            if (y < -20.0f || y > 20.0f) vy = -vy;
        }

        // 弾幕発射
        FireDanmaku3D(bullets, playerX, playerY, playerZ);
    }

private:
    void FireDanmaku3D(std::vector<Bullet>& bullets, float px, float py, float pz) {
        if (type == 0) {
            // 雑魚A: 80フレームごとにプレイヤーを正確に狙撃 (Zを狙う)
            if (patternTimer % 80 == 0) {
                TargetShot3D(bullets, px, py, pz, -4.5f);
            }
        }
        else if (type == 1) {
            // 雑魚B: 100フレームごとに2発連続で狙撃
            if (patternTimer % 100 == 0 || patternTimer % 100 == 12) {
                TargetShot3D(bullets, px, py, pz, -5.5f);
            }
        }
        else if (type == 2) {
            // ボス弾幕形態
            int mode = (patternTimer / 250) % 3;

            if (mode == 0) {
                // 弾幕1: 3方向扇状3D弾 (50フレームごと)
                if (patternTimer % 45 == 0) {
                    for (int i = -1; i <= 1; ++i) {
                        CreateBullet3D(bullets, x + i * 15.0f, y, z, i * 0.8f, -0.2f, -5.0f, 1.6f, 1.0f, 0.6f, 0.1f);
                    }
                }
            }
            else if (mode == 1) {
                // 弾幕2: プレイヤーへの高速追従弾 (20フレームごと、Z方向に高速)
                if (patternTimer % 25 == 0) {
                    TargetShot3D(bullets, px, py, pz, -8.0f);
                }
            }
            else if (mode == 2) {
                // 弾幕3: 大量の小惑星（Cube型）を奥からバラ撒く (15フレームごと)
                if (patternTimer % 15 == 0) {
                    float rx = ((float)(rand() % 100) - 50.0f) * 1.5f;
                    float ry = ((float)(rand() % 100) - 50.0f) * 0.8f;
                    CreateBullet3D(bullets, x + rx, y + ry, z, 0.0f, 0.0f, -4.0f, 4.0f, 0.6f, 0.6f, 0.7f); // グレーの隕石
                }
            }
        }
    }

    void TargetShot3D(std::vector<Bullet>& bullets, float px, float py, float pz, float speed) {
        float dx = px - x;
        float dy = py - y;
        float dz = pz - z;
        float dist = sqrt(dx*dx + dy*dy + dz*dz);
        if (dist > 0.0f) {
            // speedがマイナス（プレイヤー方向）
            float vx = (dx / dist) * abs(speed);
            float vy = (dy / dist) * abs(speed);
            float vz = speed; // Z方向は手前へ直行
            CreateBullet3D(bullets, x, y, z, vx, vy, vz, 1.4f, 1.0f, 0.2f, 0.2f);
        }
    }

    void CreateBullet3D(std::vector<Bullet>& bullets, float sx, float sy, float sz, float vx, float vy, float vz, float bulletSize, float r, float g, float b) {
        for (auto& bul : bullets) {
            if (!bul.active) {
                bul.active = true;
                bul.isEnemyBullet = true;
                bul.x = sx;
                bul.y = sy;
                bul.z = sz;
                bul.vx = vx;
                bul.vy = vy;
                bul.vz = vz;
                bul.size = bulletSize;
                bul.color[0] = r; bul.color[1] = g; bul.color[2] = b; bul.color[3] = 1.0f; // 敵弾アルファは1.0f
                break;
            }
        }
    }
};
