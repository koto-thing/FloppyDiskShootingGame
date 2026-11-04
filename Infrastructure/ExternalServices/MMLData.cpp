#if defined(SPACEYAKUZA_PORTABLE)
#include "MMLData.h"
#include "MMLSource.h"

/** @brief 移植版の埋め込み原譜を参照する @param name 曲名 @return 対応する原譜、不明時は空 */
std::string_view MMLData::GetByName(std::string_view name) {
    // 容量制限のない版ではWindows専用圧縮へ依存せず原譜を共有する
    if (name == "stage1") return MMLSource::stage1;
    if (name == "stage2") return MMLSource::stage2;
    if (name == "stage3") return MMLSource::stage3;
    if (name == "stage4") return MMLSource::stage4;
    if (name == "stage5") return MMLSource::stage5;
    if (name == "stage6") return MMLSource::stage6;
    if (name == "boss1") return MMLSource::boss1;
    if (name == "boss2") return MMLSource::boss2;
    if (name == "boss3") return MMLSource::boss3;
    if (name == "boss4") return MMLSource::boss4;
    if (name == "boss5") return MMLSource::boss5;
    if (name == "boss6") return MMLSource::boss6;
    if (name == "title") return MMLSource::title;
    if (name == "ending") return MMLSource::ending;
    return {};
}
#else
#include "MMLData.h"
#include "PackedMMLData.h"
#include <windows.h>
#include <compressapi.h>
#include <string>

#pragma comment(lib, "cabinet.lib")

namespace MMLData {

/**
 * @brief 圧縮した原譜を一度だけ復元し、名前に対応するMMLを取得する
 * @param name MML名
 * @return 対応するMML、名前が不明または復元に失敗した場合は空文字列
 */
std::string_view GetByName(std::string_view name) {
    // Windows標準の復号器を使い、ゲーム本体に復号ライブラリを追加しない
    static const std::string text = [] {
        std::string output(Packed::OriginalSize, '\0');
        DECOMPRESSOR_HANDLE decoder = nullptr;
        if (!CreateDecompressor(COMPRESS_ALGORITHM_LZMS, nullptr, &decoder)) {
            OutputDebugStringA("MML decompressor creation failed\n");
            return std::string{};
        }
        SIZE_T written = 0;
        const BOOL success = Decompress(decoder, Packed::Data, sizeof(Packed::Data),
            output.data(), output.size(), &written);
        CloseDecompressor(decoder);
        if (!success || written != output.size()) {
            OutputDebugStringA("MML decompression failed\n");
            output.clear();
        }
        return output;
    }();

    // 復元バッファは保持し、呼び出し元へコピー不要の参照を返す
    for (const auto& entry : Packed::Entries) {
        if (name == entry.name && entry.offset <= text.size() && entry.size <= text.size() - entry.offset) {
            return std::string_view(text).substr(entry.offset, entry.size);
        }
    }
    return {};
}

} // namespace MMLData

#endif
