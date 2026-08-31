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

inline constexpr BossStoryLine Stage1[] = {
    {"MOMIJI", "You're the wanted Syateigashira, KOTO!", false},                                                // MOMIJI: お前は、指名手配中の舎弟頭KOTOだな！
{"SYATEIGASHIRA KOTO", "'Sup.", true},                                                                          // KOTO: ちーっす。
{"SYATEIGASHIRA KOTO", "So you're the infamous rogue cop, MOMIJI?", true},                                      // KOTO: うわさのはぐれモノ警察MOMIJIだね？
{"MOMIJI", "Casual, aren't you.", false},                                                                       // MOMIJI: 随分と軽い挨拶だね。
{"MOMIJI", "You're coming with me for the illegal cybernetics smuggling in the Neo-Aizu Colony.", false},       // MOMIJI: ネオ・アイヅコロニーでの違法サイバネティクス密輸の件、大人しく同行願おうか。
{"SYATEIGASHIRA KOTO", "Hah, that's hilarious.", true},                                                         // KOTO: はっ、マジウケるんだけど。
{"SYATEIGASHIRA KOTO", "Coming into my turf all by yourself?", true},                                           // KOTO: たった一人で俺のシマに乗り込んでくるとか、
{"SYATEIGASHIRA KOTO", "Is your brain's OS bugged out or something?", true},                                    // KOTO: アンタ頭のOSバグってんじゃねーの？
{"MOMIJI", "I don't need a whole squad to catch a two-bit thug on the run.", false},                            // MOMIJI: 逃げ回る小悪党を捕まえるのに、大部隊は必要ない。
{"SYATEIGASHIRA KOTO", "Huh? Who're you calling a two-bit thug?!", true},                                       // KOTO: あァん？ 誰が小悪党だコラ。
{"SYATEIGASHIRA KOTO", "Do you know who I am?", true},                                                          // KOTO: 俺を誰だと思ってんの？
{"SYATEIGASHIRA KOTO", "I'm the young ace of the PandD-kai, the great KOTO!", true},                            // KOTO: 泣く子も黙る『PANDD会』の若きエース、KOTO様だぜ？
{"MOMIJI", "Hiding behind your syndicate's name to act tough...", false},                                       // MOMIJI: 組織の看板を盾にしてイキる……
{"MOMIJI", "You just look like a cocky punk to me!", false},                                                    // MOMIJI: ただの調子に乗ったチンピラにしか見えないね！
{"SYATEIGASHIRA KOTO", "You wanna get flatlined?!", true},                                                      // KOTO: てめぇ、ブッ殺されてぇのか！
{"SYATEIGASHIRA KOTO", "A cop dog like you has no right preaching to us Space Yakuza!", true},                  // KOTO: サツの犬風情が、俺たちスペースヤクザに説教垂れてんじゃねーぞ！
{"SYATEIGASHIRA KOTO", "I'm gonna smash that smug face in with my custom ship, the 'YOROSHIKU'...", true},      // KOTO: この特注の『YOROSHIKU号』で、そのスカした顔面カチ割って……
{"SYATEIGASHIRA KOTO", "...and turn you into space debris!", true},                                             // KOTO: 宇宙のデブリにしてやんよ！
{"MOMIJI", "Even a cheap AI drone can bark.", false},                                                           // MOMIJI: 吠えるだけならAIロボットでもできる。
{"MOMIJI", "I'll shut that mouth of yours with laser cuffs.", false},                                           // MOMIJI: その減らず口、レーザー手錠で塞いでやる。
{"SYATEIGASHIRA KOTO", "Gyahaha!", true},                                                                       // KOTO: ギャハハ！
{"SYATEIGASHIRA KOTO", "Like an obsolete cop could ever catch up to my speed!", true},                          // KOTO: 時代遅れのポリ公が、俺のスピードに追いつけるかよ！
{"SYATEIGASHIRA KOTO", "Now die!!", true}                                                                       // KOTO: オラァ、死に晒せェ！！
};
inline constexpr BossStoryLine Stage2[] = {
    {"PILOT", "Your fleet ends here.", false},
    {"BOSS", "Then face its strongest shield.", true}
};
inline constexpr BossStoryLine Stage3[] = {
    {"PILOT", "I will break through your line.", false},
    {"BOSS", "No one has ever passed me.", true}
};
inline constexpr BossStoryLine Stage4[] = {
    {"PILOT", "I am not backing down now.", false},
    {"BOSS", "Then disappear with the rest.", true}
};
inline constexpr BossStoryLine Stage5[] = {
    {"PILOT", "This ends with you.", false},
    {"BOSS", "Come, and witness the end.", true}
};

/**
 * @brief 指定ステージのボス戦前会話を取得する
 * @param stageNumber ステージ番号
 * @return 指定ステージに対応する会話
 */
inline constexpr BossStory ForStage(int stageNumber) {
    switch (stageNumber) {
    case 2: return Create(Stage2, "BOSS");
    case 3: return Create(Stage3, "BOSS");
    case 4: return Create(Stage4, "BOSS");
    case 5: return Create(Stage5, "BOSS");
    default: return Create(Stage1, "KOTO");
    }
}
}
