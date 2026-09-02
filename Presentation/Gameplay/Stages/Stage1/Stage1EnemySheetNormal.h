#pragma once

#include "../../GameplayRandom.h"
#include "Stage1EnemySheet.h"

/** @brief NORMAL用のステージ1敵出現シートを定義する */
class SideScrollingShooter::Stage1EnemySheetNormal final : public SideScrollingShooter::Stage1EnemySheet {
public:
    static constexpr int ChapterLength = 1000;

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
        // チャプター1は通常機と直進狙撃機を出現させる
        static constexpr EnemySpawnRule Chapter1[] = {
            {BasicEnemy, 35, 70, 2.6f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, 160, 360, 2.6f, -0.18f, 0.86f, 48.0f}
        };
        // チャプター2は通常機、円形弾幕砲台、重装機を出現させる
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 25, 65, 2.6f, 0.42f, -0.12f, 60.0f},
            {CircleShooterEnemy, 80, 210, 2.6f, 0.35f, -0.32f, 54.0f},
            {HeavyEnemy, 50, 280, 2.6f, 0.70f, 0.58f, 60.0f}
        };
        // チャプター3は通常機、直進狙撃機、円形弾幕砲台、重装機を出現させる
        static constexpr EnemySpawnRule Chapter3[] = {
            {BasicEnemy, 25, 65, 2.6f, 0.42f, -0.12f, 60.0f},
            {StraightShooterEnemy, 20, 160, 2.6f, -0.78f, 0.86f, 48.0f},
            {CircleShooterEnemy, 100, 100, 2.6f, 0.78f, -0.86f, 36.0f},
            {HeavyEnemy, 50, 200, 2.6f, 0.22f, 0.30f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        const bool selected = TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
        // YとZの生成範囲をランダム化
        if (selected) {
            spawn.y = GameplayRandom::Range(-0.86f, 0.86f);
            spawn.railX = GameplayRandom::Range(-1.0f, 1.0f);
        }

        return selected;
    }
};
