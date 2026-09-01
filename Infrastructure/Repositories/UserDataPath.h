#pragma once

#include <filesystem>
#include <windows.h>

/**
 * @brief ユーザーごとの永続データ保存先を取得する
 * @return Application.persistentDataPath相当のディレクトリパス
 */
inline std::filesystem::path UserDataPath() {
    wchar_t localAppData[MAX_PATH] {};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    const std::filesystem::path base = length > 0 && length < MAX_PATH
        ? localAppData : std::filesystem::current_path();
    return base / L"FloppyDiskShootingGame";
}
