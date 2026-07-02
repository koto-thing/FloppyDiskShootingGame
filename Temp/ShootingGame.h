#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "../Infrastructure/ExternalServices/D3D12Renderer.h"

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
 * @brief 初代スターフォックス風 3D レールシューティングゲーム管理クラス
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
    float scrollDistance; // 総スクロール距離 (Z軸前進)
    float scrollSpeed;
    int enemyKillCount;

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

        backgroundPos[0] = 0.0f;
        backgroundPos[1] = 0.0f;
        backgroundPos[2] = 0.0f;

        // 初期障害物 (奥にいくつかのビルやゲートをランダムに初期配置)
        SpawnObstacles();
    }

    void Update() {
        if (gameStatus != 0) {
            // ゲームオーバーかクリア時は、Rキーでリセット
            if (InputSystem::IsKeyPressed('R')) {
                Reset();
            }
            return;
        }

        spawnTimer++;
        scrollDistance += scrollSpeed;

        // 星空背景の位置をプレイヤーに合わせてスクロール
        backgroundPos[0] = -player.x;
        backgroundPos[1] = -player.y;

        // プレイヤー更新
        player.Update(bullets);

        // 障害物のスクロールと更新
        UpdateObstacles();

        // 敵の出現テーブル (奥 Z=400.0f に出現させる)
        SpawnEnemies();

        // 敵の更新
        for (auto& e : enemies) {
            if (e.active) {
                // プレイヤーの3D座標を渡して追従・攻撃させる
                e.Update(bullets, player.x, player.y, player.z);
            }
        }

        // 弾の更新
        for (auto& b : bullets) {
            b.Update();
        }

        // アイテムの更新 (プレイヤーへの自動吸い寄せ)
        UpdateItems();

        // 当たり判定 (3D衝突判定)
        CheckCollisions();

        // ゲームオーバー判定
        if (player.lives < 0) {
            gameStatus = 1; // GameOver
        }
    }

    void Render(D3D12Renderer& renderer) {
        int drawCount = 0;

        // A. 宇宙空間の星空背景描画 (PSO: 1: Background)
        renderer.SetPipelineState(1);
        float bgCol[4] = { backgroundPos[0], backgroundPos[1], 0.0f, 1.0f }; // XZ移動を色に乗せてシェーダーへ送る
        DrawObject3D(renderer, 0.0f, 0.0f, 1.0f, 2.0f, 2.0f, 1.0f, bgCol, drawCount++, 3, 4); // 板描画

        // B. 以降のすべての3Dオブジェクトは通常スプライト/3Dポリゴン (PSO: 0: Object)
        renderer.SetPipelineState(0);

        // 1. 3D地面グリッドの描画 (XZ平面プレートを組み合わせてグリッド線を表現)
        float gridCol[4] = { 0.15f, 0.75f, 0.25f, 1.0f }; // レトロネオングリーン
        float floorY = -18.0f; // 地面の高さ
        
        // Zスクロールのオフセット
        float zOffset = -fmod(scrollDistance, 45.0f);

        // 縦線を描画 (3D板をXZ方向に並べる)
        for (float xG = -120.0f; xG <= 120.0f; xG += 30.0f) {
            // 奥に向かう一本の線
            DrawObject3D(renderer, xG, floorY, 200.0f, 0.3f, 1.0f, 400.0f, gridCol, drawCount++, 0, 4);
        }
        // 横線を描画
        for (float zG = 0.0f; zG <= 400.0f; zG += 45.0f) {
            float curZ = zG + zOffset;
            if (curZ > 0.0f) {
                DrawObject3D(renderer, 0.0f, floorY, curZ, 240.0f, 1.0f, 0.3f, gridCol, drawCount++, 0, 4);
            }
        }

        // 2. 3D障害物 (ビル・ゲート) の描画
        for (auto& obs : obstacles) {
            if (obs.active) {
                // ビルや柱
                DrawObject3D(renderer, obs.x, obs.y, obs.z, obs.w, obs.h, obs.d, obs.color, drawCount++, obs.shapeType, 36);
                if (drawCount >= 2000) break;
            }
        }

        // 3. 敵機の描画 (Pyramid または Cube)
        for (auto& e : enemies) {
            if (e.active) {
                if (e.type == 2) {
                    // ボスは超巨大 Cube 戦艦
                    DrawObject3D(renderer, e.x, e.y, e.z, e.scaleX, e.scaleY, e.scaleZ, e.color, drawCount++, 1, 36);
                } else {
                    // 雑魚は Pyramid (戦闘機)
                    DrawObject3D(renderer, e.x, e.y, e.z, e.scaleX, e.scaleY, e.scaleZ, e.color, drawCount++, 2, 18);
                }
                if (drawCount >= 2000) break;
            }
        }

        // 4. 弾の描画 (レーザー/敵の光弾)
        for (auto& b : bullets) {
            if (b.active) {
                if (b.isEnemyBullet) {
                    // 敵弾は赤い 3D球 (Cubeで代用)
                    DrawObject3D(renderer, b.x, b.y, b.z, b.size * 2.0f, b.size * 2.0f, b.size * 2.0f, b.color, drawCount++, 1, 36);
                } else {
                    // 自機レーザーは緑の細長い 3D光条 (Cube)
                    DrawObject3D(renderer, b.x, b.y, b.z, 0.5f, 0.5f, 8.0f, b.color, drawCount++, 1, 36);
                }
                if (drawCount >= 2000) break;
            }
        }

        // 5. アイテムの描画 (回転する3D Cube)
        for (auto& it : items) {
            if (it.active) {
                float itCol[4] = { 1.0f, 0.8f, 0.0f, 1.0f }; // 黄色
                if (it.type == 0) {
                    itCol[0] = 1.0f; itCol[1] = 0.2f; itCol[2] = 0.2f; // 赤 (シールド回復)
                } else {
                    itCol[0] = 0.2f; itCol[1] = 0.6f; itCol[2] = 1.0f; // 青 (スコア)
                }
                // 回転アニメーション付きで描画
                DrawObject3D(renderer, it.x, it.y, it.z, 3.0f, 3.0f, 3.0f, itCol, drawCount++, 1, 36);
                if (drawCount >= 2000) break;
            }
        }

        // 6. プレイヤー (Arwing) の描画
        if (player.lives >= 0) {
            bool drawPlayer = true;
            if (player.invincibleFrames > 0 && (player.invincibleFrames % 4 < 2)) {
                drawPlayer = false;
            }

            if (drawPlayer) {
                float pCol[4] = { 0.8f, 0.8f, 0.85f, 1.0f }; // 白銀の機体
                float wingCol[4] = { 0.2f, 0.4f, 0.8f, 1.0f }; // 青い翼
                float noseCol[4] = { 0.9f, 0.1f, 0.1f, 1.0f }; // 赤い機首

                // A. メインボディ (三角錐)
                DrawObject3D(renderer, player.x, player.y, player.z, 3.0f, 2.5f, 9.0f, pCol, drawCount++, 2, 18);

                // B. 機首ノーズ (赤い三角錐)
                DrawObject3D(renderer, player.x, player.y, player.z + 5.0f, 1.2f, 1.0f, 4.0f, noseCol, drawCount++, 2, 18);

                // C. 左主翼 (傾き rollAngle を考慮)
                float leftWingX = player.x - 4.5f;
                float leftWingY = player.y + player.rollAngle * 4.0f;
                DrawObject3D(renderer, leftWingX, leftWingY, player.z - 1.0f, 6.0f, 0.4f, 3.0f, wingCol, drawCount++, 1, 36);

                // D. 右主翼
                float rightWingX = player.x + 4.5f;
                float rightWingY = player.y - player.rollAngle * 4.0f;
                DrawObject3D(renderer, rightWingX, rightWingY, player.z - 1.0f, 6.0f, 0.4f, 3.0f, wingCol, drawCount++, 1, 36);
            }
        }

        // 7. 3D照準HUDの描画 (PSO: 2: SpellCircle)
        // プレイヤーの少し前方 (Z = 60.0f) に追従して浮かぶロックオンカーソル
        if (player.lives >= 0) {
            renderer.SetPipelineState(2);
            float hudCol[4] = { 0.2f, 0.9f, 0.2f, 0.8f }; // ネオングリーン
            // 照準はプレイヤーの前方に浮かぶ Plate
            DrawObject3D(renderer, player.x, player.y, player.z + 65.0f, 8.0f, 8.0f, 1.0f, hudCol, drawCount++, 3, 4);
        }

        // 8. UIの描画 (PSO: 0: Object、2Dスプライトとして一番手前 z = 0.55f 付近に描画)
        renderer.SetPipelineState(0);
        float uiZ = 0.55f; // 手前に固定

        // シールドゲージ (画面左下、スターフォックス風メーター)
        float shieldRate = (float)player.shield / 100.0f;
        float shieldWidth = 24.0f * shieldRate;
        float barCol[4] = { 0.9f, 0.2f, 0.2f, 1.0f }; // 赤 (シールド低下)
        if (player.shield > 50) {
            barCol[0] = 0.1f; barCol[1] = 0.9f; barCol[2] = 0.1f; // 緑
        } else if (player.shield > 20) {
            barCol[0] = 0.9f; barCol[1] = 0.9f; barCol[2] = 0.1f; // 黄色
        }
        // メーター枠 (カメラから極小スケールで手前UIとして描画)
        float grayCol[4] = { 0.15f, 0.15f, 0.2f, 1.0f };
        // 座標変換を行わず、手前のNDC空間に直接描画するため、ZNearより奥で極小に置く
        // X = -0.7f, Y = -0.8f などの位置に Plate (shapeType 3) で描画
        DrawObjectUI(renderer, -0.6f, -0.85f, 0.3f, 0.03f, grayCol, drawCount++); // 背景枠
        if (shieldRate > 0.0f) {
            DrawObjectUI(renderer, -0.6f - (0.15f * (1.0f - shieldRate)), -0.85f, 0.3f * shieldRate, 0.03f, barCol, drawCount++); // ゲージ本体
        }

        // 残機表示 (ハートマークの代わりに小さなシルバーの Arwing アイコンを左下に並べる)
        for (int i = 0; i < player.lives; ++i) {
            float shipCol[4] = { 0.8f, 0.8f, 0.9f, 1.0f };
            DrawObjectUI(renderer, -0.9f + i * 0.07f, -0.85f, 0.04f, 0.03f, shipCol, drawCount++);
        }

        // スコア進捗ゲージ (撃破カウント)
        float killRate = (std::min)((float)enemyKillCount / 30.0f, 1.0f);
        float killWidth = 0.3f * killRate;
        float scoreCol[4] = { 0.2f, 0.6f, 1.0f, 1.0f }; // 水色
        DrawObjectUI(renderer, 0.6f, -0.85f, 0.3f, 0.03f, grayCol, drawCount++);
        if (killRate > 0.0f) {
            DrawObjectUI(renderer, 0.6f - (0.15f * (1.0f - killRate)), -0.85f, killWidth, 0.03f, scoreCol, drawCount++);
        }

        // ボスHPバー (ボス出現中のみ画面上部に描画)
        for (auto& e : enemies) {
            if (e.active && e.type == 2) {
                float hpRate = (float)e.hp / e.maxHp;
                float hpWidth = 1.4f * hpRate;
                float bossHpCol[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
                DrawObjectUI(renderer, 0.0f, 0.85f, 1.4f, 0.04f, grayCol, drawCount++); // 背景
                if (hpRate > 0.0f) {
                    DrawObjectUI(renderer, 0.0f - (0.7f * (1.0f - hpRate)), 0.85f, hpWidth, 0.04f, bossHpCol, drawCount++); // 残量
                }
            }
        }

        // 9. ゲームオーバー / クリア時の画面フィルター
        if (gameStatus == 1) {
            float filter[4] = { 0.8f, 0.1f, 0.1f, 0.35f };
            DrawObjectUI(renderer, 0.0f, 0.0f, 2.0f, 2.0f, filter, drawCount++);
        } else if (gameStatus == 2) {
            float filter[4] = { 0.9f, 0.8f, 0.1f, 0.25f };
            DrawObjectUI(renderer, 0.0f, 0.0f, 2.0f, 2.0f, filter, drawCount++);
        }
    }

private:
    void SpawnObstacles() {
        obstacles.clear();
        obstacles.resize(25);

        // Z軸方向に一定間隔でビルやゲートを散りばめる
        for (int i = 0; i < 20; ++i) {
            auto& obs = obstacles[i];
            obs.active = true;
            obs.z = 100.0f + i * 45.0f;
            
            int roll = rand() % 3;
            if (roll == 0) {
                // 左ビル
                obs.x = -60.0f;
                obs.y = -10.0f;
                obs.w = 12.0f; obs.h = 45.0f; obs.d = 12.0f;
                obs.shapeType = 1; // Cube
                obs.color[0] = 0.4f; obs.color[1] = 0.4f; obs.color[2] = 0.45f; obs.color[3] = 1.0f; // グレー
            } else if (roll == 1) {
                // 右ビル
                obs.x = 60.0f;
                obs.y = -10.0f;
                obs.w = 12.0f; obs.h = 45.0f; obs.d = 12.0f;
                obs.shapeType = 1;
                obs.color[0] = 0.4f; obs.color[1] = 0.4f; obs.color[2] = 0.45f; obs.color[3] = 1.0f;
            } else {
                // ゲートアーチ (簡易的に真ん中の柱と天井として配置。ここではZの同じ位置に左右のビルを置く)
                obs.x = -40.0f;
                obs.y = -10.0f;
                obs.w = 8.0f; obs.h = 30.0f; obs.d = 8.0f;
                obs.shapeType = 1;
                obs.color[0] = 0.6f; obs.color[1] = 0.6f; obs.color[2] = 0.2f; obs.color[3] = 1.0f; // 黄色ゲート

                // 対になる右の柱
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
                // スクロール速度分、手前に近づく
                obs.z -= scrollSpeed;
                
                // カメラの後ろに抜けたら、奥に再スポーン (無限レールループ)
                if (obs.z < -20.0f) {
                    obs.z = 400.0f + (rand() % 50);
                    // 左右の位置をランダムに入れ替え
                    if (rand() % 2 == 0) {
                        obs.x = (obs.x < 0.0f) ? 55.0f : -55.0f;
                    }
                }
            }
        }
    }

    void SpawnEnemies() {
        // 敵スポーンロジック
        // 一定カウントごとに、奥（Z=400.0f）にスポーンさせる
        if (enemyKillCount >= 30) {
            // ボス出現準備：雑魚を消してボスをスポーン
            bool bossExists = false;
            for (auto& e : enemies) {
                if (e.active && e.type == 2) bossExists = true;
            }
            if (!bossExists) {
                // ボス出現！
                for (auto& e : enemies) {
                    if (!e.active) {
                        e.Spawn(0.0f, 15.0f, 380.0f, 2); // Z = 380
                        break;
                    }
                }
            }
            return;
        }

        // 雑魚の定期スポーン
        if (spawnTimer % 90 == 0) {
            int spawnType = (rand() % 2);
            float spawnX = ((float)(rand() % 100) - 50.0f) * 1.0f;
            float spawnY = ((float)(rand() % 80) - 30.0f) * 0.4f;

            for (auto& e : enemies) {
                if (!e.active) {
                    e.Spawn(spawnX, spawnY, 400.0f, spawnType); // 奥 Z=400
                    break;
                }
            }
        }
    }

    void UpdateItems() {
        for (auto& it : items) {
            if (it.active) {
                // 手前にスクロール
                it.z -= scrollSpeed;

                // プレイヤーへ自動吸い寄せ (3D距離が近いとき)
                float dx = player.x - it.x;
                float dy = player.y - it.y;
                float dz = player.z - it.z;
                float dist = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist < 80.0f) {
                    // 吸い寄せ速度
                    it.x += (dx / dist) * 3.5f;
                    it.y += (dy / dist) * 3.5f;
                    it.z += (dz / dist) * 3.5f;
                }

                // プレイヤーと接触
                if (dist < player.size + 3.0f) {
                    it.active = false;
                    if (it.type == 0) {
                        player.shield = (std::min)(player.shield + 25, 100); // シールド回復
                    } else {
                        player.score += 500; // スコア獲得
                    }
                }

                if (it.z < -10.0f) {
                    it.active = false;
                }
            }
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

                                // アイテムドロップ (ボスなら大量、雑魚なら一定確率)
                                if (e.type == 2) {
                                    gameStatus = 2; // ボス撃破でゲームクリア！
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
                        DamagePlayer(20); // 弾に当たるとシールド20減少
                        break;
                    }
                }
            }
        }

        // 3. 障害物 vs プレイヤー (激突判定)
        if (player.invincibleFrames == 0 && player.lives >= 0) {
            for (auto& obs : obstacles) {
                if (obs.active) {
                    // Zの交差判定 (障害物の奥行き d 範囲内)
                    float halfD = obs.d / 2.0f;
                    if (player.z >= obs.z - halfD && player.z <= obs.z + halfD) {
                        // XYの交差判定 (境界枠チェック)
                        float halfW = obs.w / 2.0f;
                        float halfH = obs.h / 2.0f;
                        if (player.x >= obs.x - halfW - player.size && player.x <= obs.x + halfW + player.size &&
                            player.y >= obs.y - halfH - player.size && player.y <= obs.y + halfH + player.size) {
                            
                            // 激突！大ダメージ
                            DamagePlayer(40);
                            
                            // プレイヤーを少し押し返すか、無敵化
                            player.invincibleFrames = 60;
                            break;
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
                player.Reset(); // 残機があれば復活
            }
        } else {
            player.invincibleFrames = 30; // 被弾無敵
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

    // 3D 空間オブジェクト描画ヘルパー
    void DrawObject3D(D3D12Renderer& renderer, float x, float y, float z, float w, float h, float d, const float color[4], int index, int shapeType, int vertexCount) {
        ID3D12GraphicsCommandList* cmdList = renderer.GetCommandList();
        
        char* cbvCpuData = reinterpret_cast<char*>(renderer.GetCbvCpuData());
        float* objectData = reinterpret_cast<float*>(cbvCpuData + index * 256);
        
        // C++側でカメラ相対に変換 (カメラは 0, 0, -40.0f に固定し、プレイヤーの x, y の動きを穏やかに追従する)
        // カメラ座標の算出
        float camX = player.x * 0.75f;
        float camY = player.y * 0.75f;
        float camZ = -45.0f; // 自機の少し手前にカメラを固定

        objectData[0] = x - camX;
        objectData[1] = y - camY;
        objectData[2] = z - camZ;
        objectData[3] = 0.0f; // pad

        objectData[4] = w;
        objectData[5] = h;
        objectData[6] = d;
        objectData[7] = 0.0f; // pad

        objectData[8] = color[0];
        objectData[9] = color[1];
        objectData[10] = color[2];
        objectData[11] = color[3];

        objectData[12] = (float)spawnTimer;
        objectData[13] = (float)shapeType; // 0: Plate, 1: Cube, 2: Pyramid, 3: Sprite2D
        
        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = renderer.GetConstantBuffer()->GetGPUVirtualAddress() + index * 256;
        cmdList->SetGraphicsRootConstantBufferView(0, cbvGpuAddress);
        
        cmdList->DrawInstanced(vertexCount, 1, 0, 0);
    }

    // UI (手前の2D平面) 描画ヘルパー (透視投影を回避して直接NDCで描く)
    void DrawObjectUI(D3D12Renderer& renderer, float ndcX, float ndcY, float ndcW, float ndcH, const float color[4], int index) {
        ID3D12GraphicsCommandList* cmdList = renderer.GetCommandList();
        
        char* cbvCpuData = reinterpret_cast<char*>(renderer.GetCbvCpuData());
        float* objectData = reinterpret_cast<float*>(cbvCpuData + index * 256);
        
        // Zを極小にして、かつ shapeType = 3 (Sprite2D) で描画
        // シェーダー内の投影計算で Z=1.0 にすれば、ちょうどそのまま NDC 座標になる！
        // viewPos.z = 1.0f になると、x / 1.0f = x, y / 1.0f = y となり、透視投影を完全にスルーできる！
        // さらに、fov=1.35, aspect=1.33なので、それを打ち消す値を乗せておく。
        float fov = 1.35f;
        float aspect = 800.0f / 600.0f;

        objectData[0] = (ndcX * aspect) / fov; // X
        objectData[1] = ndcY / fov;           // Y
        objectData[2] = 1.0f;                 // Z=1.0 (これで遠近法をキャンセル)
        objectData[3] = 0.0f;

        objectData[4] = (ndcW * aspect) / fov; // W
        objectData[5] = ndcH / fov;           // H
        objectData[6] = 0.0f;                 // D
        objectData[7] = 0.0f;

        objectData[8] = color[0];
        objectData[9] = color[1];
        objectData[10] = color[2];
        objectData[11] = color[3];

        objectData[12] = (float)spawnTimer;
        objectData[13] = 3.0f; // shapeType = 3 (Sprite2D)
        
        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = renderer.GetConstantBuffer()->GetGPUVirtualAddress() + index * 256;
        cmdList->SetGraphicsRootConstantBufferView(0, cbvGpuAddress);
        
        cmdList->DrawInstanced(4, 1, 0, 0); // 4頂点で Plate スプライト描画
    }
};
