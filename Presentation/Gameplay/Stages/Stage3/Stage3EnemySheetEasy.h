#pragma once

#include "../../GameplayRandom.h"
#include "Stage3EnemySheet.h"

/**
 * @brief EASY用のStage3敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 開始フレーム, 出現間隔, 2D出現X,
 * レール出現X, 出現Y, レール出現Z } の順で指定する
 */
class SideScrollingShooter::Stage3EnemySheetEasy final : public SideScrollingShooter::Stage3EnemySheet {
public:
    static constexpr int ChapterLength = 1200;

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
            {BasicEnemy, 25, 100, 2.6f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, 50, 200, 1.10f, 0.0f, -0.88f, 50.0f},
            {DiveRusherEnemy, 100, 200, 1.12f, -0.28f, 1.5f, 60.0f},
            {HeavyEnemy, 100, 300, 2.6f, 0.00f, 0.0f, 60.0f}
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 25, 100, 2.6f, -0.72f, -0.54f, 60.0f},
            {DiveRusherEnemy, 100, 150, 1.12f, -0.28f, 1.5f, 60.0f},
            {ArmoredEnemy, 250, 200, 2.6f, 0.00f, -0.70f, 60.0f},
            {MissileShooterEnemy, 25, 350, 1.14f, 0.00f, -0.92f, 60.0f},
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {BasicEnemy, 25, 100, 2.6f, -0.72f, -0.54f, 60.0f},
            {DiveRusherEnemy, 100, 150, 1.12f, -0.28f, 1.5f, 60.0f},
            {ArmoredEnemy, 250, 200, 2.6f, 0.00f, -0.70f, 60.0f},
            {SquareShooterEnemy, 700, 1200, 2.6f, 0.0f, 0.0f, 54.0f},
            {CircleShooterEnemy, 600, 300, 2.6f, 0.0f, 0.6f, 54.0f},
            {MissileShooterEnemy, 25, 300, 1.14f, 0.40f, -0.92f, 60.0f},
            {MissileShooterEnemy, 25, 300, 1.14f, -0.40f, -0.92f, 60.0f},
            {StraightShooterEnemy, 100, 300, 1.10f, 0.0f, -0.88f, 50.0f},
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        const bool selected = TrySelectByChapters(Chapters, 3, ChapterFrameLength(), frame, spawnIndex, spawn, chapterNumber);
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
