#pragma once

#include "../Common/BossStory.h"

namespace ShooterStages::Stage4 {
inline constexpr BossStoryLine StoryLines[] = {
    {"MOMIJI", "So you're the head of the main syndicate office...", false},                                          // MOMIJI: あんたが本部の仕切り役……
    {"MOMIJI", "Honbucho BOTAMOCHI.", false},                                                                         // MOMIJI: 本部長のBOTAMOCHIだな。
    {"HONBUCHO BOTAMOCHI", "...(Evil laughter) Bota-mo~chi-mochi-mochi.", true},                                      // BOTAMOCHI: （邪悪な笑い声）……ぼたも～ちもちもち。
    {"HONBUCHO BOTAMOCHI", "You've got some nerve stepping into my fortress, rogue cop.", true},                      // BOTAMOCHI: よくぞここまで嗅ぎつけてきたな、はぐれマッポが。
    {"HONBUCHO BOTAMOCHI", "I hear KOTO and RYOTA both fell to your hands.", true},                                   // BOTAMOCHI: KOTOを叩き潰した腕前、見事なもんだ。
    {"MOMIJI", "They put up a fight, but they're in custody now.", false},                                            // MOMIJI: 派手に暴れてくれたけどね。今はブタ箱ん中だ。
    {"MOMIJI", "Now it's time to take down the brain behind the whole syndicate.", false},                            // MOMIJI: 次は組織の頭脳である、お前の番だよ。
    {"HONBUCHO BOTAMOCHI", "(Evil laughter) Bota-mochi-mochi...", true},                                              // BOTAMOCHI: （邪悪な笑い声）ぼたもちもち……。
    {"HONBUCHO BOTAMOCHI", "Those two were good men. True pillars of the PandD-kai.", true},                          // BOTAMOCHI: あいつらは優秀な舎弟だった。我がPANDD会を支える立派な柱よ。
    {"HONBUCHO BOTAMOCHI", "And you crushed their pride.", true},                                                     // BOTAMOCHI: その男たちの誇りを、テメェは土足で踏みにじった。
    {"MOMIJI", "Pride built on smuggling and violence is just crime.", false},                                        // MOMIJI: 密輸と暴力の上に成り立つ誇りなんて、ただの犯罪だ。
    {"MOMIJI", "Don't try to dress it up as chivalry.", false},                                                       // MOMIJI: 仁義面して誤魔化せると思うなよ。
    {"HONBUCHO BOTAMOCHI", "Heh... Law and order, is it?", true},                                                     // BOTAMOCHI: フッ……法の正義、か。
    {"HONBUCHO BOTAMOCHI", "In this cold, lawless outer rim, power is the only rule that matters.", true},            // BOTAMOCHI: この冷たい銀河の果てでモノを言うのは、力と覚悟だけだ。
    {"HONBUCHO BOTAMOCHI", "And I bear the weight of every brother who bled for this syndicate.", true},              // BOTAMOCHI: 俺は血を流してきたすべての組員の業を背負ってここに立っている。
    {"MOMIJI", "Then carry that weight straight into a prison cell.", false},                                         // MOMIJI: なら、その重みごと牢屋へ持っていくんだね。
    {"HONBUCHO BOTAMOCHI", "(Evil laughter) Bota-mo~chi-mochi-mochi!!", true},                                        // BOTAMOCHI: （邪悪な笑い声）ぼたも～ちもちもち！！
    {"HONBUCHO BOTAMOCHI", "You think you can break my iron wall with that rusty vessel?", true},                     // BOTAMOCHI: そのオンボロ機体で、この鉄壁を穿てると思ってンのか！
    {"HONBUCHO BOTAMOCHI", "Main guns, full power! Flagship 'TANABOTA', eliminate the target!!", true},               // BOTAMOCHI: 主砲全門展開！ 旗艦『PUROPAWA号』、目標を完全粉砕しろ！！
    {"MOMIJI", "Heavy armor just makes for a bigger target.", false},                                                 // MOMIJI: 分厚い装甲は、デカい標的になるだけさ。
    {"HONBUCHO BOTAMOCHI", "(Evil laughter) BOTA-MO~CHI-MOCHI-MOCHI!!", true},                                        // BOTAMOCHI: （邪悪な笑い声）ぼたも～ちもちもちィ！！
    {"HONBUCHO BOTAMOCHI", "Let's see if your resolve can pierce my absolute defense!!", true},                       // BOTAMOCHI: テメェの覚悟、この絶対防衛砂上戦艦にブチ当ててみせろやァッ！！
};

/**
 * @brief Stage 4のボス戦前会話を取得する
 * @return Stage 4の台詞とボス名を含む会話定義
 */
inline constexpr BossStory Story() {
    return BossStories::Create(StoryLines, "BOTAMOCHI");
}
}
