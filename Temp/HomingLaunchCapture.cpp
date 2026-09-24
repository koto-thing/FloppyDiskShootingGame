#include <cstdio>
#include <memory>
#include "../Presentation/Gameplay/SideScrollingShooter.h"

struct HomingShotTests {
    /** @brief 実際の発射と更新で3斉射の弾道をCSVへ記録する @return なし */
    static void Capture() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_stageNumber = 1;
        g.m_viewMode = Game::ViewMode::Side2D;
        g.Player().m_playerX = g.Player().m_playerY = 0.0f;
        auto& enemy = g.m_enemies[0];
        enemy.active = enemy.collisionEnabled = true;
        enemy.hp = 1000;
        enemy.x = 1.2f;
        std::puts("frame,slot,x,y,age,active");
        // 操作や描画を代用せず、ゲームの共通発射・移動・衝突判定を実行する
        for (int frame = 0; frame < 120; ++frame) {
            g.m_frame = frame;
            if (frame <= 20 && frame % 10 == 0) g.FireSpecialShots();
            g.TickShots();
            for (int slot = 0; slot < 6; ++slot) {
                const auto& shot = g.m_shots[slot];
                if (!shot.special) continue;
                std::printf("%d,%d,%.6f,%.6f,%d,%d\n", frame, slot, shot.x, shot.y, shot.age, shot.active);
            }
        }
    }
};

/** @brief 弾道記録を実行する @return 終了コード */
int main() { HomingShotTests::Capture(); }
