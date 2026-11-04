#pragma once

#include <filesystem>
#if defined(SPACEYAKUZA_PORTABLE)
#include <SDL.h>
#include <stdexcept>
#endif
#if defined(_WIN32)
#include <windows.h>
#endif

/**
 * @brief ユーザーごとの永続データ保存先を取得する
 * @return Application.persistentDataPath相当のディレクトリパス
 */
inline std::filesystem::path UserDataPath() {
#if defined(__EMSCRIPTEN__)
    // 起動時にIndexedDBの読み込みを完了してからゲームへ渡す
    return "/save";
#elif defined(SPACEYAKUZA_PORTABLE)
    char* directory = SDL_GetPrefPath("KotoThing", "SpaceYakuza");
    if (!directory) throw std::runtime_error(SDL_GetError());
    const std::filesystem::path path = std::filesystem::u8path(directory);
    SDL_free(directory);
    return path;
#else
    wchar_t localAppData[MAX_PATH] {};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    const std::filesystem::path base = length > 0 && length < MAX_PATH
        ? localAppData : std::filesystem::current_path();
    return base / L"SpaceYakuza";
#endif
}

/** @brief 完成した一時ファイルで保存データを置き換える @param path 保存先 @param temporary 一時ファイル @return 置換に成功した場合true */
inline bool ReplaceUserDataFile(const std::filesystem::path& path, const std::filesystem::path& temporary) {
    // 同一ディレクトリ内のrenameで、失敗時にも元の保存データを維持する
#if defined(_WIN32)
    return MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    return !error;
#endif
}
