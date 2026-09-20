#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5ModelView.h"

struct OrbitShotTests {
    /** @brief 龍の突進終了時に頭部と全身の装甲が画面外へ抜けることを検証する @return なし */
    static void CheckDragonRush() {
        using Game = SideScrollingShooter;
        using namespace ShooterStages::Stage5;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phase = Phase::TayamaDragonBattle;
        g.m_stage5.tayamaDragonAttack = TayamaDragonAttack::Rush;
        g.m_stage5.tayamaDragonAttackTimer =
            TayamaDragonRushWarningFrames + TayamaDragonRushActiveFrames - 1;
        auto renderer = std::make_unique<Renderer>();

        // 胴体のうねりと視点補間を変えて、復帰直前の実描画モデルを確認する
        for (int frame = 0; frame < 600; frame += 17) {
            g.m_frame = frame;
            for (int step = 0; step <= 10; ++step) {
                const float weight = step * 0.1f;
                g.m_viewMode = Game::ViewMode::Side2D;
                g.m_nextViewMode = Game::ViewMode::Rail3D;
                g.m_viewTransitionTimer = 1;
                g.m_viewTransitionProgress = weight;
                Camera3D camera;
                g.ConfigureRailCamera(camera, *renderer);
                camera.SetViewport({0, 0, 2560, 1080});
                renderer->BeginFrame();
                Game::Stage5Module::DrawTayamaDragon(g, *renderer, camera, weight);
                assert(renderer->CommandCount() > 0 && !renderer->HasOverflowed());
                for (std::size_t index = 0; index < renderer->CommandCount(); ++index) {
                    const auto& command = renderer->Command(index);
                    assert(command.type == RenderCommand::Type::Primitive3D);
                    bool outsideLeft = true;
                    bool behindCamera = true;
                    // 各プリミティブを包む箱の全頂点が同じクリップ面の外にある
                    for (float x : {-0.5f, 0.5f}) {
                        for (float y : {-0.5f, 0.5f}) {
                            for (float z : {-0.5f, 0.5f}) {
                                const Vector4 clip = command.primitive.wvpMatrix * Vector4 {x, y, z, 1.0f};
                                outsideLeft &= clip.x < -clip.w;
                                behindCamera &= clip.z < 0.0f;
                            }
                        }
                    }
                    if (!outsideLeft && !behindCamera) {
                        const auto& wvp = command.primitive.wvpMatrix;
                        std::fprintf(stderr, "Rush exit: frame=%d blend=%.1f part=%zu clip=(%.3f, %.3f, %.3f, %.3f)\n",
                            frame, weight, index, wvp.m[0][3], wvp.m[1][3], wvp.m[2][3], wvp.m[3][3]);
                    }
                    assert((outsideLeft || behindCamera) &&
                        "Dragon must leave the screen completely before rush recovery");
                }
            }
        }
        std::puts("Tayama dragon rush exit passed");
    }

    /** @brief 壁面ドローン弾が検知時の自機位置を通ることを検証する @return なし */
    static void CheckWallDrone() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::WallClimbLower;
        g.m_stage = &Game::StageForNumber(g.m_stageNumber, g.m_difficulty);
        // 上下の目標と両視点で実際に生成した弾の到達地点を確認する
        for (auto mode : {Game::ViewMode::Side2D, Game::ViewMode::Rail3D}) {
            for (float targetY : {-0.5f, 0.0f, 0.5f}) {
                g.m_shots = {};
                g.m_enemies = {};
                g.m_viewMode = g.m_nextViewMode = mode;
                g.Player().m_playerX = -0.4f;
                g.Player().m_playerY = targetY;
                auto& drone = g.m_enemies[0];
                drone.active = true;
                drone.type = 10;
                drone.baseY = mode == Game::ViewMode::Rail3D ? 6.0f : 0.0f;
                drone.motionAge = ShooterStages::Stage5::DroneMachineGunBurstFrames;
                drone.attackWarningTargetX = g.Player().m_playerX;
                drone.attackWarningTargetY = targetY;
                // 検知後に自機が動いても保存した地点へ発射する
                g.Player().m_playerY += 0.2f;
                g.TickEnemies();
                const auto& shot = g.m_shots[0];
                assert(shot.active && shot.enemy);
                const float arrival = mode == Game::ViewMode::Rail3D ?
                    (g.PlayerRailDepth() - shot.z) / shot.vz :
                    (drone.attackWarningTargetX - shot.x) / shot.vx;
                assert(arrival > 0.0f);
                assert(std::abs(shot.x + shot.vx * arrival - drone.attackWarningTargetX) < 0.0001f);
                assert(std::abs(shot.y + shot.vy * arrival - targetY) < 0.0001f);

                // 離れた照準から自機を捕捉し、再装填後にも再攻撃する
                g.m_shots = {};
                drone.motionAge = 0;
                drone.recoilAge = 0;
                drone.turretAimX = 0.8f;
                drone.turretAimY = -0.7f;
                for (int frame = 0; frame < 60 && drone.motionAge == 0; ++frame) {
                    g.TickEnemies();
                }
                assert(drone.motionAge > 0);
                assert(g.m_shots[0].active);
                assert(drone.attackWarningTargetX == g.Player().m_playerX);
                assert(drone.attackWarningTargetY == g.Player().m_playerY);
                while (drone.motionAge > 0) g.TickEnemies();
                g.m_shots = {};
                g.Player().m_playerX = 0.4f;
                for (int frame = 0; frame < ShooterStages::Stage5::DroneMachineGunCooldownFrames; ++frame) {
                    g.TickEnemies();
                    assert(drone.motionAge == 0);
                    assert(!g.m_shots[0].active);
                }
                g.TickEnemies();
                assert(drone.motionAge > 0);
                assert(g.m_shots[0].active);
                assert(drone.attackWarningTargetX == g.Player().m_playerX);
            }
        }
        std::puts("Wall drone shot tests passed");
    }

    /** @brief 包囲弾の生成、進入、消滅と通常弾の発射制限を検証する @return なし */
    static void Run() {
        using Game = SideScrollingShooter;
        using Attack = ShooterStages::Stage5::TayamaDragonAttack;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::TayamaDragonBattle;
        g.m_stage5.tayamaDragonAttack = Attack::Orbit;
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
        g.Player().m_invincible = 999;

        // 実際の5回の発射位置から自機へ到達し、通過後には消滅する
        for (int frame : {36, 48, 60, 72, 84}) {
            g.m_shots = {};
            g.m_stage5.tayamaDragonAttack = Attack::Orbit;
            const int segment = 2 + frame / 12 * 3 % 38;
            const float angle = frame / 120.0f * Math::TwoPi - segment * 0.16f;
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);
            g.SpawnShotDirect(Game::FromWorldX(dx * 10.5f), Game::FromWorldY(dy * 10.5f),
                g.PlayerRailDepth(), Game::FromWorldX(-dx * 0.21f),
                Game::FromWorldY(-dy * 0.21f), 0.0f, true, -1, 0, true);
            auto& shot = g.m_shots[0];
            assert(shot.active && shot.tayamaDragonOrbit);
            // 攻撃終了後も生成時の識別を保持する
            g.m_stage5.tayamaDragonAttack = Attack::None;
            for (int tick = 0; tick < 50; ++tick) {
                g.TickShots();
                assert(shot.active);
            }
            assert(std::abs(shot.x) < 0.001f && std::abs(shot.y) < 0.001f);
            for (int tick = 0; tick < 60; ++tick) g.TickShots();
            assert(!shot.active);
        }

        // ステージ4、別攻撃、通常敵は近距離発射を引き続き禁止する
        for (int scenario = 0; scenario < 3; ++scenario) {
            g.m_shots = {};
            g.m_stageNumber = scenario == 0 ? 4 : 5;
            g.m_stage5.tayamaDragonAttack = scenario == 1 ? Attack::BodyBarrage : Attack::Orbit;
            g.SpawnShotDirect(0.5f, 0.0f, g.PlayerRailDepth(), -0.01f, 0.0f, 0.0f,
                true, -1, 0, scenario != 2);
            assert(!g.m_shots[0].active);
        }
        // 第一形態の全フェーズで両サイトが同じ胴体から高速の単発弾を撃つ
        g.m_stageNumber = 5;
        Vector3 firstSource;
        bool hasSource = false;
        for (auto phase : {Game::Stage5Phase::TayamaFireControl,
            Game::Stage5Phase::TayamaLiftEngines, Game::Stage5Phase::TayamaCommandCore}) {
            for (int site = 0; site < 2; ++site) {
                g.m_shots = {};
                g.m_stage5.phase = phase;
                g.m_stage5.phaseTimer = 100;
                g.m_stage5.attackTimer = 80;
                g.m_stage5.guardSpawnCooldown = 999;
                g.m_stage5.tayamaStompCooldown = 999;
                g.m_stage5.tayamaTransformation = 1.0f;
                g.m_stage5.searchlights = {};
                for (auto& light : g.m_stage5.searchlights) {
                    light.phase = Game::SearchlightPhase::Cooldown;
                    light.timer = 100;
                }
                auto& light = g.m_stage5.searchlights[site];
                light.phase = Game::SearchlightPhase::Firing;
                light.timer = 0;
                light.lockedZ = g.PlayerRailDepth();
                Game::Stage5Module::TickBeforeFrame(g);
                int count = 0;
                for (const auto& shot : g.m_shots) {
                    if (!shot.active) continue;
                    ++count;
                    const Vector3 source {shot.x, shot.y, shot.z};
                    if (!hasSource) { firstSource = source; hasSource = true; }
                    assert((source - firstSource).LengthSquared() < 0.00001f);
                    const Vector3 velocity {Game::ToWorldX(shot.vx),
                        Game::ToWorldY(shot.vy), shot.vz};
                    assert(std::abs(velocity.Length() - 1.32f) < 0.001f);
                }
                assert(count == 1);
            }
        }
        std::puts("OrbitShotTests and Tayama laser sight shots passed");
    }

    /** @brief 踏みつけの発動と実モデル状態を検証する @return なし */
    static void RunStomp() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phaseTimer = 100;
        g.m_stage5.guardSpawnCooldown = 999;
        g.m_stage5.tayamaTransformation = 1.0f;
        // 足元で発動し、選択した脚だけが上昇して着地する
        for (auto phase : {Game::Stage5Phase::TayamaFireControl,
            Game::Stage5Phase::TayamaLiftEngines, Game::Stage5Phase::TayamaCommandCore}) {
            g.m_stage5.phase = phase;
            g.m_stage5.tayamaStompTimer = g.m_stage5.tayamaStompCooldown = 0;
            g.Player().m_playerY = Game::FromWorldY(
                ShooterStages::Stage5::TayamaStompTriggerMaxWorldY + 1.0f);
            Game::Stage5Module::TickTayama(g);
            assert(g.m_stage5.tayamaStompTimer == 0);
            g.Player().m_playerY = ShooterStages::Stage5::TayamaPlayerMinY;
            Game::Stage5Module::TickTayama(g);
            assert(g.m_stage5.tayamaStompTimer == 1);
            while (g.m_stage5.tayamaStompTimer < ShooterStages::Stage5::TayamaStompRaiseFrames)
                Game::Stage5Module::TickTayama(g);
            const auto raised = Game::Stage5Module::TayamaState(g);
            const auto foot = g.m_stage5.tayamaStompLeftFoot ?
                TayamaPartGroup::LeftLiftEngine : TayamaPartGroup::RightLiftEngine;
            const auto other = g.m_stage5.tayamaStompLeftFoot ?
                TayamaPartGroup::RightLiftEngine : TayamaPartGroup::LeftLiftEngine;
            assert(raised.collapseOffsets[static_cast<std::size_t>(foot)].position.y ==
                ShooterStages::Stage5::TayamaStompLiftLocalY);
            assert(raised.collapseOffsets[static_cast<std::size_t>(other)].position.y == 0.0f);
            while (g.m_stage5.tayamaStompTimer < ShooterStages::Stage5::TayamaStompImpactFrame)
                Game::Stage5Module::TickTayama(g);
            assert(Game::Stage5Module::TayamaState(g).
                collapseOffsets[static_cast<std::size_t>(foot)].position.y == 0.0f);
        }
        std::puts("Tayama stomp passed");
    }
};

/**
 * @brief 包囲弾と踏みつけの回帰チェックを実行する
 * @param argc 追加引数がある場合は踏みつけのみ検証する
 * @param argv コマンドライン引数（未使用）
 * @return 成功時0
 */
int main(int argc, char** argv) {
    (void)argv;
    OrbitShotTests::RunStomp();
    if (argc > 1) return 0;
    OrbitShotTests::CheckDragonRush();
    OrbitShotTests::Run();
    OrbitShotTests::CheckWallDrone();
    return 0;
}
