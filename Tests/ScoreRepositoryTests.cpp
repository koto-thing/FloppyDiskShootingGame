#include "../Infrastructure/Repositories/ScoreRepository.h"
#include "../Infrastructure/Repositories/SettingsRepository.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <windows.h>

/**
 * @brief ランキングへのスコア挿入が上位5件と降順を維持することを検証する
 * @return なし
 */
void RunScoreRepositoryTests() {
    const ScoreRepository::Scores scores {{ 9000, 7000, 5000, 3000, 1000 }};
    const ScoreRepository::Scores inserted = ScoreRepository::InsertScore(scores, 6000);
    const ScoreRepository::Scores expected {{ 9000, 7000, 6000, 5000, 3000 }};
    if (inserted != expected) {
        throw std::runtime_error("Ranking must retain the highest five scores in descending order");
    }

    // 保存データ由来の不正な音量が有効範囲へ補正されることを検証する
    const GameSettings sanitized = SettingsRepository::Sanitize({
        -0.5f,
        1.5f,
        (std::numeric_limits<float>::quiet_NaN)()
    });
    if (sanitized.masterVolume != 0.0f ||
        sanitized.bgmVolume != 1.0f ||
        !std::isfinite(sanitized.seVolume) || sanitized.seVolume != 1.0f) {
        throw std::runtime_error("Settings volumes must remain finite and in range");
    }

    // 一時的なユーザーデータ領域で設定の保存と再読込を検証する
    wchar_t originalLocalAppData[32767] {};
    const DWORD originalLength = GetEnvironmentVariableW(
        L"LOCALAPPDATA", originalLocalAppData, static_cast<DWORD>(std::size(originalLocalAppData)));
    const std::filesystem::path testRoot = std::filesystem::temp_directory_path() /
        (L"FloppyDiskShootingGameSettingsTests-" + std::to_wstring(GetCurrentProcessId()));
    std::error_code error;
    std::filesystem::remove_all(testRoot, error);
    SetEnvironmentVariableW(L"LOCALAPPDATA", testRoot.c_str());

    const GameSettings expectedSettings { 0.25f, 0.5f, 0.75f, false };
    SettingsRepository().Save(expectedSettings);
    const GameSettings loadedSettings = SettingsRepository().Load();

    SetEnvironmentVariableW(L"LOCALAPPDATA",
        originalLength > 0 && originalLength < std::size(originalLocalAppData)
            ? originalLocalAppData : nullptr);
    std::filesystem::remove_all(testRoot, error);
    if (loadedSettings.masterVolume != expectedSettings.masterVolume ||
        loadedSettings.bgmVolume != expectedSettings.bgmVolume ||
        loadedSettings.seVolume != expectedSettings.seVolume ||
        loadedSettings.retroEffectEnabled != expectedSettings.retroEffectEnabled) {
        throw std::runtime_error("Settings must survive a save and load round trip");
    }
}
