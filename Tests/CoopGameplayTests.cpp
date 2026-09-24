#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <memory>

#include "../Engine/Input/Input.h"
#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage3/Stage3Module.h"
#include "../Presentation/Gameplay/GameplayRandom.h"
#include "../Application/UseCases/CooperativeFrames.h"

/** @brief 全ステージの協力プレイ経路を検証する @return なし */
void RunCoopStageTests();

/** @brief カメラ検証に画面寸法だけを提供する描画先 */
class CoopCameraBackend final : public IRenderBackend {
public:
    /** @brief フレームを開始する @return なし */
    void BeginFrame() override {}
    /** @brief 円の描画を省略する @param circle 円 @param color 色 @return なし */
    void DrawCircle(const Circle&, const ColorF&) override {}
    /** @brief 矩形の描画を省略する @param rect 矩形 @param color 色 @return なし */
    void DrawRect(const Rect&, const ColorF&) override {}
    /** @brief 文字描画を省略する @param text 文字 @param position 座標 @param size サイズ @param color 色 @param spacing 字間 @return なし */
    void DrawTextCommand(std::string_view, const Vector2&, float, const ColorF&, float) override {}
    /** @brief パイプライン設定を省略する @param pipeline パイプライン @return なし */
    void SetPipeline(PipelineId) override {}
    /** @brief フレームを終了する @return なし */
    void EndFrame() override {}
    /** @brief 画面幅を取得する @return ピクセル幅 */
    int Width() const override { return 1280; }
    /** @brief 画面高さを取得する @return ピクセル高さ */
    int Height() const override { return 720; }
    /** @brief 縦横比を取得する @return 横幅と高さの比 */
    float AspectRatio() const override { return 1280.0f / 720.0f; }
};

class InputTestAccess {
public:
    /** @brief F12のフレーム入力を設定する @param down 新規押下 @return なし */
    static void SetHitboxKey(bool down) {
        Input::m_keyDown[static_cast<size_t>(KeyCode::F12)] = down;
    }
    /** @brief 実機なしで各パッドの入力を設定する @param player パッド番号 @param keys 押下キー @return なし */
    static void SetPad(int player, std::initializer_list<KeyCode> keys) {
        Input::m_playerGamepadKeys[player] = {};
        Input::m_playerGamepadDown[player] = {};
        for (KeyCode key : keys) {
            Input::m_playerGamepadKeys[player][static_cast<size_t>(key)] = true;
            Input::m_playerGamepadDown[player][static_cast<size_t>(key)] = true;
        }
    }
};

struct CoopGameplayTests {
    using Game = SideScrollingShooter;

    /** @brief Stage3の2D機体全体がバリア内に収まり視点往復後も制限されることを検証する @return なし */
    static void CheckStage3BarrierBounds() {
        auto game = std::make_unique<Game>();
        auto& g = *game;
        CoopCameraBackend backend;
        Renderer renderer(backend);
        g.m_stageNumber = 3;
        g.m_stage = &Game::Stage3Module::Definition(Easy);
        g.m_bossBattle = true;
        auto& boss = g.m_enemies[0];
        Game::Stage3Module::ConfigureBossSpawn(boss, false, 2);
        boss.active = true;
        boss.type = 2;

        // Phase2/3と両プレイヤーを四隅まで移動し、2D→3D→2Dを往復する
        for (float phase : {5.0f, 6.0f}) {
            boss.phase = phase;
            for (int playerCount : {1, 2}) {
                g.m_playerCount = playerCount;
                for (int player = 0; player < playerCount; ++player) {
                    g.m_activePlayer = player;
                    for (float xSign : {-1.0f, 1.0f}) {
                        for (float ySign : {-1.0f, 1.0f}) {
                            for (bool rail : {false, true, false}) {
                                g.RequestViewMode(rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D);
                                g.Player().m_moveLeft = xSign < 0.0f;
                                g.Player().m_moveRight = xSign > 0.0f;
                                g.Player().m_moveDown = ySign < 0.0f;
                                g.Player().m_moveUp = ySign > 0.0f;
                                for (int frame = 0; frame < 600; ++frame) {
                                    g.TickViewTransition();
                                    g.TickPlayer();
                                    const Vector2 xRange = Game::Stage3Module::PlayerXRange(g);
                                    assert(g.Player().m_playerX >= xRange.x && g.Player().m_playerX <= xRange.y);
                                }
                                assert(g.m_viewTransitionTimer == 0);
                                if (rail) {
                                    // 既存の3D移動端を維持する
                                    assert(std::abs(g.Player().m_playerX - xSign * 1.2f) < 0.0001f);
                                    const float y = Game::ToWorldY(g.Player().m_playerY);
                                    assert(std::abs(y - (ySign < 0.0f ? -21.04f : 0.85f)) < 0.0001f);
                                    continue;
                                }

                                // 描画されたバリア5面から画面上の左右上下境界を得る
                                Camera3D camera;
                                g.ConfigureSideCamera(camera, renderer);
                                renderer.BeginFrame();
                                assert(Game::Stage3Module::DrawBossModel(g, renderer, camera, boss, 0.0f));
                                std::array<Matrix4x4, 5> fields;
                                size_t fieldCount = 0;
                                for (size_t i = 0; i < renderer.CommandCount(); ++i) {
                                    const auto& command = renderer.Command(i);
                                    if (command.type != RenderCommand::Type::Primitive3D ||
                                        std::abs(command.primitive.color.a - 0.28f) > 0.00001f ||
                                        command.primitive.color.r >= 0.05f) continue;
                                    assert(fieldCount < fields.size());
                                    fields[fieldCount++] = command.primitive.wvpMatrix;
                                }
                                assert(fieldCount == fields.size());
                                const float left = fields[1].TransformPoint({}).x;
                                const float right = fields[0].TransformPoint({}).x;
                                const float bottom = fields[0].TransformPoint({0.0f, -0.5f, 0.0f}).y;
                                const float top = fields[0].TransformPoint({0.0f, 0.5f, 0.0f}).y;

                                // 機首の最大発光も含め、各部品の外接箱を透視投影して照合する
                                g.m_viewToggleCooldown = 0;
                                renderer.BeginFrame();
                                g.DrawPlayerModel(renderer, camera, Game::ToWorldX(g.Player().m_playerX),
                                    Game::ToWorldY(g.Player().m_playerY), Game::SidePlaneZ, true, Math::HalfPi);
                                assert(renderer.CommandCount() >= 4);
                                for (size_t i = 0; i < renderer.CommandCount(); ++i) {
                                    const auto& matrix = renderer.Command(i).primitive.wvpMatrix;
                                    for (float x : {-0.5f, 0.5f}) for (float y : {-0.5f, 0.5f}) for (float z : {-0.5f, 0.5f}) {
                                        const Vector3 point = matrix.TransformPoint({x, y, z});
                                        assert(point.x > left && point.x < right);
                                        assert(point.y > bottom && point.y < top);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /** @brief 無関係な敵出現と隕石を止めて実ゲーム状態を初期化する @param game 検証対象 @return なし */
    static void Prepare(Game& game) {
        game.m_playerCount = 2;
        game.m_activePlayer = 0;
        game.m_difficulty = Normal;
        game.Reset(true);
        game.m_missionStartTimer = 0;
        game.m_bossBattle = true;
        game.m_stage1.spawnFrames = 100000;
        for (auto& meteor : game.m_stage1.meteors) meteor.destroyed = true;
        InputTestAccess::SetPad(0, {});
        InputTestAccess::SetPad(1, {});
    }

    /** @brief 発射者ごとに有効な自機弾を数える @param game 検証対象 @param player 発射者 @return 有効弾数 */
    static int ShotCount(const Game& game, int player) {
        int count = 0;
        for (const auto& shot : game.m_shots)
            if (shot.active && !shot.enemy && shot.owner == player) ++count;
        return count;
    }

    /** @brief 実際の入力経路と更新処理で移動と射撃の分離を検証する @param game 検証対象 @return なし */
    static void CheckInputAndShots(Game& game) {
        Prepare(game);
        auto& blue = game.m_players[0];
        auto& green = game.m_players[1];
        blue.m_playerType = Homing;
        green.m_playerType = Piercing;
        const float startX = blue.m_playerX;
        InputTestAccess::SetPad(0, {KeyCode::RightArrow, KeyCode::Space});
        InputTestAccess::SetPad(1, {KeyCode::LeftArrow});
        game.ProcessInput();
        game.Tick();
        assert(blue.m_playerX > startX && green.m_playerX < startX);
        assert(ShotCount(game, 0) == 3 && ShotCount(game, 1) == 0);
        assert(blue.m_specialShotCooldown == Game::PlayerShotConfigs[Homing].fireIntervalFrames);
        assert(green.m_shotCooldown == 0 && green.m_specialShotCooldown == 0);

        // 2Pだけの射撃は1Pの発射間隔やタイプを変更しない
        InputTestAccess::SetPad(0, {});
        InputTestAccess::SetPad(1, {KeyCode::Space});
        game.ProcessInput();
        game.Tick();
        assert(ShotCount(game, 0) == 3 && ShotCount(game, 1) == 3);
        assert(blue.m_specialShotCooldown == Game::PlayerShotConfigs[Homing].fireIntervalFrames - 1);
        assert(green.m_specialShotCooldown == Game::PlayerShotConfigs[Piercing].fireIntervalFrames);
        for (const auto& shot : game.m_shots) {
            if (!shot.active || !shot.special) continue;
            assert(shot.playerType == (shot.owner == 0 ? Homing : Piercing));
        }

        // 視点操作は1Pだけが要求でき、切り替え時の保護は2人へ付く
        InputTestAccess::SetPad(1, {KeyCode::X});
        game.ProcessInput();
        assert(!game.m_viewToggleRequested);
        InputTestAccess::SetPad(0, {KeyCode::X});
        InputTestAccess::SetPad(1, {});
        game.ProcessInput();
        assert(game.m_viewToggleRequested);
        game.TickViewTransition();
        assert(game.m_nextViewMode == Game::ViewMode::Rail3D);
        assert(blue.m_invincible >= Game::ViewToggleInvincibleFrames);
        assert(green.m_invincible >= Game::ViewToggleInvincibleFrames);
        assert(game.m_activePlayer == 0);
    }

    /** @brief アイテムが取得者だけを回復しスコアを共有することを検証する @param game 検証対象 @return なし */
    static void CheckItems(Game& game) {
        Prepare(game);
        game.m_players[0].m_power = 1.0f;
        game.m_players[1].m_power = 2.0f;
        for (int player = 0; player < 2; ++player) {
            auto& item = game.m_items[0];
            item = {};
            item.active = true;
            item.x = game.m_players[player].m_playerX + 0.012f;
            item.y = game.m_players[player].m_playerY;
            item.power = 0.5f;
            game.TickItems();
            assert(!item.active);
            assert(game.m_players[0].m_power == 1.5f);
            assert(game.m_players[1].m_power == (player == 0 ? 2.0f : 2.5f));
        }

        // 重なった自機でも1つのアイテムを二重取得しない
        game.m_players[1].m_playerY = game.m_players[0].m_playerY;
        auto& item = game.m_items[0];
        item.active = true;
        item.x = game.m_players[0].m_playerX + 0.012f;
        item.y = game.m_players[0].m_playerY;
        game.TickItems();
        assert(game.m_players[0].m_power + game.m_players[1].m_power == 4.5f);

        // 各自のスコアアイテムは同じ合計とチャプタースコアへ加算する
        game.m_players[1].m_playerY = -0.25f;
        for (int player = 0; player < 2; ++player) {
            item = {};
            item.active = true;
            item.x = game.m_players[player].m_playerX + 0.012f;
            item.y = game.m_players[player].m_playerY;
            item.type = Game::ItemType::Score;
            item.score = 100 * (player + 1);
            game.TickItems();
        }
        assert(game.Score() == 300 && game.m_chapterResult.score == 300);
    }

    /** @brief 機首レーザー、十発の追尾ミサイル、被弾まで残るシールドを検証する @param game 検証対象 @return なし */
    static void CheckBombs(Game& game) {
        for (bool rail : {false, true}) {
            Prepare(game);
            game.m_viewMode = game.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            auto& player = game.Player();
            player.m_playerX = player.m_playerY = 0.0f;
            player.m_playerType = Piercing;
            player.m_bombRequested = true;
            game.TickBomb();
            player.m_bombRequested = false;
            const auto laser = game.MakeBombShot();
            const Vector3 start {Game::ToWorldX(laser.x - laser.vx), Game::ToWorldY(laser.y - laser.vy),
                rail ? laser.z - laser.vz : game.PlayerWorldPosition().z};
            assert(std::abs((start - game.PlayerWorldPosition()).Length() - 1.21f) < 0.0001f);
            Game::Shot hostile;
            hostile.enemy = true;
            hostile.x = rail ? 0.0f : 1.0f;
            hostile.z = game.PlayerRailDepth() + 8.0f;
            assert(game.BombClearsShot(hostile));
            hostile.y = 1.0f;
            assert(!game.BombClearsShot(hostile));
            hostile.vy = 2.0f;
            assert(game.BombClearsShot(hostile));
            hostile.enemy = false;
            assert(!game.BombClearsShot(hostile));
            auto renderer = std::make_unique<Renderer>();
            Camera3D camera;
            camera.SetViewport({0, 0, 1280, 720});
            game.DrawBomb(*renderer, camera, player.m_bomb);
            assert(renderer->CommandCount() > 0 && !renderer->HasOverflowed());
            while (player.m_bomb.active) game.TickBomb();
            assert(player.m_bomb.age == Game::BombLaserFrames + 1);

            // 通常弾プールが満杯でも、六フレーム間隔で一発ずつ合計十発を打ち上げる
            Prepare(game);
            game.m_viewMode = game.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            player.m_playerX = player.m_playerY = 0.0f;
            player.m_playerType = Homing;
            player.m_bombRequested = true;
            for (auto& shot : game.m_shots) shot.active = true;
            for (int frame = 1; frame <= 55; ++frame) {
                game.TickBomb();
                player.m_bombRequested = false;
                int count = 0;
                for (const auto& missile : player.m_bomb.missiles) if (missile.active) ++count;
                assert(count == (frame - 1) / Game::BombMissileInterval + 1);
                if (frame == 1) {
                    const auto& missile = player.m_bomb.missiles[0];
                    assert(std::abs(Game::ToWorldX(missile.vx)) < 0.026f && missile.vy > 0.0f);
                }
            }
            assert(player.m_bombCount == Game::InitialBombCount - 1);
            // 十発目が出た時点でも初弾は落下中で、全弾の噴射炎はまだ出ない
            assert(player.m_bomb.missiles.front().vy < 0.0f && player.m_bomb.missiles.back().vy > 0.0f);
            for (const auto& missile : player.m_bomb.missiles) assert(missile.homingTarget == -1);
            game.ResetShots();
            renderer->BeginFrame();
            game.DrawBomb(*renderer, camera, player.m_bomb);
            assert(renderer->CommandCount() == 40 && !renderer->HasOverflowed());
            Matrix4x4 inverse;
            assert((camera.ProjectionMatrix() * camera.ViewMatrix()).TryInverse(inverse));
            const Matrix4x4 body = inverse * renderer->Command(0).primitive.wvpMatrix;
            assert(std::abs(body.TransformVector(Vector3::Right).Length() - 0.624f) < 0.001f);

            // 各弾の判定を単独で有効にし、十発すべてが敵弾を消すことを確認する
            for (auto& missile : player.m_bomb.missiles) missile.active = false;
            for (auto& missile : player.m_bomb.missiles) {
                missile.active = true;
                hostile = {};
                hostile.enemy = true;
                hostile.x = missile.x;
                hostile.y = missile.y;
                hostile.z = missile.z;
                assert(game.BombClearsShot(hostile));
                hostile.y += 10.0f;
                assert(!game.BombClearsShot(hostile));
                missile.active = false;
            }
            for (auto& missile : player.m_bomb.missiles) missile.active = true;

            // 点火後は移動中の敵を再捕捉し、各弾が別々の位置から旋回する
            auto& enemy = game.m_enemies[0];
            enemy = {};
            enemy.active = true;
            enemy.hp = enemy.maxHp = 100;
            // 到達前の区間だけを測り、命中処理を省いた状態で標的を通り過ぎない距離に置く
            enemy.x = rail ? 1.0f : 6.0f;
            enemy.y = -0.5f;
            enemy.z = 100.0f;
            for (int frame = 0; frame < 80; ++frame) {
                enemy.y -= 0.002f;
                game.TickBomb();
                if (frame == 20) {
                    // 同じ標的へ向かっても、弾ごとに異なる曲線を描く
                    const auto& a = player.m_bomb.missiles[0];
                    const auto& b = player.m_bomb.missiles[1];
                    assert(std::abs(a.vx - b.vx) + std::abs(a.vy - b.vy) + std::abs(a.vz - b.vz) > 0.001f);
                    renderer->BeginFrame();
                    game.DrawBomb(*renderer, camera, player.m_bomb);
                    int boosters = 0;
                    for (size_t i = 0; i < renderer->CommandCount(); ++i)
                        if (renderer->Command(i).type == RenderCommand::Type::Railgun) ++boosters;
                    assert(boosters == 10);
                }
            }
            for (const auto& missile : player.m_bomb.missiles) {
                assert(missile.homingTarget >= 0);
                assert(missile.owner == 0 && missile.bomb && missile.damage == 80);
                // 曲線の広がりが収まった後は、移動する標的へ収束する
                const Vector3 delta {Game::ToWorldX(enemy.x - missile.x), Game::ToWorldY(enemy.y - missile.y),
                    rail ? enemy.z - missile.z : 0.0f};
                const Vector3 velocity {Game::ToWorldX(missile.vx), Game::ToWorldY(missile.vy), rail ? missile.vz : 0.0f};
                assert(Vector3::Dot(delta.Normalized(), velocity.Normalized()) > 0.98f);
            }
            // 一発の命中は他の九発を終了させず、通常敵へ高威力を適用する
            auto& first = player.m_bomb.missiles[0];
            enemy.x = first.x; enemy.y = first.y; enemy.z = first.z;
            game.HitPlayerShotTargets(first);
            assert(!first.active && enemy.hp == 20 && player.m_bomb.active);
            for (int i = 1; i < 10; ++i) assert(player.m_bomb.missiles[i].active);
            while (player.m_bomb.active) game.TickBomb();
            assert(player.m_bomb.age == Game::BombMissileFrames + 1);

            // 異なる乱数列でも、実際の移動と命中判定で十発すべてが移動標的へ到達する
            for (unsigned seed : {1U, 12345U}) {
                Prepare(game);
                game.m_viewMode = game.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                player.m_playerX = player.m_playerY = 0.0f;
                player.m_playerType = Homing;
                player.m_bombRequested = true;
                enemy = {};
                enemy.active = true;
                enemy.hp = enemy.maxHp = 1000;
                enemy.x = 1.0f;
                enemy.z = 45.0f;
                GameplayRandom::State = seed;
                for (int frame = 0; frame <= Game::BombMissileFrames; ++frame) {
                    enemy.y = std::sin(frame * 0.02f) * 0.25f;
                    game.TickBomb();
                    player.m_bombRequested = false;
                    game.TickShots();
                }
                assert(enemy.hp == 200 && !player.m_bomb.active);
            }

            // シールドは長時間保持され、味方の被弾やチャプター終了では失われない
            Prepare(game);
            game.m_viewMode = game.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            player.m_playerType = Spread;
            player.m_invincible = 0;
            player.m_bombRequested = true;
            game.TickBomb();
            player.m_bombRequested = false;
            // 六角形の枠が閉じ、全頂点が同じ球面上にあることを両視点で検証する
            for (int age : {0, 179, 359}) {
                player.m_bomb.age = age;
                renderer->BeginFrame();
                game.DrawBomb(*renderer, camera, player.m_bomb);
                assert(renderer->CommandCount() == 240 && !renderer->HasOverflowed());
                Vector3 shieldCenter = game.PlayerWorldPosition();
                shieldCenter.z = rail ? game.PlayerRailDepth() : Game::SidePlaneZ;
                for (size_t i = 0; i < renderer->CommandCount(); ++i) {
                    const auto& primitive = renderer->Command(i).primitive;
                    assert(primitive.shape == PrimitiveShape::Cylinder);
                    const Matrix4x4 world = inverse * primitive.wvpMatrix;
                    const Vector3 start = world.TransformPoint({0.0f, -0.5f, 0.0f});
                    const Vector3 end = world.TransformPoint({0.0f, 0.5f, 0.0f});
                    assert(std::abs((start - shieldCenter).Length() - Game::BombShieldRadius) < 0.001f);
                    assert(std::abs((end - shieldCenter).Length() - Game::BombShieldRadius) < 0.001f);
                    const size_t next = i / 12 * 12 + (i + 1) % 12;
                    const Matrix4x4 nextWorld = inverse * renderer->Command(next).primitive.wvpMatrix;
                    assert((end - nextWorld.TransformPoint({0.0f, -0.5f, 0.0f})).Length() < 0.001f);
                }
            }
            for (int frame = 0; frame < 1800; ++frame) game.TickBomb();
            assert(player.m_bomb.active);
            game.FinishChapter();
            assert(player.m_bomb.active);
            game.StartNextStage();
            assert(player.m_bomb.active);
            game.m_activePlayer = 1;
            game.Player().m_invincible = 0;
            game.DamagePlayer();
            assert(game.Player().m_playerDestructionTimer > 0 && player.m_bomb.active);
            game.m_activePlayer = 0;
            player.m_invincible = 0;
            game.TickBomb();
            hostile = {};
            hostile.active = hostile.enemy = true;
            hostile.x = player.m_bomb.x;
            hostile.y = player.m_bomb.y;
            hostile.z = player.m_bomb.z;
            assert(game.BombClearsShot(hostile));
            assert(!player.m_bomb.active && player.m_playerDestructionTimer == 0 && player.m_invincible > 0);
            game.DamagePlayer();
            assert(player.m_playerDestructionTimer == 0);
            player.m_invincible = 0;
            game.DamagePlayer();
            assert(player.m_playerDestructionTimer > 0);

            // 接触や障害物の被弾も同じ一回の防御を消費する
            Prepare(game);
            player.m_playerType = Spread;
            player.m_invincible = 0;
            player.m_bombRequested = true;
            game.TickBomb();
            game.DamagePlayer();
            assert(!player.m_bomb.active && player.m_playerDestructionTimer == 0);
        }

        // 横・縦・周回戦で、判定と円柱描画の発射端が実機モデルの先端と一致する
        for (int route : {0, 1, 2}) {
            for (bool rail : {false, true}) {
                Prepare(game);
                game.m_viewMode = game.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
                if (route != 0) {
                    game.m_stageNumber = 5;
                    game.m_stage5.phase = route == 1 ? ShooterStages::Stage5::Phase::WallClimbLower :
                        ShooterStages::Stage5::Phase::TayamaFireControl;
                    game.m_stage5.tayamaOrbitAngle = 0.7f;
                }
                game.Player().m_playerType = Piercing;
                game.Player().m_bombRequested = true;
                game.TickBomb();
                const auto laser = game.MakeBombShot();
                const Vector3 muzzle {Game::ToWorldX(laser.x - laser.vx), Game::ToWorldY(laser.y - laser.vy), laser.z - laser.vz};
                Vector3 expected = game.PlayerWorldPosition() + game.Player().m_bomb.direction * 1.21f;
                if (!game.IsRailGameplayActive()) expected.z = Game::SidePlaneZ;
                assert(Vector3::Distance(muzzle, expected) < 0.0001f);
                auto renderer = std::make_unique<Renderer>();
                Camera3D camera;
                camera.SetViewport({0, 0, 1280, 720});
                game.DrawBomb(*renderer, camera, game.Player().m_bomb);
                const auto& core = renderer->Command(0).primitive;
                assert(core.shape == PrimitiveShape::Cylinder);
                const Vector3 drawnMuzzle = core.wvpMatrix.TransformPoint({0.0f, -0.5f, 0.0f});
                const Vector3 expectedMuzzle = (camera.ProjectionMatrix() * camera.ViewMatrix()).TransformPoint(expected);
                assert(Vector3::Distance(drawnMuzzle, expectedMuzzle) < 0.00001f);
            }
        }

        // 同時発動する二人の残弾と十発の所有者を分離する
        Prepare(game);
        game.ForEachPlayer([&] { game.Player().m_playerType = Homing;
            game.Player().m_bombRequested = true; game.TickBomb(); });
        for (int i = 0; i < 2; ++i) {
            assert(game.m_players[i].m_bombCount == Game::InitialBombCount - 1);
            assert(game.m_players[i].m_bomb.missiles[0].owner == i);
        }
        // 全機体でボム課題を完了できる
        for (PlayerType type : {Homing, Piercing, Spread}) {
            Prepare(game);
            game.m_tutorialMode = true;
            game.m_tutorialStep = 3;
            game.Player().m_playerType = type;
            game.BeginTutorialStep();
            game.m_tutorialStepFrame = 239;
            game.TickTutorial();
            assert(game.m_tutorialStep == 3);
            game.Player().m_bombRequested = true;
            game.TickTutorial();
            assert(game.m_tutorialStep == 4);
            game.m_tutorialMode = false;
        }
        Prepare(game);
    }

    /** @brief 被弾、その場復帰、無敵と両者不在時の進行を検証する @param game 検証対象 @return なし */
    static void CheckRespawn(Game& game) {
        Prepare(game);
        auto& blue = game.m_players[0];
        auto& green = game.m_players[1];
        blue.m_invincible = green.m_invincible = 0;
        blue.m_power = 2.0f;
        green.m_power = 3.0f;
        game.m_score = 123;
        game.m_frame = 40;
        const float greenX = green.m_playerX;
        const float greenY = green.m_playerY;

        // 実際の敵弾接触でも2Pだけが被弾し、同時にPowerを失う
        auto& shot = game.m_shots[0];
        shot = {};
        shot.active = shot.enemy = true;
        shot.x = greenX;
        shot.y = greenY;
        game.TickShots();
        assert(!shot.active && green.m_playerDestructionTimer == Game::PlayerDestructionWaitFrames);
        assert(blue.m_playerDestructionTimer == 0 && blue.m_power == 2.0f);
        assert(green.m_power == 2.5f && game.m_frame == 40 && game.Score() == 123);
        game.m_activePlayer = 1;
        game.DamagePlayer();
        assert(green.m_power == 2.5f);
        game.m_activePlayer = 0;

        // 復活待機中も1Pの操作とステージ時間が進み、2Pは同じ座標へ戻る
        blue.m_moveRight = true;
        for (int frame = 0; frame < Game::PlayerDestructionWaitFrames; ++frame) game.Tick();
        assert(game.m_frame == 40 + Game::PlayerDestructionWaitFrames);
        assert(game.m_stageNumber == 1 && game.m_chapterNumber == 1 && game.m_restartTimer == 0);
        assert(blue.m_playerX > greenX);
        assert(green.m_playerX == greenX && green.m_playerY == greenY);
        assert(green.m_playerDestructionTimer == 0 && green.m_invincible > 0);
        assert(game.Score() == 123);
        game.m_activePlayer = 1;
        game.DamagePlayer();
        assert(green.m_playerDestructionTimer == 0 && green.m_power == 2.5f);

        // Powerが尽きてもゲームオーバーにならず、両者の復帰を個別に進める
        green.m_invincible = 0;
        green.m_power = 0.25f;
        game.DamagePlayer();
        assert(green.m_power == 0.0f);
        game.m_activePlayer = 0;
        blue.m_invincible = 0;
        game.DamagePlayer();
        const int before = game.m_frame;
        game.Tick();
        assert(game.m_frame == before + 1 && !game.m_clear);
        assert(blue.m_playerDestructionTimer == Game::PlayerDestructionWaitFrames - 1);
        assert(green.m_playerDestructionTimer == Game::PlayerDestructionWaitFrames - 1);
    }

    /** @brief 両方向の視点遷移中も離れた2機が画面内に入ることを検証する @param game 検証対象 @return なし */
    static void CheckCamera(Game& game) {
        Prepare(game);
        CoopCameraBackend backend;
        Renderer renderer(backend);
        game.m_players[0].m_playerX = Game::Side2DPlayerMinX;
        game.m_players[0].m_playerY = Game::Side2DPlayerMinY;
        game.m_players[1].m_playerX = Game::Side2DPlayerMaxX;
        game.m_players[1].m_playerY = Game::Side2DPlayerMaxY;
        for (bool reverse : {false, true}) {
            game.m_viewMode = reverse ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            game.m_nextViewMode = reverse ? Game::ViewMode::Side2D : Game::ViewMode::Rail3D;
            game.m_viewTransitionTimer = 1;
            for (int step = 0; step <= 10; ++step) {
                game.m_viewTransitionProgress = step / 10.0f;
                Camera3D camera;
                game.ConfigureRailCamera(camera, renderer);
                game.ForEachPlayer([&] {
                    Vector3 position = game.PlayerWorldPosition();
                    position.z = Math::Lerp(Game::SidePlaneZ, game.PlayerRailDepth(), game.RailBlend());
                    Vector2 screen;
                    assert(camera.TryWorldToScreen(position, screen));
                    assert(screen.x >= 0.0f && screen.x <= backend.Width());
                    assert(screen.y >= 0.0f && screen.y <= backend.Height());
                });
            }
        }
    }

    /** @brief 第2部の3D自機弾だけ遠方の表示サイズを維持することを検証する @param game 検証対象 @return なし */
    static void CheckPart2ShotSize(Game& game) {
        Prepare(game);
        CoopCameraBackend backend;
        Renderer renderer(backend);
        Camera3D camera;
        camera.SetViewport({0, 0, backend.Width(), backend.Height()});
        Game::Shot shot {};
        shot.z = 30.0f;
        shot.hitRadius = 0.01f;
        game.m_viewMode = Game::ViewMode::Rail3D;
        game.m_viewTransitionTimer = 0;

        // 同一カメラでステージ、区間、弾種だけを変えて対象範囲を検証する
        for (bool special : {false, true}) {
            shot.special = special;
            const Vector2 minimum = special ? Vector2 {0.030f, 0.015f} : Vector2 {0.04125f, 0.021f};
            for (int stage : {1, 5}) {
                game.m_stageNumber = stage;
                for (auto phase : {ShooterStages::Stage5::Phase::EastsourceBattle,
                        ShooterStages::Stage5::Phase::WallClimbLower,
                        ShooterStages::Stage5::Phase::WallClimbMiddle,
                        ShooterStages::Stage5::Phase::WallClimbUpper}) {
                    game.m_stage5.phase = phase;
                    renderer.BeginFrame();
                    game.DrawShotModel(renderer, camera, shot, 0.0f);
                    assert(renderer.CommandCount() == 1);
                    const Vector2 size = renderer.Command(0).playerShot.size;
                    if (stage == 5 && phase != ShooterStages::Stage5::Phase::EastsourceBattle) {
                        assert(size.x >= minimum.x - 0.00001f && size.y >= minimum.y - 0.00001f);
                    } else {
                        assert(size.x < minimum.x && size.y < minimum.y);
                    }
                }
            }
        }
    }

    /** @brief 通信入力が実機入力に依存せず両機に適用されることを検証する @param game 検証対象 @return なし */
    static void CheckNetworkInput(Game& game) {
        Prepare(game);
        InputTestAccess::SetPad(0, {KeyCode::RightArrow});
        InputTestAccess::SetPad(1, {KeyCode::LeftArrow});
        std::array<CooperativeInput, 2> inputs {};
        inputs[0].held = CooperativeInput::Left | CooperativeInput::Fire;
        inputs[1].held = CooperativeInput::Right | CooperativeInput::Slow;
        inputs[1].pressed = CooperativeInput::Bomb;
        game.ApplyNetworkInput(inputs);
        assert(game.m_players[0].m_moveLeft && !game.m_players[0].m_moveRight);
        assert(game.m_players[0].m_fire && !game.m_players[0].m_bombRequested);
        assert(game.m_players[1].m_moveRight && !game.m_players[1].m_moveLeft);
        assert(game.m_players[1].m_slowMove && game.m_players[1].m_bombRequested);

        // 次の同期フレームで一度きりのボム操作を繰り返さない
        inputs[1].pressed = 0;
        game.ApplyNetworkInput(inputs);
        assert(!game.m_players[1].m_bombRequested);
        game.Initialize(nullptr, Homing, Easy, 1);
        assert(!game.m_networkGame);
    }

    /** @brief 欠落待機・順序検証・リング再利用を確認する @return なし */
    static void CheckNetworkFrames() {
        CooperativeFrames frames;
        frames.Reset();
        std::array<CooperativeInput, 2> output {};
        for (unsigned i = 0; i < CooperativeFrames::Delay; ++i) assert(frames.Pop(output));
        assert(!frames.Pop(output));
        assert(!frames.Push(1, CooperativeFrames::Delay + 1, {}));
        assert(!frames.Push(1, CooperativeFrames::Delay, {65535, 0}));
        for (unsigned frame = CooperativeFrames::Delay; frame < 1024; ++frame) {
            assert(frames.Push(0, frame, {CooperativeInput::Left, 0}));
            assert(!frames.Pop(output));
            assert(frames.Push(1, frame, {CooperativeInput::Right, CooperativeInput::Bomb}));
            assert(!frames.Push(1, frame, {}));
            assert(frames.Pop(output));
            assert(output[0].held == CooperativeInput::Left && output[1].pressed == CooperativeInput::Bomb);
        }
    }

    /** @brief 同じ乱数種と入力で別インスタンスの進行が一致することを検証する @return なし */
    static void CheckNetworkSimulation() {
        auto host = std::make_unique<Game>();
        auto guest = std::make_unique<Game>();
        GameplayRandom::State = 12345;
        host->Initialize(nullptr, Homing, Normal, 2, Spread);
        unsigned hostRandom = GameplayRandom::State;
        GameplayRandom::State = 12345;
        guest->Initialize(nullptr, Homing, Normal, 2, Spread);
        unsigned guestRandom = GameplayRandom::State;
        for (int frame = 0; frame < 1800; ++frame) {
            std::array<CooperativeInput, 2> inputs {};
            inputs[0].held = CooperativeInput::Fire | (frame % 120 < 60 ? CooperativeInput::Up : CooperativeInput::Down);
            inputs[1].held = CooperativeInput::Fire | CooperativeInput::Slow;
            inputs[0].pressed = frame % 180 == 0 ? CooperativeInput::Bomb : CooperativeInput::Confirm;
            GameplayRandom::State = hostRandom;
            host->ApplyNetworkInput(inputs); host->Tick();
            hostRandom = GameplayRandom::State;
            GameplayRandom::State = guestRandom;
            guest->ApplyNetworkInput(inputs); guest->Tick();
            guestRandom = GameplayRandom::State;
            assert(hostRandom == guestRandom && host->m_frame == guest->m_frame);
            assert(host->Score() == guest->Score() && host->m_bossHp == guest->m_bossHp);
            for (int i = 0; i < 2; ++i) {
                assert(host->m_players[i].m_playerX == guest->m_players[i].m_playerX);
                assert(host->m_players[i].m_playerY == guest->m_players[i].m_playerY);
                assert(host->m_players[i].m_power == guest->m_players[i].m_power);
            }
        }
    }

    /** @brief 被弾判定球が全自機より後に深度テストなしで描画されることを検証する @param game 検証対象 @return なし */
    static void CheckHitboxOverlay(Game& game) {
        Prepare(game);
        CoopCameraBackend backend;
        Renderer renderer(backend);
        // 2D、3D、視点遷移中と低速入力の有無を検証する
        for (int view = 0; view < 3; ++view) {
            game.m_viewMode = view == 1 ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            game.m_nextViewMode = Game::ViewMode::Rail3D;
            game.m_viewTransitionTimer = view == 2 ? 10 : 0;
            game.m_viewTransitionProgress = view == 2 ? 0.5f : 0.0f;
            for (int expected = 0; expected <= 2; ++expected) {
                for (int player = 0; player < 2; ++player) {
                    game.m_players[player].m_slowMove = player < expected;
                    game.m_players[player].m_invincible = 0;
                }
                renderer.BeginFrame();
                if (view == 0) game.Render2D(renderer);
                else game.Render3D(renderer);
                PipelineId pipeline = PipelineId::Model3D;
                int hitboxes = 0;
                for (size_t i = 0; i < renderer.CommandCount(); ++i) {
                    const auto& command = renderer.Command(i);
                    if (command.type == RenderCommand::Type::ResetCamera) break;
                    if (command.type == RenderCommand::Type::Pipeline) pipeline = command.pipeline;
                    if (command.type != RenderCommand::Type::Primitive3D) continue;
                    const auto& primitive = command.primitive;
                    const bool hitbox = primitive.shape == PrimitiveShape::Sphere &&
                        primitive.color.r == 1.0f && primitive.color.g == 0.08f && primitive.color.b == 0.08f;
                    if (hitbox) {
                        assert(pipeline == PipelineId::Object && primitive.color.a >= 0.5f);
                        ++hitboxes;
                    } else {
                        assert(hitboxes == 0);
                    }
                }
                assert(hitboxes == expected);
            }
        }
    }

    /** @brief F12で表示だけが切り替わり、保持中は再反転しないことを検証する @param game 検証対象 @return なし */
    static void CheckDebugHitboxToggle(Game& game) {
#if defined(_DEBUG)
        Prepare(game);
        CoopCameraBackend backend;
        Renderer renderer(backend);
        // 全ステージの両視点で描画数と進行状態を確認する
        for (int stage = 1; stage <= 5; ++stage) {
            game.StartDebugCheckpoint(stage, 1, false);
            for (const auto mode : {Game::ViewMode::Side2D, Game::ViewMode::Rail3D}) {
                game.m_viewMode = mode;
                const auto phase = game.m_stage5.phase;
                assert(game.m_showHitboxes);
                renderer.BeginFrame();
                game.Render(renderer);
                const auto visibleCount = renderer.CommandCount();
                InputTestAccess::SetHitboxKey(true);
                game.ProcessInput();
                assert(!game.m_showHitboxes && game.m_stage5.phase == phase);
                renderer.BeginFrame();
                game.Render(renderer);
                assert(renderer.CommandCount() < visibleCount);
                InputTestAccess::SetHitboxKey(false);
                game.ProcessInput();
                assert(!game.m_showHitboxes);
                InputTestAccess::SetHitboxKey(true);
                game.ProcessInput();
                assert(game.m_showHitboxes && game.m_stage5.phase == phase);
                InputTestAccess::SetHitboxKey(false);
                renderer.BeginFrame();
                game.Render(renderer);
                assert(renderer.CommandCount() == visibleCount);
            }
        }
#endif
    }

    /** @brief ローカル協力プレイの主要ルールを検証する @return なし */
    static void Run() {
        CheckStage3BarrierBounds();
        auto game = std::make_unique<Game>();
        CheckGrazeRecovery(*game);
        CheckInputAndShots(*game);
        CheckItems(*game);
        CheckBombs(*game);
        CheckRespawn(*game);
        CheckCamera(*game);
        CheckPart2ShotSize(*game);
        CheckHitboxOverlay(*game);
        CheckDebugHitboxToggle(*game);
        CheckNetworkInput(*game);
        CheckNetworkFrames();
        CheckNetworkSimulation();
        RunCoopStageTests();
        std::puts("CoopGameplayTests passed");
    }

    /** @brief 両視点で継続グレイズと離脱時の回復速度を検証する @param game 検証対象 @return なし */
    static void CheckGrazeRecovery(Game& game) {
        for (bool rail : {false, true}) {
            Prepare(game);
            if (rail) {
                game.RequestViewMode(Game::ViewMode::Rail3D);
                for (int i = 0; i < Game::ViewTransitionFrames; ++i) game.TickViewTransition();
            }
            game.m_viewToggleCooldown = 100;
            game.Tick();
            assert(!game.m_grazing && game.m_viewToggleCooldown == 99);

            // 2Pの近傍に得点済みの静止弾を複数置いても、共有ゲージは2倍まで
            game.m_activePlayer = 1;
            const Vector3 player = game.PlayerWorldPosition();
            game.m_activePlayer = 0;
            for (int i = 0; i < 3; ++i) {
                auto& shot = game.m_shots[i];
                shot = {};
                shot.active = shot.enemy = shot.grazed = true;
                shot.x = rail ? Game::FromWorldX(player.x + 0.8f) : game.m_players[1].m_playerX + 0.13f;
                shot.y = game.m_players[1].m_playerY;
                shot.z = player.z;
            }
            game.Tick();
            assert(game.m_grazing && game.m_viewToggleCooldown == 97);
            game.Tick();
            assert(game.m_grazing && game.m_viewToggleCooldown == 95);
            assert(game.m_chapterResult.grazeCount == 0);

            // 完了直前でも負数にせず、弾が離れたフレームから通常速度へ戻す
            game.m_viewToggleCooldown = 1;
            game.Tick();
            assert(game.m_grazing && game.m_viewToggleCooldown == 0);
            game.m_shots = {};
            game.m_viewToggleCooldown = 100;
            game.Tick();
            assert(!game.m_grazing && game.m_viewToggleCooldown == 99);
        }
    }
};

/** @brief 協力プレイの回帰テストを起動する @return 成功時0 */
int main() { CoopGameplayTests::Run(); return 0; }
