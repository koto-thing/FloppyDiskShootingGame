#if defined(SPACEYAKUZA_PORTABLE)
#include "PackedAudioSamples.h"
#include "WavSamplesSource.h"
#include "../../Presentation/Gameplay/Voices/VoiceSamplesSource.h"
#include <array>
#include <algorithm>

/** @brief 原音源を同じオフセット配置で参照する @param offset バイト位置 @param size 長さ @return 範囲内のデータ、不正時nullptr */
const std::uint8_t* Audio::PackedAudioSamples::Get(std::size_t offset, std::size_t size) {
    if (size == 0 || offset > TotalSize || size > TotalSize - offset) return nullptr;
    // パック済み版と同じ索引を使い、Windows圧縮APIだけを取り除く
    static const auto samples = [] {
        std::array<std::uint8_t, TotalSize> output{};
#define COPY_SAMPLE(source, entry) static_assert(sizeof(source) == entry.size); std::copy_n(source, entry.size, output.data() + entry.offset)
        COPY_SAMPLE(WavSamplesSource::sample_AcousticGrandPiano, sample_AcousticGrandPiano);
        COPY_SAMPLE(WavSamplesSource::sample_Agogo, sample_Agogo);
        COPY_SAMPLE(WavSamplesSource::sample_BlownBottle, sample_BlownBottle);
        COPY_SAMPLE(WavSamplesSource::sample_BrassSection, sample_BrassSection);
        COPY_SAMPLE(WavSamplesSource::sample_ChurchOrgan, sample_ChurchOrgan);
        COPY_SAMPLE(WavSamplesSource::sample_Clavi, sample_Clavi);
        COPY_SAMPLE(WavSamplesSource::sample_Crash, sample_Crash);
        COPY_SAMPLE(WavSamplesSource::sample_DistortionGuitar, sample_DistortionGuitar);
        COPY_SAMPLE(WavSamplesSource::sample_DrawbarOrgan, sample_DrawbarOrgan);
        COPY_SAMPLE(WavSamplesSource::sample_Dulcimer, sample_Dulcimer);
        COPY_SAMPLE(WavSamplesSource::sample_ElectricPiano1, sample_ElectricPiano1);
        COPY_SAMPLE(WavSamplesSource::sample_FrenchHorn, sample_FrenchHorn);
        COPY_SAMPLE(WavSamplesSource::sample_GuitarFretNoise, sample_GuitarFretNoise);
        COPY_SAMPLE(WavSamplesSource::sample_Harpsichord, sample_Harpsichord);
        COPY_SAMPLE(WavSamplesSource::sample_HiHatOpen, sample_HiHatOpen);
        COPY_SAMPLE(WavSamplesSource::sample_Kick, sample_Kick);
        COPY_SAMPLE(WavSamplesSource::sample_LeadCalliope, sample_LeadCalliope);
        COPY_SAMPLE(WavSamplesSource::sample_LeadCharang, sample_LeadCharang);
        COPY_SAMPLE(WavSamplesSource::sample_LeadFifths, sample_LeadFifths);
        COPY_SAMPLE(WavSamplesSource::sample_LeadSawtooth, sample_LeadSawtooth);
        COPY_SAMPLE(WavSamplesSource::sample_LeadSquare, sample_LeadSquare);
        COPY_SAMPLE(WavSamplesSource::sample_MelodicTom, sample_MelodicTom);
        COPY_SAMPLE(WavSamplesSource::sample_MusicBox, sample_MusicBox);
        COPY_SAMPLE(WavSamplesSource::sample_NylonAcousticGuitar, sample_NylonAcousticGuitar);
        COPY_SAMPLE(WavSamplesSource::sample_Ocarina, sample_Ocarina);
        COPY_SAMPLE(WavSamplesSource::sample_OverdrivenGuitar, sample_OverdrivenGuitar);
        COPY_SAMPLE(WavSamplesSource::sample_RockOrgan, sample_RockOrgan);
        COPY_SAMPLE(WavSamplesSource::sample_Snare, sample_Snare);
        COPY_SAMPLE(WavSamplesSource::sample_StringEnsemble1, sample_StringEnsemble1);
        COPY_SAMPLE(WavSamplesSource::sample_SynthDrum, sample_SynthDrum);
        COPY_SAMPLE(WavSamplesSource::sample_Tambourine, sample_Tambourine);
        COPY_SAMPLE(WavSamplesSource::sample_TomHigh, sample_TomHigh);
        COPY_SAMPLE(WavSamplesSource::sample_TomLow, sample_TomLow);
        COPY_SAMPLE(WavSamplesSource::sample_TomMid, sample_TomMid);
        COPY_SAMPLE(WavSamplesSource::sample_Trumpet, sample_Trumpet);
        COPY_SAMPLE(WavSamplesSource::sample_Woodblock, sample_Woodblock);
        COPY_SAMPLE(WavSamplesSource::sample_ChoirAahs, sample_ChoirAahs);
        COPY_SAMPLE(WavSamplesSource::sample_Oboe, sample_Oboe);
        COPY_SAMPLE(WavSamplesSource::sample_SynthBass1, sample_SynthBass1);
        COPY_SAMPLE(WavSamplesSource::sample_Vibraphone, sample_Vibraphone);
        COPY_SAMPLE(VoiceSamplesSource::tayamaDeathData, tayamaDeathData);
        COPY_SAMPLE(VoiceSamplesSource::eastsourceDeathData, eastsourceDeathData);
        COPY_SAMPLE(VoiceSamplesSource::momijiDeathData, momijiDeathData);
        COPY_SAMPLE(VoiceSamplesSource::missionData, missionData);
        COPY_SAMPLE(VoiceSamplesSource::startData, startData);
        COPY_SAMPLE(VoiceSamplesSource::arrestedData, arrestedData);
        COPY_SAMPLE(VoiceSamplesSource::suspectData, suspectData);
        COPY_SAMPLE(VoiceSamplesSource::kotoDeathData, kotoDeathData);
        COPY_SAMPLE(VoiceSamplesSource::ryotaDeathData, ryotaDeathData);
        COPY_SAMPLE(VoiceSamplesSource::lumiDeathData, lumiDeathData);
        COPY_SAMPLE(VoiceSamplesSource::botamochiDeathBotaData, botamochiDeathBotaData);
        COPY_SAMPLE(VoiceSamplesSource::botamochiDeathMochiData, botamochiDeathMochiData);
        COPY_SAMPLE(VoiceSamplesSource::momijiDeathRattle2Data, momijiDeathRattle2Data);
        COPY_SAMPLE(VoiceSamplesSource::momijiDeathRattle5Data, momijiDeathRattle5Data);
#undef COPY_SAMPLE
        return output;
    }();
    return samples.data() + offset;
}
#else
#define FLOPPY_AUDIO_IMPLEMENTATION
#include "PackedAudioSamples.h"
#include <windows.h>
#include <compressapi.h>
#include <vector>
#ifdef FLOPPY_AUDIO_TESTS
#include <cassert>
#endif

#pragma comment(lib, "cabinet.lib")

namespace Audio::PackedAudioSamples {
namespace {
/**
 * @brief Windows標準のMSZIP復号器で指定バッファへ復元する
 * @param data 圧縮データ
 * @param size 圧縮バイト数
 * @param output 復元先
 * @param outputSize 必要な復元バイト数
 * @return 指定サイズへ完全に復元した場合true
 */
bool Unpack(const std::uint8_t* data, std::size_t size,
    std::uint8_t* output, std::size_t outputSize) {
    // OSの復号器を使い、復元サイズも検証してハンドルを解放する
    DECOMPRESSOR_HANDLE decoder = nullptr;
    if (!CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, nullptr, &decoder)) return false;
    SIZE_T written = 0;
    const BOOL success = Decompress(decoder, data, size, output, outputSize, &written);
    CloseDecompressor(decoder);
    return success && written == outputSize;
}
}

/**
 * @brief 全楽器・声を一度だけ復元し、指定音源のバイト列を参照する
 * @param offset 全音源バッファ内の位置
 * @param size 参照するバイト数
 * @return 復元データ、失敗または範囲外の場合nullptr
 */
const std::uint8_t* Get(std::size_t offset, std::size_t size) {
    // サイズ加算のオーバーフローを避け、範囲外を復元前に拒否する
    if (size == 0 || offset > TotalSize || size > TotalSize - offset) return nullptr;
    static const std::vector<std::uint8_t> samples = [] {
        std::vector<std::uint8_t> output(TotalSize);
        if (!Unpack(InstrumentsData, sizeof(InstrumentsData), output.data(), InstrumentsSize) ||
            !Unpack(VoicesData, sizeof(VoicesData), output.data() + InstrumentsSize, VoicesSize)) {
            OutputDebugStringA("Audio sample decompression failed\n");
            output.clear();
        }
        return output;
    }();

    // バッファの寿命を保ち、以降の利用では復号やコピーを繰り返さない
    return samples.empty() ? nullptr : samples.data() + offset;
}
#ifdef FLOPPY_AUDIO_TESTS
/**
 * @brief 不正な圧縮データと復元サイズ不一致を拒否することを検証する
 * @return なし
 */
void RunFailureChecks() {
    // OSの復号器の失敗と、成功しても想定サイズと違う場合を確認する
    std::vector<std::uint8_t> output(InstrumentsSize + 1);
    const std::uint8_t invalid[] = {0, 0, 0, 0};
    assert(!Unpack(invalid, sizeof(invalid), output.data(), InstrumentsSize));
    assert(!Unpack(InstrumentsData, sizeof(InstrumentsData) - 1, output.data(), InstrumentsSize));
    assert(!Unpack(InstrumentsData, sizeof(InstrumentsData), output.data(), InstrumentsSize - 1));
    assert(!Unpack(InstrumentsData, sizeof(InstrumentsData), output.data(), InstrumentsSize + 1));
}
#endif
} // namespace Audio::PackedAudioSamples

#endif
