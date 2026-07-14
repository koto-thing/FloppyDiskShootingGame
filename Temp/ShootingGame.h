#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "../Infrastructure/ExternalServices/D3D12RenderingService.h"

// 256バイトにアライメントされた定数バッファ構造体 (HLSL側の TransformBuffer と完全一致させる)
struct TransformBufferData {
    DirectX::XMFLOAT4X4 u_wvpMatrix; // 64バイト
    DirectX::XMFLOAT4 u_Color;       // 16バイト
    float u_time;                    // 4バイト
    float u_shapeType;               // 4バイト
    float u_rotAngle;                // 4バイト (自転回転角)
    float u_pad[41];                 // 164バイト (計256バイト)
};

// 3D空間のアイテムクラス
struct Item3D {
    float x, y, z;
    float vx, vy, vz;
    int type; // 0: Shield (P), 1: Score
    bool active;
};

// 3D障害物 (ゲートやビル)
struct Obstacle3D {
    float x, y, z;
    float w, h, d;
    float color[4];
    int shapeType; // 1: Cube, 0: Plate
    bool active;
};

/**
 * @brief 初代スターフォックス風 3D レール / 2D切り替えシューティングゲーム管理クラス
 */
class ShootingGame {
public:
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Item3D> items;
    std::vector<Obstacle3D> obstacles;

    int spawnTimer;
    int gameStatus; // 0: Play, 1: GameOver, 2: Clear
    float scrollDistance; // 総スクロール距離
    float scrollSpeed;
    int enemyKillCount;

    // 次元モード管理
    int dimensionMode;          // 0: 3Dレール, 1: 2D縦スクロール, 2: 2D横スクロール
    int nextDimensionMode;      // 遷移先次元モード
    int transitionTimer;        // 次元シフト移行タイマー
    const int maxTransitionFrames = 90; // トランジション時間 (1.5秒)
    float transitionProgress;   // 0.0f (旧モード) ~ 1.0f (新モード) への移行度
    
    // カメラ行列 (DirectXMath)
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projMatrix;

    // 星空背景用データ
    float backgroundPos[3];

    ShootingGame() {
        Reset();
    }

    void Reset() {
        player.Reset();
        enemies.clear();
        bullets.clear();
        items.clear();
        obstacles.clear();

        enemies.resize(12);    // 最大同時出現敵
        bullets.resize(500);   // 最大同時出現弾
        items.resize(40);      // 最大同時出現アイテム
        obstacles.resize(25);  // 同時出現障害物

        spawnTimer = 0;
        gameStatus = 0;
        scrollDistance = 0.0f;
        scrollSpeed = 1.6f;
        enemyKillCount = 0;

        dimensionMode = 0;
        nextDimensionMode = 0;
        transitionTimer = 0;
        transitionProgress = 0.0f;

        backgroundPos[0] = 0.0f;
        backgroundPos[1] = 0.0f;
        backgroundPos[2] = 0.0f;

        // 初期障害物 (3Dモード用)
        SpawnObstacles();
    }

    void Update() {
        if (gameStatus != 0) {
            // ゲームオーバーかクリア時は、Rキーでリセット
            if (InputService::IsKeyPressed('R')) {
                Reset();
            }
            return;
        }

        spawnTimer++;
        scrollDistance += scrollSpeed;

        // --- 次元移行 (Dimension Shift) 管理 ---
        if (transitionTimer > 0) {
            transitionTimer--;
            transitionProgress = 1.0f - ((float)transitionTimer / maxTransitionFrames);
            
            if (transitionTimer == 0) {
                dimensionMode = nextDimensionMode;
                transitionProgress = 0.0f;
            }
        } else {
            transitionProgress = 0.0f;
            // スクロール距離に応じて次元モードの切り替えを判定 (特定のポイント)
            if (dimensionMode == 0 && scrollDistance >= 350.0f && scrollDistance < 360.0f) {
                // 3Dレール -> 2D縦スクロール
                nextDimensionMode = 1;
                transitionTimer = maxTransitionFrames;
                scrollSpeed = 1.0f;
            }
            else if (dimensionMode == 1 && scrollDistance >= 750.0f && scrollDistance < 760.0f) {
                // 2D縦スクロール -> 2D横スクロール
                nextDimensionMode = 2;
                transitionTimer = maxTransitionFrames;
                scrollSpeed = 1.0f;
            }
            else if (dimensionMode == 2 && scrollDistance >= 1150.0f && scrollDistance < 1160.0f) {
                // 2D横スクロール -> 3Dレール (ボス戦へ)
                nextDimensionMode = 0;
                transitionTimer = maxTransitionFrames;
                scrollSpeed = 1.6f;
            }
        }

        // プレイヤー更新 (入力移動軸の処理は Player 側で優勢な activeMode に従って決定される)
        player.Update(bullets, dimensionMode, nextDimensionMode, transitionProgress);

        // --- プレイヤー固定軸（次元制約）の滑らかな吸い寄せと強制適用 ---
        if (transitionTimer > 0) {
            ApplyDimensionConstraints(dimensionMode, player, 1.0f - transitionProgress);
            ApplyDimensionConstraints(nextDimensionMode, player, transitionProgress);
        } else {
            ApplyDimensionConstraints(dimensionMode, player, 1.0f);
        }

        // --- シームレスなカメラパラメータの補間処理 (超望遠ズームモーフィング) ---
        float fov1, posX1, posY1, posZ1, targetX1, targetY1, targetZ1, upX1, upY1, upZ1;
        GetCameraParams(dimensionMode, fov1, posX1, posY1, posZ1, targetX1, targetY1, targetZ1, upX1, upY1, upZ1);

        float fov2, posX2, posY2, posZ2, targetX2, targetY2, targetZ2, upX2, upY2, upZ2;
        GetCameraParams(nextDimensionMode, fov2, posX2, posY2, posZ2, targetX2, targetY2, targetZ2, upX2, upY2, upZ2);

        // トランジション進行度でブレンド
        float fov = fov1 + (fov2 - fov1) * transitionProgress;
        float px = posX1 + (posX2 - posX1) * transitionProgress;
        float py = posY1 + (posY2 - posY1) * transitionProgress;
        float pz = posZ1 + (posZ2 - posZ1) * transitionProgress;
        float tx = targetX1 + (targetX2 - targetX1) * transitionProgress;
        float ty = targetY1 + (targetY2 - targetY1) * transitionProgress;
        float tz = targetZ1 + (targetZ2 - targetZ1) * transitionProgress;
        float ux = upX1 + (upX2 - upX1) * transitionProgress;
        float uy = upY1 + (upY2 - upY1) * transitionProgress;
        float uz = upZ1 + (upZ2 - upZ1) * transitionProgress;

        // ビュー行列の構築 (LookAt)
        DirectX::XMVECTOR camPos = DirectX::XMVectorSet(px, py, pz, 0.0f);
        DirectX::XMVECTOR camTarget = DirectX::XMVectorSet(tx, ty, tz, 0.0f);
        DirectX::XMVECTOR camUp = DirectX::XMVectorSet(ux, uy, uz, 0.0f);
        m_viewMatrix = DirectX::XMMatrixLookAtLH(camPos, camTarget, camUp);

        // プロジェクション行列の構築 (Fovブレンドによる望遠効果)
        float aspect = 800.0f / 600.0f;
        m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov), aspect, 0.5f, 1500.0f);

        // --- 星空背景の位置をスクロール ---
        float bgX1 = 0, bgY1 = 0;
        GetBgScrollOffset(dimensionMode, bgX1, bgY1);
        float bgX2 = 0, bgY2 = 0;
        GetBgScrollOffset(nextDimensionMode, bgX2, bgY2);

        backgroundPos[0] = bgX1 + (bgX2 - bgX1) * transitionProgress;
        backgroundPos[1] = bgY1 + (bgY2 - bgY1) * transitionProgress;

        // 障害物のスクロールと更新 (3Dレールモード移行中、または3D中のみ有効)
        if (dimensionMode == 0 && nextDimensionMode == 0) {
            UpdateObstacles();
        } else if (transitionTimer > 0 && nextDimensionMode == 0) {
            UpdateObstacles();
        } else {
            for (auto& obs : obstacles) obs.active = false;
        }

        // 敵の出現テーブル (モードごとにスポーン位置を切り替え)
        SpawnEnemies();

        // 敵の更新
        for (auto& e : enemies) {
            if (e.active) {
                e.Update(bullets, player.x, player.y, player.z, dimensionMode, nextDimensionMode, transitionProgress);
            }
        }

        // 弾の更新
        for (auto& b : bullets) {
            b.Update(dimensionMode, nextDimensionMode, transitionProgress);
        }

        // アイテムの更新 (モードごとにスクロール方向をブレンド)
        UpdateItems(transitionProgress);

        // 当たり判定 (3D衝突判定)
        CheckCollisions();

        // ゲームオーバー判定
        if (player.lives < 0) {
            gameStatus = 1; // GameOver
        }
    }

    void Render(D3D12RenderingService& renderer) {
        int drawCount = 0;

        // A. 宇宙空間の星空背景描画 (PSO: 1: Background)
        // ビュー・プロジェクションを適用せず、NDC空間全体 (Z=0.99) に直接描くことでカメラ回転に追従させる
        renderer.SetPipelineState(1);
        float bgCol[4] = { backgroundPos[0], backgroundPos[1], 0.0f, 1.0f };
        DrawObjectUI(renderer, 0.0f, 0.0f, 0.99f, 2.0f, 2.0f, bgCol, drawCount++);

        // B. 以降のすべての3Dオブジェクトは通常スプライト/3Dポリゴン (PSO: 0: Object)
        renderer.SetPipelineState(0);

        // 1. 地面グリッドの描画 (次元モードに応じてクロスフェード)
        float gridCol[4] = { 0.15f, 0.75f, 0.25f, 1.0f }; // レトロネオングリーン
        float floorY = -18.0f; // 地面の高さ
        
        float alpha3D = 0.0f;
        float alpha2DV = 0.0f;
        float alpha2DH = 0.0f;

        if (transitionTimer > 0) {
            SetGridAlpha(dimensionMode, 1.0f - transitionProgress, alpha3D, alpha2DV, alpha2DH);
            SetGridAlpha(nextDimensionMode, transitionProgress, alpha3D, alpha2DV, alpha2DH);
        } else {
            SetGridAlpha(dimensionMode, 1.0f, alpha3D, alpha2DV, alpha2DH);
        }

        // 3Dレールモードのグリッド (XZ平面)
        if (alpha3D > 0.01f) {
            float col[4] = { gridCol[0], gridCol[1], gridCol[2], alpha3D };
            float zOffset = -fmod(scrollDistance, 45.0f);
            for (float xG = -120.0f; xG <= 120.0f; xG += 30.0f) {
                DrawObject3D(renderer, xG, floorY, 200.0f, 0.3f, 1.0f, 400.0f, col, drawCount++, 0, 4);
            }
            for (float zG = 0.0f; zG <= 400.0f; zG += 45.0f) {
                float curZ = zG + zOffset;
                if (curZ > 0.0f) {
                    DrawObject3D(renderer, 0.0f, floorY, curZ, 240.0f, 1.0f, 0.3f, col, drawCount++, 0, 4);
                }
            }
        }
        
        // 2D縦スクロールグリッド (Z=60のXZ平面に正対配置)
        if (alpha2DV > 0.01f) {
            float col[4] = { gridCol[0], gridCol[1], gridCol[2], alpha2DV };
            float zOffset = -fmod(scrollDistance * 0.8f, 20.0f);
            
            // 縦線：X軸上に並べ、Z軸方向に長いラインを描く
            for (float xG = -60.0f; xG <= 60.0f; xG += 15.0f) {
                DrawObject3D(renderer, xG, 0.0f, 60.0f, 0.2f, 100.0f, 1.0f, col, drawCount++, 3, 4);
            }
            // 横線：Z軸上のスクロール、X軸方向に広いラインを描く
            for (float zG = 10.0f; zG <= 110.0f; zG += 20.0f) {
                float curZ = zG + zOffset;
                if (curZ >= 15.0f && curZ <= 105.0f) {
                    DrawObject3D(renderer, 0.0f, 0.0f, curZ, 120.0f, 0.2f, 1.0f, col, drawCount++, 3, 4);
                }
            }
        }
        
        // 2D横スクロールグリッド (Z=60のYZ平面に正対配置)
        if (alpha2DH > 0.01f) {
            float col[4] = { gridCol[0], gridCol[1], gridCol[2], alpha2DH };
            float zOffset = -fmod(scrollDistance * 0.8f, 20.0f);
            
            // 横線：Y軸上に並べ、Z軸方向に長いラインを描く
            for (float yG = -40.0f; yG <= 40.0f; yG += 10.0f) {
                DrawObject3D(renderer, 0.0f, yG, 60.0f, 140.0f, 0.2f, 1.0f, col, drawCount++, 3, 4);
            }
            // 縦線：Z軸上のスクロール、Y軸方向に高いラインを描く
            for (float zG = 10.0f; zG <= 110.0f; zG += 20.0f) {
                float curZ = zG + zOffset;
                if (curZ >= 15.0f && curZ <= 105.0f) {
                    DrawObject3D(renderer, 0.0f, 0.0f, curZ, 0.2f, 80.0f, 1.0f, col, drawCount++, 3, 4);
                }
            }
        }

        // 2. 3D障害物の描画
        if (dimensionMode == 0 || nextDimensionMode == 0) {
            float obsAlpha = (dimensionMode == 0 && nextDimensionMode == 0) ? 1.0f : 
                             ((nextDimensionMode == 0) ? transitionProgress : 1.0f - transitionProgress);
            
            for (auto& obs : obstacles) {
                if (obs.active) {
                    float col[4] = { obs.color[0], obs.color[1], obs.color[2], obs.color[3] * obsAlpha };
                    DrawObject3D(renderer, obs.x, obs.y, obs.z, obs.w, obs.h, obs.d, col, drawCount++, obs.shapeType, 36);
                    if (drawCount >= 2000) break;
                }
            }
        }

        // 3. 敵機の描画 (自転角度を反映)
        for (auto& e : enemies) {
            if (e.active) {
                if (e.type == 2) {
                    DrawObject3D(renderer, e.x, e.y, e.z, e.scaleX, e.scaleY, e.scaleZ, e.color, drawCount++, 1, 36, e.rotAngle);
                } else {
                    DrawObject3D(renderer, e.x, e.y, e.z, e.scaleX, e.scaleY, e.scaleZ, e.color, drawCount++, 2, 18, e.rotAngle);
                }
                if (drawCount >= 2000) break;
            }
        }

        // 4. 弾の描画
        int currentActiveMode = (transitionTimer > 0) ? ((transitionProgress < 0.5f) ? dimensionMode : nextDimensionMode) : dimensionMode;
        for (auto& b : bullets) {
            if (b.active) {
                if (b.isEnemyBullet) {
                    DrawObject3D(renderer, b.x, b.y, b.z, b.size * 2.0f, b.size * 2.0f, b.size * 2.0f, b.color, drawCount++, 1, 36);
                } else {
                    // レーザーの形状
                    float laserW = 0.5f;
                    float laserH = 0.5f;
                    float laserD = 8.0f;
                    if (currentActiveMode == 1) {
                        laserW = 0.5f; laserH = 0.5f; laserD = 8.0f; // 2D縦も進行方向はZ軸
                    } else if (currentActiveMode == 2) {
                        laserW = 0.5f; laserH = 0.5f; laserD = 8.0f; // 2D横も進行方向はZ軸
                    }
                    DrawObject3D(renderer, b.x, b.y, b.z, laserW, laserH, laserD, b.color, drawCount++, 1, 36);
                }
                if (drawCount >= 2000) break;
            }
        }

        // 5. アイテムの描画 (自転させる)
        for (auto& it : items) {
            if (it.active) {
                float itCol[4] = { 1.0f, 0.8f, 0.0f, 1.0f };
                if (it.type == 0) {
                    itCol[0] = 1.0f; itCol[1] = 0.2f; itCol[2] = 0.2f;
                } else {
                    itCol[0] = 0.2f; itCol[1] = 0.6f; itCol[2] = 1.0f;
                }
                DrawObject3D(renderer, it.x, it.y, it.z, 3.0f, 3.0f, 3.0f, itCol, drawCount++, 1, 36, (float)spawnTimer * 0.04f);
                if (drawCount >= 2000) break;
            }
        }

        // 6. プレイヤーの描画 (主翼ロールは3Dのみ)
        if (player.lives >= 0) {
            bool drawPlayer = true;
            if (player.invincibleFrames > 0 && (player.invincibleFrames % 4 < 2)) {
                drawPlayer = false;
            }

            if (drawPlayer) {
                float pCol[4] = { 0.8f, 0.8f, 0.85f, 1.0f };
                float wingCol[4] = { 0.2f, 0.4f, 0.8f, 1.0f };
                float noseCol[4] = { 0.9f, 0.1f, 0.1f, 1.0f };

                float rollFactor = 0.0f;
                if (dimensionMode == 0 && nextDimensionMode == 0) {
                    rollFactor = player.rollAngle;
                } else if (transitionTimer > 0 && nextDimensionMode == 0) {
                    rollFactor = player.rollAngle * transitionProgress;
                } else if (transitionTimer > 0 && dimensionMode == 0) {
                    rollFactor = player.rollAngle * (1.0f - transitionProgress);
                }

                // A. メインボディ
                DrawObject3D(renderer, player.x, player.y, player.z, 3.0f, 2.5f, 9.0f, pCol, drawCount++, 2, 18);

                // B. 機首ノーズ (進行方向は3D/2D共にZプラス方向)
                DrawObject3D(renderer, player.x, player.y, player.z + 5.0f, 1.2f, 1.0f, 4.0f, noseCol, drawCount++, 2, 18);

                // C. 左主翼 (主翼の傾きは位置をずらすことで表現)
                float leftWingX = player.x - 4.5f;
                float leftWingY = player.y + rollFactor * 4.0f;
                DrawObject3D(renderer, leftWingX, leftWingY, player.z - 1.0f, 6.0f, 0.4f, 3.0f, wingCol, drawCount++, 1, 36);

                // D. 右主翼
                float rightWingX = player.x + 4.5f;
                float rightWingY = player.y - rollFactor * 4.0f;
                DrawObject3D(renderer, rightWingX, rightWingY, player.z - 1.0f, 6.0f, 0.4f, 3.0f, wingCol, drawCount++, 1, 36);
            }
        }

        // 7. 3D照準HUDの描画 (3Dモード中のみ、アルファブレンド出現)
        float hudAlpha = (dimensionMode == 0 && nextDimensionMode == 0) ? 1.0f : 
                         ((nextDimensionMode == 0) ? transitionProgress : 1.0f - transitionProgress);
        if (hudAlpha > 0.01f && player.lives >= 0) {
            renderer.SetPipelineState(2);
            float hudCol[4] = { 0.2f, 0.9f, 0.2f, 0.8f * hudAlpha };
            DrawObject3D(renderer, player.x, player.y, player.z + 65.0f, 8.0f, 8.0f, 1.0f, hudCol, drawCount++, 3, 4);
        }

        // 8. UIの描画 (2Dスプライトとして一番手前に描画)
        renderer.SetPipelineState(0);
        float grayCol[4] = { 0.15f, 0.15f, 0.2f, 1.0f };

        // シールドゲージ
        float shieldRate = (float)player.shield / 100.0f;
        float barCol[4] = { 0.9f, 0.2f, 0.2f, 1.0f };
        if (player.shield > 50) {
            barCol[0] = 0.1f; barCol[1] = 0.9f; barCol[2] = 0.1f;
        } else if (player.shield > 20) {
            barCol[0] = 0.9f; barCol[1] = 0.9f; barCol[2] = 0.1f;
        }
        DrawObjectUI(renderer, -0.6f, -0.85f, 0.1f, 0.3f, 0.03f, grayCol, drawCount++);
        if (shieldRate > 0.0f) {
            DrawObjectUI(renderer, -0.6f - (0.15f * (1.0f - shieldRate)), -0.85f, 0.09f, 0.3f * shieldRate, 0.03f, barCol, drawCount++);
        }

        // 残機表示
        for (int i = 0; i < player.lives; ++i) {
            float shipCol[4] = { 0.8f, 0.8f, 0.9f, 1.0f };
            DrawObjectUI(renderer, -0.9f + i * 0.07f, -0.85f, 0.1f, 0.04f, 0.03f, shipCol, drawCount++);
        }

        // スコア進捗ゲージ
        float killRate = (std::min)((float)enemyKillCount / 30.0f, 1.0f);
        float killWidth = 0.3f * killRate;
        float scoreCol[4] = { 0.2f, 0.6f, 1.0f, 1.0f };
        DrawObjectUI(renderer, 0.6f, -0.85f, 0.1f, 0.3f, 0.03f, grayCol, drawCount++);
        if (killRate > 0.0f) {
            DrawObjectUI(renderer, 0.6f - (0.15f * (1.0f - killRate)), -0.85f, 0.09f, killWidth, 0.03f, scoreCol, drawCount++);
        }

        // ボスHPバー
        for (auto& e : enemies) {
            if (e.active && e.type == 2) {
                float hpRate = (float)e.hp / e.maxHp;
                float hpWidth = 1.4f * hpRate;
                float bossHpCol[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
                DrawObjectUI(renderer, 0.0f, 0.85f, 0.1f, 1.4f, 0.04f, grayCol, drawCount++);
                if (hpRate > 0.0f) {
                    DrawObjectUI(renderer, 0.0f - (0.7f * (1.0f - hpRate)), 0.85f, 0.09f, hpWidth, 0.04f, bossHpCol, drawCount++);
                }
            }
        }

        // 9. ゲームオーバー / クリア時の画面フィルター
        if (gameStatus == 1) {
            float filter[4] = { 0.8f, 0.1f, 0.1f, 0.35f };
            DrawObjectUI(renderer, 0.0f, 0.0f, 0.05f, 2.0f, 2.0f, filter, drawCount++);
            renderer.RenderText("GAME OVER", { -0.35f, 0.0f }, 0.08f, { 1.0f, 1.0f, 1.0f, 1.0f });
            drawCount += 9;
        } else if (gameStatus == 2) {
            float filter[4] = { 0.9f, 0.8f, 0.1f, 0.25f };
            DrawObjectUI(renderer, 0.0f, 0.0f, 0.05f, 2.0f, 2.0f, filter, drawCount++);
            renderer.RenderText("STAGE CLEAR!", { -0.4f, 0.0f }, 0.08f, { 1.0f, 1.0f, 1.0f, 1.0f });
            drawCount += 12;
        }

        // 10. 次元シフト移行テキストアラート
        if (transitionTimer > 0) {
            float blink = sin(spawnTimer * 0.3f) * 0.5f + 0.5f;
            DirectX::XMFLOAT4 alertCol = { 1.0f, 0.1f, 0.1f, blink };
            
            renderer.RenderText("WARNING: DIMENSION SHIFT DETECTED", { -0.55f, 0.3f }, 0.035f, alertCol);
            drawCount += 35;

            const char* modeStr = "PREPARING MODE CHANGE...";
            if (nextDimensionMode == 0) modeStr = "PREPARING 3D RAIL MODE";
            else if (nextDimensionMode == 1) modeStr = "PREPARING 2D VERTICAL MODE";
            else if (nextDimensionMode == 2) modeStr = "PREPARING 2D HORIZONTAL MODE";
            
            renderer.RenderText(modeStr, { -0.35f, 0.1f }, 0.025f, { 1.0f, 1.0f, 1.0f, 1.0f });
            drawCount += 25;
        }
    }

private:
    // 次元移行に応じたプレイヤーの固定軸制御 (Lerp引き込み)
    void ApplyDimensionConstraints(int mode, Player& p, float weight) {
        if (mode == 0) {
            // 3D: Z軸(奥行き)を 0.0f に固定
            p.z = p.z + (0.0f - p.z) * weight;
        } else if (mode == 1) {
            // 2D縦: Y軸(高さ)を 0.0f に固定、移行中はZ軸を60.0f(画面中央)へと引き寄せる
            p.y = p.y + (0.0f - p.y) * weight;
            if (nextDimensionMode == 1 && transitionTimer > 0) {
                p.z = p.z + (60.0f - p.z) * weight;
            }
        } else if (mode == 2) {
            // 2D横: X軸(左右)を 0.0f に固定、移行中はZ軸を60.0f(画面中央)へと引き寄せる
            p.x = p.x + (0.0f - p.x) * weight;
            if (nextDimensionMode == 2 && transitionTimer > 0) {
                p.z = p.z + (60.0f - p.z) * weight;
            }
        }
    }

    void GetCameraParams(int mode, float& fov, float& px, float& py, float& pz, float& tx, float& ty, float& tz, float& ux, float& uy, float& uz) {
        if (mode == 0) {
            // 3Dレールモード：自機を斜め後ろから追尾
            fov = 60.0f;
            px = player.x * 0.75f;
            py = player.y * 0.75f + 3.0f;
            pz = player.z - 45.0f;
            tx = player.x * 0.5f;
            ty = player.y * 0.5f;
            tz = player.z + 100.0f;
            ux = 0.0f; uy = 1.0f; uz = 0.0f;
        } else if (mode == 1) {
            // 2D縦スクロール：真上から超望遠見下ろし (Z=60のXZ平面を見下ろす)
            fov = 18.0f; // 視野をさらに広げる (カメラを遠ざける)
            px = 0.0f;
            py = 1050.0f; // 遥か上空から見下ろす
            pz = 60.0f;  // Z=60の中心
            tx = 0.0f;
            ty = 0.0f;
            tz = 60.0f;
            ux = 0.0f; uy = 0.0f; uz = 1.0f; // 上方向がZプラス
        } else {
            // 2D横スクロール：真横から超望遠 (Xプラス側から左向きに見る)
            fov = 18.0f; // 視野をさらに広げる (カメラを遠ざける)
            px = 1050.0f; // 遥か右から見つめる
            py = 0.0f;
            pz = 60.0f;
            tx = 0.0f;
            ty = 0.0f;
            tz = 60.0f;
            ux = 0.0f; uy = 1.0f; uz = 0.0f; // 上方向がYプラス
        }
    }

    void GetBgScrollOffset(int mode, float& x, float& y) {
        if (mode == 0) {
            x = -player.x;
            y = -player.y;
        } else if (mode == 1) {
            x = 0.0f;
            y = -scrollDistance * 0.5f;
        } else {
            x = -scrollDistance * 0.5f;
            y = 0.0f;
        }
    }

    void SetGridAlpha(int mode, float value, float& a3d, float& a2dv, float& a2dh) {
        if (mode == 0) a3d += value;
        else if (mode == 1) a2dv += value;
        else if (mode == 2) a2dh += value;
    }

    void SpawnObstacles() {
        obstacles.clear();
        obstacles.resize(25);

        for (int i = 0; i < 20; ++i) {
            auto& obs = obstacles[i];
            obs.active = true;
            obs.z = 100.0f + i * 45.0f;
            
            int roll = rand() % 3;
            if (roll == 0) {
                obs.x = -60.0f;
                obs.y = -10.0f;
                obs.w = 12.0f; obs.h = 45.0f; obs.d = 12.0f;
                obs.shapeType = 1; // Cube
                obs.color[0] = 0.4f; obs.color[1] = 0.4f; obs.color[2] = 0.45f; obs.color[3] = 1.0f;
            } else if (roll == 1) {
                obs.x = 60.0f;
                obs.y = -10.0f;
                obs.w = 12.0f; obs.h = 45.0f; obs.d = 12.0f;
                obs.shapeType = 1;
                obs.color[0] = 0.4f; obs.color[1] = 0.4f; obs.color[2] = 0.45f; obs.color[3] = 1.0f;
            } else {
                obs.x = -40.0f;
                obs.y = -10.0f;
                obs.w = 8.0f; obs.h = 30.0f; obs.d = 8.0f;
                obs.shapeType = 1;
                obs.color[0] = 0.6f; obs.color[1] = 0.6f; obs.color[2] = 0.2f; obs.color[3] = 1.0f;

                if (i + 1 < 20) {
                    auto& obsRight = obstacles[++i];
                    obsRight.active = true;
                    obsRight.x = 40.0f;
                    obsRight.y = -10.0f;
                    obsRight.z = obs.z;
                    obsRight.w = 8.0f; obsRight.h = 30.0f; obsRight.d = 8.0f;
                    obsRight.shapeType = 1;
                    obsRight.color[0] = 0.6f; obsRight.color[1] = 0.6f; obsRight.color[2] = 0.2f; obsRight.color[3] = 1.0f;
                }
            }
        }
    }

    void UpdateObstacles() {
        for (auto& obs : obstacles) {
            if (obs.active) {
                obs.z -= scrollSpeed;
                if (obs.z < -20.0f) {
                    obs.z = 400.0f + (rand() % 50);
                    if (rand() % 2 == 0) {
                        obs.x = (obs.x < 0.0f) ? 55.0f : -55.0f;
                    }
                }
            }
        }
    }

    void SpawnEnemies() {
        if (transitionTimer > 0) return; // シフト移行中はスポーンさせない

        if (enemyKillCount >= 30) {
            bool bossExists = false;
            for (auto& e : enemies) {
                if (e.active && e.type == 2) bossExists = true;
            }
            if (!bossExists) {
                for (auto& e : enemies) {
                    if (!e.active) {
                        if (dimensionMode == 0) {
                            e.Spawn(0.0f, 15.0f, 380.0f, 2, dimensionMode);
                        } else if (dimensionMode == 1) {
                            e.Spawn(0.0f, 0.0f, 95.0f, 2, dimensionMode); // 2D縦ボス: Z=95.0f (画面上端)
                        } else {
                            e.Spawn(0.0f, 0.0f, 95.0f, 2, dimensionMode); // 2D横ボス: Z=95.0f (画面右端)
                        }
                        break;
                    }
                }
            }
            return;
        }

        // 雑魚の定期スポーン
        if (spawnTimer % 90 == 0) {
            int spawnType = (rand() % 2);
            for (auto& e : enemies) {
                if (!e.active) {
                    if (dimensionMode == 0) {
                        float spawnX = ((float)(rand() % 100) - 50.0f) * 1.0f;
                        float spawnY = ((float)(rand() % 80) - 30.0f) * 0.4f;
                        e.Spawn(spawnX, spawnY, 400.0f, spawnType, dimensionMode);
                    }
                    else if (dimensionMode == 1) {
                        float spawnX = ((float)(rand() % 100) - 50.0f);
                        float spawnY = 0.0f;
                        float spawnZ = 95.0f; // 2D縦雑魚: Z=95.0f (画面上端)
                        e.Spawn(spawnX, spawnY, spawnZ, spawnType, dimensionMode);
                    }
                    else if (dimensionMode == 2) {
                        float spawnX = 0.0f;
                        float spawnY = ((float)(rand() % 70) - 35.0f);
                        float spawnZ = 95.0f; // 2D横雑魚: Z=95.0f (画面右端)
                        e.Spawn(spawnX, spawnY, spawnZ, spawnType, dimensionMode);
                    }
                    break;
                }
            }
        }
    }

    void UpdateItems(float progress) {
        for (auto& it : items) {
            if (it.active) {
                float vx1 = 0, vy1 = 0, vz1 = 0;
                GetItemScrollDelta(dimensionMode, vx1, vy1, vz1);
                float vx2 = 0, vy2 = 0, vz2 = 0;
                GetItemScrollDelta(nextDimensionMode, vx2, vy2, vz2);

                it.x += vx1 + (vx2 - vx1) * progress;
                it.y += vy1 + (vy2 - vy1) * progress;
                it.z += vz1 + (vz2 - vz1) * progress;

                // プレイヤーへ自動吸い寄せ (3D距離が近いとき)
                float dx = player.x - it.x;
                float dy = player.y - it.y;
                float dz = player.z - it.z;
                float dist = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist < 80.0f) {
                    it.x += (dx / dist) * 3.5f;
                    it.y += (dy / dist) * 3.5f;
                    it.z += (dz / dist) * 3.5f;
                }

                // プレイヤーと接触
                if (dist < player.size + 3.0f) {
                    it.active = false;
                    if (it.type == 0) {
                        player.shield = (std::min)(player.shield + 25, 100);
                    } else {
                        player.score += 500;
                    }
                }

                // 画面外判定
                CheckItemBounds(it, progress);
            }
        }
    }

    void GetItemScrollDelta(int mode, float& vx, float& vy, float& vz) {
        if (mode == 0) {
            vx = 0; vy = 0; vz = -scrollSpeed;
        } else if (mode == 1) {
            vx = 0; vy = 0; vz = -scrollSpeed; // 2D縦：Zマイナスへスクロール (画面下へ流れる)
        } else {
            vx = 0; vy = 0; vz = -scrollSpeed; // 2D横：Zマイナスへスクロール (画面左へ流れる)
        }
    }

    void CheckItemBounds(Item3D& it, float progress) {
        float limitZ = -10.0f;
        if (dimensionMode == 1 || nextDimensionMode == 1 || dimensionMode == 2 || nextDimensionMode == 2) {
            limitZ = 15.0f; // 2D画面端境界
        }

        if (((dimensionMode == 0 && nextDimensionMode == 0) && it.z < limitZ) ||
            ((dimensionMode == 1 || nextDimensionMode == 1) && it.z < limitZ) ||
            ((dimensionMode == 2 || nextDimensionMode == 2) && it.z < limitZ)) {
            it.active = false;
        }
    }

    void CheckCollisions() {
        // 1. 自機弾 vs 敵
        for (auto& b : bullets) {
            if (b.active && !b.isEnemyBullet) {
                for (auto& e : enemies) {
                    if (e.active) {
                        float dx = b.x - e.x;
                        float dy = b.y - e.y;
                        float dz = b.z - e.z;
                        float dist = sqrt(dx*dx + dy*dy + dz*dz);
                        
                        if (dist < e.size + b.size) {
                            b.active = false;
                            e.hp--;
                            if (e.hp <= 0) {
                                e.active = false;
                                enemyKillCount++;
                                player.score += (e.type == 2) ? 10000 : 200;

                                if (e.type == 2) {
                                    gameStatus = 2;
                                    SpawnItem(e.x, e.y, e.z, 0);
                                    SpawnItem(e.x + 10.0f, e.y, e.z, 1);
                                    SpawnItem(e.x - 10.0f, e.y, e.z, 1);
                                } else {
                                    if (rand() % 3 == 0) {
                                        SpawnItem(e.x, e.y, e.z, rand() % 2);
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        // 2. 敵弾 vs プレイヤー
        if (player.invincibleFrames == 0 && player.lives >= 0) {
            for (auto& b : bullets) {
                if (b.active && b.isEnemyBullet) {
                    float dx = b.x - player.x;
                    float dy = b.y - player.y;
                    float dz = b.z - player.z;
                    float dist = sqrt(dx*dx + dy*dy + dz*dz);

                    if (dist < player.size + b.size) {
                        b.active = false;
                        DamagePlayer(20);
                        break;
                    }
                }
            }
        }

        // 3. 障害物 vs プレイヤー (3Dレールモード移行中または3Dモード中のみ)
        if ((dimensionMode == 0 || nextDimensionMode == 0) && player.invincibleFrames == 0 && player.lives >= 0) {
            float collFactor = (dimensionMode == 0 && nextDimensionMode == 0) ? 1.0f :
                               ((nextDimensionMode == 0) ? transitionProgress : 1.0f - transitionProgress);
            
            if (collFactor > 0.3f) {
                for (auto& obs : obstacles) {
                    if (obs.active) {
                        float halfD = obs.d / 2.0f;
                        if (player.z >= obs.z - halfD && player.z <= obs.z + halfD) {
                            float halfW = obs.w / 2.0f;
                            float halfH = obs.h / 2.0f;
                            if (player.x >= obs.x - halfW - player.size && player.x <= obs.x + halfW + player.size &&
                                player.y >= obs.y - halfH - player.size && player.y <= obs.y + halfH + player.size) {
                                
                                DamagePlayer(40);
                                player.invincibleFrames = 60;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    void DamagePlayer(int amount) {
        player.shield -= amount;
        if (player.shield <= 0) {
            player.lives--;
            if (player.lives >= 0) {
                player.Reset();
            }
        } else {
            player.invincibleFrames = 30;
        }
    }

    void SpawnItem(float sx, float sy, float sz, int itemType) {
        for (auto& it : items) {
            if (!it.active) {
                it.active = true;
                it.x = sx;
                it.y = sy;
                it.z = sz;
                it.type = itemType;
                break;
            }
        }
    }

    // 3D 空間オブジェクト描画ヘルパー (WVP行列と自転角を転送、かつ2D正対回転を処理)
    void DrawObject3D(D3D12RenderingService& renderer, float x, float y, float z, float w, float h, float d, const float color[4], int index, int shapeType, int vertexCount, float rotAngle = 0.0f) {
        ID3D12GraphicsCommandList* cmdList = renderer.GetCommandList();
        
        char* cbvCpuData = reinterpret_cast<char*>(renderer.GetCbvCpuData());
        TransformBufferData* cbData = reinterpret_cast<TransformBufferData*>(cbvCpuData + index * 256);
        
        // 1. ワールド行列の構築 (平行移動 と スケーリング)
        DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixScaling(w, h, d);

        // 2D縦・横モード時の Sprite2D(shapeType=3) をカメラに正対させる回転処理
        int activeMode = (transitionProgress < 0.5f) ? dimensionMode : nextDimensionMode;
        if (shapeType == 3) {
            if (activeMode == 1) {
                // 2D縦: 真上から見下ろすので、X軸周りに90度回転させて XZ平面に向ける
                worldMatrix = worldMatrix * DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90.0f));
            } else if (activeMode == 2) {
                // 2D横: 真横から見るので、Y軸周りに90度回転させて YZ平面に向ける
                worldMatrix = worldMatrix * DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(90.0f));
            }
        }

        worldMatrix = worldMatrix * DirectX::XMMatrixTranslation(x, y, z);

        // 2. WVPの合成 (DirectXMath 行列乗算)
        DirectX::XMMATRIX wvpMatrix = worldMatrix * m_viewMatrix * m_projMatrix;
        
        // 3. 定数バッファへコピー (HLSL用に転置)
        DirectX::XMStoreFloat4x4(&cbData->u_wvpMatrix, DirectX::XMMatrixTranspose(wvpMatrix));
        cbData->u_Color = DirectX::XMFLOAT4(color[0], color[1], color[2], color[3]);
        cbData->u_time = (float)spawnTimer;
        cbData->u_shapeType = (float)shapeType;
        cbData->u_rotAngle = rotAngle;

        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = renderer.GetConstantBuffer()->GetGPUVirtualAddress() + index * 256;
        cmdList->SetGraphicsRootConstantBufferView(0, cbvGpuAddress);
        
        cmdList->DrawInstanced(vertexCount, 1, 0, 0);
    }

    // UI 描画ヘルパー (ビュー・プロジェクション行列を適用せずNDC空間に直接スケーリング・配置)
    void DrawObjectUI(D3D12RenderingService& renderer, float ndcX, float ndcY, float ndcZ, float ndcW, float ndcH, const float color[4], int index) {
        ID3D12GraphicsCommandList* cmdList = renderer.GetCommandList();
        
        char* cbvCpuData = reinterpret_cast<char*>(renderer.GetCbvCpuData());
        TransformBufferData* cbData = reinterpret_cast<TransformBufferData*>(cbvCpuData + index * 256);
        
        // UI用のWVP行列をスケーリングと平行移動のみで直接構築
        DirectX::XMMATRIX uiMatrix = DirectX::XMMatrixScaling(ndcW, ndcH, 1.0f) * DirectX::XMMatrixTranslation(ndcX, ndcY, ndcZ);
        
        DirectX::XMStoreFloat4x4(&cbData->u_wvpMatrix, DirectX::XMMatrixTranspose(uiMatrix));
        cbData->u_Color = DirectX::XMFLOAT4(color[0], color[1], color[2], color[3]);
        cbData->u_time = (float)spawnTimer;
        cbData->u_shapeType = 3.0f; // shapeType = 3 (Sprite2D)
        cbData->u_rotAngle = 0.0f;
        
        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = renderer.GetConstantBuffer()->GetGPUVirtualAddress() + index * 256;
        cmdList->SetGraphicsRootConstantBufferView(0, cbvGpuAddress);
        
        cmdList->DrawInstanced(4, 1, 0, 0);
    }
};
