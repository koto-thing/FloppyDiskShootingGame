#pragma once

#include "../Common/BossStory.h"

namespace ShooterStages::Stage5 {
inline constexpr BossStoryLine EastsourceStoryLines[] = {
    {"MOMIJI", "...Out of the way.", false},                                                                               // MOMIJI: ……そこをどきな。
    {"MOMIJI", "The Boss is just beyond this gate, isn't he?", false},                                                     // MOMIJI: この扉の向こうに、あんたらの親玉がいるんだろう。
    {"HITMAN EASTSOURCE", "Hahahahaha! Hyaho~!!", true},                                                                   // EASTSOURCE: ハーッハッハッハ！ ヒャッホォォゥ！！
    {"HITMAN EASTSOURCE", "Hold your horses, copper-chan! Where d'ya think you're goin' so fast?!", true},                 // EASTSOURCE: ちょお待ちぃな、マッポちゃん！ どこ急ぎ足で行こうとしとんねん！
    {"MOMIJI", "EASTSOURCE... The PandD-kai's loose cannon hitman.", false},                                               // MOMIJI: EASTSOURCE……PANDD会のイカれた鉄砲玉だな。
    {"HITMAN EASTSOURCE", "Bingo, hit the jackpot! Yer smarter than you look, gal!", true},                                // EASTSOURCE: ピンポォン、大正解！ 見た目より頭回るやんけ、姉ちゃん！
    {"HITMAN EASTSOURCE", "Ya made mincemeat outta KOTO, RYOTA, and even BOTAMOCHI, right?", true},                        // EASTSOURCE: KOTOもRYOTAも、あのBOTAMOCHIの旦那までミンチにしたんやってなぁ？
    {"HITMAN EASTSOURCE", "Goddamn, you're terrifying! Ya made me so horny for a fight, my teeth are chatterin'!!", true}, // EASTSOURCE: ゾクゾクするわぁ！ ワシもうなぁ、嬉しゅうて奥歯ガタガタ鳴っとんねん！！
    {"MOMIJI", "You're clearly not wired right.", false},                                                                  // MOMIJI: 頭のネジ、何本か宇宙空間に落としてきたみたいだね。
    {"MOMIJI", "If you don't wanna end up like them, stand down.", false},                                                 // MOMIJI: あいつらと同じ目に遭いたくないなら、素直に道を空けろ。
    {"HITMAN EASTSOURCE", "Hahaha! Stand down?! Like hell I will!!", true},                                                // EASTSOURCE: ハハッ！ 退けやとぉ？！ 誰が退くかいドアホ！！
    {"HITMAN EASTSOURCE", "I'm the syndicate's crazy dog, EASTSOURCE-sama!", true},                                        // EASTSOURCE: ワシは組の狂犬、EASTSOURCE様やぞ！
    {"HITMAN EASTSOURCE", "The Patriarch is chillin' inside. I ain't lettin' a cheap cop ruin his buzz!", true},           // EASTSOURCE: 奥でお待ちのオヤジのシマ荒らされてたまるかい！
    {"MOMIJI", "So you're the last stray dog on the leash, huh.", false},                                                  // MOMIJI: ボスの前で尻尾振る、最後の番犬ってわけか。
    {"MOMIJI", "Fine. I'll put you to sleep right here.", false},                                                          // MOMIJI: 上等だ。ここでまとめて眠らせてやるよ。
    {"HITMAN EASTSOURCE", "Gyaahahaha! That's the spirit, lady!!", true},                                                  // EASTSOURCE: ギャーッハハハ！ ええ面構えや、最高やんけぇ！！
    {"HITMAN EASTSOURCE", "'DOTONBORI' thrusters, full throttle! Break off the limiter!!", true},                          // EASTSOURCE: 愛機『DOTONBORI号』、全開バリバリ！ リミッター外したれェ！
    {"HITMAN EASTSOURCE", "Let's party 'til one of us turns into cold scrap, MOMIJI-chan!!", true},                        // EASTSOURCE: どっちが鉄クズになるか、ド派手に踊り狂おうやァ、MOMIJIちゃぁぁん！！
    {"MOMIJI", "Thrusters maximum output. Target: Hostile interceptor, engaging!", false},                                 // MOMIJI: スラスター最大出力。迎撃機、叩き落とす！
    {"HITMAN EASTSOURCE", "HYAHAHAHA! LET'S GET NUTS!! DIE, COP DOGGY!!", true},                                           // EASTSOURCE: ヒャハハハ！ 狂い咲いたるわァ！ 死に晒せェ、サツの犬コロォォッ！！
};

/**
 * @brief Stage 5のボス戦前会話を取得する
 * @return Stage 5の台詞とボス名を含む会話定義
 */
inline constexpr BossStory EastsourceStory() {
    return BossStories::Create(EastsourceStoryLines, "EASTSOURCE");
}

inline constexpr BossStoryLine TayamaStoryLines[] = {
    {"CHAIRMAN TAYAMA", "So you are the officer who tore through my entire fleet.", true},          // TAYAMA: 我が艦隊をことごとく潰した警官とは、お前か
    {"MOMIJI", "TAYAMA. Your syndicate ends here.", false},                                         // MOMIJI: TAYAMA。あんたの組もここまでだ
    {"CHAIRMAN TAYAMA", "An organization is immortal while its chairman still stands.", true},      // TAYAMA: 会長が立つ限り、組織は不滅だ
    {"MOMIJI", "Then I'll bring down the chairman and the whole rotten tower with him.", false},     // MOMIJI: なら会長ごと、その腐った塔を叩き潰す
    {"CHAIRMAN TAYAMA", "Know your place. This city, this sky, and your life belong to me.", true},  // TAYAMA: 身の程を知れ。この街も空も、お前の命も儂のものだ
    {"MOMIJI", "Not anymore. MOMIJI, engaging the final target!", false},                            // MOMIJI: 今日で終わりだ。最終目標TAYAMA、交戦開始！
};

/**
 * @brief Stage 5のラスボス戦前会話を取得する
 * @return TAYAMA戦の台詞とボス名を含む会話定義
 */
inline constexpr BossStory TayamaStory() {
    return BossStories::Create(TayamaStoryLines, "TAYAMA");
}
}
