#pragma once
#include <vector>
#include <cmath>
#include "Bullet.h"

/**
 * @brief 敵機クラス (シームレス2D/3D対応)
 */
class Enemy {
public:
    float x;
    float y;
    float z;            // 3D座標
    float vx;
    float vy;
    float vz;           // 速度
    int hp;
    int maxHp;
    float size;         // 衝突判定半径
    float scaleX, scaleY, scaleZ; // 描画サイズ
    float color[4];
    bool active;
    int patternTimer;
    int type;           // 0: 雑魚A, 1: 雑魚B, 2: ボス
    float rotAngle;     // 自転アニメーション用

    Enemy() 
        : x(0), y(0), z(0), vx(0), vy(0), vz(0), hp(1), maxHp(1), size(4.0f),
          scaleX(5.0f), scaleY(5.0f), scaleZ(5.0f), active(false), patternTimer(0), type(0), rotAngle(0.0f) {
        color[0] = 0.8f; color[1] = 0.2f; color[2] = 0.2f; color[3] = 1.0f;
    }

    void Spawn(float startX, float startY, float startZ, int enemyType, int dimensionMode) {
        x = startX;
        y = startY;
        z = startZ;
        type = enemyType;
        active = true;
        patternTimer = 0;
        rotAngle = 0.0f;

        if (dimensionMode == 0) {
            // 3Dモードの初期化
            if (type == 0) {
                hp = maxHp = 3;
                vx = 0.0f; vy = 0.0f; vz = -3.2f; // 手前に突進
                size = 4.0f;
                scaleX = scaleY = scaleZ = 5.0f;
                color[0] = 0.9f; color[1] = 0.4f; color[2] = 0.1f; color[3] = 1.0f;
            } else if (type == 1) {
                hp = maxHp = 6;
                vx = 0.4f; vy = 0.2f; vz = -2.0f; // 少しゆっくり迫る
                size = 5.0f;
                scaleX = scaleY = scaleZ = 6.0f;
                color[0] = 0.7f; color[1] = 0.2f; color[2] = 0.9f; color[3] = 1.0f;
            } else if (type == 2) {
                hp = maxHp = 300;
                vx = 0.8f; vy = 0.4f; vz = 0.0f; // 奥に滞在
                size = 35.0f;
                scaleX = 40.0f; scaleY = 12.0f; scaleZ = 20.0f;
                color[0] = 0.3f; color[1] = 0.5f; color[2] = 0.8f; color[3] = 1.0f;
            }
        } else {
            // 2Dモードの初期化
            if (type == 0) {
                hp = maxHp = 2;
                size = 4.0f;
                scaleX = scaleY = scaleZ = 4.5f;
                color[0] = 0.9f; color[1] = 0.4f; color[2] = 0.1f; color[3] = 1.0f;
                // 2D縦・横いずれも、進行方向は Zマイナス (画面下・左) 方向
                vx = 0.0f; vy = 0.0f; vz = -1.2f;
            } else if (type == 1) {
                hp = maxHp = 4;
                size = 4.5f;
                scaleX = scaleY = scaleZ = 5.0f;
                color[0] = 0.7f; color[1] = 0.2f; color[2] = 0.9f; color[3] = 1.0f;
                if (dimensionMode == 1) {
                    vx = 0.3f; vy = 0.0f; vz = -0.8f; // 2D縦: 左右(X軸)に揺れながらZマイナスへ
                } else {
                    vx = 0.0f; vy = 0.3f; vz = -0.8f; // 2D横: 上下(Y軸)に揺れながらZマイナスへ
                }
            } else if (type == 2) {
                hp = maxHp = 200;
                size = 20.0f;
                scaleX = 22.0f; scaleY = 22.0f; scaleZ = 5.0f;
                color[0] = 0.3f; color[1] = 0.5f; color[2] = 0.8f; color[3] = 1.0f;
                if (dimensionMode == 1) {
                    vx = 0.6f; vy = 0.0f; vz = 0.0f; // 2D縦ボス: 左右往復 (X軸)
                } else {
                    vx = 0.0f; vy = 0.6f; vz = 0.0f; // 2D横ボス: 上下往復 (Y軸)
                }
            }
        }
    }

    void Update(std::vector<Bullet>& bullets, float playerX, float playerY, float playerZ, int dimMode, int nextDimMode, float progress) {
        if (!active) return;

        patternTimer++;
        rotAngle += 0.03f;

        // 1. 移動量の算出
        float dx1 = 0.0f, dy1 = 0.0f, dz1 = 0.0f;
        GetMovementDelta(dimMode, playerX, playerY, playerZ, dx1, dy1, dz1);

        float dx2 = 0.0f, dy2 = 0.0f, dz2 = 0.0f;
        GetMovementDelta(nextDimMode, playerX, playerY, playerZ, dx2, dy2, dz2);

        // 線形補間
        x += dx1 + (dx2 - dx1) * progress;
        y += dy1 + (dy2 - dy1) * progress;
        z += dz1 + (dz2 - dz1) * progress;

        // 2. 特殊補間 (3D -> 2D 移行中、敵のZ座標を滑らかに 60.0f 平面へとシフトさせる。ただし2D縦横移行中は不要)
        int nextActiveMode = (progress < 0.5f) ? dimMode : nextDimMode;
        if (dimMode == 0 && nextActiveMode == 1) {
            float targetZ = 60.0f;
            z = z + (targetZ - z) * progress * 0.05f;
        }

        // 3. 画面外消滅判定
        CheckActiveBounds(dimMode, nextDimMode, progress);

        // 4. 弾幕発射
        int activeShootMode = (progress < 0.5f) ? dimMode : nextDimMode;
        FireDanmaku(bullets, playerX, playerY, playerZ, activeShootMode);
    }

private:
    void GetMovementDelta(int mode, float px, float py, float pz, float& outDx, float& outDy, float& outDz) {
        if (mode == 0) {
            // 3Dレールモード
            if (type == 0) {
                outDz = vz;
                outDx = sin(patternTimer * 0.1f) * 0.8f;
                outDy = cos(patternTimer * 0.1f) * 0.5f;
            } else if (type == 1) {
                outDz = vz;
                outDx = (px - x) * 0.015f + vx;
                outDy = (py - y) * 0.015f + vy;
            } else if (type == 2) {
                outDx = vx;
                outDy = vy;
                float targetZ = 320.0f + sin(patternTimer * 0.02f) * 30.0f;
                outDz = (targetZ - z) * 0.1f;
            }
        }
        else if (mode == 1) {
            // 2D縦スクロールモード (Zマイナス方向へスクロール)
            outDy = 0.0f;
            if (type == 0) {
                outDz = vz; // -1.2f
                outDx = sin(patternTimer * 0.1f) * 0.6f;
            } else if (type == 1) {
                outDz = vz; // -0.8f
                outDx = (px - x) * 0.015f;
            } else if (type == 2) {
                outDx = vx; // 左右(X軸)往復
                outDz = 0.0f;
            }
        }
        else {
            // 2D横スクロールモード (Zマイナス方向へスクロール)
            outDx = 0.0f;
            if (type == 0) {
                outDz = vz; // -1.2f
                outDy = sin(patternTimer * 0.1f) * 0.6f;
            } else if (type == 1) {
                outDz = vz; // -0.8f
                outDy = (py - y) * 0.015f;
            } else if (type == 2) {
                outDz = 0.0f;
                outDy = vy; // 上下(Y軸)往復
            }
        }
    }

    void CheckActiveBounds(int dimMode, int nextDimMode, float progress) {
        // 3D用のZ手前限界
        if (dimMode == 0 || nextDimMode == 0) {
            if (z < -15.0f) active = false;
        }

        // 2D縦および2D横での消滅判定 (画面下端・左端はともに Z < 15.0f)
        if (dimMode == 1 || nextDimMode == 1 || dimMode == 2 || nextDimMode == 2) {
            if (z < 15.0f) active = false;
        }
    }

    void FireDanmaku(std::vector<Bullet>& bullets, float px, float py, float pz, int activeMode) {
        if (activeMode == 0) {
            // 3D弾幕
            if (type == 0 && patternTimer % 80 == 0) {
                TargetShot(bullets, px, py, pz, -4.5f, activeMode);
            }
            else if (type == 1 && (patternTimer % 100 == 0 || patternTimer % 100 == 12)) {
                TargetShot(bullets, px, py, pz, -5.5f, activeMode);
            }
            else if (type == 2) {
                int mode = (patternTimer / 250) % 3;
                if (mode == 0 && patternTimer % 45 == 0) {
                    for (int i = -1; i <= 1; ++i) {
                        CreateBullet(bullets, x + i * 15.0f, y, z, i * 0.8f, -0.2f, -5.0f, 1.6f, 1.0f, 0.6f, 0.1f);
                    }
                }
                else if (mode == 1 && patternTimer % 25 == 0) {
                    TargetShot(bullets, px, py, pz, -8.0f, activeMode);
                }
                else if (mode == 2 && patternTimer % 15 == 0) {
                    float rx = ((float)(rand() % 100) - 50.0f) * 1.5f;
                    float ry = ((float)(rand() % 100) - 50.0f) * 0.8f;
                    CreateBullet(bullets, x + rx, y + ry, z, 0.0f, 0.0f, -4.0f, 4.0f, 0.6f, 0.6f, 0.7f);
                }
            }
        }
        else {
            // 2D弾幕 (Zマイナス方向へ発射)
            if (type == 0 && patternTimer % 90 == 0) {
                TargetShot(bullets, px, py, pz, -2.0f, activeMode);
            }
            else if (type == 1 && patternTimer % 110 == 0) {
                TargetShot(bullets, px, py, pz, -2.5f, activeMode);
            }
            else if (type == 2) {
                int mode = (patternTimer / 200) % 2;
                if (mode == 0 && patternTimer % 40 == 0) {
                    if (activeMode == 1) {
                        // 2D縦: 下向き扇状5方向弾 (X-Z平面)
                        for (int i = -2; i <= 2; ++i) {
                            CreateBullet(bullets, x, y, z - 5.0f, i * 0.5f, 0.0f, -1.8f, 1.4f, 1.0f, 0.2f, 0.2f);
                        }
                    } else {
                        // 2D横: 左向き扇状5方向弾 (Z-Y平面)
                        for (int i = -2; i <= 2; ++i) {
                            CreateBullet(bullets, x, y, z - 5.0f, 0.0f, i * 0.5f, -1.8f, 1.4f, 1.0f, 0.2f, 0.2f);
                        }
                    }
                }
                else if (mode == 1 && patternTimer % 30 == 0) {
                    TargetShot(bullets, px, py, pz, -3.0f, activeMode);
                }
            }
        }
    }

    void TargetShot(std::vector<Bullet>& bullets, float px, float py, float pz, float speed, int activeMode) {
        if (activeMode == 0) {
            float dx = px - x;
            float dy = py - y;
            float dz = pz - z;
            float dist = sqrt(dx*dx + dy*dy + dz*dz);
            if (dist > 0.0f) {
                float bulletVx = (dx / dist) * abs(speed);
                float bulletVy = (dy / dist) * abs(speed);
                float bulletVz = speed;
                CreateBullet(bullets, x, y, z, bulletVx, bulletVy, bulletVz, 1.4f, 1.0f, 0.2f, 0.2f);
            }
        } else {
            if (activeMode == 1) {
                // 2D縦: X と Z (左右はX、上下はZ)
                float dx = px - x;
                float dz = pz - z;
                float dist = sqrt(dx*dx + dz*dz);
                if (dist > 0.0f) {
                    float bulletVx = (dx / dist) * abs(speed);
                    float bulletVz = (dz / dist) * abs(speed);
                    CreateBullet(bullets, x, y, z, bulletVx, 0.0f, bulletVz, 1.4f, 1.0f, 0.2f, 0.2f);
                }
            } else {
                // 2D横: Z と Y (左右はZ、上下はY)
                float dz = pz - z;
                float dy = py - y;
                float dist = sqrt(dz*dz + dy*dy);
                if (dist > 0.0f) {
                    float bulletVz = (dz / dist) * abs(speed);
                    float bulletVy = (dy / dist) * abs(speed);
                    CreateBullet(bullets, x, y, z, 0.0f, bulletVy, bulletVz, 1.4f, 1.0f, 0.2f, 0.2f);
                }
            }
        }
    }

    void CreateBullet(std::vector<Bullet>& bullets, float sx, float sy, float sz, float bulVx, float bulVy, float bulVz, float bulletSize, float r, float g, float b) {
        for (auto& bul : bullets) {
            if (!bul.active) {
                bul.active = true;
                bul.isEnemyBullet = true;
                bul.x = sx;
                bul.y = sy;
                bul.z = sz;
                bul.vx = bulVx;
                bul.vy = bulVy;
                bul.vz = bulVz;
                bul.size = bulletSize;
                bul.color[0] = r; bul.color[1] = g; bul.color[2] = b; bul.color[3] = 1.0f;
                break;
            }
        }
    }
};
