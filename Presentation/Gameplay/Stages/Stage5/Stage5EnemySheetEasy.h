#pragma once

// 自動生成ファイル: 直接編集せず Stage5EnemySheetEasy.json を Tools/StageEditor.html で編集すること

#include "../../GameplayRandom.h"
#include "Stage5EnemySheet.h"

/**
 * @brief EASY用のStage5敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 位置乱数化, 開始フレーム, 出現間隔,
 * 2D出現X, レール出現X, 出現Y, レール出現Z } の順で指定する
 */
class SideScrollingShooter::Stage5EnemySheetEasy final : public SideScrollingShooter::Stage5EnemySheet {
public:
    static constexpr int ChapterLength = 1800;

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
            {LinkedLaserEnemy, false, 150, 600, 1.12f, -0.45f, 0.0f, 60.0f},
            {CircleShooterEnemy, false, 50, 900, 1.6f, 0.6f, 0.6f, 54.0f},
            {CircleShooterEnemy, false, 50, 900, 1.6f, -0.6f, -0.6f, 54.0f},
            {SquareShooterEnemy, false, 100, 300, 2.6f, 0.0f, 0.0f, 54.0f},
            {MissileShooterEnemy, false, 50, 300, 1.6f, -0.8f, -1.6f, 60.0f},
            {MissileShooterEnemy, false, 50, 300, 1.6f, 0.8f, -1.6f, 60.0f},
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {DiveRusherEnemy, false, 50, 150, 1.6f, 0.0f, 0.0f, 40.0f},
            {StraightShooterEnemy, false, 25, 300, 1.6f, 1.0f, 1.0f, 40.0f},
            {StraightShooterEnemy, false, 25, 300, 1.6f, -1.0f, -1.0f, 40.0f},
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {BasicEnemy, false, 25, 150, 2.6f, 0.42f, -0.8f, 60.0f},
            {BasicEnemy, false, 25, 150, 2.6f, 0.42f, 0.8f, 60.0f},
            {HeavyEnemy, false, 50, 150, 1.6f, 0.7f, 0.0f, 60.0f},
            {HeavyEnemy, false, 50, 150, 1.6f, -0.7f, 1.6f, 60.0f},
            {ArmoredEnemy, false, 50, 300, 1.6f, 0.0f, -1.6f, 60.0f},
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        const bool selected = TrySelectByChapters(Chapters, 3, ChapterFrameLength(),
            frame, spawnIndex, spawn, chapterNumber);
        return selected;
    }
};
