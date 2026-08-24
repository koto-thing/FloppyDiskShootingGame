#pragma once

#include "CreditSceneColumn.h"

#include <vector>

/**
 * @brief クレジットシーン全体の表示内容を管理するクラス
 * @details columnsの先頭から順に、画面下部から上部へ流れるクレジットを構成します
 */
class CreditSceneContent {
public:
    /**
     * @brief ゲーム内で使用する標準のクレジット内容を作成する
     * @return 表示順を設定したクレジット内容
     */
    static CreditSceneContent CreateDefault();

    /**
     * @brief クレジットの表示列を追加します
     * @param column 追加するクレジットの表示列
     */
    void AddColumn(CreditSceneColumn column);

    /**
     * @brief 上から下への表示順でクレジットの表示列を取得します
     * @return クレジットの表示列一覧
     */
    const std::vector<CreditSceneColumn>& GetColumns() const;

    /**
     * @brief 先頭行からクレジット末尾までのスクロール距離を取得します
     * @return NDC座標系におけるクレジット全体の高さ
     */
    float GetScrollLength() const;

private:
    std::vector<CreditSceneColumn> m_columns;
};
