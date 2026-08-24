#pragma once

#include <string>
#include <vector>

/**
 * @brief クレジット内の文字揃えを表す列挙型
 */
enum class CreditTextAlignment {
    Left,
    Center,
    Right
};

/**
 * @brief クレジットで表示する1行分の情報を表す構造体
 * @details CreditSceneColumn内の並び順が、上から下へ並ぶ表示順になります
 */
struct CreditSceneLine {
    std::string text;
    float textSize = 0.02f;
    CreditTextAlignment alignment = CreditTextAlignment::Center;
};

/**
 * @brief クレジットのひとまとまりを表す構造体
 * @details 見出しと担当者名などの行をまとめ、次のまとまりまでの間隔を定義します
 */
struct CreditSceneColumn {
    std::vector<CreditSceneLine> lines;
    float spacingAfter = 0.05f;
};
