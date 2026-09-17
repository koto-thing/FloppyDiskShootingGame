#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Common/StageDefinition.h"

/** @brief コード変換と実際の再開地点をGPUなしで検証する */
struct ResumeCodeTests {
    /** @brief 正常系、入力境界、全地点の復元を検証する @return なし */
    static void Run() {
        ResumeCode decoded;
        assert(ResumeCode::Parse("23EH25010000", decoded));
        assert(decoded.stage == 2 && decoded.chapter == '3' && decoded.power == 250 && decoded.score == 10000);
        assert(ResumeCode::Parse("522EH25010000", decoded) && decoded.part == 2 && decoded.chapter == '2');
        assert(ResumeCode::Parse("52BEH25010000", decoded) && decoded.chapter == 'B');
        assert(ResumeCode::Parse("52DEH25010000", decoded) && decoded.chapter == 'D');
        assert(ResumeCode::Parse("11HS400999999999-3-1000000-1000000-1000000-1000000", decoded));
        for (const char* invalid : {"", "1", "51", "11EH000", "522EH00", "61EH0000", "14EH0000",
            "51DEH0000", "53BEH0000", "52EH25010000", "23XH25010000", "23EX25010000",
            "23EH40110000", "23EH250-1", "23EH2502147483648", "23EH25010000junk",
            "23EH25010000-4-0-0-0-0", "23EH25010000-3-0-0-0", "23EH25010000-3-0-0-0-0-0",
            "23EH25010000-3-0-0-0-", "23EH25010000-3-0-0-0-1000001"}) {
            const int previous = decoded.score;
            assert(!ResumeCode::Parse(invalid, decoded));
            assert(decoded.score == previous);
        }

        auto game = std::make_unique<SideScrollingShooter>();
        auto& g = *game;
        // 全難易度・機体・チャプター・ボスで実際の初期化と再エンコードを確認する
        for (int stage = 1; stage <= 5; ++stage) {
            for (int part = 1; part <= (stage == 5 ? 2 : 1); ++part) {
                for (char chapter : {'1', '2', '3', 'B', 'D'}) {
                    if (chapter == 'D' && (stage != 5 || part != 2)) continue;
                    for (int difficulty = 0; difficulty < 3; ++difficulty) {
                        for (int player = 0; player < 3; ++player) {
                            ResumeCode code;
                            code.stage = stage;
                            code.part = part;
                            code.chapter = chapter;
                            code.difficulty = static_cast<DifficultyType>(difficulty);
                            code.player = static_cast<PlayerType>(player);
                            code.power = 250;
                            code.score = 10000;
                            code.bombs = 1;
                            code.kills = 42;
                            code.retries = {1, 2, 3};
                            const std::string text = code.Encode();
                            assert(g.RestoreResumeCode(text));
                            assert(g.GetResumeCode() == text);
                            assert(g.m_difficulty == code.difficulty && g.Player().m_playerType == code.player);
                            assert(std::abs(g.Player().m_power - 2.5f) < 0.0001f);
                            assert(g.m_score == 10000 && g.Player().m_bombCount == 1 && g.m_kills == 42);
                            assert(g.m_chapterRetryCounts == code.retries);
                            assert(g.m_chapterStartScore == 10000 && g.m_stage5.checkpointScore == 10000);
                            const bool boss = chapter == 'B' || chapter == 'D';
                            if (!boss) assert(g.m_frame == g.m_stage->ChapterEndFrame(chapter - '1'));
                            if (stage == 5 && part == 2) {
                                using Phase = ShooterStages::Stage5::Phase;
                                const Phase expected = chapter == '1' ? Phase::WallClimbLower :
                                    chapter == '2' ? Phase::WallClimbMiddle : chapter == '3' ? Phase::WallClimbUpper :
                                    chapter == 'B' ? Phase::TayamaFireControl : Phase::TayamaDragonBattle;
                                assert(g.m_stage5.phase == expected);
                            }
                            g.Tick();
                            ResumeCode after;
                            assert(ResumeCode::Parse(g.GetResumeCode(), after));
                            assert(after.stage == stage && after.part == part && after.chapter == chapter);
                        }
                    }
                }
            }
        }

        // 被弾後もコードで設定した開始スコアを使う
        assert(g.RestoreResumeCode("522EH25010000-1-42-1-2-3"));
        g.m_score = 11000;
        g.RestartCurrentChapter();
        assert(g.m_score == 10000 && g.m_chapterNumber == 2);
        assert(g.RestoreResumeCode("23EH25010000-1-42-1-2-3"));
        g.m_score = 11000;
        g.RestartCurrentChapter();
        assert(g.m_score == 10000 && g.m_chapterNumber == 3);

        // 協力プレイ・ネットワーク・チュートリアルからは生成も復元もできない
        g.m_playerCount = 2;
        assert(g.GetResumeCode().empty() && !g.RestoreResumeCode("23EH25010000"));
        g.m_playerCount = 1;
        g.m_networkGame = true;
        assert(g.GetResumeCode().empty() && !g.RestoreResumeCode("23EH25010000"));
        g.m_networkGame = false;
        g.m_tutorialMode = true;
        assert(g.GetResumeCode().empty() && !g.RestoreResumeCode("23EH25010000"));
    }
};

/** @brief 再開コードの回帰チェックを実行する @return 成功時0 */
int main() {
    ResumeCodeTests::Run();
    std::puts("Resume code tests passed");
    return 0;
}
