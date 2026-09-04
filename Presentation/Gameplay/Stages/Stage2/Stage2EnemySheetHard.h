#pragma once

#include "../../GameplayRandom.h"
#include "Stage2EnemySheet.h"

/** @brief HARD用のステージ2敵出現シートを定義する */
class SideScrollingShooter::Stage2EnemySheetHard final : public SideScrollingShooter::Stage2EnemySheet {
public:
    static constexpr int ChapterLength = 1200;

    /**
     * @brief 1チャプターの長さを取得する
     * @return HARD用チャプターのフレーム数
     */
    int ChapterFrameLength() const override {
        return ChapterLength;
    }

    /**
     * @brief 指定フレームで出現させるHARD用の敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        // チャプター1
        static constexpr EnemySpawnRule Chapter1[] = {
            {BasicEnemy, 35, 150, 2.6f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, 200, 400, 2.6f, -0.18f, 0.86f, 48.0f},
            {StraightShooterEnemy, 400, 400, 2.6f, -0.18f, -0.86f, 48.0f},
            {SquareShooterEnemy, 500, 1200, 2.6f, 0.0f, 0.0f, 54.0f}
        };
        // チャプター2
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 25, 150, 2.6f, 0.42f, -0.12f, 60.0f},
            {StraightShooterEnemy, 300, 400, 2.6f, 0.0f, 0.0f, 48.0f},
            {SquareShooterEnemy, 600, 1200, 2.6f, -0.6f, 0.0f, 54.0f},
            {SquareShooterEnemy, 600, 1200, 2.6f, 0.6f, 0.0f, 54.0f}
        };
        // チャプター3
        static constexpr EnemySpawnRule Chapter3[] = {
            {BasicEnemy, 25, 120, 2.6f, 0.42f, -0.12f, 60.0f},
            {SquareShooterEnemy, 300, 1000, 2.6f, -0.9f, 0.5f, 54.0f},
            {SquareShooterEnemy, 300, 1000, 2.6f, 0.9f, -0.5f, 54.0f},
            {CircleShooterEnemy, 600, 1000, 2.6f, 0.0f, 0.0f, 36.0f},
            {SquareShooterEnemy, 900, 1000, 2.6f, 0.9f, 0.5f, 54.0f},
            {SquareShooterEnemy, 900, 1000, 2.6f, -0.9f, -0.5f, 54.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        const bool selected = TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
        // YとZの生成範囲をランダム化
        if (selected) {
            if (spawn.enemyType == BasicEnemy) {
                spawn.y = GameplayRandom::Range(-0.86f, 0.86f);
                spawn.railX = GameplayRandom::Range(-1.0f, 1.0f);
            }
        }

        return selected;
    }
};
