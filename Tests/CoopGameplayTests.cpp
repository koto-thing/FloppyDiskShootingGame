#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <memory>

#include "../Engine/Input/Input.h"
#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
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

    /** @brief 同時ボムの個別消費と共通敵弾の消去を検証する @param game 検証対象 @return なし */
    static void CheckBombs(Game& game) {
        Prepare(game);
        game.m_players[0].m_bombCount = 1;
        game.m_players[1].m_bombCount = 3;
        game.ForEachPlayer([&] { game.Player().m_bombRequested = true; game.TickBomb(); });
        assert(game.m_players[0].m_bombCount == 0 && game.m_players[1].m_bombCount == 2);
        assert(game.m_players[0].m_bomb.active && game.m_players[1].m_bomb.active);
        game.ForEachPlayer([&] { game.Player().m_bombRequested = false; });
        game.m_shots[0].active = game.m_shots[0].enemy = true;
        game.m_shots[1].active = game.m_shots[1].enemy = true;
        game.m_shots[2].active = true;
        for (int frame = 1; frame < Game::BombTravelFrames + Game::BombChargeFrames; ++frame)
            game.ForEachPlayer([&] { game.TickBomb(); });
        assert(!game.m_shots[0].active && !game.m_shots[1].active && game.m_shots[2].active);
        assert(!game.m_players[0].m_bomb.active && !game.m_players[1].m_bomb.active);
        assert(game.m_players[0].m_bombCount == 0 && game.m_players[1].m_bombCount == 2);
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

    /** @brief ローカル協力プレイの主要ルールを検証する @return なし */
    static void Run() {
        auto game = std::make_unique<Game>();
        CheckInputAndShots(*game);
        CheckItems(*game);
        CheckBombs(*game);
        CheckRespawn(*game);
        CheckCamera(*game);
        CheckNetworkInput(*game);
        CheckNetworkFrames();
        CheckNetworkSimulation();
        RunCoopStageTests();
        std::puts("CoopGameplayTests passed");
    }
};

/** @brief 協力プレイの回帰テストを起動する @return 成功時0 */
int main() { CoopGameplayTests::Run(); return 0; }
