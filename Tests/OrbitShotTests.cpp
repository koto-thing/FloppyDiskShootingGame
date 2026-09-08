#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Presentation/Gameplay/SideScrollingShooter.h"

struct OrbitShotTests {
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
        g.m_playerX = g.m_playerY = 0.0f;
        g.m_invincible = 999;

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
        std::puts("OrbitShotTests passed");
    }
};

/** @brief 包囲弾の回帰チェックを実行する @return 成功時0 */
int main() {
    OrbitShotTests::Run();
    return 0;
}
