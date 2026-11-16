#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Common/StageDefinition.h"
#include "../Presentation/Gameplay/Stages/Stage1/Stage1Module.h"

/** @brief 敵の行動スクリプトの実行結果を検証する */
struct EnemyScriptTests {
    using Game = SideScrollingShooter;
    using Behavior = Game::ScriptedEnemyBehavior;

    /**
     * @brief 2Dのステージ1として最小限の状態を作る
     * @return 生成したゲーム
     */
    static std::unique_ptr<Game> MakeGame() {
        auto game = std::make_unique<Game>();
        game->m_stageNumber = 1;
        game->m_difficulty = Normal;
        game->m_stage = &Game::Stage1Module::Definition(Normal);
        return game;
    }

    /**
     * @brief 有効な敵弾の数を数える
     * @param game 対象のゲーム
     * @return 有効な敵弾の数
     */
    static int CountEnemyShots(const Game& game) {
        int count = 0;
        for (const auto& shot : game.m_shots) {
            if (shot.active && shot.enemy) ++count;
        }
        return count;
    }

    /**
     * @brief 浮動小数点数がほぼ等しいか判定する
     * @param a 比較する値
     * @param b 比較する値
     * @return 差が1e-4未満ならtrue
     */
    static bool Near(float a, float b) {
        return std::abs(a - b) < 1e-4f;
    }

    /** @brief MoveToが指定フレームで目的地へ着き、次のコマンドへ進むことを検証する @return なし */
    static void CheckMoveTo() {
        using namespace EnemyScript;
        static constexpr Command Commands[] = {
            {MoveTo, 10, 1.0f, 0.5f, 0.0f},
            {Move, 5, 0.0f, 0.0f, 0.0f}
        };
        const Definition script {Commands, 2, 10, 100};
        auto game = MakeGame();
        Game::Enemy enemy;
        enemy.x = 2.0f;

        // 10フレームで等速移動して目的地に着く
        for (int i = 0; i < 10; ++i) Behavior::Step(*game, enemy, script);
        assert(Near(enemy.x, 1.0f) && Near(enemy.y, 0.5f));
        assert(enemy.scriptPc == 0);

        // 次のフレームで待機へ進み、その場に留まる
        Behavior::Step(*game, enemy, script);
        assert(enemy.scriptPc == 1);
        assert(Near(enemy.x, 1.0f) && Near(enemy.y, 0.5f));
    }

    /** @brief Repeatが指定回数だけ戻り、発射コマンドがその回数撃つことを検証する @return なし */
    static void CheckRepeatAndRing() {
        using namespace EnemyScript;
        static constexpr Command Commands[] = {
            {FireRing, 0, 4.0f, 0.01f, 0.0f},
            {Move, 5, 0.0f, 0.0f, 0.0f},
            {Repeat, 0, 0.0f, 2.0f, 0.0f}
        };
        const Definition script {Commands, 3, 10, 100};
        auto game = MakeGame();
        Game::Enemy enemy;
        enemy.x = 1.0f;

        // 初回と2回の繰り返しで4発ずつ、合計12発を撃つ
        for (int i = 0; i < 40; ++i) Behavior::Step(*game, enemy, script);
        assert(CountEnemyShots(*game) == 12);
        assert(enemy.scriptPc == 3);
        assert(enemy.scriptLoop == 0);

        // 全周弾の1発目は右向き（0度）に飛ぶ
        bool foundRight = false;
        for (const auto& shot : game->m_shots) {
            if (shot.active && shot.enemy && Near(shot.vx, 0.01f) && Near(shot.vy, 0.0f)) foundRight = true;
        }
        assert(foundRight);
    }

    /** @brief FireFanの1発が自機方向へ飛ぶことを検証する @return なし */
    static void CheckAimedFan() {
        using namespace EnemyScript;
        static constexpr Command Commands[] = {
            {FireFan, 0, 1.0f, 0.0f, 0.02f}
        };
        const Definition script {Commands, 1, 10, 100};
        auto game = MakeGame();
        game->Player().m_playerX = -1.0f;
        game->Player().m_playerY = 0.25f;
        Game::Enemy enemy;
        enemy.x = 1.0f;
        enemy.y = 0.25f;

        // 自機は真左にいるため左向きに撃つ
        Behavior::Step(*game, enemy, script);
        assert(CountEnemyShots(*game) == 1);
        for (const auto& shot : game->m_shots) {
            if (!shot.active || !shot.enemy) continue;
            assert(Near(shot.vx, -0.02f) && Near(shot.vy, 0.0f));
        }
    }

    /** @brief Sineの上下移動と、終了後に画面外へ離脱することを検証する @return なし */
    static void CheckSineAndExit() {
        using namespace EnemyScript;
        static constexpr Command Commands[] = {
            {Sine, 40, -0.01f, 0.3f, 40.0f}
        };
        const Definition script {Commands, 1, 10, 100};
        auto game = MakeGame();
        Game::Enemy enemy;
        enemy.x = 1.0f;
        enemy.y = 0.1f;

        // 周期の1/4で振幅分だけ上にいる
        for (int i = 0; i < 10; ++i) Behavior::Step(*game, enemy, script);
        assert(Near(enemy.y, 0.4f));
        assert(Near(enemy.x, 0.9f));

        // 終了後もX速度を保って左へ進む
        for (int i = 0; i < 40; ++i) Behavior::Step(*game, enemy, script);
        assert(enemy.scriptPc == 1);
        assert(Near(enemy.x, 0.5f));
    }

    /** @brief 時間を使わない無限ループでも1フレームで処理が戻ることを検証する @return なし */
    static void CheckInstantLoopGuard() {
        using namespace EnemyScript;
        static constexpr Command Commands[] = {
            {FireRing, 0, 1.0f, 0.01f, 0.0f},
            {Repeat, 0, 0.0f, 0.0f, 0.0f}
        };
        const Definition script {Commands, 2, 10, 100};
        auto game = MakeGame();
        Game::Enemy enemy;
        enemy.x = 1.0f;

        // 16コマンドで打ち切るため、発射は8回だけ
        Behavior::Step(*game, enemy, script);
        assert(CountEnemyShots(*game) == 8);
    }

    /** @brief SpawnEnemyがスクリプト番号を渡し、範囲外の番号でも0番を使うことを検証する @return なし */
    static void CheckSpawn() {
        auto game = MakeGame();
        game->SpawnEnemy(Game::Stage::ScriptedEnemy, 2.6f, 0.0f, 0.2f, 60.0f, 200);
        const Game::Enemy* spawned = nullptr;
        for (const auto& enemy : game->m_enemies) {
            if (enemy.active) spawned = &enemy;
        }
        assert(spawned != nullptr);
        assert(spawned->type == Game::Stage::ScriptedEnemy);
        assert(spawned->behavior == &Game::ScriptedEnemyBehaviorInstance());
        Game::Enemy expected;
        expected.hp = EnemyScript::Scripts[0].hp;
        game->ApplyDifficultyToEnemyHp(expected);
        assert(spawned->maxHp == expected.maxHp);
        assert(spawned->behavior->Score(*spawned) == EnemyScript::Scripts[0].score);
        assert(spawned->shotInterval == 0);
    }
};

/** @brief 行動スクリプトのチェックを実行する @return 成功時0 */
int main() {
    EnemyScriptTests::CheckMoveTo();
    EnemyScriptTests::CheckRepeatAndRing();
    EnemyScriptTests::CheckAimedFan();
    EnemyScriptTests::CheckSineAndExit();
    EnemyScriptTests::CheckInstantLoopGuard();
    EnemyScriptTests::CheckSpawn();
    std::puts("EnemyScriptTests passed");
    return 0;
}
