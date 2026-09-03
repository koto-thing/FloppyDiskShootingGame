#include <cassert>
#include <cstdint>

#include "../Presentation/Gameplay/Voices/VoiceDpcmDecoder.h"

/**
 * @brief 自機撃破音声の先頭無音除去と無線加工を検証する
 * @return なし
 */
void RunVoiceDpcmDecoderTests() {
    const std::vector<std::int16_t> plain =
        VoiceCodec::DecodeForAudioService(VoiceSamples::momijiDeath);
    const std::vector<std::int16_t> radio =
        VoiceCodec::DecodeRadioForAudioService(VoiceSamples::momijiDeath);

    // 加工後は先頭無音が短くなり、全サンプルが粗い量子化幅へ揃う
    assert(!radio.empty());
    assert(radio.size() < plain.size());
    for (const std::int16_t value : radio) assert(value % 256 == 0);
}
