#include "../Infrastructure/Repositories/ScoreRepository.h"

#include <stdexcept>

/**
 * @brief ランキングへのスコア挿入が上位5件と降順を維持することを検証する
 * @return なし
 */
void RunScoreRepositoryTests() {
    const ScoreRepository::Scores scores {{ 9000, 7000, 5000, 3000, 1000 }};
    const ScoreRepository::Scores inserted = ScoreRepository::InsertScore(scores, 6000);
    const ScoreRepository::Scores expected {{ 9000, 7000, 6000, 5000, 3000 }};
    if (inserted != expected) {
        throw std::runtime_error("Ranking must retain the highest five scores in descending order");
    }
}
