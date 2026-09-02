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
    {"PILOT", "I am not backing down now.", false},
    {"BOSS", "Then disappear with the rest.", true}
};
inline constexpr BossStoryLine Stage3[] = {
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
inline constexpr BossStoryLine Stage4[] = {
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
inline constexpr BossStoryLine Stage5[] = {
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
 * @brief 指定ステージのボス戦前会話を取得する
 * @param stageNumber ステージ番号
 * @return 指定ステージに対応する会話
 */
inline constexpr BossStory ForStage(int stageNumber) {
    switch (stageNumber) {
    case 2: return Create(Stage2, "RYOTA");
    case 3: return Create(Stage3, "BOSS");
    case 4: return Create(Stage4, "BOSS");
    case 5: return Create(Stage5, "EASTSOURCE");
    default: return Create(Stage1, "KOTO");
    }
}
}
