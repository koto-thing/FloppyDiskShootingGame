#include <cassert>
#include <cstdint>

#include "../Presentation/Gameplay/Voices/VoiceDpcmDecoder.h"

namespace {
/**
 * @brief PCMの先頭と末尾に有効な音が残っているか判定する
 * @param pcm 判定するPCMデータ
 * @param sampleRate PCMのサンプルレート
 * @return 両端20msのRMSが-45dB以上の場合true
 */
bool HasAudibleEdges(const std::vector<std::int16_t>& pcm, std::uint32_t sampleRate) {
    constexpr std::int64_t MinimumAudibleRms = 184;
    const std::size_t window = (std::min)(pcm.size(),
        (std::max<std::size_t>)(1, sampleRate / 50));
    const auto IsAudible = [&](std::size_t offset) {
        std::int64_t energy = 0;
        for (std::size_t i = 0; i < window; ++i) {
            const std::int64_t value = pcm[offset + i];
            energy += value * value;
        }
        return energy >= static_cast<std::int64_t>(window) *
            MinimumAudibleRms * MinimumAudibleRms;
    };
    return IsAudible(0) && IsAudible(pcm.size() - window);
}
}

/**
 * @brief IMA ADPCM音声の復号と無線加工を検証する
 * @return なし
 */
void RunVoiceDpcmDecoderTests() {
    // 空入力・無効レート・単一点・等倍変換でも境界を安全に扱う
    assert(VoiceCodec::ResampleCubic({}, 6000, 44100).empty());
    assert(VoiceCodec::ResampleCubic({1234}, 0, 44100).empty());
    assert(VoiceCodec::ResampleCubic({1234}, 6000, 0).empty());
    assert(VoiceCodec::ResampleCubic({1234}, 6000, 12000) ==
        std::vector<std::int16_t>({1234, 1234}));
    assert(VoiceCodec::ResampleCubic({-32768, 32767}, 6000, 6000) ==
        std::vector<std::int16_t>({-32768, 32767}));

    // 既知の正弦波で線形補間より復元誤差が小さいことを確認する
    constexpr double Pi = 3.14159265358979323846;
    std::vector<std::int16_t> tone(600);
    for (std::size_t i = 0; i < tone.size(); ++i) {
        tone[i] = static_cast<std::int16_t>(std::lround(12000 * std::sin(2 * Pi * i / 10)));
    }
    const auto upsampled = VoiceCodec::ResampleCubic(tone, 6000, 44100);
    assert(upsampled.size() == 4410);
    double cubicError = 0;
    double linearError = 0;
    for (std::size_t i = 20; i + 20 < upsampled.size(); ++i) {
        const double position = i * 6000.0 / 44100;
        const auto left = static_cast<std::size_t>(position);
        const double linear = std::lerp(static_cast<double>(tone[left]),
            static_cast<double>(tone[left + 1]), position - left);
        const double ideal = 12000 * std::sin(2 * Pi * position / 10);
        cubicError += std::pow(upsampled[i] - ideal, 2);
        linearError += std::pow(linear - ideal, 2);
    }
    assert(cubicError < linearError * 0.1);

    // 補間のオーバーシュートを16bit範囲へ飽和させる
    const auto peaks = VoiceCodec::ResampleCubic({-32768, 32767, 32767, -32768}, 6000, 12000);
    assert(peaks[3] == 32767);

    const std::vector<std::int16_t> source =
        VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeath);
    const std::vector<std::int16_t> plain =
        VoiceCodec::DecodeForAudioService(VoiceSamples::momijiDeath);
    const std::vector<std::int16_t> radio =
        VoiceCodec::DecodeRadioForAudioService(VoiceSamples::momijiDeath);

    // 各音声を宣言されたPCMサンプル数まで復号できることを確認する
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::mission).size() ==
        VoiceSamples::mission.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::start).size() ==
        VoiceSamples::start.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::arrested).size() ==
        VoiceSamples::arrested.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::suspect).size() ==
        VoiceSamples::suspect.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::kotoDeath).size() ==
        VoiceSamples::kotoDeath.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::ryotaDeath).size() ==
        VoiceSamples::ryotaDeath.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::lumiDeath).size() ==
        VoiceSamples::lumiDeath.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::botamochiDeathBota).size() ==
        VoiceSamples::botamochiDeathBota.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::botamochiDeathMochi).size() ==
        VoiceSamples::botamochiDeathMochi.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeathRattle2).size() ==
        VoiceSamples::momijiDeathRattle2.sampleCount);
    assert(VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeathRattle5).size() ==
        VoiceSamples::momijiDeathRattle5.sampleCount);

    // 端部の無音と低レベルノイズが再混入していないことを確認する
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::tayamaDeath),
        VoiceSamples::tayamaDeath.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::eastsourceDeath),
        VoiceSamples::eastsourceDeath.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeath),
        VoiceSamples::momijiDeath.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::mission),
        VoiceSamples::mission.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::start),
        VoiceSamples::start.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::arrested),
        VoiceSamples::arrested.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::suspect),
        VoiceSamples::suspect.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::ryotaDeath),
        VoiceSamples::ryotaDeath.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::lumiDeath),
        VoiceSamples::lumiDeath.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::botamochiDeathBota),
        VoiceSamples::botamochiDeathBota.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::botamochiDeathMochi),
        VoiceSamples::botamochiDeathMochi.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeathRattle2),
        VoiceSamples::momijiDeathRattle2.sampleRate));
    assert(HasAudibleEdges(VoiceCodec::DecodeImaAdpcm(VoiceSamples::momijiDeathRattle5),
        VoiceSamples::momijiDeathRattle5.sampleRate));

    // 自機撃破音声は発声直前から始め、加工で再生時間を変えない
    assert(source.size() == VoiceSamples::momijiDeath.sampleCount);
    assert(std::any_of(source.begin(), source.begin() + 80, [](std::int16_t value) {
        return std::abs(static_cast<int>(value)) >= 2048;
    }));
    assert(!radio.empty());
    assert(radio.size() == plain.size());
    // 短い端部フェードで発声を残したまま開始・終了の段差をなくす
    assert(plain.front() == 0 && plain.back() == 0);
    assert(HasAudibleEdges(plain, 44100));
    for (const std::int16_t value : radio) assert(value % 256 == 0);
}
