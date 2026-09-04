#pragma once

#include "../Common/BossStory.h"

namespace ShooterStages::Stage2 {
inline constexpr BossStoryLine StoryLines[] = {
    {"MOMIJI", "Found you. LUMI, boss of the PandD-kai.", false},                            // MOMIJI: 見つけたよ。『PANDD会』組長、LUMI。
    {"BOSS LUMI", "You came all the way to the edge of space? Hilarious.", true},           // LUMI: わざわざ宇宙の果てまで来たの？ウケる
    {"MOMIJI", "You're coming quietly.", false},                                            // MOMIJI: 大人しくお縄についてもらうよ。
    {"BOSS LUMI", "That's got nothing to do with me. TAYAMA does whatever he wants.", true}, // LUMI: それ、あーし関係ないじゃん。TAYAMAっちが勝手にやってるだけ。
    {"BOSS LUMI", "Besides, I'm not even into fighting.", true},                            // LUMI: そもそも、あーし戦闘とか興味ないし。
    {"MOMIJI", "...Then why become a yakuza?", false},                                     // MOMIJI: ……なら、どうして極道なんかやっている。
    {"BOSS LUMI", "Hmm... money?", true},                                                   // LUMI: んー……お金？
    {"MOMIJI", "You can't be serious...", false},                                           // MOMIJI: まさか……
    {"BOSS LUMI", "Makeup, accessories... oh, and bags.", true},                            // LUMI: コスメとか～、アクセとか～、あとバッグとか～。
    {"MOMIJI", "You became a yakuza for that...?", false},                                  // MOMIJI: あんた、そんな理由で……
    {"BOSS LUMI", "Being a girl is expensive, y'know?", true},                              // LUMI: だってぇ、オンナノコってお金かかるじゃん？
    {"MOMIJI", "What you people stole can't be replaced with makeup and bags!", false},     // MOMIJI: あんたたちが奪ったものは、コスメやバッグなんかじゃ埋め合わせられないのよ！
    {"BOSS LUMI", "Whoa, did you just switch into lecture mode? Lame.", true},              // LUMI: なんか説教モード入っちゃった？ダル。
    {"MOMIJI", "I'm only saying what any cop would.", false},                               // MOMIJI: 警察として当然のことを言ってるだけだ。
    {"MOMIJI", "Surrender. Now.", false},                                                    // MOMIJI: 今すぐに投降しろ。
    {"BOSS LUMI", "Ugh... I seriously can't do combat.", true},                             // LUMI: えー……戦闘とかムリ～。
    {"BOSS LUMI", "I just got my nails done. They'll get all chipped.", true},              // LUMI: せっかくネイル新しくしたのに、禿げちゃうじゃん。
    {"MOMIJI", "...Are you messing with me?", false},                                       // MOMIJI: ……ふざけてるのか？
    {"BOSS LUMI", "I'm dead serious. I don't wanna ruin my makeup either.", true},          // LUMI: ふざけられないって。メイクも崩したくないもん。
    {"MOMIJI", "Pathetic. Save the rest of your chatter for the courtroom.", false},        // MOMIJI: くだらない。これ以上のお喋りは法廷で聞かせてもらうよ。
    {"BOSS LUMI", "Ugh... this is literally the worst. And I forgot to update my socials.", true}, // LUMI: はぁ～……ほんっと最悪。てか、SNSの更新忘れてたし。
    {"MOMIJI", "Even now...? Get ready, LUMI.", false},                                     // MOMIJI: こんな時まで……覚悟しな、LUMI。
    {"BOSS LUMI", "Okay, but, like... go easy on me?", true}                                // LUMI: ま、ほどほどにお手柔らかに〜
};

/**
 * @brief Stage 2のボス戦前会話を取得する
 * @return Stage 2の台詞とボス名を含む会話定義
 */
inline constexpr BossStory Story() {
    return BossStories::Create(StoryLines, "LUMI");
}
}
