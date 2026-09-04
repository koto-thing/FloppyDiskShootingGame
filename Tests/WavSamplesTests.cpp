#include <cassert>
#include <cstdint>
#include <vector>

#include "../Infrastructure/ExternalServices/WavSamples.h"

/**
 * @brief 埋め込みPCMの差分圧縮データを検証する
 * @return なし
 */
void RunWavSamplesTests() {
    for (const auto& sample : Audio::WavSamples::SAMPLES) {
        std::vector<std::uint8_t> pcm;

        // 全サンプルを復号し、変換前PCMのサイズとチェックサムを照合する
        assert(Audio::WavSamples::Decode(sample, pcm));
        assert(pcm.size() == sample.sampleCount);
        std::uint32_t checksum = 2166136261u;
        for (const std::uint8_t value : pcm) {
            checksum = (checksum ^ value) * 16777619u;
        }
        assert(checksum == sample.checksum);
    }
}
