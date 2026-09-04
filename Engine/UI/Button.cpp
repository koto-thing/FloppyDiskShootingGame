#include "Button.h"

#include <utility>

Button::Button(Rect bounds, std::string text)
    : m_bounds(bounds), m_text(std::move(text)) {}

Button::Button(Vector2 size, RectAlign alignment, std::string text, Vector2 offset)
    : Button(Renderer::CreateAlignedRect(size, alignment, offset), std::move(text)) {}

void Button::Update(const UIInputState& input) {
    m_hovered = m_enabled && m_bounds.Contains(input.position);
    // フォーカス喪失や切断で解放イベントが破棄された押下を取り消す
    if (!input.primaryDown && !input.primaryReleased) m_pressed = false;
    if (!m_enabled) return;

    if (input.primaryPressed && m_hovered) {
        m_pressed = true;
    }

    if (input.primaryReleased) {
        const bool clicked = m_pressed && m_hovered;
        m_pressed = false;
        if (clicked) {
            // シーン遷移でボタンが破棄される前に効果音を再生する
            if (s_clickSoundHandler) s_clickSoundHandler(m_clickSound);
            if (m_onClick) m_onClick();
        }
    }
}

void Button::Render(Renderer& renderer) const {
    const ColorF& color = !m_enabled ? disabledColor : (m_pressed ? pressedColor : (m_hovered ? hoverColor : normalColor));

    // Rectのpositionは左下を表すため、描画用には中心座標と半サイズへ変換する
    const Rect drawBounds { m_bounds.Center(), m_bounds.size * 0.5f };
    renderer.Draw(drawBounds, color);

    if (!m_text.empty()) {
        // テキスト描画は先頭文字の中心座標を受け取るため、文字列全体をボタン中央へ寄せる
        const float firstGlyphX = m_bounds.Center().x -
            static_cast<float>(m_text.size() - 1) * (textSize * 1.5f + characterSpacing) * 0.5f;
        renderer.DrawText(m_text, { firstGlyphX, m_bounds.Center().y }, textSize, textColor, characterSpacing);
    }
}
