#include "../Infrastructure/Repositories/ScoreRepository.h"
#include "../Infrastructure/Repositories/SettingsRepository.h"

#include <cmath>
#include <filesystem>
#include <fstream>
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
        !std::isfinite(sanitized.seVolume) || sanitized.seVolume != 1.0f ||
        sanitized.galleryUnlocks != DefaultGalleryUnlocks ||
        (sanitized.galleryUnlocks &
            GalleryEntryBit(GalleryEntry::WallSecurityDrone)) == 0u) {
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

    const GameSettings expectedSettings {0.25f, 0.5f, 0.75f, false,
        GalleryEntryBit(GalleryEntry::Player) |
        GalleryEntryBit(GalleryEntry::Stage3Boss) |
        GalleryEntryBit(GalleryEntry::WallSecurityDrone) |
        GalleryEntryBit(GalleryEntry::NeoAizuBuildings)};
    SettingsRepository repository;
    repository.Save(expectedSettings);
    const GameSettings loadedSettings = repository.Load();

    // 新規解放だけを追記し、重複解放と既存設定の上書きを防ぐ
    const bool unlocked = repository.UnlockGalleryEntry(GalleryEntry::LightEnemy);
    const bool duplicateUnlock = repository.UnlockGalleryEntry(GalleryEntry::LightEnemy);
    const GameSettings unlockedSettings = repository.Load();

    // 既存の1人用ファイル形式を読み取り、協力プレイの保存先から隔離する
    const std::filesystem::path soloPath = testRoot / L"SpaceYakuza" / L"rankings.dat";
    std::ofstream legacyScores(soloPath);
    for (int difficulty = 0; difficulty < ScoreRepository::DifficultyCount; ++difficulty) {
        for (int rank = 0; rank < ScoreRepository::RankCount; ++rank) {
            legacyScores << (difficulty == static_cast<int>(Normal) ? scores[rank] : 0) << '\n';
        }
    }
    legacyScores.close();
    const ScoreRepository scoreRepository;
    const ScoreRepository::Rankings legacyRankings = scoreRepository.Load();
    const ScoreRepository::Rankings emptyCoopRankings = scoreRepository.Load(true);
    scoreRepository.Save(Easy, 1200);
    scoreRepository.Save(Easy, 9000, true);
    scoreRepository.Save(Hard, 4200, true);
    scoreRepository.Save(Easy, 6000, true);
    scoreRepository.Save(Normal, 8000);
    const ScoreRepository::Rankings soloRankings = scoreRepository.Load();
    const ScoreRepository::Rankings coopRankings = scoreRepository.Load(true);
    const bool separateRankingFiles = std::filesystem::exists(soloPath) &&
        std::filesystem::exists(testRoot / L"SpaceYakuza" / L"rankings-coop.dat");

    SetEnvironmentVariableW(L"LOCALAPPDATA",
        originalLength > 0 && originalLength < std::size(originalLocalAppData)
            ? originalLocalAppData : nullptr);
    std::filesystem::remove_all(testRoot, error);
    ScoreRepository::Rankings expectedSoloRankings {};
    expectedSoloRankings[Easy][0] = 1200;
    expectedSoloRankings[Normal] = {{ 9000, 8000, 7000, 5000, 3000 }};
    ScoreRepository::Rankings expectedCoopRankings {};
    expectedCoopRankings[Easy] = {{ 9000, 6000, 0, 0, 0 }};
    expectedCoopRankings[Hard][0] = 4200;
    if (legacyRankings[Normal] != scores || emptyCoopRankings != ScoreRepository::Rankings {} ||
        soloRankings != expectedSoloRankings || coopRankings != expectedCoopRankings || !separateRankingFiles) {
        throw std::runtime_error("Solo rankings must retain legacy scores and remain separate from co-op rankings");
    }
    if (loadedSettings.masterVolume != expectedSettings.masterVolume ||
        loadedSettings.bgmVolume != expectedSettings.bgmVolume ||
        loadedSettings.seVolume != expectedSettings.seVolume ||
        loadedSettings.retroEffectEnabled != expectedSettings.retroEffectEnabled ||
        loadedSettings.galleryUnlocks != expectedSettings.galleryUnlocks) {
        throw std::runtime_error("Settings must survive a save and load round trip");
    }
    if (!unlocked || duplicateUnlock ||
        unlockedSettings.masterVolume != expectedSettings.masterVolume ||
        (unlockedSettings.galleryUnlocks & GalleryEntryBit(GalleryEntry::LightEnemy)) == 0u) {
        throw std::runtime_error("Gallery unlocks must persist without replacing settings");
    }
}
