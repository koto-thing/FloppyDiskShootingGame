#if defined(SPACEYAKUZA_PORTABLE)
#include "PackedAudioSamples.h"
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
        // 新SFC音源化に伴い楽器音源は除外され、ボイス音源を展開する
        if (!Unpack(VoicesData, sizeof(VoicesData), output.data() + (TotalSize - VoicesSize), VoicesSize)) {
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
    std::vector<std::uint8_t> output(VoicesSize + 1);
    const std::uint8_t invalid[] = {0, 0, 0, 0};
    assert(!Unpack(invalid, sizeof(invalid), output.data(), VoicesSize));
    assert(!Unpack(VoicesData, sizeof(VoicesData) - 1, output.data(), VoicesSize));
    assert(!Unpack(VoicesData, sizeof(VoicesData), output.data(), VoicesSize - 1));
    assert(!Unpack(VoicesData, sizeof(VoicesData), output.data(), VoicesSize + 1));
}
#endif
} // namespace Audio::PackedAudioSamples

#endif
