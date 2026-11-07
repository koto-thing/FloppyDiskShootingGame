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
#include "../Presentation/Gameplay/Stages/Stage3/Stage3BarrierCageView.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5ModelView.h"
#include "../Presentation/Gameplay/Stages/Stage4/Stage4Module.h"
#include "../Presentation/Gameplay/Stages/Common/StageDefinition.h"

/** @brief ステージ固有の演出と被弾判定の協力プレイ回帰チェック */
struct CoopStageTests {
    /** @brief 骨の初回案内、切り替え、リトライとステージ再開始を検証する @return なし */
    static void CheckStage2ViewHint() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 2;
        g.m_viewMode = Game::ViewMode::Side2D;
        g.m_scroll = 3.15f;
        Game::Stage2Module::TickWorld(g);
        assert(!g.m_stage2.viewHintShown && g.m_stage2.viewHintTimer == 0);
        g.m_scroll = 3.17f;
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintShown && g.m_stage2.viewHintTimer == 240);
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintTimer == 239);

        // 3Dへの遷移開始で消え、2Dへ戻しても再表示しない
        g.RequestViewMode(Game::ViewMode::Rail3D);
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintTimer == 0);
        g.m_viewTransitionTimer = 0;
        g.m_viewMode = Game::ViewMode::Side2D;
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintTimer == 0);

        // リトライでは表示済み状態を維持し、新しい開始では初期化する
        g.m_isRestartingChapter = true;
        Game::Stage2Module::Reset(g);
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintShown && g.m_stage2.viewHintTimer == 0);
        g.m_isRestartingChapter = false;
        Game::Stage2Module::Reset(g);
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintTimer == 240);
        for (int i = 0; i < 240; ++i) Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintTimer == 0);

        // 破壊済みまたは3Dで通過した最初の骨には案内を出さない
        Game::Stage2Module::Reset(g);
        g.m_stage2.boneArchDestroyed = true;
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintTimer == 0);
        Game::Stage2Module::Reset(g);
        g.m_viewMode = Game::ViewMode::Rail3D;
        Game::Stage2Module::TickWorld(g);
        assert(g.m_stage2.viewHintShown && g.m_stage2.viewHintTimer == 0);
    }

    /** @brief 太陽の画面上の命中位置、耐久値、視点制限とリセットを検証する @return なし */
    static void CheckStage3Sun() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 3;
        g.m_frame = 800;
        g.m_viewMode = Game::ViewMode::Side2D;
        g.m_viewTransitionTimer = 0;
        for (const Viewport viewport : {Viewport{0, 0, 1280, 720}, Viewport{0, 0, 800, 600}}) {
            Game::Stage3Module::Reset(g);
            g.m_aimViewport = viewport;
            const float halfHeight = 46.0f * std::tan(Math::ToRadians(38.0f) * 0.5f) * 1.01f;
            const float projection = 25.6f / 37.6f;
            Game::Shot shot {};
            shot.active = true;
            shot.x = Game::FromWorldX(halfHeight * viewport.AspectRatio() * 0.56f * projection);
            shot.y = Game::FromWorldY(halfHeight * 0.56f * projection);
            shot.damage = 1;
            shot.hitRadius = 0.02f;
            // 敵弾、3D、視点遷移中、外れ弾は太陽を傷つけない
            shot.enemy = true;
            assert(!Game::Stage3Module::TryDamageStageTarget(g, shot));
            shot.enemy = false;
            g.m_viewMode = Game::ViewMode::Rail3D;
            assert(!Game::Stage3Module::TryDamageStageTarget(g, shot));
            g.m_viewMode = Game::ViewMode::Side2D;
            g.m_viewTransitionTimer = 1;
            assert(!Game::Stage3Module::TryDamageStageTarget(g, shot));
            g.m_viewTransitionTimer = 0;
            shot.y -= 0.5f;
            assert(!Game::Stage3Module::TryDamageStageTarget(g, shot));
            shot.y += 0.5f;
            // 両プレイヤーのダメージを合算し、20発目だけで演出を開始する
            for (int hit = 1; hit <= 20; ++hit) {
                shot.active = true;
                shot.owner = hit % 2;
                assert(Game::Stage3Module::TryDamageStageTarget(g, shot));
                assert(!shot.active && g.m_stage3.sunHp == 20 - hit);
                assert(g.m_stage3.sunCryStartFrame == (hit == 20 ? 800 : -1));
            }
            // 泣き顔の待ち時間、夜への補間、視点切替後の夜の維持を描画色で確認する
            Renderer renderer;
            float previousRed = 1.0f;
            for (int age : {0, 60, 120, 180, 600}) {
                g.m_frame = 800 + age;
                g.m_viewMode = age == 600 ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                renderer.BeginFrame();
                Game::Stage3Module::DrawSky(g, renderer);
                const float red = renderer.Command(0).color.r;
                assert(red <= previousRed);
                if (age <= 60) assert(std::abs(red - 0.30f) < 0.0001f);
                if (age >= 180) assert(std::abs(red - 0.015f) < 0.0001f);
                previousRed = red;
            }
            g.m_viewMode = Game::ViewMode::Side2D;
            g.m_frame = 1000;
            assert(!Game::Stage3Module::TryDamageStageTarget(g, shot));
            assert(g.m_stage3.sunCryStartFrame == 800);
            Game::Stage3Module::Reset(g);
            assert(g.m_stage3.sunHp == 20 && g.m_stage3.sunCryStartFrame == -1);
            g.m_frame = 800;
            // 移動区間が太陽を横切る高速弾と、大ダメージでもHPは負にならない
            shot.active = true;
            shot.x += 0.3f;
            shot.vx = 0.6f;
            shot.damage = 80;
            assert(Game::Stage3Module::TryDamageStageTarget(g, shot));
            assert(g.m_stage3.sunHp == 0);
        }
    }
    /** @brief EASTSOURCEボムの追尾期限、予告時間、バリア耐久を両視点で検証する @return なし */
    static void CheckEastsourceBombs() {
        using Game = SideScrollingShooter;
        using namespace ShooterStages::Stage5;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage = &Game::Stage5Module::Definition(Easy);
        g.m_playerCount = 2;
        for (bool rail : {false, true}) {
            g.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            g.m_viewTransitionTimer = 0;
            g.m_stage5.phase = Phase::EastsourceBattle;
            g.m_stage5.eastsourceLaserTimer = -1;
            g.m_stage5.eastsourceShieldHp = 0;
            auto& boss = g.m_enemies[0];
            g.m_stage->ConfigureBoss(boss, rail);
            g.m_stage->ConfigureBossPartHp(boss);
            if (!rail) g.m_stage->ConfigureBossSideAnchor(boss);
            g.ResetShots();
            // ボムと同じ10発を生成し、両機体を狙うことを確認する
            for (int frame = 0; frame < 10 * Game::BombMissileInterval; ++frame) {
                boss.age = EastsourceBombStartFrames + frame;
                g.m_stage->TickBoss(g, boss);
            }
            int missiles = 0;
            int targets = 0;
            for (auto& shot : g.m_shots) {
                if (!shot.active || shot.stage2.kind != ShooterStages::Stage2::ShotKind::Funnel) continue;
                ++missiles;
                targets |= 1 << shot.owner;
                for (int frame = 0; frame < EastsourceMissileHomingFrames; ++frame)
                    Game::Stage5Module::TickSpecialShotBeforeMove(g, shot);
                const Vector3 velocity {Game::ToWorldX(shot.vx), Game::ToWorldY(shot.vy), shot.vz};
                assert(std::abs(velocity.Length() - EastsourceMissileSpeed) < 0.0001f);
                assert(velocity.Length() < Game::BombMissileSpeed * 0.4f);
                g.m_players[shot.owner].m_playerY += 0.8f;
                Game::Stage5Module::TickSpecialShotBeforeMove(g, shot);
                assert(shot.vx == Game::FromWorldX(velocity.x) && shot.vy == Game::FromWorldY(velocity.y));
                assert(shot.vz == velocity.z);
            }
            assert(missiles == 10 && targets == 3);

            // 予告180フレームは無傷、181回目の更新で予告方向上の両機へ命中する
            boss.age = EastsourceBombStartFrames + EastsourceBombIntervalFrames;
            g.m_players[0].m_playerX = g.m_players[1].m_playerX = -0.6f;
            g.m_players[0].m_playerY = g.m_players[1].m_playerY = 0.0f;
            g.ForEachPlayer([&] {
                g.Player().m_invincible = 0;
                g.Player().m_playerDestructionTimer = 0;
                g.Player().m_bomb = {};
                g.Player().m_bomb.type = Spread;
                g.Player().m_bomb.active = true;
            });
            for (int frame = 0; frame < EastsourceLaserWarningFrames; ++frame) {
                g.m_stage->TickBoss(g, boss);
                ++boss.age;
                assert(g.m_players[0].m_bomb.shieldHits == 0 && g.m_players[1].m_bomb.shieldHits == 0);
            }
            g.m_stage->TickBoss(g, boss);
            assert(g.m_stage5.eastsourceLaserTimer == 180);
            assert(g.m_players[0].m_bomb.shieldHits == 1 && g.m_players[1].m_bomb.shieldHits == 1);
            g.RequestViewMode(rail ? Game::ViewMode::Side2D : Game::ViewMode::Rail3D);
            ++boss.age;
            g.m_stage->TickBoss(g, boss);
            assert(g.m_stage5.eastsourceLaserTimer == 0);

            // 高耐久バリアは貫通弾・ボムを吸収して本体HPを保護する
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            g.m_viewTransitionTimer = 0;
            g.m_stage5.eastsourceLaserTimer = -1;
            boss.age = EastsourceBombStartFrames + EastsourceBombIntervalFrames * 2;
            g.m_stage->TickBoss(g, boss);
            assert(g.m_stage5.eastsourceShieldHp == EastsourceShieldHp);
            const int hp = boss.hp;
            for (bool bomb : {false, true}) {
                Game::Shot shot;
                shot.x = boss.x;
                shot.y = boss.y;
                shot.z = boss.z;
                shot.damage = 8;
                shot.active = shot.piercing = true;
                shot.bomb = bomb;
                g.HitPlayerShotTargets(shot);
                assert(!shot.active && boss.hp == hp);
            }
            assert(g.m_stage5.eastsourceShieldHp == EastsourceShieldHp - 16);
            g.m_stage5.eastsourceShieldHp = 1;
            Game::Shot finalShot;
            finalShot.x = boss.x;
            finalShot.y = boss.y;
            finalShot.z = boss.z;
            finalShot.active = true;
            g.HitPlayerShotTargets(finalShot);
            assert(g.m_stage5.eastsourceShieldHp == 0 && boss.hp == hp);
            Game::Stage5Module::Reset(g);
            assert(g.m_stage5.eastsourceLaserTimer == -1 && g.m_stage5.eastsourceShieldHp == 0);
        }
        // 全パーツの外接球より大きいバリアで翼端と機首まで覆う
        EastsourceModelView::VisitParts({{}, {}, 0.72f}, {},
            [&](PrimitiveShape, const Matrix4x4& world, const ColorF&, EastsourcePartGroup) {
                assert(world.TransformPoint(Vector3::Zero).Length() +
                    Stage5ModelDetail::WorldPartRadius(world) <= EastsourceShieldRadius);
            });
        // 両視点と遷移途中でも予告線・照射本体・バリアの描画を維持する
        auto renderer = std::make_unique<Renderer>();
        Camera3D camera;
        camera.SetViewport({0, 0, 1280, 720});
        g.m_stage5.phase = Phase::EastsourceBattle;
        g.m_stage5.eastsourceLaserDirection = {-1.0f, 0.0f, 0.0f};
        g.m_viewMode = Game::ViewMode::Side2D;
        g.m_nextViewMode = Game::ViewMode::Rail3D;
        g.m_viewTransitionTimer = 1;
        for (float weight : {0.0f, 0.5f, 1.0f}) {
            g.m_viewTransitionProgress = weight;
            for (int timer : {179, 180}) {
                g.m_stage5.eastsourceLaserTimer = timer;
                g.m_stage5.eastsourceShieldHp = EastsourceShieldHp;
                renderer->BeginFrame();
                assert(Game::Stage5Module::DrawBossModel(g, *renderer, camera, g.m_enemies[0]));
                int beams = 0;
                for (std::size_t index = 0; index < renderer->CommandCount(); ++index) {
                    const auto& command = renderer->Command(index);
                    if (command.type != RenderCommand::Type::Railgun) continue;
                    ++beams;
                    for (const auto& row : command.railgun.wvpMatrix.m)
                        for (float value : row) assert(std::isfinite(value));
                }
                assert(beams == (timer == 179 ? 1 : 2));
                assert(!renderer->HasOverflowed());
                const auto intact = renderer->CommandCount();
                g.m_stage5.eastsourceShieldHp = EastsourceShieldHp / 3;
                renderer->BeginFrame();
                Game::Stage5Module::DrawBossModel(g, *renderer, camera, g.m_enemies[0]);
                assert(renderer->CommandCount() + 14 * 12 == intact);
            }
        }
    }

    /** @brief EASTSOURCEの2D配置と全フェーズの弾数をKOTOと比較する @return なし */
    static void CheckEastsourceBattle() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto kotoGame = std::make_unique<Game>();
        auto& g = *game;
        auto& k = *kotoGame;
        g.m_stageNumber = 5;
        g.m_stage = &Game::Stage5Module::Definition(Easy);
        g.m_stage5.phase = Game::Stage5Phase::EastsourceBattle;
        k.m_stageNumber = 1;
        k.m_stage = &Game::Stage1Module::Definition(Easy);
        for (bool rail : {false, true}) {
            g.m_viewMode = k.m_viewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            for (auto phase : {Game::BossNormalPhase1, Game::BossSpecialPhase1,
                Game::BossNormalPhase2, Game::BossSpecialPhase2}) {
                g.m_stage5.eastsourceLaserTimer = -1;
                auto& boss = g.m_enemies[0];
                auto& koto = k.m_enemies[0];
                g.m_stage->ConfigureBoss(boss, rail);
                g.m_stage->ConfigureBossPartHp(boss);
                if (!rail) g.m_stage->ConfigureBossSideAnchor(boss);
                k.m_stage->ConfigureBoss(koto, rail);
                k.m_stage->ConfigureBossPartHp(koto);
                if (!rail) k.m_stage->ConfigureBossSideAnchor(koto);
                boss.bossPhase = koto.bossPhase = phase;
                int eastsourceShots = 0;
                int kotoShots = 0;
                // 発射直後に回収し、弾プール上限や衝突から独立して同じ時間の発射数を比較する
                for (int frame = 0; frame < 840; ++frame) {
                    boss.age = koto.age = frame;
                    g.m_stage->TickBoss(g, boss);
                    k.m_stage->TickBoss(k, koto);
                    if (!rail) {
                        assert(std::abs(boss.x - koto.baseX) < 0.0001f);
                        assert(std::abs(boss.z - Game::ToRailZFromSideX(boss.x)) < 0.0001f);
                    }
                    if (frame % k.m_stage->BossAttackInterval(phase) == 0) k.FireBossPartBarrage(koto);
                    if (frame % koto.shotInterval == 0) ++kotoShots;
                    for (auto& shot : g.m_shots) {
                        if (shot.active && shot.enemy) ++eastsourceShots;
                        shot.active = false;
                    }
                    for (auto& shot : k.m_shots) {
                        if (shot.active && shot.enemy) ++kotoShots;
                        shot.active = false;
                    }
                }
                assert(eastsourceShots > kotoShots);
            }
        }
        // 往復の座標初期化後も共通アンカーから右側の移動を継続する
        g.m_viewMode = Game::ViewMode::Side2D;
        g.InitializeSideObjects();
        g.m_stage->TickBoss(g, g.m_enemies[0]);
        assert(g.m_enemies[0].x > 1.7f);
        g.m_viewMode = Game::ViewMode::Rail3D;
        g.InitializeRailObjects();
        g.m_enemies[0].bossPhase = Game::BossNormalPhase1;
        g.m_stage->TickBoss(g, g.m_enemies[0]);
        assert(std::abs(g.m_enemies[0].x) <= 0.58f);
    }

    /** @brief EASY道中の削減後の敵数と4面より5面が多いことを検証する @return なし */
    static void CheckEasyEnemyCounts() {
        using Game = SideScrollingShooter;
        int counts[2] = {};
        const Game::Stage* stages[] = {
            &Game::Stage4Module::Definition(Easy), &Game::Stage5Module::Definition(Easy)
        };
        // 全チャプターの出現規則を走査し、連結レーザーは2体として数える
        for (int i = 0; i < 2; ++i) {
            for (int frame = 0; frame < stages[i]->ChapterFrameLength() * 3; ++frame) {
                Game::Stage::EnemySpawnRule spawn;
                int chapter = 0;
                for (int index = 0; stages[i]->TrySelectEnemySpawn(frame, index, spawn, chapter); ++index) {
                    counts[i] += spawn.enemyType == Game::Stage::LinkedLaserEnemy ? 2 : 1;
                }
            }
        }
        assert(counts[0] == 79 && counts[0] < 111);
        assert(counts[1] == 106 && counts[1] < 160);
        assert(counts[0] < counts[1]);
    }

    /** @brief 第2部道中の協力カメラと連射時の弾プール回収を検証する @return なし */
    static void CheckStage5RouteCameraAndShots() {
        using Game = SideScrollingShooter;
        using namespace ShooterStages::Stage5;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage = &Game::Stage5Module::Definition(Easy);
        g.m_playerCount = 2;
        const Viewport viewport {0, 0, 800, 600};
        for (auto phase : {Phase::WallClimbLower, Phase::WallClimbMiddle, Phase::WallClimbUpper}) {
            g.m_stage5.phase = phase;
            g.m_stage5.phaseTimer = WallClimbFadeFrames;
            // 上下左右の端へ離れても、カメラをシャフトの外へ出さず両機を映す
            for (int upper : {0, 1}) {
                g.m_players[upper].m_playerX = Game::Side2DPlayerMinX;
                g.m_players[upper].m_playerY = Part2RailPlayerMaxY;
                g.m_players[1 - upper].m_playerX = Game::Side2DPlayerMaxX;
                g.m_players[1 - upper].m_playerY = Part2RailPlayerMinY;
                for (bool entering : {true, false}) {
                    g.m_viewMode = entering ? Game::ViewMode::Side2D : Game::ViewMode::Rail3D;
                    g.m_nextViewMode = entering ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                    g.m_viewTransitionTimer = 1;
                    for (int step = 0; step <= 20; ++step) {
                        g.m_viewTransitionProgress = step / 20.0f;
                        Camera3D camera;
                        g.ConfigureRailCamera(camera, viewport);
                        if (g.RailBlend() == 1.0f) {
                            assert(std::abs(camera.Position().z - (Game::PlayerRailZ - 9.0f)) < 0.001f);
                            assert(std::abs(camera.Position().x) < Game::ToWorldX(ShaftHalfWidth));
                            assert(std::abs(camera.Position().y) < Game::ToWorldY(ShaftHalfHeight));
                        }
                        g.ForEachPlayer([&] {
                            Vector3 position = g.PlayerWorldPosition();
                            position.z = Math::Lerp(Game::SidePlaneZ, g.PlayerRailDepth(), g.RailBlend());
                            Vector2 screen;
                            float depth;
                            assert(camera.TryWorldToScreen(position, screen, &depth));
                            assert(viewport.Contains(screen) && depth > 0.0f && depth < 1.0f);
                        });
                    }
                }
            }

            // 2Pの追尾と命中判定も、奥行きを保った敵の位置へ届く
            g.m_viewTransitionTimer = 0;
            g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
            g.m_aimViewport = viewport;
            auto& enemy = g.m_enemies[0];
            enemy = {};
            enemy.active = enemy.collisionEnabled = true;
            enemy.hp = 10;
            enemy.x = 0.3f;
            enemy.y = 0.2f;
            enemy.z = 34.0f;
            Game::Shot shot {};
            shot.z = g.PlayerRailDepth() + 2.0f;
            shot.special = shot.active = true;
            Vector3 target;
            g.m_activePlayer = 1;
            assert(g.FindShotTarget(shot, target) == 0);
            assert(std::abs(target.z - enemy.z) < 0.001f);
            shot.x = enemy.x;
            shot.y = enemy.y;
            shot.z = enemy.z;
            g.HitPlayerShotTargets(shot);
            assert(enemy.hp == 9 && g.m_activePlayer == 1);
            enemy.active = false;
            g.m_activePlayer = 0;

            // 最も長く弾が残る下端から、全機体を最大Powerで二人同時に連射する
            for (bool rail : {false, true}) {
                g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                for (auto type : {Homing, Piercing, Spread}) {
                    g.m_shots = {};
                    for (auto& player : g.m_players) {
                        player.m_playerType = type;
                        player.m_power = 4.0f;
                        player.m_playerX = 0.0f;
                        player.m_playerY = rail ? Part2RailPlayerMinY : Game::Side2DPlayerMinY;
                        player.m_shotCooldown = player.m_specialShotCooldown = 0;
                        player.m_fire = true;
                    }
                    assert(std::abs(g.MakeNormalPlayerShot(false).vy - (rail ? 0.0f : 0.18f)) < 0.00001f);
                    for (int frame = 0; frame < 1800; ++frame) {
                        g.m_frame = frame;
                        g.ForEachPlayer([&] {
                            g.Player().m_fire = frame < 1200;
                            g.Player().m_shotCooldown = (std::max)(0, g.Player().m_shotCooldown - 1);
                            g.Player().m_specialShotCooldown = (std::max)(0, g.Player().m_specialShotCooldown - 1);
                            g.TickPlayerWeapons();
                        });
                        g.TickShots();
                        int active = 0;
                        for (const auto& shot : g.m_shots) active += shot.active;
                        assert(active < g.ActiveShotCapacity() * 3 / 4);
                        if (frame == 1799) assert(active == 0);
                    }
                }
            }
        }
    }

    /** @brief バリア内で二人を映し、両方向の視点切り替えを連続させる @return なし */
    static void CheckStage3BarrierCamera() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 3;
        g.m_stage = &Game::Stage3Module::Definition(Easy);
        g.m_bossBattle = true;
        g.m_enemies[0].active = true;
        g.m_playerCount = 2;
        const float rearZ = 20.0f - Stage3BarrierCageView::BarrierHalfLength * 4.0f;

        // 展開完了、Phase2、Phase3で上下左右の最大距離とプレイヤー入れ替えを確認する
        for (float phase : {4.0f, 5.0f, 6.0f}) {
            g.m_enemies[0].phase = phase;
            for (const Viewport viewport : {Viewport {0, 0, 1280, 720}, Viewport {0, 0, 800, 600}}) {
                for (int upperPlayer : {0, 1}) {
                    g.m_viewMode = g.m_nextViewMode = Game::ViewMode::Rail3D;
                    const Vector2 range = Game::Stage3Module::PlayerXRange(g);
                    g.m_players[upperPlayer].m_playerX = range.x;
                    g.m_players[upperPlayer].m_playerY = Game::Stage3Module::RailPlayerMaxY(g);
                    g.m_players[1 - upperPlayer].m_playerX = range.y;
                    g.m_players[1 - upperPlayer].m_playerY = g.PlayerRailMinY();
                    Camera3D camera;
                    g.ConfigureRailCamera(camera, viewport);
                    assert(camera.Position().z > rearZ + camera.NearClip());
                    assert(camera.FieldOfView() > Math::ToRadians(46.0f));

                    // 補間の各段階で両機を画面に収め、後退制限の出入りで跳ばない
                    for (bool entering : {true, false}) {
                        g.m_viewMode = entering ? Game::ViewMode::Side2D : Game::ViewMode::Rail3D;
                        g.m_nextViewMode = entering ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                        g.m_viewTransitionTimer = 1;
                        Camera3D previous;
                        for (int step = 0; step <= 100; ++step) {
                            g.m_viewTransitionProgress = step / 100.0f;
                            g.ConfigureRailCamera(camera, viewport);
                            g.ForEachPlayer([&] {
                                Vector3 position = g.PlayerWorldPosition();
                                position.z = Math::Lerp(Game::SidePlaneZ, Game::PlayerRailZ, g.RailBlend());
                                Vector2 screen;
                                float depth;
                                assert(camera.TryWorldToScreen(position, screen, &depth));
                                assert(viewport.Contains(screen) && depth > 0.0f && depth < 1.0f);
                            });
                            if (step > 0) {
                                assert((camera.Position() - previous.Position()).Length() < 1.0f);
                                assert(std::abs(camera.FieldOfView() - previous.FieldOfView()) < 0.05f);
                            }
                            previous = camera;
                        }
                    }
                    g.m_viewTransitionTimer = 0;
                }
            }
        }
    }

    /** @brief ウミヘビの描画から投影した中心と接触範囲を検証する @return なし */
    static void CheckSeaSerpentHitboxes() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 3;
        Renderer renderer;
        Camera3D camera;
        camera.SetViewport({0, 0, 1280, 720});
        camera.SetProjectionMode(ProjectionMode::Orthographic);
        Matrix4x4 inverseViewProjection;
        assert((camera.ProjectionMatrix() * camera.ViewMatrix()).TryInverse(inverseViewProjection));

        // 全3種と左右両方向の跳躍で、描画コマンドを判定の独立した基準にする
        for (int cycle = 0; cycle < 6; ++cycle) {
            g.m_frame = cycle * 420 + 48 + (cycle * 97) % 170 + 55;
            for (bool shaking : {false, true}) {
                g.m_screenShakeFrames = g.m_screenShakeDurationFrames = shaking ? 8 : 0;
                g.m_screenShakeIntensity = 0.5f;
                const Vector2 shake = g.ScreenShakeOffset();
                for (bool rail : {false, true}) {
                    g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                    renderer.BeginFrame();
                    if (rail) Game::Stage3Module::DrawBackground3D(g, renderer, camera, 1.0f);
                    else Game::Stage3Module::DrawBackground2D(g, renderer, camera);
                    assert(!renderer.HasOverflowed());
                    struct Segment { Vector3 center; Vector3 radii; };
                    Segment segments[23];
                    int count = 0;
                    for (std::size_t i = 0; i < renderer.CommandCount(); ++i) {
                        const auto& command = renderer.Command(i);
                        const auto& color = command.primitive.color;
                        if (command.type != RenderCommand::Type::Primitive3D ||
                            color.r != 0.05f || color.g != 0.24f || color.b != 0.20f) continue;
                        const Matrix4x4 world = inverseViewProjection * command.primitive.wvpMatrix;
                        Vector3 center = world.TransformPoint(Vector3::Zero);
                        Vector3 radii {std::abs(world.m[0][0]), std::abs(world.m[1][1]), std::abs(world.m[2][2])};
                        radii *= 0.5f * 0.55f;
                        if (!rail) {
                            // 描画深度から自機平面へカメラの視線を延ばす
                            const Vector3 eye {shake.x, shake.y, -16.0f};
                            const float scale = (Game::SidePlaneZ - eye.z) / (center.z - eye.z);
                            center = eye + (center - eye) * scale;
                            radii *= scale;
                        }
                        assert(count < 23);
                        segments[count++] = {center, radii};
                    }
                    assert(count > 0);
                    // 中心と上下境界の内外を、対象の半径を加えた全節の和集合と照合する
                    for (int i = 0; i < count; ++i) {
                        for (float radius : {0.0f, 0.055f}) {
                            for (float offset : {0.0f, -1.05f, -0.95f, 0.95f, 1.05f}) {
                                Vector3 point = segments[i].center;
                                point.y += offset * (segments[i].radii.y + radius * (rail ? Game::WorldXScale : Game::WorldYScale));
                                bool expected = false;
                                for (int j = 0; j < count; ++j) {
                                    const Vector3 d = point - segments[j].center;
                                    const Vector3 r = segments[j].radii + Vector3 {
                                        radius * Game::WorldXScale,
                                        radius * (rail ? Game::WorldXScale : Game::WorldYScale), radius * Game::WorldXScale};
                                    const float distance = d.x * d.x / (r.x * r.x) + d.y * d.y / (r.y * r.y) +
                                        (rail ? d.z * d.z / (r.z * r.z) : 0.0f);
                                    expected |= distance <= 1.0f;
                                }
                                assert(Game::Stage3Module::HitsHazard(g, Game::FromWorldX(point.x),
                                    Game::FromWorldY(point.y), point.z, radius) == expected);
                            }
                        }
                    }
                }
            }
        }
    }

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
        CheckStage2ViewHint();
        CheckStage3Sun();
        CheckEastsourceBombs();
        CheckEastsourceBattle();
        CheckEasyEnemyCounts();
        CheckStage5RouteCameraAndShots();
        CheckStage3BarrierCamera();
        CheckSeaSerpentHitboxes();
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

        // 第2部の全Chapterで2D退場は縦スクロール方向へ流し、3Dの既存退場方向も維持する
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
                    const bool downward = stage == 5 && !rail &&
                        ShooterStages::Stage5::IsPart2RoutePhase(phase);
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
            boss.motionAge = Game::Stage3Module::SurvivalFrames(g.m_difficulty) - 120;
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
