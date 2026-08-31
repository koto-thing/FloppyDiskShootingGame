#include "ScoreRepository.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <windows.h>

namespace {
/**
 * @brief ユーザーごとのランキング保存先を取得する
 * @return Application.persistentDataPath相当のランキングファイルパス
 */
std::filesystem::path RankingPath() {
    wchar_t localAppData[MAX_PATH] {};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    const std::filesystem::path base = length > 0 && length < MAX_PATH
        ? localAppData : std::filesystem::current_path();
    return base / L"FloppyDiskShootingGame" / L"rankings.dat";
}
}

/**
 * @brief 保存済みランキングを取得する
 * @return 難易度ごとの高得点順ランキング
 */
ScoreRepository::Rankings ScoreRepository::Load() const {
    Rankings rankings {};
    std::ifstream input(RankingPath());
    for (Scores& scores : rankings) {
        for (int& score : scores) {
            if (!(input >> score) || score < 0) return Rankings {};
        }
        std::sort(scores.rbegin(), scores.rend());
    }
    return rankings;
}

/**
 * @brief スコアを難易度別ランキングへ登録する
 * @param difficulty 登録先の難易度
 * @param score 登録するスコア
 */
void ScoreRepository::Save(DifficultyType difficulty, int score) const {
    Rankings rankings = Load();
    const int difficultyIndex = (std::clamp)(static_cast<int>(difficulty), 0, DifficultyCount - 1);
    rankings[static_cast<size_t>(difficultyIndex)] = InsertScore(
        rankings[static_cast<size_t>(difficultyIndex)], score);

    const std::filesystem::path path = RankingPath();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return;

    const std::filesystem::path temporaryPath = path.parent_path() / L"rankings.tmp";
    std::ofstream output(temporaryPath, std::ios::trunc);
    if (!output) return;
    for (const Scores& scores : rankings) {
        for (const int savedScore : scores) output << savedScore << '\n';
    }
    output.close();
    if (!output) return;

    // 一時ファイルを置き換えて、書き込み途中のデータ破損を防ぐ
    if (std::filesystem::exists(path, error)) {
        ReplaceFileW(path.c_str(), temporaryPath.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
    } else {
        MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
    }
}

/**
 * @brief スコアを上位5件へ挿入する
 * @param scores 挿入前の高得点順スコア
 * @param score 挿入するスコア
 * @return 挿入後の高得点順スコア
 */
ScoreRepository::Scores ScoreRepository::InsertScore(Scores scores, int score) {
    if (score < 0) return scores;
    scores[RankCount - 1] = (std::max)(scores[RankCount - 1], score);
    std::sort(scores.rbegin(), scores.rend());
    return scores;
}
