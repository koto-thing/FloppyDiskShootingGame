#pragma once

#include "Stage2EnemySheet.h"

/** @brief HARD用のステージ2敵出現シートを定義する */
class SideScrollingShooter::Stage2EnemySheetHard final : public SideScrollingShooter::Stage2EnemySheet {
public:
    static constexpr int ChapterLength = 500;

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
     * @return 敵を出現させる場合true
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        // チャプター1は通常機と狙撃機を高頻度で出現させる
        static constexpr EnemySpawnRule Chapter1[] = {
            {BasicEnemy, 20, 48, 1.08f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, 100, 210, 1.10f, -0.18f, 0.86f, 48.0f}
        };
        // チャプター2は4種類の敵を高頻度で出現させる
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 10, 42, 1.12f, 0.42f, -0.12f, 60.0f},
            {CircleShooterEnemy, 50, 120, 1.14f, 0.35f, -0.32f, 54.0f},
            {HeavyEnemy, 110, 160, 1.16f, 0.70f, 0.58f, 60.0f},
            {SquareShooterEnemy, 150, 160, 1.14f, -0.62f, 0.32f, 54.0f}
        };
        // チャプター3は全種類を高頻度で出現させる
        static constexpr EnemySpawnRule Chapter3[] = {
            {StraightShooterEnemy, 10, 100, 1.10f, -0.78f, 0.86f, 48.0f},
            {CircleShooterEnemy, 40, 110, 1.14f, 0.78f, -0.86f, 36.0f},
            {HeavyEnemy, 90, 130, 1.16f, 0.22f, 0.30f, 60.0f},
            {ArmoredEnemy, 160, 240, 1.16f, -0.36f, -0.30f, 60.0f},
            {SquareShooterEnemy, 120, 120, 1.14f, 0.42f, 0.32f, 54.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }
};
