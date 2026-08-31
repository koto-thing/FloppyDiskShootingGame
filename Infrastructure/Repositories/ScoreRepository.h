#pragma once

#include <array>

#include "../../Domain/ValueObjects/DifficultyType.h"

/** @brief 難易度別ランキングをユーザーの永続データ領域へ保存するリポジトリ */
class ScoreRepository {
public:
    static constexpr int DifficultyCount = 3;
    static constexpr int RankCount = 5;
    using Scores = std::array<int, RankCount>;
    using Rankings = std::array<Scores, DifficultyCount>;

    /**
     * @brief 保存済みランキングを取得する
     * @return 難易度ごとの高得点順ランキング
     */
    Rankings Load() const;
    /**
     * @brief スコアを難易度別ランキングへ登録する
     * @param difficulty 登録先の難易度
     * @param score 登録するスコア
     */
    void Save(DifficultyType difficulty, int score) const;
    /**
     * @brief スコアを上位5件へ挿入する
     * @param scores 挿入前の高得点順スコア
     * @param score 挿入するスコア
     * @return 挿入後の高得点順スコア
     */
    static Scores InsertScore(Scores scores, int score);
};
