#pragma once

#include <string>
#include <string_view>
#include <cstddef>
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
#include <span>
#endif

namespace Localization {
enum class Language {
    English, Japanese, Spanish, Korean, ChineseSimplified, French, PortugueseBrazil, Count
};

#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
/** @brief 原文と追加6言語の翻訳 */
struct Entry { const char* source; const char* translated[6]; };

/** @brief 使用中の表示言語を取得する @return 表示言語 */
Language GetLanguage();
/** @brief 表示言語を変更する @param language 表示言語 @return なし */
void SetLanguage(Language language);
/** @brief 原文の翻訳を取得する @param source 英語の原文 @return 翻訳または原文 */
const char* Text(const char* source);
/** @brief 描画する文字列を翻訳する @param source 英語の原文 @return 翻訳または原文 */
std::string Translate(std::string_view source);
/** @brief 言語の自称を取得する @param language 言語 @return 言語名 */
const char* LanguageName(Language language);
/** @brief Windowsの表示言語から初期言語を選ぶ @return 対応言語、未対応なら英語 */
Language DetectSystemLanguage();
/** @brief 検証用の翻訳カタログを取得する @return カタログ */
std::span<const Entry> Entries();
/** @brief 翻訳後の文字列用の領域を確保する @param original 元の必要容量 @return 翻訳に必要な容量 */
constexpr std::size_t BufferSize(std::size_t original) { return original < 512 ? 512 : original; }
#else
/** @brief Floppy版では原文をそのまま返す @param source 原文 @return 原文 */
inline const char* Text(const char* source) { return source; }
/** @brief Floppy版では原文をそのまま返す @param source 原文 @return 原文 */
inline std::string Translate(std::string_view source) { return std::string(source); }
/** @brief Floppy版の文字列領域を維持する @param original 元の必要容量 @return 元の必要容量 */
constexpr std::size_t BufferSize(std::size_t original) { return original; }
#endif
}
