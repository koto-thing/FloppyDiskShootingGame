#pragma once

#include <string_view>

namespace MMLData {

/**
 * @brief 圧縮した原譜を一度だけ復元し、名前に対応するMMLを取得する
 * @param name MML名
 * @return 対応するMML、名前が不明または復元に失敗した場合は空文字列
 */
std::string_view GetByName(std::string_view name);

/**
 * @brief ステージ番号に対応するBGMを取得する
 * @param stage ステージ番号
 * @return ステージBGM
 */
inline std::string_view GetStageBgm(int stage) {
    switch (stage) {
        case 1: return GetByName("stage1");
        case 2: return GetByName("stage2");
        case 3: return GetByName("stage3");
        case 4: return GetByName("stage4").empty() ? GetByName("stage1") : GetByName("stage4");
        case 5: return GetByName("stage5");
        case 6: return GetByName("stage6");
        default: return GetByName("stage1");
    }
}

/**
 * @brief ステージ番号に対応するボスBGMを取得する
 * @param stage ステージ番号
 * @return ボスBGM
 */
inline std::string_view GetBossBgm(int stage) {
    switch (stage) {
        case 1: return GetByName("boss1");
        case 2: return GetByName("boss2");
        case 3: return GetByName("boss3");
        case 4: return GetByName("boss4");
        case 5: return GetByName("boss5");
        case 6: return GetByName("boss6");
        default: return GetByName("boss1");
    }
}

/**
 * @brief タイトルBGMを取得する
 * @return タイトルBGM
 */
inline std::string_view GetTitleBgm() { return GetByName("title"); }

/**
 * @brief エンディングBGMを取得する
 * @return エンディングBGM
 */
inline std::string_view GetEndingBgm() { return GetByName("ending"); }

} // namespace MMLData
