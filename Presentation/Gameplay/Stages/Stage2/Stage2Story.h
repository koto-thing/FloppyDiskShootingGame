#pragma once

#include "../Common/BossStory.h"

namespace ShooterStages::Stage2 {
inline constexpr BossStoryLine StoryLines[] = {
    {"PILOT", "I am not backing down now.", false},
    {"BOSS", "Then disappear with the rest.", true}
};

/**
 * @brief Stage 2のボス戦前会話を取得する
 * @return Stage 2の台詞とボス名を含む会話定義
 */
inline constexpr BossStory Story() {
    return BossStories::Create(StoryLines, "RYOTA");
}
}
