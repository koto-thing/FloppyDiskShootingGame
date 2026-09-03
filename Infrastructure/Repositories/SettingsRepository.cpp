#include "SettingsRepository.h"

#include "UserDataPath.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <windows.h>

namespace {
/**
 * @brief 設定ファイルの保存先を取得する
 * @return 設定ファイルパス
 */
std::filesystem::path SettingsPath() {
    return UserDataPath() / L"settings.dat";
}

/**
 * @brief 音量を有効範囲へ補正する
 * @param volume 補正前の音量
 * @return 0.0から1.0へ補正した音量、非数値時は1.0
 */
float SanitizeVolume(float volume) {
    return std::isfinite(volume) ? (std::clamp)(volume, 0.0f, 1.0f) : 1.0f;
}
}

/**
 * @brief 保存済み設定を取得する
 * @return 保存済み設定、未保存または破損時は既定値
 */
GameSettings SettingsRepository::Load() const {
    GameSettings settings;
    std::ifstream input(SettingsPath());
    if (!(input >> settings.masterVolume >> settings.bgmVolume >> settings.seVolume)) {
        return {};
    }
    // 旧形式の音量3行だけの設定は、映像効果ONとして読み込む
    int retroEffectEnabled = 1;
    if (input >> retroEffectEnabled) settings.retroEffectEnabled = retroEffectEnabled != 0;
    // 旧形式ではプレイヤー展示だけを初期解放する
    input >> settings.galleryUnlocks;
    int tutorialCompleted = 0;
    if (input >> tutorialCompleted) settings.tutorialCompleted = tutorialCompleted != 0;
    return Sanitize(settings);
}

/**
 * @brief ゲーム設定を保存する
 * @param settings 保存する設定
 */
void SettingsRepository::Save(const GameSettings& settings) const {
    const std::filesystem::path path = SettingsPath();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return;

    const std::filesystem::path temporaryPath = path.parent_path() / L"settings.tmp";
    const GameSettings sanitized = Sanitize(settings);
    std::ofstream output(temporaryPath, std::ios::trunc);
    if (!output) return;
    output << sanitized.masterVolume << '\n'
           << sanitized.bgmVolume << '\n'
           << sanitized.seVolume << '\n'
           << (sanitized.retroEffectEnabled ? 1 : 0) << '\n'
           << sanitized.galleryUnlocks << '\n'
           << (sanitized.tutorialCompleted ? 1 : 0) << '\n';
    output.close();
    if (!output) return;

    // 一時ファイルを置き換えて、書き込み途中のデータ破損を防ぐ
    if (std::filesystem::exists(path, error)) {
        ReplaceFileW(path.c_str(), temporaryPath.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
    } else {
        MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
    }
}

/**
 * @brief 展示を解放して既存設定とともに保存する
 * @param entry 解放する展示
 * @return この呼び出しで初めて解放された場合true
 */
bool SettingsRepository::UnlockGalleryEntry(GalleryEntry entry) const {
    const std::uint32_t bit = GalleryEntryBit(entry);
    GameSettings settings = Load();
    if ((settings.galleryUnlocks & bit) != 0u) return false;

    // 新しい解放だけを書き込み、既存の音量と映像設定を維持する
    settings.galleryUnlocks |= bit;
    Save(settings);
    return true;
}

/**
 * @brief 設定値を有効範囲へ補正する
 * @param settings 補正前の設定
 * @return 補正後の設定
 */
GameSettings SettingsRepository::Sanitize(GameSettings settings) {
    settings.masterVolume = SanitizeVolume(settings.masterVolume);
    settings.bgmVolume = SanitizeVolume(settings.bgmVolume);
    settings.seVolume = SanitizeVolume(settings.seVolume);
    settings.galleryUnlocks = (settings.galleryUnlocks & ValidGalleryUnlocks) | DefaultGalleryUnlocks;
    return settings;
}
