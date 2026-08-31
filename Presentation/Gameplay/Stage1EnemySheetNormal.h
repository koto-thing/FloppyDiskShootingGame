#pragma once

#include "Stage1EnemySheet.h"

/** @brief NORMAL用のステージ1敵出現シートを定義する */
class SideScrollingShooter::Stage1EnemySheetNormal final : public SideScrollingShooter::Stage1EnemySheet {
public:
    /**
     * @brief 指定チャプターの終了フレームを取得する
     * @param chapterNumber チャプター番号
     * @return NORMAL用チャプターの終了フレーム
     */
    int ChapterEndFrame(int chapterNumber) const override {
        return chapterNumber * 1000;
    }

    /**
     * @brief 指定フレームで出現させるNORMAL用の敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true
     */
    bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const override {
        // チャプター1は通常機と直進狙撃機を出現させる
        static constexpr EnemySpawnRule Chapter1[] = {
            {BasicEnemy, 35, 70, 2.6f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, 160, 360, 2.6f, -0.18f, 0.86f, 48.0f}
        };
        // チャプター2は通常機、円形弾幕砲台、重装機を出現させる
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 525, 65, 2.6f, 0.42f, -0.12f, 60.0f},
            {CircleShooterEnemy, 580, 210, 2.6f, 0.35f, -0.32f, 54.0f},
            {HeavyEnemy, 650, 280, 2.6f, 0.70f, 0.58f, 60.0f}
        };
        // チャプター3は直進狙撃機、円形弾幕砲台、重装機を出現させる
        static constexpr EnemySpawnRule Chapter3[] = {
            {StraightShooterEnemy, 1020, 160, 2.6f, -0.78f, 0.86f, 48.0f},
            {CircleShooterEnemy, 1080, 200, 2.6f, 0.78f, -0.86f, 36.0f},
            {HeavyEnemy, 1160, 220, 2.6f, 0.22f, 0.30f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            {0, 1000, Chapter1, 2}, {1000, 2000, Chapter2, 3}, {2000, 3000, Chapter3, 3}
        };
        return TrySelectByChapters(Chapters, 3, frame, spawn, chapterNumber);
    }
};
