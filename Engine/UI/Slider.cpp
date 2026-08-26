#include "Slider.h"

#include <algorithm>
#include <utility>

Slider::Slider(Rect bounds, float minimum, float maximum, float value) : m_bounds(bounds) {
    SetRange(minimum, maximum);
    SetValue(value);
}

Slider::Slider(Vector2 size, RectAlign alignment, float minimum, float maximum, float value, Vector2 offset)
    : Slider(Renderer::CreateAlignedRect(size, alignment, offset), minimum, maximum, value) {}

void Slider::SetRange(float minimum, float maximum) {
    m_minimum = (std::min)(minimum, maximum);
    m_maximum = (std::max)(minimum, maximum);
    SetValue(m_value);
}

void Slider::SetValue(float value) {
    const float clampedValue = std::clamp(value, m_minimum, m_maximum);
    if (clampedValue == m_value) return;
    m_value = clampedValue;
    if (m_onValueChanged) m_onValueChanged(m_value);
}

float Slider::NormalizedValue() const {
    const float range = m_maximum - m_minimum;
    return range > 0.0f ? (m_value - m_minimum) / range : 0.0f;
}

void Slider::Update(const UIInputState& input) {
    if (!m_enabled) return;
    if (input.primaryPressed && m_bounds.Contains(input.position)) {
        m_dragging = true;
        SetValueFromPosition(input.position.x);
    }
    if (m_dragging && input.primaryDown) SetValueFromPosition(input.position.x);
    if (input.primaryReleased) m_dragging = false;
}

void Slider::Render(Renderer& renderer) const {
    const ColorF& baseColor = m_enabled ? trackColor : disabledColor;
    renderer.Draw(Rect{ m_bounds.Center(), m_bounds.size * 0.5f }, baseColor);

    const Vector2 minimum = m_bounds.Min();
    const Vector2 maximum = m_bounds.Max();
    const float handleCenter = minimum.x + (maximum.x - minimum.x) * NormalizedValue();
    const Rect fillRect{ minimum, { handleCenter - minimum.x, maximum.y - minimum.y } };
    renderer.Draw(Rect{ fillRect.Center(), fillRect.size * 0.5f }, m_enabled ? fillColor : disabledColor);

    const Rect handleRect{ { handleCenter - handleWidth * 0.5f, minimum.y }, { handleWidth, maximum.y - minimum.y } };
    renderer.Draw(Rect{ handleRect.Center(), handleRect.size * 0.5f }, m_enabled ? handleColor : disabledColor);
}

void Slider::SetValueFromPosition(float x) {
    const Vector2 minimum = m_bounds.Min();
    const Vector2 maximum = m_bounds.Max();
    const float width = maximum.x - minimum.x;
    const float normalized = width > 0.0f ? std::clamp((x - minimum.x) / width, 0.0f, 1.0f) : 0.0f;
    SetValue(m_minimum + (m_maximum - m_minimum) * normalized);
}
