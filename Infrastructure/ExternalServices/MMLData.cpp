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
