#pragma once

#include <functional>
#include <utility>

#include "../Geometry/Rect.h"
#include "../Graphics/Color.h"
#include "../Graphics/Renderer.h"
#include "UIInput.h"

/** @brief 水平ドラッグで範囲内の値を変更するUIスライダー */
class Slider {
public:
    explicit Slider(Rect bounds = {}, float minimum = 0.0f, float maximum = 1.0f, float value = 0.0f);

    void SetBounds(const Rect& bounds) { m_bounds = bounds; }
    const Rect& Bounds() const { return m_bounds; }
    void SetRange(float minimum, float maximum);
    float Minimum() const { return m_minimum; }
    float Maximum() const { return m_maximum; }
    void SetValue(float value);
    float Value() const { return m_value; }
    float NormalizedValue() const;
    void SetEnabled(bool enabled) { m_enabled = enabled; if (!enabled) m_dragging = false; }
    bool Enabled() const { return m_enabled; }
    bool IsDragging() const { return m_dragging; }
    void SetOnValueChanged(std::function<void(float)> callback) { m_onValueChanged = std::move(callback); }

    /** @brief UI入力を処理する。トラック上のクリックとドラッグをサポートする */
    void Update(const UIInputState& input);
    /** @brief 背景トラック、塗りつぶし、つまみを描画する */
    void Render(Renderer& renderer) const;

    ColorF trackColor {0.18f, 0.18f, 0.18f, 1.0f};
    ColorF fillColor {0.18f, 0.55f, 0.90f, 1.0f};
    ColorF handleColor {0.90f, 0.90f, 0.95f, 1.0f};
    ColorF disabledColor {0.30f, 0.30f, 0.30f, 1.0f};
    float handleWidth = 0.03f;

private:
    void SetValueFromPosition(float x);

    Rect m_bounds;
    float m_minimum = 0.0f;
    float m_maximum = 1.0f;
    float m_value = 0.0f;
    std::function<void(float)> m_onValueChanged;
    bool m_enabled = true;
    bool m_dragging = false;
};
