#pragma once

#include "MMLParser.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Audio {

// 波形プロバイダー
class IWaveformProvider {
public:
    virtual ~IWaveformProvider() = default;
    virtual float Sample(uint8_t customWaveId, float phase, float frequency, int sampleRate, float timeInNoteSec, bool isNoteStart) = 0;
};

// PCM波形ジェネレーター
class SynthWaveGenerator {
public:
    SynthWaveGenerator();
    ~SynthWaveGenerator() = default;

    void SetWaveformProvider(std::shared_ptr<IWaveformProvider> provider) {
        customProvider_ = provider;
    }

    std::vector<int16_t> GeneratePCM(const MMLSequence& sequence, int sampleRate = 44100);

private:
    float GenerateOscillatorSample(
        WaveformType type, uint8_t customId, float phase, float frequency, int sampleRate, uint32_t lfsrState, float timeInNoteSec, int trackIndex
    );

    std::shared_ptr<IWaveformProvider> customProvider_ = nullptr;
    float fadeTimeSec_ = 0.003f;
};

// PCMサンプルプロバイダー
class SFCPCMSampleProvider : public IWaveformProvider {
public:
    SFCPCMSampleProvider();
    virtual ~SFCPCMSampleProvider() = default;

    virtual float Sample(uint8_t customWaveId, float phase, float frequency, int sampleRate, float timeInNoteSec, bool isNoteStart) override;
};

} // namespace Audio
