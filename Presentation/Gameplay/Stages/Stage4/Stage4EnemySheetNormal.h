#pragma once

// 自動生成ファイル: 直接編集せず Stage4EnemySheetNormal.json を Tools/StageEditor.html で編集すること

#include "../../GameplayRandom.h"
#include "Stage4EnemySheet.h"

/**
 * @brief NORMAL用のStage4敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 位置乱数化, スクリプト番号, 開始フレーム,
 * 出現間隔, 2D出現X, レール出現X, 出現Y, レール出現Z } の順で指定する
 */
class SideScrollingShooter::Stage4EnemySheetNormal final : public SideScrollingShooter::Stage4EnemySheet {
public:
    static constexpr int ChapterLength = 1500;

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
            {LinkedLaserEnemy, false, 0, 10, 400, 1.12f, -0.45f, 0.0f, 60.0f},
            {BasicEnemy, true, 0, 200, 50, 2.6f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, false, 0, 200, 150, 2.6f, 0.0f, 1.2f, 40.0f},
            {MissileShooterEnemy, false, 0, 200, 300, 1.14f, -0.4f, -0.92f, 60.0f},
            {MissileShooterEnemy, false, 0, 350, 300, 1.14f, 0.4f, -0.92f, 60.0f},
        };
        static constexpr EnemySpawnRule Chapter2[] = {
            {LinkedLaserEnemy, false, 0, 10, 400, 1.12f, -0.45f, 0.0f, 60.0f},
            {StraightShooterEnemy, false, 0, 50, 150, 2.6f, 0.0f, 1.1f, 40.0f},
            {StraightShooterEnemy, false, 0, 50, 150, 2.6f, 0.0f, -1.0f, 40.0f},
            {SquareShooterEnemy, false, 0, 300, 300, 2.6f, 0.0f, 0.0f, 54.0f},
        };
        static constexpr EnemySpawnRule Chapter3[] = {
            {LinkedLaserEnemy, false, 0, 10, 400, 1.12f, -0.45f, 0.0f, 60.0f},
            {BasicEnemy, true, 0, 50, 100, 2.6f, -0.72f, -0.54f, 60.0f},
            {DiveRusherEnemy, false, 0, 50, 75, 1.6f, 0.0f, 1.1f, 40.0f},
            {StraightShooterEnemy, false, 0, 50, 75, 2.6f, 0.0f, -0.8f, 40.0f},
            {MissileShooterEnemy, false, 0, 300, 600, 1.14f, -0.8f, -0.92f, 60.0f},
            {MissileShooterEnemy, false, 0, 300, 600, 1.14f, 0.8f, -0.92f, 60.0f},
            {CircleShooterEnemy, false, 0, 400, 300, 1.6f, 0.4f, 0.6f, 54.0f},
            {CircleShooterEnemy, false, 0, 400, 300, 1.6f, 0.0f, 1.2f, 54.0f},
            {CircleShooterEnemy, false, 0, 400, 300, 1.6f, -0.4f, 0.6f, 54.0f},
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
