#pragma once

#include "Stage5EnemySheet.h"

/** @brief HARD用のStage5敵出現シートを定義する */
class SideScrollingShooter::Stage5EnemySheetHard final : public SideScrollingShooter::Stage5EnemySheet {
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
            {CircleShooterEnemy, 14, 34, 1.14f, -0.82f, 0.86f, 50.0f},
            {ArmoredEnemy, 60, 135, 1.12f, -0.28f, -0.42f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {CircleShooterEnemy, 10, 72, 1.14f, 0.28f, -0.32f, 56.0f},
            {StraightShooterEnemy, 42, 82, 1.10f, 0.82f, 0.88f, 34.0f},
            {HeavyEnemy, 95, 112, 1.16f, -0.85f, 0.54f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {ArmoredEnemy, 10, 62, 1.12f, -0.82f, -0.68f, 60.0f},
            {CircleShooterEnemy, 32, 60, 1.14f, 0.82f, 0.32f, 56.0f},
            {StraightShooterEnemy, 70, 72, 1.10f, -0.28f, -0.88f, 40.0f},
            {HeavyEnemy, 125, 92, 1.16f, 0.28f, 0.68f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, ChapterFrameLength(), frame, spawnIndex, spawn, chapterNumber);
    }
};
