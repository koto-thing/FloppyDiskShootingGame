#pragma once

#include "../Common/StageDefinition.h"

/**
 * @brief Stage5の敵出現シート共通処理を定義する
 */
class SideScrollingShooter::Stage5EnemySheet {
public:
    using EnemySpawnRule = SideScrollingShooter::Stage::EnemySpawnRule;
    static constexpr int HeavyEnemy = SideScrollingShooter::Stage::HeavyEnemy;
    static constexpr int StraightShooterEnemy = SideScrollingShooter::Stage::StraightShooterEnemy;
    static constexpr int ArmoredEnemy = SideScrollingShooter::Stage::ArmoredEnemy;
    static constexpr int CircleShooterEnemy = SideScrollingShooter::Stage::CircleShooterEnemy;

    struct Chapter {
        const EnemySpawnRule* spawnRules = nullptr;
        int spawnRuleCount = 0;
    };

    virtual ~Stage5EnemySheet() = default;

    /**
     * @brief 1チャプターの長さを取得する
     * @return チャプターのフレーム数
     */
    virtual int ChapterFrameLength() const {
        return SideScrollingShooter::ChapterLengthFrames;
    }

    /**
     * @brief 指定フレームで出現させる敵を取得する
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 敵を出現させる場合true、出現させない場合false
     */
    virtual bool TrySelectEnemySpawn(int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) const = 0;

protected:
    /**
     * @brief チャプター配列参照を生成する
     * @param rules チャプター内の出現規則配列
     * @return 出現規則配列と要素数
     */
    template<int RuleCount>
    static constexpr Chapter MakeChapter(const EnemySpawnRule (&rules)[RuleCount]) {
        return {rules, RuleCount};
    }

    /**
     * @brief チャプター単位で現在フレームの出現規則を選択する
     * @param chapters チャプター配列
     * @param chapterCount チャプター数
     * @param chapterFrameLength 1チャプターの長さ
     * @param frame 現在のステージフレーム
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param spawn 出現設定の格納先
     * @param chapterNumber 現在チャプター番号の格納先
     * @return 出現規則を選択した場合true
     */
    static bool TrySelectByChapters(const Chapter* chapters, int chapterCount,
        int chapterFrameLength, int frame, int spawnIndex,
        EnemySpawnRule& spawn, int& chapterNumber) {
        for (int chapterIndex = 0; chapterIndex < chapterCount; ++chapterIndex) {
            const int chapterFirstFrame = chapterIndex * chapterFrameLength;
            if (frame < chapterFirstFrame || frame >= chapterFirstFrame + chapterFrameLength) continue;
            chapterNumber = chapterIndex + 1;
            const Chapter& chapter = chapters[chapterIndex];
            return TrySelectByRules(chapter.spawnRules, chapter.spawnRuleCount,
                spawnIndex, frame - chapterFirstFrame, spawn);
        }
        return false;
    }

    /**
     * @brief 出現規則配列から現在フレームの規則を選択する
     * @param rules 出現規則配列
     * @param ruleCount 出現規則数
     * @param spawnIndex 同一フレーム内で取得する出現候補の番号
     * @param frame チャプター内フレーム
     * @param spawn 出現設定の格納先
     * @return 出現規則を選択した場合true
     */
    static bool TrySelectByRules(const EnemySpawnRule* rules, int ruleCount,
        int spawnIndex, int frame, EnemySpawnRule& spawn) {
        int matchedIndex = 0;
        for (int i = 0; i < ruleCount; ++i) {
            const EnemySpawnRule& rule = rules[i];
            if (frame < rule.firstFrame) continue;
            if (frame != rule.firstFrame &&
                (rule.interval <= 0 || (frame - rule.firstFrame) % rule.interval != 0)) {
                continue;
            }
            if (matchedIndex++ != spawnIndex) continue;
            spawn = rule;
            return true;
        }
        return false;
    }
};
