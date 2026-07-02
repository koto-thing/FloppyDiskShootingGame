#pragma once

/**
 * @brief シューティングゲームの3D弾クラス (スターフォックス風)
 */
class Bullet {
public:
    float x;
    float y;
    float z;      // 3D奥座標
    float vx;
    float vy;
    float vz;     // Z方向への速度
    float size;   // 3Dモデルスケール
    float color[4];
    bool isEnemyBullet;
    bool active;

    Bullet() : x(0), y(0), z(0), vx(0), vy(0), vz(0), size(1.0f), isEnemyBullet(false), active(false) {
        color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = 1.0f;
    }

    void Update() {
        if (!active) return;
        x += vx;
        y += vy;
        z += vz;

        // あまりに奥に行きすぎたか、手前（カメラの後ろ）に抜けたら非アクティブ化
        if (z < -10.0f || z > 800.0f) {
            active = false;
        }
        // X, Y の拡散限界
        if (x < -300.0f || x > 300.0f || y < -200.0f || y > 200.0f) {
            active = false;
        }
    }
};
