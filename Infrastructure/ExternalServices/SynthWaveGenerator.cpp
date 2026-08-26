#include "SynthWaveGenerator.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

namespace Audio {

constexpr float PI_F = 3.14159265358979323846f;

SFCPCMSampleProvider::SFCPCMSampleProvider() = default;

float SFCPCMSampleProvider::Sample(uint8_t instrumentId, float phase, float frequency, int sampleRate, float t, bool isNoteStart) {
    if (instrumentId == 0) {
        float hammerNoise = 0.0f;
        if (t < 0.10f) {
            float noiseEnv = std::exp(-t * 90.0f);
            hammerNoise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * noiseEnv * 0.4f;
        }

        float loopSec = 0.16f;
        float effectiveT = t;
        if (t > 0.12f) {
            effectiveT = 0.12f + std::fmod(t - 0.12f, loopSec);
        }

        float fund = std::sin(2.0f * PI_F * frequency * effectiveT);
        float h2 = 0.40f * std::sin(2.0f * PI_F * (frequency * 2.0f) * effectiveT);
        float h3 = 0.20f * std::sin(2.0f * PI_F * (frequency * 3.0f) * effectiveT);
        float env = std::exp(-t * 1.5f);

        return (fund + h2 + h3 + hammerNoise) * env;
    }

    if (instrumentId == 1) {
        float pickSnap = 0.0f;
        if (t < 0.08f) {
            float snapEnv = std::exp(-t * 160.0f);
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float pluck = std::sin(2.0f * PI_F * 880.0f * t) * std::exp(-t * 100.0f);
            pickSnap = (noise * 0.7f + pluck * 0.4f) * snapEnv;
        }

        float loopSec = 0.14f;
        float effectiveT = t;
        if (t > 0.08f) {
            effectiveT = 0.08f + std::fmod(t - 0.08f, loopSec);
        }

        float sub = std::sin(2.0f * PI_F * frequency * effectiveT);
        float saw = (std::fmod(effectiveT * frequency, 1.0f) - 0.5f) * 0.4f;
        float env = std::exp(-t * 1.8f);

        return (sub + saw + pickSnap) * env;
    }

    if (instrumentId == 2) {
        float pickNoise = 0.0f;
        if (t < 0.06f) {
            pickNoise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 100.0f) * 0.3f;
        }

        float loopSec = 0.16f;
        float effectiveT = t;
        if (t > 0.08f) {
            effectiveT = 0.08f + std::fmod(t - 0.08f, loopSec);
        }

        float raw = (std::fmod(effectiveT * frequency, 1.0f) - 0.5f) + pickNoise;
        float dist = std::tanh(raw * 7.0f);
        if (dist > 0.4f) dist = 0.4f + (dist - 0.4f) * 0.2f;
        dist *= 1.8f;

        float env = std::exp(-t * 1.4f);
        return dist * env;
    }

    if (instrumentId == 3) {
        float pluckNoise = 0.0f;
        if (t < 0.08f) {
            pluckNoise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 110.0f) * 0.4f;
        }

        float loopSec = 0.17f;
        float effectiveT = t;
        if (t > 0.08f) {
            effectiveT = 0.08f + std::fmod(t - 0.08f, loopSec);
        }

        float fund = std::sin(2.0f * PI_F * frequency * effectiveT);
        float bodyRes = 0.30f * std::sin(2.0f * PI_F * (frequency * 1.5f) * effectiveT);
        float env = std::exp(-t * 2.2f);

        return (fund + bodyRes + pluckNoise) * env;
    }

    if (instrumentId == 5) {
        // ブラス (アタックで滑らかに最大音量になり、ロングトーンも美しく伸びる)
        float brassAttack = std::sin(PI_F * 0.5f * std::min(1.0f, t / 0.06f));
        float saw1 = (phase - 0.5f) * 2.0f;
        float saw2 = (std::fmod(phase * 1.003f, 1.0f) - 0.5f) * 2.0f;
        float env = brassAttack * std::exp(-t * 0.25f);

        return (saw1 + saw2) * 0.4f * env;
    }

    if (instrumentId == 6) {
        // ストリングス / リード
        float stringsAttack = std::sin(PI_F * 0.5f * std::min(1.0f, t / 0.12f));
        float saw1 = (phase - 0.5f) * 2.0f;
        float saw2 = (std::fmod(phase * 1.005f, 1.0f) - 0.5f) * 2.0f;
        float env = stringsAttack * std::exp(-t * 0.20f);

        return (saw1 + saw2) * 0.35f * env;
    }

    if (instrumentId == 8 || instrumentId == 7) {
        // キックドラム / タム (ノート周波数 frequency に応じて音高が変化する)
        float pitchScale = std::clamp(frequency / 110.0f, 0.3f, 4.0f);
        float pEnv = (240.0f * pitchScale) * std::exp(-t * 40.0f) + (50.0f * pitchScale);
        float click = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 180.0f) * 0.5f;
        float body = std::sin(2.0f * PI_F * pEnv * t);
        float env = std::exp(-t * 12.0f);
        return (body + click) * env * 2.5f;
    }

    if (instrumentId == 9) {
        // スネアドラム
        float pitchScale = std::clamp(frequency / 180.0f, 0.5f, 2.0f);
        float tone = std::sin(2.0f * PI_F * (320.0f * pitchScale * std::exp(-t * 25.0f)) * t) * std::exp(-t * 18.0f);
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 14.0f);
        return (tone * 0.8f + noise * 0.9f) * 2.0f;
    }

    if (instrumentId == 10) {
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
        float env = std::exp(-t * 60.0f);
        return noise * env * 0.8f;
    }

    if (instrumentId == 11) {
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
        float env = std::exp(-t * 10.0f);
        return noise * env * 0.8f;
    }

    if (instrumentId == 12) {
        float f1 = std::sin(2.0f * PI_F * 3420.0f * t);
        float f2 = std::sin(2.0f * PI_F * 5130.0f * t);
        float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
        float env = std::exp(-t * 5.0f);
        return (noise * 0.6f + (f1 + f2) * 0.2f) * env;
    }

    if (instrumentId == 4) {
        if (frequency < 140.0f) {
            float pitchScale = std::clamp(frequency / 110.0f, 0.3f, 4.0f);
            float pEnv = (240.0f * pitchScale) * std::exp(-t * 40.0f) + (50.0f * pitchScale);
            float click = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 180.0f) * 0.5f;
            float body = std::sin(2.0f * PI_F * pEnv * t);
            float env = std::exp(-t * 12.0f);
            return (body + click) * env * 2.5f;
        } 
        else if (frequency >= 140.0f && frequency < 220.0f) {
            float pitchScale = std::clamp(frequency / 180.0f, 0.5f, 2.0f);
            float tone = std::sin(2.0f * PI_F * (320.0f * pitchScale * std::exp(-t * 25.0f)) * t) * std::exp(-t * 18.0f);
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * std::exp(-t * 14.0f);
            return (tone * 0.8f + noise * 0.9f) * 2.0f;
        }
        else if (frequency >= 220.0f && frequency < 350.0f) {
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float env = std::exp(-t * 60.0f);
            return noise * env * 0.8f;
        }
        else if (frequency >= 350.0f && frequency < 500.0f) {
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float env = std::exp(-t * 10.0f);
            return noise * env * 0.8f;
        }
        else {
            float f1 = std::sin(2.0f * PI_F * 3420.0f * t);
            float f2 = std::sin(2.0f * PI_F * 5130.0f * t);
            float noise = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f);
            float env = std::exp(-t * 5.0f);
            return (noise * 0.6f + (f1 + f2) * 0.2f) * env;
        }
    }

    return std::sin(2.0f * PI_F * frequency * t);
}

SynthWaveGenerator::SynthWaveGenerator() {
    customProvider_ = std::make_shared<SFCPCMSampleProvider>();
}

std::vector<int16_t> SynthWaveGenerator::GeneratePCM(const MMLSequence& sequence, int sampleRate) {
    if (sequence.tracks.empty() || sequence.maxDurationSec <= 0.0) {
        return {};
    }

    size_t totalSamples = static_cast<size_t>(sequence.maxDurationSec * sampleRate);
    if (totalSamples == 0) {
        return {};
    }

    std::vector<float> mixBuffer(totalSamples, 0.0f);
    int fadeSamples = std::max(1, static_cast<int>(fadeTimeSec_ * sampleRate));

    int trackIndex = 0;
    for (const auto& track : sequence.tracks) {
        uint32_t lfsr = 0x7FFF;

        for (const auto& event : track.events) {
            size_t startSample = static_cast<size_t>(event.startTimeSec * sampleRate);
            size_t durationSamples = static_cast<size_t>(event.durationSec * sampleRate);
            size_t endSample = std::min(totalSamples, startSample + durationSamples);

            if (startSample >= totalSamples || durationSamples == 0) {
                continue;
            }

            int noteFadeSamples = std::min(fadeSamples, static_cast<int>(durationSamples / 2));
            float phase = 0.0f;
            float phaseIncrement = event.frequency / static_cast<float>(sampleRate);

            for (size_t s = startSample; s < endSample; ++s) {
                size_t sampleIndexInNote = s - startSample;
                float timeInNoteSec = static_cast<float>(sampleIndexInNote) / static_cast<float>(sampleRate);
                
                // Envelope (Attack & Release)
                float env = 1.0f;
                if (sampleIndexInNote < static_cast<size_t>(noteFadeSamples)) {
                    env = static_cast<float>(sampleIndexInNote) / static_cast<float>(noteFadeSamples);
                } else if (sampleIndexInNote > durationSamples - static_cast<size_t>(noteFadeSamples)) {
                    size_t rem = durationSamples - sampleIndexInNote;
                    env = static_cast<float>(rem) / static_cast<float>(noteFadeSamples);
                }

                if (event.waveType == WaveformType::Noise) {
                    uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
                    lfsr = (lfsr >> 1) | (bit << 14);
                }

                float oscSample = GenerateOscillatorSample(
                    event.waveType, event.customWaveId, phase, event.frequency, sampleRate, lfsr, timeInNoteSec, trackIndex
                );

                mixBuffer[s] += oscSample * event.volume * env;

                phase += phaseIncrement;
                if (phase >= 1.0f) {
                    phase -= std::floor(phase);
                }
            }
        }
        trackIndex++;
    }

    std::vector<int16_t> pcmBuffer(totalSamples);
    float trackCountFactor = std::sqrt(static_cast<float>(std::max<size_t>(1, sequence.tracks.size())));
    float scale = 0.55f / trackCountFactor;

    for (size_t i = 0; i < totalSamples; ++i) {
        float sample = mixBuffer[i] * scale;
        sample = std::tanh(sample);
        pcmBuffer[i] = static_cast<int16_t>(sample * 30000.0f);
    }

    return pcmBuffer;
}

float SynthWaveGenerator::GenerateOscillatorSample(
    WaveformType type, uint8_t customId, float phase, float frequency, int sampleRate, uint32_t lfsrState, float timeInNoteSec, int trackIndex) 
{
    uint8_t instrumentId = customId;
    if (customProvider_) {
        return customProvider_->Sample(instrumentId, phase, frequency, sampleRate, timeInNoteSec, (timeInNoteSec == 0.0f));
    }

    switch (type) {
        case WaveformType::Square:
            return (phase < 0.5f) ? 1.0f : -1.0f;
        case WaveformType::Triangle:
            return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        case WaveformType::Sawtooth:
            return 2.0f * phase - 1.0f;
        case WaveformType::Noise:
            return (lfsrState & 1) ? 1.0f : -1.0f;
        default:
            return (phase < 0.5f) ? 1.0f : -1.0f;
    }
}

} // namespace Audio
