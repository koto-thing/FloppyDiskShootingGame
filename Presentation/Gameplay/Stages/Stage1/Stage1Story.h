#pragma once

#include "../Common/BossStory.h"

namespace ShooterStages::Stage1 {
inline constexpr BossStoryLine StoryLines[] = {
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

/**
 * @brief Stage 1のボス戦前会話を取得する
 * @return Stage 1の台詞とボス名を含む会話定義
 */
inline constexpr BossStory Story() {
    return BossStories::Create(StoryLines, "KOTO");
}
}
