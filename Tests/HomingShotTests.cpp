#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"
#include "../Presentation/Gameplay/Stages/Stage2/Stage2Module.h"

struct HomingShotTests {
    /** @brief 実際の追尾と部位衝突をGPUなしで検証する @return なし */
    static void Run() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
        g.m_stageNumber = 1;
        auto& nearEnemy = g.m_enemies[0];
        nearEnemy.active = nearEnemy.collisionEnabled = true;
        nearEnemy.hp = 100;
        nearEnemy.x = 0.4f;
        nearEnemy.y = 0.1f;
        auto& farEnemy = g.m_enemies[1];
        farEnemy = nearEnemy;
        farEnemy.hp = 1;
        farEnemy.x = 0.9f;
        farEnemy.y = -0.3f;
        Game::Shot shot;
        shot.vx = Game::PlayerShotConfigs[Homing].speed;

        // 遠い低HP敵より近い敵を優先し、破壊後は残った敵へ再選択する
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == 0 && shot.vy > 0.0f);
        nearEnemy.active = false;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == Game::BossPartCount + 1);
        farEnemy.active = false;
        const float vx = shot.vx;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == -1 && shot.vx == vx);

        // 部位照準が全ステージの2D/3D命中判定と一致し、破壊済み部位を返さない
        for (int stage = 1; stage <= 5; ++stage) {
            for (bool rail : {false, true}) {
                g.m_stageNumber = stage;
                g.m_stage5.phase = Game::Stage5Phase::EastsourceBattle;
                g.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                auto& boss = g.m_enemies[0];
                boss = {};
                boss.type = 2;
                boss.active = boss.collisionEnabled = true;
                boss.hp = 100;
                boss.x = 0.6f;
                boss.z = 30.0f;
                boss.bossPartHp.fill(10);
                int count = 0;
                for (int i = 0; i < Game::BossPartCount; ++i) {
                    auto part = static_cast<Game::BossPart>(i);
                    Vector3 position;
                    if (!g.TryHitBossPart({}, boss, part, &position)) continue;
                    ++count;
                    Game::Shot probe;
                    probe.x = Game::FromWorldX(position.x);
                    probe.y = Game::FromWorldY(position.y);
                    probe.z = position.z;
                    assert(g.TryHitBossPart(probe, boss, part));
                    boss.bossPartHp[i] = 0;
                    part = static_cast<Game::BossPart>(i);
                    assert(!g.TryHitBossPart({}, boss, part, &position));
                    boss.bossPartHp[i] = 10;
                }
                assert(count > 0);
            }
        }

        // 近距離の小さな標的へ収束し、追尾中の速度を維持する
        g.m_stageNumber = 1;
        g.m_viewMode = Game::ViewMode::Side2D;
        nearEnemy = {};
        nearEnemy.active = nearEnemy.collisionEnabled = true;
        nearEnemy.hp = 10;
        nearEnemy.x = 0.3f;
        nearEnemy.y = 0.15f;
        shot = {};
        shot.vx = Game::PlayerShotConfigs[Homing].speed;
        bool hit = false;
        for (int frame = 0; frame < 30 && !hit; ++frame) {
            g.UpdateHomingShot(shot);
            assert(std::abs(std::hypot(shot.vx, shot.vy) - Game::PlayerShotConfigs[Homing].speed) < 0.00001f);
            shot.x += shot.vx;
            shot.y += shot.vy;
            hit = Game::Hit(shot.x, shot.y, shot.hitRadius, nearEnemy.x, nearEnemy.y, 0.03f);
        }
        assert(hit);

        // 3Dでも低HPより近距離を優先し、ワールド弾速を維持する
        g.m_viewMode = Game::ViewMode::Rail3D;
        nearEnemy.x = 0.1f;
        nearEnemy.y = 0.0f;
        nearEnemy.z = g.PlayerRailDepth() + 10.0f;
        farEnemy = nearEnemy;
        farEnemy.z += 20.0f;
        farEnemy.hp = 1;
        shot = {};
        shot.z = g.PlayerRailDepth();
        shot.vz = 1.45f;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == 0);
        assert(std::abs(Vector3(Game::ToWorldX(shot.vx), Game::ToWorldY(shot.vy), shot.vz).LengthSquared() - 1.45f * 1.45f) < 0.0001f);

        // 縦スクロール3Dでは敵の奥行きではなく弾平面へ投影した位置に向かう
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::WallClimbLower;
        farEnemy.active = false;
        nearEnemy.x = 0.2f;
        nearEnemy.y = 0.6f;
        nearEnemy.z = 46.0f;
        shot = {};
        shot.z = g.PlayerRailDepth();
        shot.vy = Game::PlayerShotConfigs[Homing].speed;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget == 0 && shot.vx > 0.0f && shot.vy > 0.0f && shot.vz == 0.0f);

        // 周回でZ負方向へ撃つ場合も有効な弱点を狙い、破壊済み弱点を除外する
        nearEnemy.active = false;
        g.m_stage5.phase = Game::Stage5Phase::TayamaFireControl;
        g.m_stage5.tayamaOrbitAngle = Math::Pi;
        g.m_frame = 1;
        auto& weakpoint = g.m_stage5.tayamaWeakpoints[0];
        weakpoint.type = Game::TayamaWeakpoint::LeftSearchlight;
        weakpoint.active = true;
        weakpoint.destroyed = false;
        weakpoint.hp = 10;
        Vector3 weakpointPosition;
        assert(Game::Stage5Module::GetHomingTarget(g, 0, weakpointPosition));
        const Vector3 player = g.PlayerWorldPosition();
        shot = {};
        shot.x = Game::FromWorldX(player.x);
        shot.y = Game::FromWorldY(player.y);
        shot.z = player.z;
        shot.vz = weakpointPosition.z > player.z ? 1.45f : -1.45f;
        g.UpdateHomingShot(shot);
        assert(shot.homingTarget >= static_cast<int>(g.m_enemies.size()) * (Game::BossPartCount + 1));
        weakpoint.destroyed = true;
        assert(!Game::Stage5Module::GetHomingTarget(g, 0, weakpointPosition));
        // Stage2主砲は入力の継続先を狙い、予測で実際の自機位置を変更しない
        g.m_stageNumber = 2;
        g.m_difficulty = Hard;
        // 照準予測の検証中に発射される弾で移動状態を中断しない
        g.Player().m_invincible = 9999;
        g.Player().m_playerDestructionTimer = 0;
        for (bool rail : {false, true}) {
            g.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            for (bool slow : {false, true}) {
                for (int input = 0; input < 16; ++input) {
                    g.Player().m_moveLeft = (input & 1) != 0;
                    g.Player().m_moveRight = (input & 2) != 0;
                    g.Player().m_moveUp = (input & 4) != 0;
                    g.Player().m_moveDown = (input & 8) != 0;
                    g.Player().m_slowMove = slow;
                    for (int cycle : {0, 30, 59}) {
                        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
                        for (int frame = cycle + 1; frame < 90; ++frame) g.TickPlayer();
                        const float expectedX = g.Player().m_playerX;
                        const float expectedY = g.Player().m_playerY;
                        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
                        Game::Enemy boss {};
                        boss.phase = 3.0f;
                        boss.hp = 30;
                        boss.maxHp = 100;
                        boss.age = 1;
                        g.m_stage2.boss = {};
                        g.m_stage2.boss.actionAge = cycle;
                        Game::Stage2Module::TickBoss(g, boss);
                        assert(g.Player().m_playerX == 0.0f && g.Player().m_playerY == 0.0f);
                        if (cycle == 0) assert(boss.attackWarningFrames == 90);
                        // 一度に予測位置へ飛ばず、各軸で予測位置との間へ進む
                        assert(expectedX == 0.0f ? boss.actionX == 0.0f :
                            boss.actionX / expectedX > 0.0f && boss.actionX / expectedX < 1.0f);
                        assert(expectedY == 0.0f ? boss.actionY == 0.0f :
                            boss.actionY / expectedY > 0.0f && boss.actionY / expectedY < 1.0f);
                        const float aimX = boss.actionX;
                        const float aimY = boss.actionY;
                        const float aimZ = boss.actionZ;
                        // HARDは確定後の0.5秒間と発射中に照準を固定する
                        g.Player().m_moveRight = !g.Player().m_moveRight;
                        for (int lockedFrame = 60; lockedFrame < 102; ++lockedFrame) {
                            g.m_stage2.boss.actionAge = lockedFrame;
                            Game::Stage2Module::TickBoss(g, boss);
                            assert(boss.actionX == aimX && boss.actionY == aimY && boss.actionZ == aimZ);
                        }
                        g.Player().m_moveRight = !g.Player().m_moveRight;
                    }
                }
            }
        }
        // 次の予告開始と発射後の追従再開でも照準を初期化しない
        for (int cycle : {0, 102, 179}) {
            Game::Enemy boss {};
            boss.phase = 3.0f;
            boss.hp = 30;
            boss.maxHp = 100;
            boss.age = 1;
            boss.actionX = 1.0f;
            g.Player().m_playerX = g.Player().m_playerY = 0.0f;
            g.Player().m_moveLeft = g.Player().m_moveRight = g.Player().m_moveUp = g.Player().m_moveDown = false;
            g.m_stage2.boss.actionAge = cycle;
            Game::Stage2Module::TickBoss(g, boss);
            assert(boss.actionX > 0.8f && boss.actionX < 1.0f);
        }
        // EASY/NORMALは移動入力があっても現在位置へ追従し、確定後は固定する
        for (auto difficulty : {Easy, Normal}) {
            g.m_difficulty = difficulty;
            g.Player().m_moveRight = true;
            g.Player().m_playerX = 0.5f;
            Game::Enemy boss {};
            boss.phase = 3.0f;
            boss.hp = 30;
            boss.maxHp = 100;
            boss.age = 1;
            boss.actionX = g.Player().m_playerX;
            g.m_stage2.boss.actionAge = 30;
            Game::Stage2Module::TickBoss(g, boss);
            assert(boss.actionX == g.Player().m_playerX);
            const float aimX = boss.actionX;
            const float aimY = boss.actionY;
            const float aimZ = boss.actionZ;
            g.Player().m_playerX = -0.5f;
            for (int frame = 60; frame < 132; ++frame) {
                g.m_stage2.boss.actionAge = frame;
                Game::Stage2Module::TickBoss(g, boss);
                assert(boss.actionX == aimX && boss.actionY == aimY && boss.actionZ == aimZ);
            }
        }
        // ラスボス第2形態の3D移動端と、その位置から撃つ弾の生存を確認する
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::TayamaDragonBattle;
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        g.Player().m_moveLeft = g.Player().m_moveRight = g.Player().m_moveUp = g.Player().m_moveDown = false;
        for (float edge : {-1.35f, 1.35f}) {
            g.Player().m_playerX = edge * 2.0f;
            g.Player().m_playerY = 4.0f;
            g.TickPlayer();
            assert(g.Player().m_playerX == edge && g.Player().m_playerY == 3.5f);
            g.m_shots.fill({});
            auto& edgeShot = g.m_shots[0];
            edgeShot.active = true;
            edgeShot.x = edge;
            edgeShot.y = g.Player().m_playerY;
            edgeShot.z = 10.0f;
            g.TickShots();
            assert(edgeShot.active);
        }
        std::puts("HomingShotTests passed");
    }
};

/** @brief 追尾回帰チェックを実行する @return 成功時0 */
int main() {
    HomingShotTests::Run();
    return 0;
}
