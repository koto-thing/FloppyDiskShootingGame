#pragma once

// 自動生成ファイル: 直接編集せず Stage2EnemySheetHard.json を Tools/StageEditor.html で編集すること

#include "../../GameplayRandom.h"
#include "Stage2EnemySheet.h"

/**
 * @brief HARD用のStage2敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 位置乱数化, 開始フレーム, 出現間隔,
 * 2D出現X, レール出現X, 出現Y, レール出現Z } の順で指定する
 */
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
            {BasicEnemy, true, 35, 150, 2.6f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, false, 200, 400, 2.6f, -0.18f, 0.86f, 48.0f},
            {StraightShooterEnemy, false, 400, 400, 2.6f, -0.18f, -0.86f, 48.0f},
            {SquareShooterEnemy, false, 500, 1200, 2.6f, 0.0f, 0.0f, 54.0f},
        };
        // チャプター2
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, true, 25, 150, 2.6f, 0.42f, -0.12f, 60.0f},
            {StraightShooterEnemy, false, 300, 400, 2.6f, 0.0f, 0.0f, 48.0f},
            {SquareShooterEnemy, false, 600, 1200, 2.6f, -0.6f, 0.0f, 54.0f},
            {SquareShooterEnemy, false, 600, 1200, 2.6f, 0.6f, 0.0f, 54.0f},
        };
        // チャプター3
        static constexpr EnemySpawnRule Chapter3[] = {
            {BasicEnemy, true, 25, 120, 2.6f, 0.42f, -0.12f, 60.0f},
            {SquareShooterEnemy, false, 300, 1000, 2.6f, -0.9f, 0.5f, 54.0f},
            {SquareShooterEnemy, false, 300, 1000, 2.6f, 0.9f, -0.5f, 54.0f},
            {CircleShooterEnemy, false, 600, 1000, 2.6f, 0.0f, 0.0f, 36.0f},
            {SquareShooterEnemy, false, 900, 1000, 2.6f, 0.9f, 0.5f, 54.0f},
            {SquareShooterEnemy, false, 900, 1000, 2.6f, -0.9f, -0.5f, 54.0f},
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        const bool selected = TrySelectByChapters(Chapters, 3, ChapterFrameLength(),
            frame, spawnIndex, spawn, chapterNumber);

        // 位置乱数化を指定した敵はYとレールXをランダムに決める
        if (selected && spawn.randomPosition) {
            spawn.y = GameplayRandom::Range(-0.86f, 0.86f);
            spawn.railX = GameplayRandom::Range(-1.0f, 1.0f);
        }
        return selected;
    }
};
