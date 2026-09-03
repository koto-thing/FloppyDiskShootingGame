#pragma once

#include "Stage3EnemySheet.h"

/**
 * @brief EASY用のStage3敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 開始フレーム, 出現間隔, 2D出現X,
 * レール出現X, 出現Y, レール出現Z } の順で指定する
 */
class SideScrollingShooter::Stage3EnemySheetEasy final : public SideScrollingShooter::Stage3EnemySheet {
public:
    static constexpr int ChapterLength = 500;

    /**
     * @brief 1チャプターの長さを取得する
     * @return EASY用チャプターのフレーム数
     */
    int ChapterFrameLength() const override {
        return ChapterLength;
    }

    /**
     * @brief 指定フレームで出現させるEASY用の敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true、出現させない場合false
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        static constexpr EnemySpawnRule Chapter1[] = {
            {StraightShooterEnemy, 40, 84, 1.10f, -0.82f, 0.88f, 50.0f},
            {DiveRusherEnemy, 160, 260, 1.12f, -0.28f, 0.96f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {StraightShooterEnemy, 30, 82, 1.10f, 0.28f, -0.88f, 40.0f},
            {CircleShooterEnemy, 140, 300, 1.14f, 0.82f, 0.32f, 56.0f},
            {DiveRusherEnemy, 220, 300, 1.12f, -0.58f, 0.96f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {ArmoredEnemy, 40, 240, 1.12f, -0.85f, -0.18f, 60.0f},
            {CircleShooterEnemy, 120, 280, 1.14f, -0.28f, 0.86f, 50.0f},
            {DiveRusherEnemy, 190, 260, 1.12f, 0.48f, 0.96f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(
            Chapters, 3, ChapterFrameLength(), frame, spawnIndex, spawn, chapterNumber);
    }
};
