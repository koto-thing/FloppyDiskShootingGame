#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

/** @brief UTF-8文字列のデコードとビットマップ文字の共通計測 */
namespace Utf8Text {
/**
 * @brief 次のUnicode文字を読み、不正な入力は置換文字へ変換する
 * @param text UTF-8文字列
 * @param offset 読み取り位置、読み取ったバイト数だけ進む
 * @return Unicodeコードポイント、入力末尾では0
 */
inline std::uint32_t Next(std::string_view text, std::size_t& offset) {
    if (offset >= text.size()) return 0;
    const auto lead = static_cast<unsigned char>(text[offset++]);
    if (lead < 0x80) return lead;

    // 過長形式、サロゲート、Unicode範囲外も受け入れない
    const unsigned count = lead >= 0xc2 && lead <= 0xdf ? 1 :
        lead >= 0xe0 && lead <= 0xef ? 2 : lead >= 0xf0 && lead <= 0xf4 ? 3 : 0;
    if (count == 0 || text.size() - offset < count) return 0xfffd;
    std::uint32_t codepoint = lead & (0x7f >> count);
    for (unsigned index = 0; index < count; ++index) {
        const auto byte = static_cast<unsigned char>(text[offset + index]);
        if ((byte & 0xc0) != 0x80) return 0xfffd;
        codepoint = (codepoint << 6) | (byte & 0x3f);
    }
    if ((count == 1 && codepoint < 0x80) || (count == 2 && codepoint < 0x800) ||
        (count == 3 && codepoint < 0x10000) || codepoint > 0x10ffff ||
        (codepoint >= 0xd800 && codepoint <= 0xdfff)) return 0xfffd;
    offset += count;
    return codepoint;
}

/**
 * @brief UTF-8文字の途中を避けてバイト上限以内の接頭辞を選ぶ
 * @param text UTF-8文字列
 * @param byteLimit 最大バイト数
 * @return 文字境界までのバイト数
 */
inline std::size_t PrefixLength(std::string_view text, std::size_t byteLimit) {
    std::size_t end = 0;
    std::size_t offset = 0;
    while (offset < text.size()) {
        Next(text, offset);
        if (offset > byteLimit) break;
        end = offset;
    }
    return end;
}

/**
 * @brief 日本語、中国語、韓国語などの全角文字を判定する
 * @param codepoint Unicodeコードポイント
 * @return 全角の送り幅を使う場合true
 */
inline bool IsWide(std::uint32_t codepoint) {
    return (codepoint >= 0x1100 && codepoint <= 0x11ff) ||
        (codepoint >= 0x2e80 && codepoint <= 0xa4cf) ||
        (codepoint >= 0xac00 && codepoint <= 0xd7af) ||
        (codepoint >= 0xf900 && codepoint <= 0xfaff) ||
        (codepoint >= 0xfe10 && codepoint <= 0xfe6f) ||
        (codepoint >= 0xff01 && codepoint <= 0xff60);
}

/**
 * @brief 1文字を描画した後の水平移動量を求める
 * @param codepoint Unicodeコードポイント
 * @param size 文字の半サイズ
 * @param spacing 追加の字間
 * @return NDC座標の水平移動量
 */
inline float Advance(std::uint32_t codepoint, float size, float spacing) {
    return size * (IsWide(codepoint) ? 2.1f : 1.5f) + spacing;
}

/** @brief 文字列全体の描画領域と先頭文字の中心位置 */
struct Metrics {
    float width = 0.0f;
    float height = 0.0f;
    float firstGlyphOffset = 0.0f;
    std::size_t lineCount = 1;
};

/**
 * @brief 描画と同じ全角文字幅で文字列を計測する
 * @param text UTF-8文字列
 * @param size 文字の半サイズ
 * @param spacing 追加の字間
 * @param aspect 画面の横幅と高さの比率
 * @return 改行を含む描画領域
 */
inline Metrics Measure(std::string_view text, float size, float spacing, float aspect = 1.0f) {
    Metrics metrics;
    metrics.firstGlyphOffset = size;
    float advance = 0.0f;
    float lineWidth = 0.0f;
    for (std::size_t offset = 0; offset < text.size();) {
        const auto codepoint = Next(text, offset);
        if (codepoint == '\n') {
            metrics.width = (std::max)(metrics.width, lineWidth);
            advance = lineWidth = 0.0f;
            ++metrics.lineCount;
        } else {
            lineWidth = advance + size * 2.0f;
            advance += Advance(codepoint, size, spacing);
        }
    }
    metrics.width = (std::max)(metrics.width, lineWidth);
    metrics.height = size * aspect * (2.0f + static_cast<float>(metrics.lineCount - 1) * 2.4f);
    return metrics;
}

/**
 * @brief 指定幅に収まる先頭行を単語またはUnicode文字境界で切り出す
 * @param text UTF-8文字列
 * @param maxWidth 行の最大幅
 * @param size 文字の半サイズ
 * @param spacing 追加の字間
 * @param aspect 画面の縦横比、横幅の計算には影響しない
 * @return 先頭行末尾のバイト位置、非空文字列は最小1文字を返す
 */
inline std::size_t WrapLineEnd(std::string_view text, float maxWidth, float size, float spacing,
                               float aspect = 1.0f) {
    (void)aspect;
    float advance = 0.0f;
    std::size_t lastSpace = 0;
    for (std::size_t offset = 0; offset < text.size();) {
        const auto start = offset;
        const auto codepoint = Next(text, offset);
        if (codepoint == '\n') return start == 0 ? offset : start;
        if (advance + size * 2.0f > maxWidth) return lastSpace > 0 ? lastSpace : start > 0 ? start : offset;
        if (codepoint == ' ') lastSpace = start;
        advance += Advance(codepoint, size, spacing);
    }
    return text.size();
}
}
