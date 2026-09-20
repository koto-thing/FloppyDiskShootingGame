#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

#include "../Engine/Graphics/Renderer.h"
#include "../Engine/Geometry/Sphere.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Common/CityBackgroundModule.h"
#include "../Presentation/Gameplay/Stages/Stage1/Stage1Module.h"
#include "../Presentation/Gameplay/Stages/Stage2/Stage2Module.h"
#include "../Presentation/Gameplay/Stages/Stage3/Stage3Module.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"

/** @brief ステージ固有の演出と被弾判定の協力プレイ回帰チェック */
struct CoopStageTests {
    /** @brief Stage4トラックの車線と描画・接触判定の一致を検証する @return なし */
    static void CheckStage4TruckLanes() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 4;
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
        Renderer renderer;
        Camera3D camera;
        camera.SetViewport({0, 0, 1280, 720});
        // 遠方の透視除算による誤差を避けてワールド位置を検証する
        camera.SetProjectionMode(ProjectionMode::Orthographic);
        Matrix4x4 inverseViewProjection;
        assert((camera.ProjectionMatrix() * camera.ViewMatrix()).TryInverse(inverseViewProjection));

        // 実際の描画コマンドから荷台のワールド位置を取得する
        const auto truckCenter = [&](float railWeight, bool side2D = false) {
            renderer.BeginFrame();
            if (side2D) Game::CityBackgroundModule::DrawBackground2D(g, renderer, camera);
            else Game::CityBackgroundModule::DrawBackground3D(g, renderer, camera, railWeight);
            assert(!renderer.HasOverflowed());
            for (std::size_t i = 0; i < renderer.CommandCount(); ++i) {
                const auto& command = renderer.Command(i);
                const auto& color = command.primitive.color;
                if (command.type == RenderCommand::Type::Primitive3D &&
                    color.r == 0.11f && color.g == 0.04f && color.b == 0.24f) {
                    return (inverseViewProjection * command.primitive.wvpMatrix).TransformPoint(Vector3::Zero);
                }
            }
            assert(false && "Stage4 truck must be drawn");
            return Vector3::Zero;
        };

        // 2巡分を確認し、奥から手前へ接近する間は同じ車線を保つ
        constexpr float Lanes[] = {3.0f, -3.0f, 9.0f, -9.0f};
        for (int pass = 0; pass < 8; ++pass) {
            for (float distance : {219.875f, 110.0f, 37.0f, 0.125f}) {
                if (pass == 0 && distance > 37.0f) continue;
                g.m_scroll = (37.0f + pass * 220.0f - distance) / 92.0f;
                const Vector3 rail = truckCenter(1.0f);
                assert(std::abs(rail.x - Lanes[pass % 4]) < 0.001f);
                assert(std::abs(rail.z - (12.0f + distance)) < 0.001f);
                for (float lane : Lanes) {
                    assert(Game::CityBackgroundModule::HitsTruck(g,
                        Game::FromWorldX(lane), Game::FromWorldY(rail.y), rail.z, 0.0f) ==
                        (lane == Lanes[pass % 4]));
                }

                // 両方向の視点補間が通常2Dと同じ位置から同じ車線へつながる
                const Vector3 side = truckCenter(0.0f, true);
                for (float weight : {0.0f, 0.5f, 1.0f, 0.5f, 0.0f}) {
                    const Vector3 expected = Vector3::Lerp(side, rail, weight);
                    assert((truckCenter(weight) - expected).LengthSquared() < 0.0001f);
                }
            }
        }
    }

    /** @brief 実際のステージ処理をGPUなしで検証する @return なし */
    static void Run() {
        CheckStage4TruckLanes();
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_playerCount = 2;
        g.m_clear = true;
        g.m_players[0].m_playerX = g.m_players[1].m_playerX = 0.0f;

        // Stage1のボス撃破演出で両者が退避する
        Game::Stage1Module::TickBossDefeat(g);
        assert(g.m_players[0].m_playerX < 0.0f);
        assert(g.m_players[1].m_playerX == g.m_players[0].m_playerX);
        g.m_clear = false;

        // 第2部の全Chapterで2D退場は壁面のX/Zを保ち、3Dの既存退場方向も維持する
        for (int stage : {4, 5}) {
            g.m_stageNumber = stage;
            for (auto phase : {Game::Stage5Phase::WallClimbTransition,
                    Game::Stage5Phase::WallClimbLower, Game::Stage5Phase::WallClimbMiddle,
                    Game::Stage5Phase::WallClimbUpper, Game::Stage5Phase::RooftopArrival}) {
                g.m_stage5.phase = phase;
                for (bool rail : {false, true}) {
                    g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                    auto& enemy = g.m_enemies[0];
                    enemy = {};
                    enemy.active = true;
                    enemy.x = 1.0f;
                    enemy.y = enemy.baseY = 0.5f;
                    enemy.z = 40.0f;
                    g.TickChapterExitEnemies();
                    const bool downward = stage == 5 &&
                        (phase == Game::Stage5Phase::WallClimbUpper ||
                            (!rail && (phase == Game::Stage5Phase::WallClimbLower ||
                                phase == Game::Stage5Phase::WallClimbMiddle)));
                    assert(!enemy.collisionEnabled);
                    if (downward) {
                        assert(enemy.y < 0.5f && enemy.baseY == enemy.y);
                        assert(enemy.x == 1.0f && enemy.z == 40.0f);
                    } else {
                        assert(enemy.y == 0.5f);
                        assert(rail ? enemy.z < 40.0f : enemy.x < 1.0f);
                    }
                    for (int frame = 0; frame < 300; ++frame) g.TickChapterExitEnemies();
                    assert(!enemy.active);
                }
            }
        }

        // 1Pが無敵でも2Pだけに当たるStage2/3レーザーを両視点で検知する
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            g.m_players[0].m_playerX = -1.0f;
            g.m_players[0].m_invincible = 100;
            g.m_players[1].m_playerX = g.m_players[1].m_playerY = 0.0f;
            g.m_players[1].m_playerDestructionTimer = 0;
            g.m_stageNumber = 2;
            g.m_stage = &Game::Stage2Module::Definition(Easy);
            g.m_stage2.boneArchDestroyed = true;
            g.m_stage2.boss.actionAge = 120;
            auto& boss = g.m_enemies[0];
            boss = {};
            boss.active = true;
            boss.phase = 3.0f;
            boss.x = 1.0f;
            boss.z = 40.0f;
            boss.actionZ = Game::PlayerRailZ;
            boss.bossPartHp[Game::BossNose] = 10;
            assert(!Game::Stage2Module::HandleBossInteractionAfterTick(g, boss));
            assert(g.m_players[0].m_playerDestructionTimer == 0);
            assert(g.m_players[1].m_playerDestructionTimer > 0);

            g.m_players[1].m_playerDestructionTimer = 0;
            g.m_stageNumber = 3;
            g.m_stage = &Game::Stage3Module::Definition(Easy);
            boss.phase = 5.0f;
            boss.motionAge = Game::Stage3Module::Phase2SurvivalFrames - 120;
            g.m_stage3.laserTargetInitialized = true;
            g.m_stage3.laserTargetX = g.m_stage3.laserTargetY = 0.0f;
            g.m_stage3.laserTargetZ = rail ? Game::PlayerRailZ : Game::SidePlaneZ;
            Game::Stage3Module::FireBossPartBarrage(g, boss);
            assert(g.m_players[0].m_playerDestructionTimer == 0);
            assert(g.m_players[1].m_playerDestructionTimer > 0);
        }

        // 雲海への遷移は両者のボムを終了し、位置と無敵時間を設定する
        g.m_stageNumber = 5;
        g.m_stage = &Game::Stage5Module::Definition(Easy);
        g.m_stage5.phase = Game::Stage5Phase::TayamaCollapse;
        g.m_stage5.phaseTimer = ShooterStages::Stage5::TayamaCollapseFrames - 1;
        for (auto& player : g.m_players) {
            player.m_playerDestructionTimer = player.m_invincible = 0;
            player.m_bomb.active = true;
            player.m_playerY = 10.0f;
        }
        Game::Stage5Module::TickBeforeFrame(g);
        assert(g.m_stage5.phase == Game::Stage5Phase::CloudSea);
        for (const auto& player : g.m_players) {
            assert(!player.m_bomb.active && player.m_invincible > 0);
            assert(player.m_playerY == 0.0f);
        }
        assert(g.m_players[0].m_playerX != g.m_players[1].m_playerX);

        // 2Pの追尾弾から参照しても同じ龍の節へ命中する
        g.m_stage5.phase = Game::Stage5Phase::TayamaDragonBattle;
        g.m_stage5.tayamaDragonAttack = ShooterStages::Stage5::TayamaDragonAttack::Orbit;
        g.m_stage5.tayamaDragonAttackTimer = ShooterStages::Stage5::TayamaDragonOrbitMoveFrames + 1;
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            Vector3 firstTarget;
            Vector3 secondTarget;
            g.m_activePlayer = 0;
            assert(Game::Stage5Module::GetHomingTarget(g, 3, firstTarget));
            g.m_activePlayer = 1;
            assert(Game::Stage5Module::GetHomingTarget(g, 3, secondTarget));
            assert((firstTarget - secondTarget).LengthSquared() < 0.0001f);
            assert(g.m_activePlayer == 1);
        }

        // 龍の頭部レーザーも1Pの無敵状態と独立して2Pに命中する
        g.m_activePlayer = 0;
        g.m_players[0].m_invincible = 100;
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            g.m_players[1].m_invincible = g.m_players[1].m_playerDestructionTimer = 0;
            g.m_stage5.tayamaDragonAttack = ShooterStages::Stage5::TayamaDragonAttack::HeadLaser;
            g.m_stage5.tayamaDragonAttackTimer = ShooterStages::Stage5::TayamaHeadLaserWarningFrames - 1;
            g.m_stage5.headLaserArmed = true;
            g.m_activePlayer = 1;
            g.m_stage5.headLaserTarget = g.PlayerWorldPosition();
            g.m_activePlayer = 0;
            Game::Stage5Module::TickBeforeFrame(g);
            assert(g.m_players[0].m_playerDestructionTimer == 0);
            assert(g.m_players[1].m_playerDestructionTimer > 0);
        }

        // 最終爆発は一度だけ破裂点を固定し、残光が消えるまでクリアへ進めない
        using namespace ShooterStages::Stage5;
        g.m_clear = false;
        g.m_explosions = {};
        g.m_stage5.phase = Phase::TayamaDragonCollapse;
        g.m_stage5.phaseTimer = TayamaDragonCollapseHeadExplosionFrame - 1;
        Game::Stage5Module::TickBeforeFrame(g);
        const Vector3 blastCenter = g.m_stage5.tayamaDragonExplosionCenter;
        assert(blastCenter.LengthSquared() > 0.0f);
        auto renderer = std::make_unique<Renderer>();
        Camera3D camera;
        camera.SetViewport({0, 0, 1280, 720});
        camera.SetPosition({0.0f, 12.0f, -24.0f});
        camera.LookAt({0.0f, 9.0f, 62.0f});
        renderer->BeginFrame();
        Game::Stage5Module::DrawOverlay3D(g, *renderer, camera);
        const auto& flash = renderer->Command(renderer->CommandCount() - 1);
        assert(flash.type == RenderCommand::Type::Rect);
        assert(flash.color.r == 1.0f && flash.color.g == 1.0f &&
            flash.color.b == 1.0f && flash.color.a == 1.0f);
        for (int age = 1; age < TayamaDragonFinalAfterglowFrames; ++age) {
            ++g.m_frame;
            Game::Stage5Module::TickBeforeFrame(g);
            assert(g.m_stage5.phase == Phase::TayamaDragonCollapse && !g.m_clear);
            assert((g.m_stage5.tayamaDragonExplosionCenter - blastCenter).LengthSquared() == 0.0f);

            // 火球・衝撃波・光片の描画を両視点と補間中で確認する
            if (age != 36 && age != TayamaDragonFinalAfterglowFrames - 1) continue;
            for (float railWeight : {0.0f, 0.5f, 1.0f}) {
                renderer->BeginFrame();
                Game::Stage5Module::DrawCloudSeaWorld(g, *renderer, camera, railWeight);
                int explosions = 0;
                int rays = 0;
                for (std::size_t index = 0; index < renderer->CommandCount(); ++index) {
                    const auto& command = renderer->Command(index);
                    const bool explosion = command.type == RenderCommand::Type::Explosion;
                    const bool ray = command.type == RenderCommand::Type::Railgun;
                    if (!explosion && !ray) continue;
                    explosions += explosion ? 1 : 0;
                    rays += ray ? 1 : 0;
                    const auto& matrix = explosion ? command.explosion.wvpMatrix : command.railgun.wvpMatrix;
                    for (const auto& row : matrix.m) {
                        for (float value : row) assert(std::isfinite(value));
                    }
                }
                if (age == 36) assert(explosions > 10 && rays > 20);
                else assert(explosions == 0 && rays == 0);
            }
        }
        Game::Stage5Module::TickBeforeFrame(g);
        assert(g.m_stage5.phase == Phase::EndingReady && g.m_clear);

        // 夜から朝への変化はHPだけに連動し、両視点と双方向の遷移中も色を共有する
        g.m_stage5.tayamaMaxHp = TayamaDragonMaxHp;
        for (float railWeight : {0.0f, 0.5f, 1.0f, 0.5f, 0.0f}) {
            std::array<ColorF, 6> previous {};
            float previousStarAlpha = 1.0f;
            for (int step = 0; step <= 4; ++step) {
                g.m_stage5.phase = Phase::TayamaDragonBattle;
                g.m_stage5.tayamaHp = TayamaDragonMaxHp * (4 - step) / 4;
                renderer->BeginFrame();
                Game::Stage5Module::DrawSky(g, *renderer);
                Game::Stage5Module::DrawCloudSeaWorld(g, *renderer, camera, railWeight);
                assert(!renderer->HasOverflowed());

                // 空・地平線・雲の影・明部・上面・遠景を実際の描画コマンドから確認する
                constexpr std::size_t StarCount = 96;
                const std::size_t cloudStart = 2 + (step < 4 ? StarCount : 0);
                // 遠景が直方体へ戻らず、雲の丸い輪郭を波の五層とも保つ
                for (std::size_t cloud = 0; cloud < 5 * 19 * 3; ++cloud) {
                    const auto& command = renderer->Command(cloudStart + 480 + cloud);
                    assert(command.type == RenderCommand::Type::Primitive3D);
                    assert(command.primitive.shape == PrimitiveShape::Sphere);
                }
                const std::array<ColorF, 6> colors {renderer->Command(0).color,
                    renderer->Command(1).color,
                    renderer->Command(cloudStart).primitive.color,
                    renderer->Command(cloudStart + 1).primitive.color,
                    renderer->Command(cloudStart + 3).primitive.color,
                    renderer->Command(cloudStart + 480).primitive.color};
                for (std::size_t index = 0; index < colors.size(); ++index) {
                    const auto& color = colors[index];
                    assert(color.a == 1.0f);
                    if (step > 0) {
                        assert(color.r > previous[index].r);
                        assert(color.g > previous[index].g);
                        assert(color.b > previous[index].b);
                    }
                }
                const float starAlpha = step < 4 ? renderer->Command(2).primitive.color.a : 0.0f;
                assert(step == 0 ? starAlpha == 1.0f : starAlpha < previousStarAlpha);
                previousStarAlpha = starAlpha;
                previous = colors;

                // 開幕と撃破後の色を保ち、フレーム経過だけでは夜明けを進めない
                if (step == 0) g.m_stage5.phase = Phase::CloudSea;
                if (step == 4) g.m_stage5.phase = Phase::EndingReady;
                g.m_frame += 600;
                renderer->BeginFrame();
                Game::Stage5Module::DrawSky(g, *renderer);
                Game::Stage5Module::DrawCloudSeaWorld(g, *renderer, camera, railWeight);
                const ColorF cloud = renderer->Command(cloudStart).primitive.color;
                assert(cloud.r == colors[2].r && cloud.g == colors[2].g && cloud.b == colors[2].b);
                assert(renderer->Command(0).color.r == colors[0].r);
            }
        }

        // 通常2Dと遷移終点のカメラを一致させ、奥の雲をクリップ範囲に収める
        g.m_playerCount = 1;
        g.m_stage5.phase = Phase::TayamaDragonBattle;
        g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Side2D;
        g.m_viewTransitionTimer = 0;
        g.m_screenShakeFrames = 0;
        Camera3D sideCamera;
        Camera3D transitionCamera;
        g.ConfigureSideCamera(sideCamera, *renderer);
        g.ConfigureRailCamera(transitionCamera, *renderer);
        assert(sideCamera.FarClip() > 230.0f);
        assert(sideCamera.FieldOfView() == transitionCamera.FieldOfView());
        assert(sideCamera.FarClip() == transitionCamera.FarClip());

        // 朝焼けの帯の上端から雲海の床まで、両視点と遷移中の全幅を雲の壁で覆う
        for (float weight : {0.0f, 0.5f, 1.0f, 0.5f, 0.0f}) {
            g.m_viewMode = Game::ViewMode::Side2D;
            g.m_nextViewMode = Game::ViewMode::Rail3D;
            g.m_viewTransitionTimer = 1;
            g.m_viewTransitionProgress = weight;
            g.m_stage5.phase = Phase::EndingReady;
            g.ConfigureRailCamera(camera, *renderer);
            camera.SetViewport({0, 0, 1920, 1080});
            renderer->BeginFrame();
            Game::Stage5Module::DrawCloudSeaWorld(g, *renderer, camera, weight);
            std::array<Matrix4x4, 5 * 19 * 3> inverseClouds;
            for (std::size_t index = 0; index < inverseClouds.size(); ++index) {
                assert(renderer->Command(480 + index).primitive.wvpMatrix.TryInverse(inverseClouds[index]));
            }
            for (int x = -20; x <= 20; ++x) {
                for (float y : {0.26f, 0.0f, -0.15f}) {
                    bool covered = false;
                    for (const auto& inverse : inverseClouds) {
                        const Vector3 nearPoint = inverse.TransformPoint({x * 0.05f, y, 0.0f});
                        const Vector3 farPoint = inverse.TransformPoint({x * 0.05f, y, 1.0f});
                        if (Ray {nearPoint, farPoint - nearPoint}.Intersects(Sphere {{}, 0.5f})) {
                            covered = true;
                            break;
                        }
                    }
                    assert(covered && "Cloud wave must cover the rectangular horizon band");
                }
            }
        }
    }
};

/** @brief ステージ固有の協力プレイ回帰チェックを実行する @return なし */
void RunCoopStageTests() {
    CoopStageTests::Run();
    std::puts("CoopStageTests passed");
}
