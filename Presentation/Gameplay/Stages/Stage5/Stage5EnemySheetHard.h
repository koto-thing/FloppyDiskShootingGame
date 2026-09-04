#pragma once

#include "Stage5EnemySheet.h"

/** @brief HARD用のStage5敵出現シートを定義する */
class SideScrollingShooter::Stage5EnemySheetHard final : public SideScrollingShooter::Stage5EnemySheet {
public:
    static constexpr int ChapterLength = 1800;

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
            {LinkedLaserEnemy, 150, 400, 1.12f, -0.45f, 0.0f, 60.0f },
            {CircleShooterEnemy, 50, 600, 1.6f, 0.6f, 0.6f, 54.0f},
            {CircleShooterEnemy, 50, 600, 1.6f, -0.6f, -0.6f, 54.0f},
            {SquareShooterEnemy, 100, 200, 2.6f, 0.00f, 0.00f, 54.0f},
            {MissileShooterEnemy,  50, 200, 1.6f, -0.80f, -1.60f, 60.0f},
            {MissileShooterEnemy,  50, 200, 1.6f, 0.80f, -1.60f, 60.0f},
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {DiveRusherEnemy, 50, 50, 1.6f, 0.00f, 0.0f, 40.0f},
            {StraightShooterEnemy, 25, 100, 1.6f, 1.00f, 1.00f, 40.0f},
            {StraightShooterEnemy, 25, 100, 1.6f, -1.00f, -1.00f, 40.0f},
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {BasicEnemy, 25, 50, 2.6f, 0.42f, -0.8f, 60.0f},
            {BasicEnemy, 25, 50, 2.6f, 0.42f, 0.80f, 60.0f},
            {HeavyEnemy, 50, 50, 1.6f, 0.70f, 0.0f, 60.0f},
            {HeavyEnemy, 50, 50, 1.6f, -0.70f, 1.60f, 60.0f},
            {ArmoredEnemy, 50, 100, 1.6f, 0.0f, -1.60f, 60.0f},
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };

        const bool selected = TrySelectByChapters(Chapters, 3, ChapterFrameLength(), frame, spawnIndex, spawn, chapterNumber);
        return selected;
    }
};
