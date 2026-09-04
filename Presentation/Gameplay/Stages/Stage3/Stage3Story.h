#pragma once

#include "../Common/BossStory.h"

namespace ShooterStages::Stage3 {
inline constexpr BossStoryLine StoryLines[] = {
    {"MOMIJI", "Found you. RYOTA, Wakagashira of the PandD-kai.", false},                                          // MOMIJI: 見つけたよ。『PANDD会』若頭、RYOTA。
    {"WAKAGASHIRA RYOTA", "...Huh?", true},                                                                        // RYOTA: ……あァ？
    {"WAKAGASHIRA RYOTA", "So you're the cop who's been sniffing around my boys lately.", true},                   // RYOTA: 誰かと思えば、最近ウチの若い衆を嗅ぎ回ってるサツか。
    {"MOMIJI", "Sniffing around, huh.", false},                                                                    // MOMIJI: 嗅ぎ回ってる、ね。
    {"MOMIJI", "KOTO and the rest of your crew made enough noise that the investigation was pretty easy.", false}, // MOMIJI: KOTOを筆頭に、そっちが派手に暴れ回ってくれたおかげで捜査は楽だったよ。
    {"WAKAGASHIRA RYOTA", "...That damn KOTO.", true},                                                             // RYOTA: ……KOTOの野郎。
    {"WAKAGASHIRA RYOTA", "I told that idiot not to pull any unnecessary crap.", true},                            // RYOTA: あれほど余計な真似すんなっつったのによ。
    {"MOMIJI", "Sounds like raising your subordinates is tough work, Wakagashira.", false},                        // MOMIJI: 部下の教育には苦労してるみたいだね、若頭。
    {"WAKAGASHIRA RYOTA", "Hah. You've got some nerve.", true},                                                    // RYOTA: ハッ。言ってくれるじゃねぇか。
    {"WAKAGASHIRA RYOTA", "But I'll give you credit for having the guts to come this far.", true},                 // RYOTA: だが、ここまで来た度胸だけは褒めてやるよ。
    {"MOMIJI", "I'm honored.", false},                                                                             // MOMIJI: お褒めにあずかり光栄だね。
    {"MOMIJI", "I'd be even happier if you'd surrender nice and quietly.", false},                                 // MOMIJI: じゃあ、そのまま大人しく投降してくれるともっと嬉しいんだけど。
    {"WAKAGASHIRA RYOTA", "...Surrender?", true},                                                                  // RYOTA: ……投降？
    {"WAKAGASHIRA RYOTA", "Heh... Hahahahaha!", true},                                                             // RYOTA: ククッ……ハハハハ！
    {"WAKAGASHIRA RYOTA", "You ever play baseball?", true},                                                        // RYOTA: お前、野球やったことあるか？
    {"MOMIJI", "...Where did that come from?", false},                                                             // MOMIJI: ……急に何の話？
    {"WAKAGASHIRA RYOTA", "I used to be a pitcher.", true},                                                        // RYOTA: 俺ァ昔、ピッチャーだったんだよ。
    {"WAKAGASHIRA RYOTA", "Once I stepped onto the mound--", true},                                                // RYOTA: マウンドに立ったら最後――
    {"WAKAGASHIRA RYOTA", "didn't matter who was at bat. I never left until I struck 'em out.", true},             // RYOTA: 相手が誰だろうと、三振取るまで降りねぇ。
    {"MOMIJI", "I see.", false},                                                                                   // MOMIJI: なるほど。
    {"MOMIJI", "A baseball kid who grew up to be a Space Yakuza Wakagashira.", false},                             // MOMIJI: 野球少年が今じゃスペースヤクザの若頭か。
    {"MOMIJI", "You've really thrown your life down a rough path.", false},                                        // MOMIJI: 随分と荒れた人生を投げてきたもんだ。
    {"WAKAGASHIRA RYOTA", "...You bitch.", true},                                                                  // RYOTA: ……テメェ。
    {"WAKAGASHIRA RYOTA", "You got a problem with the way I've lived my life?", true},                             // RYOTA: 俺の人生にケチつける気か？
    {"MOMIJI", "Not a problem. Just an official ruling from the police.", false},                                  // MOMIJI: ケチじゃない。警察としての判定だよ。
    {"MOMIJI", "You've been three strikes out for a long time now.", false},                                       // MOMIJI: あんたはもう、とっくにスリーアウトだ。
    {"WAKAGASHIRA RYOTA", "...Fine by me.", true},                                                                 // RYOTA: ……上等じゃねぇか。
    {"WAKAGASHIRA RYOTA", "Then play umpire and try calling me out yourself!", true},                              // RYOTA: だったらテメェが審判気取りで、俺に引導を渡してみろ！
    {"WAKAGASHIRA RYOTA", "But here's the thing--", true},                                                         // RYOTA: ただし――
    {"WAKAGASHIRA RYOTA", "my pitches are a hell of a lot faster than they used to be.", true},                    // RYOTA: 俺の球は、昔よりずっと速ぇぞ。
    {"MOMIJI", "Funny coincidence.", false},                                                                       // MOMIJI: 奇遇だね。
    {"MOMIJI", "I've always been pretty good at hitting bad pitches.", false},                                     // MOMIJI: 私も昔から、悪球打ちには自信がある。
    {"WAKAGASHIRA RYOTA", "Hah! Now that's what I like to hear!", true},                                           // RYOTA: ハッ！ 面白ぇ！
    {"WAKAGASHIRA RYOTA", "'SAYONARA', pitching system online!", true},                                            // RYOTA: 『SAYONARA号』、投球システム起動！
    {"WAKAGASHIRA RYOTA", "I'm throwing heat from the very first pitch!!", true},                                  // RYOTA: 初球から全力で行くぞォ！！
    {"WAKAGASHIRA RYOTA", "I'll fire one straight through your goddamn center!!", true}                            // RYOTA: テメェのド真ん中、ブチ抜いてやる！！
};

/**
 * @brief Stage 3のボス戦前会話を取得する
 * @return Stage 3の台詞とボス名を含む会話定義
 */
inline constexpr BossStory Story() {
    return BossStories::Create(StoryLines, "RYOTA");
}
}
