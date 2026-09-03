#pragma once

#include "Stage4EnemySheet.h"

/** @brief NORMAL用のStage4敵出現シートを定義する */
class SideScrollingShooter::Stage4EnemySheetNormal final : public SideScrollingShooter::Stage4EnemySheet {
public:
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
            {ArmoredEnemy, 20, 48, 1.10f, -0.82f, -0.68f, 60.0f},
            {CircleShooterEnemy, 90, 210, 1.14f, -0.28f, 0.86f, 50.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {StraightShooterEnemy, 10, 52, 1.10f, 0.28f, -0.88f, 40.0f},
            {LinkedLaserEnemy, 46, 260, 1.12f, -0.45f, 0.0f, 60.0f},
            {CircleShooterEnemy, 70, 165, 1.14f, 0.82f, 0.32f, 56.0f},
            {MissileShooterEnemy, 96, 220, 1.14f, -0.40f, -0.92f, 60.0f},
            {DiveRusherEnemy, 112, 188, 1.12f, -0.58f, 0.96f, 60.0f},
            {HeavyEnemy, 150, 230, 1.16f, 0.05f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {CircleShooterEnemy, 10, 110, 1.14f, -0.82f, 0.86f, 50.0f},
            {ArmoredEnemy, 50, 105, 1.12f, 0.82f, -0.18f, 60.0f},
            {StraightShooterEnemy, 120, 135, 1.10f, 0.28f, -0.88f, 40.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }
};
