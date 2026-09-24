#include "Localization.h"

#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
#include <Windows.h>
#include <algorithm>
#include <array>
#include <iterator>

namespace Localization {
namespace {
Language currentLanguage = Language::English;
constexpr Entry entries[] = {
#include "LocalizationUi.inc"
#include "LocalizationDynamic.inc"
#include "LocalizationStory.inc"
};

/** @brief 完全一致する翻訳を検索する @param source 原文 @return 翻訳、未登録ならnullptr */
const char* Lookup(std::string_view source) {
    // 初回だけ原文順の索引を作り、描画時は二分探索する
    static const auto sorted = [] {
        std::array<const Entry*, std::size(entries)> result {};
        for (std::size_t index = 0; index < result.size(); ++index) result[index] = &entries[index];
        std::sort(result.begin(), result.end(), [](const Entry* left, const Entry* right) {
            return std::string_view(left->source) < std::string_view(right->source);
        });
        return result;
    }();
    const auto match = std::lower_bound(sorted.begin(), sorted.end(), source,
        [](const Entry* entry, std::string_view text) { return std::string_view(entry->source) < text; });
    return match != sorted.end() && source == (*match)->source ?
        (*match)->translated[static_cast<int>(currentLanguage) - 1] : nullptr;
}
}

Language GetLanguage() { return currentLanguage; }

void SetLanguage(Language language) {
    // 永続化ファイルなどから範囲外の値が来た場合は英語へ戻す
    currentLanguage = language >= Language::English && language < Language::Count ? language : Language::English;
}

const char* Text(const char* source) {
    // 英語と空文字は検索せず、未登録の表示は原文を保つ
    if (!source || currentLanguage == Language::English) return source;
    const char* translated = Lookup(source);
    return translated ? translated : source;
}

std::string Translate(std::string_view source) {
    // 描画APIが所有する文字列にも同じ完全一致の翻訳を適用する
    const char* translated = currentLanguage == Language::English ? nullptr : Lookup(source);
    return translated ? std::string(translated) : std::string(source);
}

const char* LanguageName(Language language) {
    // 言語選択は現在の言語によらず各言語の自称を表示する
    static constexpr const char* names[] = {"English", "日本語", "Español", "한국어", "简体中文", "Français", "Português (Brasil)"};
    return names[language >= Language::English && language < Language::Count ? static_cast<int>(language) : 0];
}

Language DetectSystemLanguage() {
    // 地域の設定ではなくWindowsのUI言語を使用する
    switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
    case LANG_JAPANESE: return Language::Japanese;
    case LANG_SPANISH: return Language::Spanish;
    case LANG_KOREAN: return Language::Korean;
    case LANG_CHINESE: return Language::ChineseSimplified;
    case LANG_FRENCH: return Language::French;
    case LANG_PORTUGUESE: return Language::PortugueseBrazil;
    default: return Language::English;
    }
}

std::span<const Entry> Entries() { return entries; }
}
#endif
