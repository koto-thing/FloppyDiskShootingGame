#pragma once

#include "Stage2EnemySheet.h"

/**
 * @brief EASY用のステージ2敵出現シートを定義する
 *
 * EnemySpawnRuleは { 敵構成種別, 開始フレーム, 出現間隔, 2D出現X,
 * レール出現X, 出現Y, レール出現Z } の順で指定する
 */
class SideScrollingShooter::Stage2EnemySheetEasy final : public SideScrollingShooter::Stage2EnemySheet {
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
     * @return 敵を出現させる場合true
     */
    bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const override {
        // チャプター1は通常機だけを低頻度で出現させる
        static constexpr EnemySpawnRule Chapter1[] = {
            {BasicEnemy, 70, 110, 1.08f, -0.72f, -0.54f, 60.0f}
        };
        // チャプター2は通常機と円形弾幕砲台を出現させる
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 40, 100, 1.12f, 0.42f, -0.12f, 60.0f},
            {CircleShooterEnemy, 150, 320, 1.14f, 0.35f, -0.32f, 54.0f}
        };
        // チャプター3は狙撃機、重装機、正方形弾幕砲台を低頻度で出現させる
        static constexpr EnemySpawnRule Chapter3[] = {
            {StraightShooterEnemy, 60, 280, 1.10f, -0.78f, 0.86f, 48.0f},
            {HeavyEnemy, 200, 360, 1.16f, 0.22f, 0.30f, 60.0f},
            {SquareShooterEnemy, 260, 360, 1.14f, 0.62f, -0.32f, 54.0f}
        };
        constexpr Chapter Chapters[] = {
            MakeChapter(Chapter1), MakeChapter(Chapter2), MakeChapter(Chapter3)
        };
        return TrySelectByChapters(Chapters, 3, frame, spawnIndex, spawn, chapterNumber);
    }
};
