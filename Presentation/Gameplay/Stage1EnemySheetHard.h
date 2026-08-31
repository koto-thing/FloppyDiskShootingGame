#pragma once

#include "Stage1EnemySheet.h"

/** @brief HARD用のステージ1敵出現シートを定義する */
class SideScrollingShooter::Stage1EnemySheetHard final : public SideScrollingShooter::Stage1EnemySheet {
public:
    /**
     * @brief 指定フレームで出現させるHARD用の敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true
     */
    bool TrySelectEnemySpawn(int frame, EnemySpawnRule& spawn, int& chapterNumber) const override {
        // チャプター1は通常機と狙撃機を高頻度で出現させる
        static constexpr EnemySpawnRule Chapter1[] = {
            {BasicEnemy, 20, 48, 1.08f, -0.72f, -0.54f, 60.0f},
            {StraightShooterEnemy, 100, 210, 1.10f, -0.18f, 0.86f, 48.0f}
        };
        // チャプター2は3種類の敵を高頻度で出現させる
        static constexpr EnemySpawnRule Chapter2[] = {
            {BasicEnemy, 510, 42, 1.12f, 0.42f, -0.12f, 60.0f},
            {CircleShooterEnemy, 550, 120, 1.14f, 0.35f, -0.32f, 54.0f},
            {HeavyEnemy, 610, 160, 1.16f, 0.70f, 0.58f, 60.0f}
        };
        // チャプター3は全種類を高頻度で出現させる
        static constexpr EnemySpawnRule Chapter3[] = {
            {StraightShooterEnemy, 1010, 100, 1.10f, -0.78f, 0.86f, 48.0f},
            {CircleShooterEnemy, 1040, 110, 1.14f, 0.78f, -0.86f, 36.0f},
            {HeavyEnemy, 1090, 130, 1.16f, 0.22f, 0.30f, 60.0f},
            {ArmoredEnemy, 1160, 240, 1.16f, -0.36f, -0.30f, 60.0f}
        };
        constexpr Chapter Chapters[] = {
            {0, 500, Chapter1, 2}, {500, 1000, Chapter2, 3}, {1000, 1500, Chapter3, 4}
        };
        return TrySelectByChapters(Chapters, 3, frame, spawn, chapterNumber);
    }
};
