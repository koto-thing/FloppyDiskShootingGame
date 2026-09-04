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
    {"MOMIJI", "...Hah, hah... Finally made it to the rooftop.", false},                                              // MOMIJI: ……はぁ、はぁ……ようやく本部の屋上まで追いつめたぞ。
    {"MOMIJI", "Come out, Chairman TAYAMA! There's nowhere left to run!", false},                                      // MOMIJI: 姿を現しな、TAYAMA会長！ もう逃げ場はないよ！
    {"CHAIRMAN TAYAMA", "...Run? Who is running from whom, you insolent whelp?", true},                               // TAYAMA: ……逃げる？ 誰が誰から逃げるというのじゃ、身の程知らずの小娘が。
    {"CHAIRMAN TAYAMA", "Look upon this night sky. The neon lights below look just like an offering of lotus flowers.", true}, // TAYAMA: 見下ろしてみせい、この夜景を。眼下に広がるネオンは、まるで散華した極楽の蓮の花よ。
    {"MOMIJI", "...What the hell is that massive machine behind you?!", false},                                       // MOMIJI: ……その背後にある巨大な機体、一体何なんだ？！
    {"CHAIRMAN TAYAMA", "Ku... Kuhahaha! Splendid, is it not?", true},                                                // TAYAMA: クッ……クハハハハ！ 壮観であろう？
    {"CHAIRMAN TAYAMA", "Behold my heavenly soul, the Super-Dreadnought Titan 'DARUTANYAN'!", true},                   // TAYAMA: これぞ我が極道魂の具現、超巨大決戦機甲『堕流多虐（DARUTANYAN）』じゃ！
    {"CHAIRMAN TAYAMA", "KOTO, RYOTA, BOTAMOCHI, and EASTSOURCE... They all died to fuel this ultimate god!", true},  // TAYAMA: 散っていった幹部どもの血肉と怨嗟、その全てを炉にくべた究極の神仏よ！
    {"MOMIJI", "Using the corpses of your own subordinates to power a machine... You're sick to the core!", false},  // MOMIJI: 身内の命まで動力源にするなんて……どこまで腐りきってるんだ！
    {"CHAIRMAN TAYAMA", "Insolence! A mere police dog cannot grasp the sublime harmony of violence and art!", true},   // TAYAMA: 戯言を！ 暴力と極道芸術が織りなす究極の調和、サツの犬風情に理解できるものか！
    {"CHAIRMAN TAYAMA", "From atop this colossal titan, I shall purge every law from this galaxy!", true},             // TAYAMA: ワシはこの『堕流多虐』の頂より、銀河の法ことごとくを灰燼に帰すのだ！
    {"MOMIJI", "I'm ending your mad delusion right here on this rooftop.", false},                                    // MOMIJI: あんたの狂った妄執も、この屋上で幕引きだ。
    {"MOMIJI", "I don't care how big you are. You're still going down!", false},                                      // MOMIJI: 相手が神仏だろうと超巨大ロボだろうと、撃ち落とすことに変わりはない！
    {"CHAIRMAN TAYAMA", "Guhahaha! Then offer your flesh, blood, and iron to my mechanical god!", true},              // TAYAMA: グハハハ！ ならばテメェの血肉と鉄屑、この鋼の神仏に捧げるがいい！
    {"CHAIRMAN TAYAMA", "ALL WEAPONS UNLEASHED!! BOW DOWN BEFORE ME AND TURN TO ASHES!!", true},                      // TAYAMA: 『堕流多虐』全武装解放！！ ワシにひれ伏し、灰となって消え失せぇぇいッ！！
};

/**
 * @brief Stage 5のラスボス戦前会話を取得する
 * @return TAYAMA戦の台詞とボス名を含む会話定義
 */
inline constexpr BossStory TayamaStory() {
    return BossStories::Create(TayamaStoryLines, "TAYAMA");
}
}
