#pragma once

#include "Stage3EnemySheet.h"

/** @brief HARD用のStage3敵出現シートを定義する */
class SideScrollingShooter::Stage3EnemySheetHard final : public SideScrollingShooter::Stage3EnemySheet {
public:
    static constexpr int ChapterLength = 500;

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
        static constexpr EnemySpawnRule Chapter1[] = {
            {StraightShooterEnemy, 18, 44, 1.10f, -0.82f, 0.88f, 50.0f},
            {DiveRusherEnemy, 56, 132, 1.12f, -0.28f, 0.96f, 60.0f},
            {ArmoredEnemy, 100, 210, 1.12f, -0.40f, -0.42f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {StraightShooterEnemy, 10, 42, 1.10f, 0.28f, -0.88f, 40.0f},
            {CircleShooterEnemy, 55, 132, 1.14f, 0.82f, 0.32f, 56.0f},
            {MissileShooterEnemy, 72, 160, 1.14f, -0.40f, -0.92f, 60.0f},
            {DiveRusherEnemy, 90, 140, 1.12f, -0.58f, 0.96f, 60.0f},
            {HeavyEnemy, 130, 170, 1.16f, 0.05f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {ArmoredEnemy, 10, 110, 1.12f, -0.85f, -0.18f, 60.0f},
            {CircleShooterEnemy, 50, 118, 1.14f, -0.28f, 0.86f, 50.0f},
            {MissileShooterEnemy, 62, 140, 1.14f, 0.20f, -0.92f, 60.0f},
            {DiveRusherEnemy, 70, 126, 1.12f, 0.48f, 0.96f, 60.0f},
            {HeavyEnemy, 110, 150, 1.16f, 0.82f, -0.68f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(
            Chapters, 3, ChapterFrameLength(), frame, spawnIndex, spawn, chapterNumber);
    }
};
