#include "CreditContentPresenter.h"

#include "CreditSceneContent.h"
#include "../../Domain/Entities/GameObject.h"
#include "../../Engine/Graphics/Renderer.h"

namespace {
constexpr float kCreditCharacterSpacing = 0.01f;

/**
 * @brief 指定された揃えの文字列描画開始X座標を取得する
 * @param line 描画するクレジット行
 * @return NDC座標系における描画開始X座標
 */
float GetTextStartX(const CreditSceneLine& line) {
    // 描画処理と同じ文字送りと字間を使用して文字列全体の幅を求める
    const float characterAdvance = line.textSize * 1.5f + kCreditCharacterSpacing;
    const std::size_t characterCount = line.text.size();
    const float firstGlyphToLastGlyph = characterCount > 0
        ? static_cast<float>(characterCount - 1) * characterAdvance
        : 0.0f;

    switch (line.alignment) {
    case CreditTextAlignment::Left:
        return -0.9f + line.textSize;
    case CreditTextAlignment::Right:
        return 0.9f - line.textSize - firstGlyphToLastGlyph;
    case CreditTextAlignment::Center:
    default:
        return -firstGlyphToLastGlyph * 0.5f;
    }
}
}

/**
 * @brief クレジットをGameObjectの現在位置で描画する
 * @param renderer 描画コマンドを記録するRenderer
 * @param creditObject スクロール位置を保持するGameObject
 * @param content 描画するクレジット内容
 */
void CreditContentPresenter::Render(
    Renderer& renderer,
    const GameObject& creditObject,
    const CreditSceneContent& content
) const {
    // GameObjectのY座標を先頭行のスクロール位置として使用する
    float y = creditObject.GetPosition().y;

    // 定義順にクレジット行を上から下へ並べて描画する
    for (const CreditSceneColumn& column : content.GetColumns()) {
        for (const CreditSceneLine& line : column.lines) {
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
            // 翻訳後の文字幅で配置し、スクロール位置は従来の行高を維持する
            const auto alignment = line.alignment == CreditTextAlignment::Left ? TextAlign::CenterLeft :
                line.alignment == CreditTextAlignment::Right ? TextAlign::CenterRight : TextAlign::Center;
            const float inset = line.alignment == CreditTextAlignment::Left ? 0.1f :
                line.alignment == CreditTextAlignment::Right ? -0.1f : 0.0f;
            renderer.DrawText(line.text, alignment, line.textSize, ColorF::White(), {inset, y}, kCreditCharacterSpacing);
#else
            renderer.DrawText(line.text, { GetTextStartX(line), y }, line.textSize, ColorF::White(), kCreditCharacterSpacing);
#endif

            // 文字の高さに加えて固定の余白を取り、行同士の重なりを防ぐ
            y -= line.textSize * 4.0f + 0.025f;
        }

        y -= column.spacingAfter;
    }
}
