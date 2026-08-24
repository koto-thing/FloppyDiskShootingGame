#pragma once

#include <functional>
#include <string>
#include <utility>

#include "../Geometry/Rect.h"
#include "../Graphics/Color.h"
#include "../Graphics/Renderer.h"
#include "UIInput.h"


/** @brief クリック可能な矩形UIコントロール */
class Button {
public:
    explicit Button(Rect bounds = {}, std::string text = {});

    void SetBounds(const Rect& bounds) { m_bounds = bounds; }
    const Rect& Bounds() const { return m_bounds; }
    void SetText(std::string text) { m_text = std::move(text); }
    const std::string& Text() const { return m_text; }
    void SetEnabled(bool enabled) { m_enabled = enabled; if (!enabled) m_pressed = false; }
    bool Enabled() const { return m_enabled; }
    bool IsHovered() const { return m_hovered; }
    bool IsPressed() const { return m_pressed; }
    void SetOnClick(std::function<void()> callback) { m_onClick = std::move(callback); }

    /** @brief UI入力を処理し、ボタン内で押して離した時にコールバックを実行する */
    void Update(const UIInputState& input);
    /** @brief 現在の状態でボタンを描画する */
    void Render(Renderer& renderer) const;

    ColorF normalColor {0.20f, 0.30f, 0.55f, 1.0f};
    ColorF hoverColor {0.28f, 0.43f, 0.75f, 1.0f};
    ColorF pressedColor {0.12f, 0.20f, 0.40f, 1.0f};
    ColorF disabledColor {0.25f, 0.25f, 0.25f, 1.0f};
    ColorF textColor = ColorF::White();
    float textSize = 0.012f;

private:
    Rect m_bounds;
    std::string m_text;
    std::function<void()> m_onClick;
    bool m_enabled = true;
    bool m_hovered = false;
    bool m_pressed = false;
};
