#include "CreditSceneContent.h"

#include <utility>

/**
 * @brief ゲーム内で使用する標準のクレジット内容を作成する
 * @return 表示順を設定したクレジット内容
 */
CreditSceneContent CreditSceneContent::CreateDefault() {
    CreditSceneContent content;

    // タイトルと制作チームを追加する
    content.AddColumn({
        {
            { "SPACE YAKUZA", 0.012f, CreditTextAlignment::Center },
            { "MOMIJI GUMI", 0.009f, CreditTextAlignment::Center }
        },
        0.08f
    });
    
    content.AddColumn({
        {
            { "LEAD PROGRAMMER", 0.011f, CreditTextAlignment::Center },
            { "Ryota", 0.009f, CreditTextAlignment::Center }
        }
    });

    // 担当とメンバー名を追加する
    content.AddColumn({
        {
            { "PROGRAMMERS", 0.011f, CreditTextAlignment::Center },
            { "botamochi", 0.009f, CreditTextAlignment::Center },
            { "lumi", 0.009f, CreditTextAlignment::Center },
            { "koto", 0.009f, CreditTextAlignment::Center }
        },
        0.06f
    });
    
    content.AddColumn({
        {
            { "LEAD GRAPHIC DESIGNER", 0.011f, CreditTextAlignment::Center },
            { "lumi", 0.009f, CreditTextAlignment::Center },
        },
        0.06f
    });
    
    content.AddColumn({
        {
            { "LEAD SOUND DESIGNER", 0.011f, CreditTextAlignment::Center },
            { "botamochi", 0.009f, CreditTextAlignment::Center },
        },
        0.06f
    });
    
    content.AddColumn({
        {
            { "DIRECTOR", 0.011f, CreditTextAlignment::Center },
            { "koto", 0.009f, CreditTextAlignment::Center }
        },
        0.06f
    });
    
    content.AddColumn({
        {
            { "PRODUCER", 0.011f, CreditTextAlignment::Center },
            { "momiji", 0.009f, CreditTextAlignment::Center }
        }
    });
    
    content.AddColumn({
        {
            { "SPECIAL THANKS", 0.011f, CreditTextAlignment::Center },
            { "All Players", 0.009f, CreditTextAlignment::Center }
        },
        0.2f
    });

    // 終端メッセージを追加する
    content.AddColumn({
        {
            { "THANK YOU FOR PLAYING", 0.012f, CreditTextAlignment::Center }
        },
        0.0f
    });

    return content;
}

/**
 * @brief クレジットの表示列を追加します
 * @param column 追加するクレジットの表示列
 */
void CreditSceneContent::AddColumn(CreditSceneColumn column) {
    // 追加順を上から下へ並ぶ表示順として保持します
    m_columns.push_back(std::move(column));
}

/**
 * @brief 上から下への表示順でクレジットの表示列を取得します
 * @return クレジットの表示列一覧
 */
const std::vector<CreditSceneColumn>& CreditSceneContent::GetColumns() const {
    return m_columns;
}

/**
 * @brief 先頭行からクレジット末尾までのスクロール距離を取得します
 * @return NDC座標系におけるクレジット全体の高さ
 */
float CreditSceneContent::GetScrollLength() const {
    float scrollLength = 0.0f;

    // 描画時と同じ行間と列間を合計して末尾までの距離を求める
    for (const CreditSceneColumn& column : m_columns) {
        for (const CreditSceneLine& line : column.lines) {
            scrollLength += line.textSize * 4.0f + 0.025f;
        }

        scrollLength += column.spacingAfter;
    }

    return scrollLength;
}
