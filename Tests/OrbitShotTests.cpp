#include <array>
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

    /** @brief 警備ドローン弾が検知時の自機位置を通ることを検証する @return なし */
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
                drone.baseY = mode == Game::ViewMode::Rail3D ? 1.0f : 1.4f;
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
                g.m_stage5.phaseTimer = 200;
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
        g.m_stage5.phaseTimer = 200;
        g.m_stage5.guardSpawnCooldown = 999;
        g.m_stage5.tayamaTransformation = 1.0f;
        // 足元で発動し、選択した脚だけが上昇して着地する
        for (auto phase : {Game::Stage5Phase::TayamaFireControl,
            Game::Stage5Phase::TayamaLiftEngines, Game::Stage5Phase::TayamaCommandCore}) {
            g.m_stage5.phase = phase;
            g.m_stage5.tayamaStompTimer = g.m_stage5.tayamaStompCooldown = 0;
            g.m_stage5.tayamaAttack = ShooterStages::Stage5::TayamaAttack::None;
            g.m_stage5.attackTimer = 0;
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

    /** @brief 第一形態の攻撃が段階の候補から一つずつ進み、予告した目標を保つことを検証する @return なし */
    static void RunAttackSequence() {
        using Game = SideScrollingShooter;
        using Attack = ShooterStages::Stage5::TayamaAttack;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phaseTimer = 200;
        g.m_stage5.tayamaTransformation = 1.0f;
        g.Player().m_playerY = Game::FromWorldY(
            ShooterStages::Stage5::TayamaStompTriggerMaxWorldY + 20.0f);
        for (auto& light : g.m_stage5.searchlights) {
            light.phase = Game::SearchlightPhase::Cooldown;
            light.timer = 1000000;
        }

        g.m_difficulty = Normal;
        // 周回位置を変えながら各段階を流し、候補の攻撃だけが順に出ることを観測する
        for (auto phase : {Game::Stage5Phase::TayamaFireControl,
            Game::Stage5Phase::TayamaLiftEngines, Game::Stage5Phase::TayamaCommandCore}) {
            g.m_stage5.phase = phase;
            g.m_stage5.tayamaAttack = Attack::None;
            g.m_stage5.attackTimer = 0;
            const unsigned candidates = ShooterStages::Stage5::TayamaStageAttacks(phase);
            std::array<int, static_cast<std::size_t>(Attack::Count)> seen {};
            Attack current = Attack::None;
            float lockedX = 0.0f;
            for (int tick = 0; tick < 40000; ++tick) {
                g.m_stage5.guardSpawnCooldown = 999;
                g.m_stage5.tayamaStompCooldown = 999;
                g.m_stage5.tayamaOrbitAngle = g.m_stage5.tayamaSideViewAngle =
                    tick % 4000 < 2000 ? 0.0f : Math::Pi;
                g.Player().m_invincible = 999;
                for (auto& shot : g.m_shots) shot.active = false;
                const Attack before = current;
                const int previousFrame = g.m_stage5.attackTimer;
                Game::Stage5Module::TickTayama(g);
                current = g.m_stage5.tayamaAttack;
                const int frame = g.m_stage5.attackTimer;
                assert(frame < ShooterStages::Stage5::TayamaAttackDuration(current));

                if (current != before) {
                    // 別の攻撃へ直接移らず、必ず待機を挟む
                    assert(before == Attack::None || current == Attack::None);
                    assert(frame == 0);
                    if (current != Attack::None) {
                        ++seen[static_cast<std::size_t>(current)];
                        assert(current != g.m_stage5.previousTayamaAttack);
                        assert(previousFrame + 1 >= ShooterStages::Stage5::TayamaDifficultyCooldown(g.m_difficulty));
                        lockedX = g.m_stage5.coreTargetX;
                    }
                } else {
                    // 攻撃中は予告と発射で使う目標を他の攻撃が上書きしない
                    assert(frame == previousFrame + 1);
                    if (current != Attack::None) assert(g.m_stage5.coreTargetX == lockedX);
                }
            }
            for (int attack = 1; attack < static_cast<int>(Attack::Count); ++attack) {
                // 踏みつけは再使用待ちで止めて他の攻撃の順次進行だけを見る
                const bool candidate = attack != static_cast<int>(Attack::Stomp) && (candidates &
                    ShooterStages::Stage5::TayamaAttackBit(static_cast<Attack>(attack))) != 0;
                assert(candidate == (seen[attack] > 0));
            }
        }

        // 段階の候補を全て封じても発動可能な攻撃へ切り替えて手詰まりにしない
        g.m_stage5.phase = Game::Stage5Phase::TayamaFireControl;
        g.m_stage5.tayamaOrbitAngle = g.m_stage5.tayamaSideViewAngle = 0.0f;
        for (auto& weakpoint : g.m_stage5.tayamaWeakpoints) weakpoint.destroyed = true;
        g.m_stage5.tayamaAttack = Attack::None;
        g.m_stage5.attackTimer = 0;
        bool fallback = false;
        for (int tick = 0; tick < 5000; ++tick) {
            g.m_stage5.guardSpawnCooldown = 999;
            g.m_stage5.tayamaStompCooldown = 999;
            g.Player().m_invincible = 999;
            Game::Stage5Module::TickTayama(g);
            const Attack attack = g.m_stage5.tayamaAttack;
            assert(attack != Attack::RadarBurst && attack != Attack::ChestSweep &&
                attack != Attack::LegVolley && attack != Attack::CoreBarrage);
            if (attack != Attack::None) fallback = true;
        }
        assert(fallback);
        std::puts("Tayama attack sequence passed");
    }

    /** @brief 光線と衝撃波が周回軌道まで届き、予告どおりの動きで避けられることを検証する @return なし */
    static void RunBeams() {
        using Game = SideScrollingShooter;
        using Attack = ShooterStages::Stage5::TayamaAttack;
        namespace Stage5 = ShooterStages::Stage5;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::TayamaCommandCore;
        g.m_stage5.phaseTimer = 200;
        g.m_stage5.tayamaTransformation = 1.0f;
        for (auto& light : g.m_stage5.searchlights) {
            light.phase = Game::SearchlightPhase::Cooldown;
            light.timer = 1000000;
        }
        const float lockedY = Game::FromWorldY(60.0f);

        // 高度と周回角を固定して一回の攻撃を流し、被弾したかを返す
        const auto Hit = [&](Attack attack, float playerY, float angle) {
            g.Player().m_playerY = lockedY;
            g.m_stage5.tayamaOrbitAngle = g.m_stage5.tayamaSideViewAngle = 0.0f;
            const Vector3 lock = g.PlayerWorldPosition();
            g.m_stage5.headLaserTarget = lock;
            g.m_stage5.coreTargetY = lockedY;
            g.Player().m_playerY = playerY;
            g.m_stage5.tayamaOrbitAngle = g.m_stage5.tayamaSideViewAngle = angle;
            g.Player().m_playerDestructionTimer = 0;
            g.Player().m_invincible = 0;
            g.m_stage5.tayamaAttack = attack;
            g.m_stage5.attackTimer = -1;
            g.m_stage5.tayamaStompCooldown = 999;
            g.m_stage5.guardSpawnCooldown = 999;
            bool hit = false;
            const int duration = Stage5::TayamaAttackDuration(attack);
            for (int tick = 0; tick < duration - 1; ++tick) {
                for (auto& shot : g.m_shots) shot.active = false;
                Game::Stage5Module::TickTayama(g);
                if (g.Player().m_playerDestructionTimer > 0) hit = true;
            }
            return hit;
        };

        // 腕の光刃は全周どこでも周回軌道へ届き、高度を変えれば当たらない
        const float dodgeY = lockedY + (Stage5::TayamaArmSpinHitRadius + 1.0f) / Game::WorldYScale;
        for (float angle : {0.0f, Math::HalfPi, Math::Pi, -Math::HalfPi}) {
            assert(Hit(Attack::ArmSpin, lockedY, angle));
            assert(!Hit(Attack::ArmSpin, dodgeY, angle));
        }

        // コア収束砲は固定地点を撃ち抜き、周回して離れれば当たらない
        assert(Hit(Attack::CoreCannon, lockedY, 0.0f));
        assert(!Hit(Attack::CoreCannon, lockedY, 0.5f));

        // 両目の光線は固定地点で交差し、十分に離れれば当たらない
        assert(Hit(Attack::HeadLaser, lockedY, 0.0f));
        assert(!Hit(Attack::HeadLaser, lockedY, Math::HalfPi));

        // 衝撃波は低空だけを巻き込む
        const auto ShockwaveHit = [&](float worldY) {
            g.m_stage5.tayamaAttack = Attack::None;
            g.m_stage5.attackTimer = 0;
            g.m_stage5.tayamaShockwaveTimer = 1;
            g.m_stage5.tayamaShockwaveCenter = {0.0f, Stage5::RooftopSurfaceY, Stage5::TayamaArenaCenterZ};
            g.m_stage5.tayamaOrbitAngle = g.m_stage5.tayamaSideViewAngle = 0.0f;
            g.Player().m_playerY = Game::FromWorldY(worldY);
            g.Player().m_playerDestructionTimer = 0;
            g.Player().m_invincible = 0;
            g.m_stage5.tayamaStompCooldown = 999;
            for (int tick = 0; tick < Stage5::TayamaShockwaveFrames; ++tick) {
                g.m_stage5.attackTimer = 0;
                g.m_stage5.guardSpawnCooldown = 999;
                Game::Stage5Module::TickTayama(g);
            }
            assert(g.m_stage5.tayamaShockwaveTimer == 0);
            return g.Player().m_playerDestructionTimer > 0;
        };
        assert(ShockwaveHit(Stage5::RooftopSurfaceY + 1.0f));
        assert(!ShockwaveHit(Stage5::RooftopSurfaceY + Stage5::TayamaShockwaveHeight + 2.0f));
        std::puts("Tayama beams and shockwave passed");
    }

    /** @brief 難易度で弾数と間隔が変わり、協力プレイで標的が交互になることを検証する @return なし */
    static void RunDifficultyAndTargets() {
        using Game = SideScrollingShooter;
        using Attack = ShooterStages::Stage5::TayamaAttack;
        namespace Stage5 = ShooterStages::Stage5;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_stage5.phase = Game::Stage5Phase::TayamaFireControl;
        g.m_stage5.phaseTimer = 200;
        g.m_stage5.tayamaTransformation = 1.0f;
        g.Player().m_playerY = Game::FromWorldY(60.0f);
        for (auto& light : g.m_stage5.searchlights) {
            light.phase = Game::SearchlightPhase::Cooldown;
            light.timer = 1000000;
        }

        // 一回の攻撃で撃った弾を数える
        const auto CountShots = [&](DifficultyType difficulty, Attack attack) {
            g.m_difficulty = difficulty;
            g.m_shots = {};
            g.m_stage5.tayamaAttack = attack;
            g.m_stage5.attackTimer = -1;
            g.m_stage5.headLaserTarget = g.PlayerWorldPosition();
            for (int tick = 0; tick < Stage5::TayamaAttackDuration(attack) - 1; ++tick) {
                g.m_stage5.guardSpawnCooldown = 999;
                g.m_stage5.tayamaStompCooldown = 999;
                Game::Stage5Module::TickTayama(g);
            }
            int count = 0;
            for (const auto& shot : g.m_shots) if (shot.active) ++count;
            return count;
        };
        assert(CountShots(Easy, Attack::RadarBurst) == 12 * 2);
        assert(CountShots(Normal, Attack::RadarBurst) == 16 * 3);
        assert(CountShots(Hard, Attack::RadarBurst) == 24 * 4);
        assert(CountShots(Easy, Attack::ChestSweep) == 9 * 2);
        assert(CountShots(Hard, Attack::ChestSweep) == 9 * 4);

        // 協力プレイでは攻撃ごとに標的の自機が入れ替わる
        g.m_difficulty = Normal;
        g.m_playerCount = 2;
        g.m_players[0].m_playerY = Game::FromWorldY(60.0f);
        g.m_players[1].m_playerY = Game::FromWorldY(90.0f);
        g.m_stage5.tayamaAttack = Attack::None;
        g.m_stage5.attackTimer = 0;
        float previousTarget = -1000.0f;
        int switches = 0;
        Attack current = Attack::None;
        for (int tick = 0; tick < 3000; ++tick) {
            g.m_stage5.guardSpawnCooldown = 999;
            g.m_stage5.tayamaStompCooldown = 999;
            g.m_players[0].m_invincible = g.m_players[1].m_invincible = 999;
            for (auto& shot : g.m_shots) shot.active = false;
            Game::Stage5Module::TickTayama(g);
            if (g.m_stage5.tayamaAttack != Attack::None && current == Attack::None) {
                if (std::abs(g.m_stage5.coreTargetY - previousTarget) > 1.0f) ++switches;
                previousTarget = g.m_stage5.coreTargetY;
            }
            current = g.m_stage5.tayamaAttack;
        }
        assert(switches >= 4);
        std::puts("Tayama difficulty and targets passed");
    }

    /** @brief 第一形態の段階移行、コア決着、到達段階からの再開を検証する @return なし */
    static void RunStageProgress() {
        using Game = SideScrollingShooter;
        using Phase = Game::Stage5Phase;
        using Weakpoint = Game::TayamaWeakpoint;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 5;
        g.m_chapterNumber = 3;
        g.m_stage = &Game::StageForNumber(5, g.m_difficulty);
        Game::Stage5Module::Reset(g);
        const auto Part = [&](Weakpoint type) -> Game::TayamaWeakpointState& {
            return g.m_stage5.tayamaWeakpoints[static_cast<int>(type)];
        };
        const int maxHp = g.m_stage5.tayamaMaxHp;
        const int coreStartHp = Part(Weakpoint::CommandCore).maxHp;
        g.m_stage5.tayamaTransformation = 1.0f;
        Game::Stage5Module::StartTayamaPhase(g, Phase::TayamaFireControl);
        assert(g.m_stage5.tayamaHp == maxHp);
        assert(!Part(Weakpoint::CommandCore).active && Part(Weakpoint::FireControlRadar).active);

        // 一度の大ダメージでも段階を飛ばさず、敵弾と攻撃を片付けて次へ移る
        g.m_shots[0].active = g.m_shots[0].enemy = true;
        g.m_stage5.tayamaAttack = ShooterStages::Stage5::TayamaAttack::ArmSpin;
        g.m_stage5.tayamaHp = 0;
        Game::Stage5Module::UpdateTayamaBossHp(g);
        assert(g.m_stage5.phase == Phase::TayamaLiftEngines);
        assert(g.m_stage5.tayamaHp == maxHp * 2 / 3);
        assert(g.m_stage5.checkpoint == Game::Stage5Checkpoint::TayamaLiftEngines);
        assert(g.m_stage5.tayamaAttack == ShooterStages::Stage5::TayamaAttack::None);
        assert(!g.m_shots[0].active && g.m_stage5.phaseTimer == 0);
        assert(!Part(Weakpoint::CommandCore).active);

        // 第二段階で壊した部位は最終段階へ持ち越す
        Part(Weakpoint::LeftSearchlight).destroyed = true;
        g.m_stage5.tayamaHp = 0;
        Game::Stage5Module::UpdateTayamaBossHp(g);
        assert(g.m_stage5.phase == Phase::TayamaCommandCore);
        assert(g.m_stage5.tayamaHp == maxHp / 3);
        assert(g.m_stage5.checkpoint == Game::Stage5Checkpoint::TayamaCommandCore);
        assert(g.m_stage5.tayamaStageShifted);
        assert(Part(Weakpoint::CommandCore).active);
        assert(!Part(Weakpoint::LeftSearchlight).active);
        assert(g.m_stage5.searchlights[0].destroyed);

        // コアを壊すまで本体HPは尽きない
        g.m_stage5.tayamaHp = 0;
        Game::Stage5Module::UpdateTayamaBossHp(g);
        assert(g.m_stage5.phase == Phase::TayamaCommandCore && g.m_stage5.tayamaHp == 1);

        // 再開は最終段階の開始時点へ戻し、以降に壊した部位だけ復元する
        Part(Weakpoint::RightSearchlight).destroyed = true;
        Part(Weakpoint::CommandCore).hp = 1;
        Game::Stage5Module::RestartCheckpoint(g);
        assert(!g.m_stage5.tayamaStageShifted);
        assert(g.m_stage5.phase == Phase::TayamaCommandCore);
        assert(g.m_stage5.tayamaHp == maxHp / 3);
        assert(Part(Weakpoint::LeftSearchlight).destroyed);
        assert(!Part(Weakpoint::RightSearchlight).destroyed);
        assert(Part(Weakpoint::CommandCore).hp == coreStartHp);

        // コア破壊で初めて崩壊へ進む
        Part(Weakpoint::CommandCore).destroyed = true;
        Game::Stage5Module::UpdateTayamaBossHp(g);
        assert(g.m_stage5.phase == Phase::TayamaCollapse && g.m_stage5.tayamaHp == 0);
        std::puts("Tayama stage progress passed");
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
    OrbitShotTests::RunAttackSequence();
    OrbitShotTests::RunStageProgress();
    OrbitShotTests::RunBeams();
    OrbitShotTests::RunDifficultyAndTargets();
    OrbitShotTests::CheckDragonRush();
    OrbitShotTests::Run();
    OrbitShotTests::CheckWallDrone();
    return 0;
}
