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

    void Update(int dimMode, int nextDimMode, float progress) {
        if (!active) return;
        x += vx;
        y += vy;
        z += vz;

        // 境界判定の動的補間
        float limitX1 = (dimMode == 0) ? 300.0f : 80.0f;
        float limitX2 = (nextDimMode == 0) ? 300.0f : 80.0f;
        float limitX = limitX1 + (limitX2 - limitX1) * progress;

        float limitY1 = (dimMode == 0) ? 200.0f : 60.0f;
        float limitY2 = (nextDimMode == 0) ? 200.0f : 60.0f;
        float limitY = limitY1 + (limitY2 - limitY1) * progress;

        if (x < -limitX || x > limitX || y < -limitY || y > limitY) {
            active = false;
        }

        // Z消滅判定 (3Dモード、または 2D横モードの場合に有効)
        if (dimMode == 0 || nextDimMode == 0) {
            if (z < -15.0f || z > 850.0f) {
                active = false;
            }
        }
        else if (dimMode == 2 || nextDimMode == 2) {
            // 2D横スクロール時の左右消滅境界
            if (z < 10.0f || z > 110.0f) {
                active = false;
            }
        }
    }
};
