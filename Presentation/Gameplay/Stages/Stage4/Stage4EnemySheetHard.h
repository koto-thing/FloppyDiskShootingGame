#pragma once

#include "Stage4EnemySheet.h"

/** @brief HARD用のStage4敵出現シートを定義する */
class SideScrollingShooter::Stage4EnemySheetHard final : public SideScrollingShooter::Stage4EnemySheet {
public:
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
            {ArmoredEnemy, 20, 48, 1.10f, -0.82f, -0.68f, 60.0f},
            {CircleShooterEnemy, 90, 210, 1.14f, -0.28f, 0.86f, 50.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {ArmoredEnemy, 10, 46, 1.12f, 0.28f, 0.18f, 60.0f},
            {StraightShooterEnemy, 60, 125, 1.10f, 0.82f, -0.88f, 34.0f},
            {HeavyEnemy, 130, 190, 1.16f, -0.40f, 0.54f, 60.0f}
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
