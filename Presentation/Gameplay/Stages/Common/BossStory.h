#pragma once

#include <cstddef>

/** @brief ボス戦開始前に表示する一行分の会話 */
struct BossStoryLine {
    const char* speaker;
    const char* text;
    bool isBoss;
};

/** @brief ステージごとのボス戦前会話 */
struct BossStory {
    const BossStoryLine* lines;
    int lineCount;
    const char* bossName;
};

namespace BossStories {
/**
 * @brief 台詞配列から会話定義を生成する
 * @param lines 会話に使用する台詞配列
 * @param bossName HUDに表示するボス名
 * @return 台詞数とボス名を含む会話定義
 */
template <std::size_t LineCount>
constexpr BossStory Create(const BossStoryLine (&lines)[LineCount], const char* bossName) {
    return {lines, static_cast<int>(LineCount), bossName};
}
}
