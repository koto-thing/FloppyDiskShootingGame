#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Infrastructure/Repositories/SettingsRepository.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Common/StageDefinition.h"

/** @brief ステージ選択からの開始と進行状況の解放をGPUなしで検証する */
struct StageSelectTests {
    /** @brief 全ステージの開始地点、リスタート、解放条件を検証する @return なし */
    static void Run() {
        auto game = std::make_unique<SideScrollingShooter>();
        auto& g = *game;

        // 全難易度・機体で各ステージの先頭から正しい状態で開始できる
        for (int stage = 1; stage <= 5; ++stage) {
            const float power = static_cast<float>(stage - 1);
            for (int difficulty = 0; difficulty < 3; ++difficulty) {
                for (int player = 0; player < 3; ++player) {
                    g.m_difficulty = static_cast<DifficultyType>(difficulty);
                    g.Reset(true);
                    g.Player().m_playerType = static_cast<PlayerType>(player);
                    g.StartFromStage(stage);
                    assert(g.m_stageNumber == stage && g.m_chapterNumber == 1);
                    assert(g.m_frame == g.m_stage->ChapterEndFrame(0));
                    assert(std::abs(g.Player().m_power - power) < 0.0001f);
                    assert(g.Player().m_power <= SideScrollingShooter::MaxPower);
                    assert(g.m_score == 0 && g.m_kills == 0);
                    assert(g.Player().m_bombCount == SideScrollingShooter::InitialBombCount);
                    assert(std::abs(g.m_stage5.checkpointPower - power) < 0.0001f);
                    g.Tick();
                    assert(g.m_stageNumber == stage);

                    // 被弾後も選択したステージと開始時スコアへ戻り、パワーは通常の減少だけを受ける
                    g.m_score = 1000;
                    g.RestartCurrentChapter();
                    assert(g.m_stageNumber == stage && g.m_score == 0);
                    assert(g.Player().m_power <= power + 0.0001f);
                }
            }

            // ボス戦でボムを使い切っても、繰り返しのリスタートで初期数へ戻る
            g.StartDebugCheckpoint(stage, 3, true);
            for (int remaining : {0, 1, 2}) {
                g.Player().m_bombCount = remaining;
                g.RestartCurrentChapter();
                assert(g.Player().m_bombCount == SideScrollingShooter::InitialBombCount);
            }
        }

        // ボス撃破で次だけを解放し、チュートリアルと最終ステージでは解放しない
        const auto tickCleared = [&](int stage, int unlocked, bool tutorial) {
            g.Reset(true);
            g.StartDebugCheckpoint(stage, 1, false);
            g.m_unlockedStage = unlocked;
            g.m_tutorialMode = tutorial;
            g.m_clear = true;
            g.m_clearTimer = 100;
            g.m_missionStartTimer = 0;
            g.Tick();
            g.m_tutorialMode = false;
            return g.m_unlockedStage;
        };
        assert(tickCleared(1, 1, false) == 2);
        assert(SettingsRepository {}.Load().unlockedStage >= 2);
        assert(tickCleared(4, 4, false) == 5);
        assert(tickCleared(1, 3, false) == 3);
        assert(tickCleared(5, 5, false) == 5);
        assert(tickCleared(2, 2, true) == 2);
    }
};

/** @brief ステージ選択の回帰チェックを実行する @return 成功時0 */
int main() {
    StageSelectTests::Run();
    std::puts("Stage select tests passed");
    return 0;
}
