#include "../Application/UseCases/Localization.h"
#include "../Infrastructure/Repositories/SettingsRepository.h"
#include "../Engine/Graphics/Renderer.h"
#include "../Engine/Graphics/Utf8Text.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <set>
#include <vector>
#include <windows.h>

/** @brief 書式指定子を出現順に取得する @param text 書式文字列 @return 指定子の一覧 */
static std::vector<std::string> Formats(const char* text) {
    const std::string value(text);
    const std::regex format("%([-+ #0]*[0-9]*(\\.[0-9]+)?[a-zA-Z]|%)");
    std::vector<std::string> result;
    for (auto match = std::sregex_iterator(value.begin(), value.end(), format);
         match != std::sregex_iterator(); ++match) result.push_back(match->str());
    return result;
}

/** @brief 翻訳とUnicode処理と永続化を検査する @return 成功時0 */
int main() {
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
    // 全言語のUTF-8、同梱字形、書式指定子、重複キーを検査する
    std::ifstream fontFile("Resources/UnicodeFont.bin", std::ios::binary);
    const std::vector<unsigned char> font((std::istreambuf_iterator<char>(fontFile)), {});
    assert(font.size() == 65536 * 32);
    std::set<std::string_view> keys;
    for (const auto& entry : Localization::Entries()) {
        assert(keys.insert(entry.source).second);
        const auto formats = Formats(entry.source);
        for (int index = 0; index < 6; ++index) {
            const char* value = entry.translated[index];
            assert(value && value[0]);
            assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, nullptr, 0) > 0);
            assert(Formats(value) == formats);
            assert(std::string_view(value).size() < RenderCommand::TextCapacity);
            for (std::size_t offset = 0; offset < std::string_view(value).size();) {
                const auto cp = Utf8Text::Next(value, offset);
                assert(cp < 65536);
                if (cp <= ' ') continue;
                const auto glyph = font.begin() + cp * 32;
                assert(std::any_of(glyph, glyph + 32, [](unsigned char pixel) { return pixel != 0; }));
            }
            Localization::SetLanguage(static_cast<Localization::Language>(index + 1));
            assert(Localization::Translate(entry.source) == value);
        }
    }
    assert(Localization::Entries().size() > 300);

    // 英語と未知キーは原文を保ち、不正な言語値は英語へ戻す
    Localization::SetLanguage(Localization::Language::English);
    assert(Localization::Translate("OPTIONS") == "OPTIONS");
    Localization::SetLanguage(static_cast<Localization::Language>(99));
    assert(Localization::GetLanguage() == Localization::Language::English);
    assert(Localization::Translate("123456") == "123456");

    // 各言語の会話が文字境界で二行に分かれ、文字を失わないことを検査する
    for (const auto& entry : Localization::Entries()) for (const char* value : entry.translated) {
        const std::string_view text(value);
        const auto end = Utf8Text::WrapLineEnd(text, 1.55f, 0.016f, 0.0015f);
        assert(end > 0 && end <= text.size());
        assert(Utf8Text::PrefixLength(text, end) == end);
        if (text.find('\n') == std::string_view::npos)
            assert(Utf8Text::Measure(text.substr(0, end), 0.016f, 0.0015f).width <= 1.551f);
    }

    // 実際の共通設定形式を保ちながら、別ファイルへ言語を保存する
    const auto testRoot = std::filesystem::temp_directory_path() /
        (L"SpaceYakuzaLocalization-" + std::to_wstring(GetCurrentProcessId()));
    SetEnvironmentVariableW(L"LOCALAPPDATA", testRoot.c_str());
    SettingsRepository repository;
    assert(repository.LoadLanguage() == Localization::DetectSystemLanguage());
    repository.Save({0.2f, 0.3f, 0.4f});
    const auto readSettings = [&]() {
        std::ifstream input(testRoot / "SpaceYakuza/settings.dat");
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    const std::string before = readSettings();
    for (int index = 0; index < static_cast<int>(Localization::Language::Count); ++index) {
        const auto language = static_cast<Localization::Language>(index);
        repository.SaveLanguage(language);
        assert(repository.LoadLanguage() == language);
        assert(readSettings() == before);
        repository.Save(repository.Load());
        assert(repository.LoadLanguage() == language);
    }
    for (const char* invalid : {"-1", "999", "broken", ""}) {
        std::ofstream(testRoot / "SpaceYakuza/language.dat", std::ios::trunc) << invalid;
        assert(repository.LoadLanguage() == Localization::DetectSystemLanguage());
    }
    // 一時データはこのプロセス固有のディレクトリ内だけを削除する
    assert(std::filesystem::equivalent(testRoot.parent_path(), std::filesystem::temp_directory_path()));
    std::filesystem::remove_all(testRoot);
    std::cout << Localization::Entries().size() << " entries in all six languages passed\n";
#else
    // Floppyでは翻訳関数と文字領域が従来の値を返す
    assert(Localization::Text("OPTIONS") == std::string_view("OPTIONS"));
    static_assert(Localization::BufferSize(24) == 24);
    static_assert(sizeof(RenderCommand{}.text) == 128);
    std::cout << "Floppy localization exclusion passed\n";
#endif
}
