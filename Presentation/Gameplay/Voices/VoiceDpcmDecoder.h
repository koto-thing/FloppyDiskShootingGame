#pragma once

#include "VoiceSamples.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace VoiceCodec {

/**
 * @brief IMA ADPCMデータをPCMデータへ復号する
 * @param sample 復号するIMA ADPCMサンプル
 * @return 復号した16bit PCMデータ
 */
inline std::vector<std::int16_t> DecodeImaAdpcm(
    const VoiceSamples::ImaAdpcmSample& sample)
{
    static constexpr std::array<int, 89> StepTable = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
        143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
        494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
        4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
        10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
        24623, 27086, 29794, 32767
    };
    static constexpr std::array<int, 16> IndexTable = {
        -1, -1, -1, -1, 2, 4, 6, 8,
        -1, -1, -1, -1, 2, 4, 6, 8
    };

    std::vector<std::int16_t> output;
    if (sample.sampleCount == 0) return output;
    // 可逆圧縮の復元失敗や不足したADPCMデータを再生前に拒否する
    const auto* data = Audio::PackedAudioSamples::Get(sample.offset, sample.byteCount);
    if (!data || sample.sampleCount / 2 > sample.byteCount) return output;
    output.reserve(sample.sampleCount);

    int predictor = sample.initialPredictor;
    int stepIndex = std::clamp<int>(sample.initialStepIndex, 0, 88);
    output.push_back(static_cast<std::int16_t>(predictor));

    for (std::size_t i = 0; i < sample.byteCount && output.size() < sample.sampleCount; ++i) {
        const std::uint8_t packed = data[i];
        for (int shift : {0, 4}) {
            if (output.size() >= sample.sampleCount) break;
            const int code = (packed >> shift) & 0x0F;
            const int step = StepTable[stepIndex];
            int delta = step >> 3;
            if (code & 4) delta += step;
            if (code & 2) delta += step >> 1;
            if (code & 1) delta += step >> 2;
            predictor += (code & 8) ? -delta : delta;
            predictor = std::clamp(predictor, -32768, 32767);
            stepIndex = std::clamp(stepIndex + IndexTable[code], 0, 88);
            output.push_back(static_cast<std::int16_t>(predictor));
        }
    }
    return output;
}

/**
 * @brief PCMデータを4点の三次補間でリサンプリングする
 * @param input 入力PCMデータ
 * @param sourceRate 入力サンプルレート
 * @param destinationRate 出力サンプルレート
 * @return リサンプリングした16bit PCMデータ
 */
inline std::vector<std::int16_t> ResampleCubic(
    const std::vector<std::int16_t>& input,
    std::uint32_t sourceRate,
    std::uint32_t destinationRate)
{
    if (input.empty() || sourceRate == 0 || destinationRate == 0) return {};
    if (sourceRate == destinationRate) return input;

    const std::size_t outputCount = static_cast<std::size_t>(
        std::llround(static_cast<double>(input.size()) * destinationRate / sourceRate));
    std::vector<std::int16_t> output(outputCount);
    const double ratio = static_cast<double>(sourceRate) / destinationRate;

    for (std::size_t i = 0; i < outputCount; ++i) {
        const double sourcePosition = i * ratio;
        const std::size_t left = std::min<std::size_t>(
            static_cast<std::size_t>(sourcePosition), input.size() - 1);
        // 端では端点を延長し、Catmull-Rom補間で波形の傾きを滑らかにつなぐ
        const double p0 = input[left > 0 ? left - 1 : 0];
        const double p1 = input[left];
        const double p2 = input[std::min(left + 1, input.size() - 1)];
        const double p3 = input[std::min(left + 2, input.size() - 1)];
        const double fraction = sourcePosition - left;
        // ponytail: 三次補間は帯域制限フィルターではないため、縮小用途ではsinc方式へ変更する
        const double value = p1 + 0.5 * fraction * (p2 - p0 + fraction *
            (2 * p0 - 5 * p1 + 4 * p2 - p3 + fraction * (3 * (p1 - p2) + p3 - p0)));
        output[i] = static_cast<std::int16_t>(std::clamp(
            static_cast<int>(std::lround(value)), -32768, 32767));
    }
    return output;
}

/**
 * @brief AudioService向けの44100Hz PCMデータへ変換する
 * @param sample 変換するIMA ADPCMサンプル
 * @return 44100Hzへ変換した16bit PCMデータ
 */
inline std::vector<std::int16_t> DecodeForAudioService(
    const VoiceSamples::ImaAdpcmSample& sample)
{
    std::vector<std::int16_t> output = ResampleCubic(DecodeImaAdpcm(sample), sample.sampleRate, 44100);

    // 両端2msをフェードし、切り詰め済み音声を無音へ滑らかにつなぐ
    const std::size_t fade = std::min<std::size_t>(88, output.size() / 2);
    for (std::size_t i = 0; i < fade; ++i) {
        const float gain = static_cast<float>(i) / static_cast<float>(fade);
        output[i] = static_cast<std::int16_t>(std::lround(output[i] * gain));
        output[output.size() - 1 - i] = static_cast<std::int16_t>(
            std::lround(output[output.size() - 1 - i] * gain));
    }
    return output;
}

/**
 * @brief 音声へ無線風の帯域制限と粗さを加える
 * @param sample 変換するIMA ADPCM音声
 * @return AudioServiceで再生可能な44100Hz PCMデータ
 */
inline std::vector<std::int16_t> DecodeRadioForAudioService(
    const VoiceSamples::ImaAdpcmSample& sample)
{
    constexpr std::uint32_t OutputRate = 44100;
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float HighPassCutoff = 320.0f;
    constexpr float LowPassCutoff = 3200.0f;
    constexpr int QuantizationStep = 256;
    std::vector<std::int16_t> output = DecodeForAudioService(sample);

    // 通信音声の狭い帯域へ絞り、軽いクリップと量子化でざらつきを加える
    const float interval = 1.0f / static_cast<float>(OutputRate);
    const float highPassRc = 1.0f / (2.0f * Pi * HighPassCutoff);
    const float lowPassRc = 1.0f / (2.0f * Pi * LowPassCutoff);
    const float highPassAlpha = highPassRc / (highPassRc + interval);
    const float lowPassAlpha = interval / (lowPassRc + interval);
    float previousInput = 0.0f;
    float highPassed = 0.0f;
    float lowPassed = 0.0f;
    for (std::int16_t& value : output) {
        const float input = static_cast<float>(value);
        highPassed = highPassAlpha * (highPassed + input - previousInput);
        lowPassed += lowPassAlpha * (highPassed - lowPassed);
        previousInput = input;
        const int clipped = std::clamp(static_cast<int>(std::lround(lowPassed * 1.8f)), -16384, 16384);
        value = static_cast<std::int16_t>(clipped / QuantizationStep * QuantizationStep);
    }
    return output;
}

} // namespace VoiceCodec
