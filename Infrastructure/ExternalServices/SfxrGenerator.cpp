#include "SfxrGenerator.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace Audio {

SfxrParams SfxrParams::CreatePreset(SfxrPreset preset) {
    SfxrParams p;
    switch (preset) {
        case SfxrPreset::LaserShoot:
            p.waveType = SfxrWaveType::Square;
            p.startFrequency = 0.6f;
            p.minFrequency = 0.1f;
            p.slide = -0.35f;
            p.sustainTime = 0.08f;
            p.decayTime = 0.12f;
            p.squareDuty = 0.4f;
            break;
        case SfxrPreset::Explosion:
            p.waveType = SfxrWaveType::Noise;
            p.startFrequency = 0.2f;
            p.slide = -0.1f;
            p.sustainTime = 0.15f;
            p.decayTime = 0.4f;
            break;
        case SfxrPreset::HitHurt:
            p.waveType = SfxrWaveType::Noise;
            p.startFrequency = 0.4f;
            p.slide = -0.2f;
            p.sustainTime = 0.05f;
            p.decayTime = 0.15f;
            break;
        case SfxrPreset::BlipSelect:
            p.waveType = SfxrWaveType::Square;
            p.startFrequency = 0.2f;
            p.sustainTime = 0.05f;
            p.decayTime = 0.05f;
            p.squareDuty = 0.5f;
            break;
    }
    return p;
}

std::vector<int16_t> SfxrGenerator::GeneratePCM(const SfxrParams& params, int sampleRate) {
    // 時間・周波数のパラメータ計算
    float fAttack = params.attackTime * params.attackTime * 100000.0f;
    float fSustain = params.sustainTime * params.sustainTime * 100000.0f;
    float fDecay = params.decayTime * params.decayTime * 100000.0f;

    int totalSamples = static_cast<int>(fAttack + fSustain + fDecay);
    if (totalSamples <= 0) {
        totalSamples = sampleRate / 10;
    }

    std::vector<int16_t> pcm(totalSamples);

    double fPeriod = 100.0 / (params.startFrequency * params.startFrequency * 0.99 + 0.001);
    double fMaxPeriod = 100.0 / (params.minFrequency * params.minFrequency * 0.99 + 0.001);
    double fSlide = 1.0 - pow(params.slide, 3.0) * 0.01;

    double squareDuty = 0.5 - params.squareDuty * 0.5;

    double period = fPeriod;
    double phase = 0.0;

    // ノイズテーブルの作成
    std::vector<double> noise(32);
    for (int i = 0; i < 32; ++i) {
        noise[i] = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
    }

    int envStage = 0;
    int envTime = 0;
    int envLength[3] = { static_cast<int>(fAttack), static_cast<int>(fSustain), static_cast<int>(fDecay) };
    float envVolume = 0.0f;

    for (int i = 0; i < totalSamples; ++i) {
        // 周波数スイープの適用
        period *= fSlide;
        if (period > fMaxPeriod) {
            period = fMaxPeriod;
        }

        // エンベロープ処理
        envTime++;
        if (envStage < 3 && envTime >= envLength[envStage]) {
            envTime = 0;
            envStage++;
        }

        if (envStage == 0) {
            envVolume = static_cast<float>(envTime) / std::max(1, envLength[0]);
        } else if (envStage == 1) {
            envVolume = 1.0f;
        } else if (envStage == 2) {
            envVolume = 1.0f - static_cast<float>(envTime) / std::max(1, envLength[2]);
        } else {
            envVolume = 0.0f;
        }

        // 波形生成
        phase += 1.0 / period;
        int p = static_cast<int>(phase);
        double sample = 0.0;

        switch (params.waveType) {
            case SfxrWaveType::Square:
                sample = (phase - p < squareDuty) ? 0.5 : -0.5;
                break;
            case SfxrWaveType::Sawtooth:
                sample = 1.0 - (phase - p) * 2.0;
                break;
            case SfxrWaveType::Sine:
                sample = sin((phase - p) * 2.0 * 3.141592653589793);
                break;
            case SfxrWaveType::Noise:
                sample = noise[(i / 32) % 32];
                break;
        }

        // 音量とADSRエンベロープの適用
        sample *= envVolume * params.masterVolume;
        sample = std::clamp(sample, -1.0, 1.0);

        pcm[i] = static_cast<int16_t>(sample * 32000.0);
    }

    return pcm;
}

} // namespace Audio
