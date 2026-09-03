#include <cassert>
#include <cstdint>

#include "../Presentation/Gameplay/Voices/VoiceDpcmDecoder.h"

/**
 * @brief 自機撃破音声の先頭無音除去と無線加工を検証する
 * @return なし
 */
void RunVoiceDpcmDecoderTests() {
    const std::vector<std::int16_t> source =
        VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeath);
    const std::vector<std::int16_t> plain =
        VoiceCodec::DecodeForAudioService(VoiceSamples::momijiDeath);
    const std::vector<std::int16_t> radio =
        VoiceCodec::DecodeRadioForAudioService(VoiceSamples::momijiDeath);

    // 素材配列自体を発声直前から始め、加工で再生時間を変えない
    assert(source.size() == VoiceSamples::momijiDeath.sampleCount);
    assert(std::any_of(source.begin(), source.begin() + 80, [](std::int16_t value) {
        return std::abs(static_cast<int>(value)) >= 2048;
    }));
    assert(!radio.empty());
    assert(radio.size() == plain.size());
    for (const std::int16_t value : radio) assert(value % 256 == 0);
}
