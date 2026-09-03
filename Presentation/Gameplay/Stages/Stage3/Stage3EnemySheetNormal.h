#pragma once

#include "../../GameplayRandom.h"
#include "Stage3EnemySheet.h"

/** @brief NORMAL用のStage3敵出現シートを定義する */
class SideScrollingShooter::Stage3EnemySheetNormal final : public SideScrollingShooter::Stage3EnemySheet {
public:
    static constexpr int ChapterLength = 1200;

    /**
     * @brief 1チャプターの長さを取得する
     * @return NORMAL用チャプターのフレーム数
     */
    int ChapterFrameLength() const override {
        return ChapterLength;
    }

    /**
     * @brief 指定フレームで出現させるNORMAL用の敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {
            {StraightShooterEnemy, 24, 54, 1.10f, -0.82f, 0.88f, 50.0f},
            {DiveRusherEnemy, 72, 166, 1.12f, -0.28f, 1.5f, 60.0f},
            {ArmoredEnemy, 110, 250, 1.12f, -0.40f, -0.42f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {StraightShooterEnemy, 10, 52, 1.10f, 0.28f, -0.88f, 40.0f},
            {CircleShooterEnemy, 70, 165, 1.14f, 0.82f, 0.32f, 56.0f},
            {DiveRusherEnemy, 112, 188, 1.12f, -0.58f, 0.96f, 60.0f},
            {HeavyEnemy, 150, 230, 1.16f, 0.05f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {ArmoredEnemy, 10, 140, 1.12f, -0.85f, -0.18f, 60.0f},
            {CircleShooterEnemy, 60, 145, 1.14f, -0.28f, 0.86f, 50.0f},
            {DiveRusherEnemy, 92, 180, 1.12f, 0.48f, 0.96f, 60.0f},
            {HeavyEnemy, 120, 180, 1.16f, 0.82f, -0.68f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        const bool selected = TrySelectByChapters(Chapters, 3, ChapterFrameLength(), frame, spawnIndex, spawn, chapterNumber);
        // YとZの生成範囲をランダム化
        if (selected) {
            if (spawn.enemyType != DiveRusherEnemy) {
                spawn.y = GameplayRandom::Range(-0.86f, 0.86f);
                spawn.railX = GameplayRandom::Range(-1.0f, 1.0f);
            }
        }

        return selected;
    }
};
