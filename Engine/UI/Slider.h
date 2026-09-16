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
    /**
     * @brief 矩形UIスライダーを生成する
     * @param bounds スライダーの境界
     * @param minimum 最小値
     * @param maximum 最大値
     * @param value 初期値
     */
    explicit Slider(Rect bounds = {}, float minimum = 0.0f, float maximum = 1.0f, float value = 0.0f);
    /**
     * @brief 配置基準からUIスライダーを生成する
     * @param size スライダーサイズ
     * @param alignment 配置基準
     * @param minimum 最小値
     * @param maximum 最大値
     * @param value 初期値
     * @param offset 配置オフセット
     */
    Slider(Vector2 size, RectAlign alignment, float minimum = 0.0f, float maximum = 1.0f,
           float value = 0.0f, Vector2 offset = Vector2::Zero);

    /** @brief スライダー境界を設定する */
    void SetBounds(const Rect& bounds) { m_bounds = bounds; }
    /** @brief 画面内の配置基準とサイズからスライダー境界を設定する */
    void SetBounds(const Vector2& size, RectAlign alignment, const Vector2& offset = Vector2::Zero) {
        m_bounds = Renderer::CreateAlignedRect(size, alignment, offset);
    }
    /** @brief スライダー境界を取得する */
    const Rect& Bounds() const { return m_bounds; }
    /** @brief 値の範囲を設定する */
    void SetRange(float minimum, float maximum);
    /** @brief 最小値を取得する */
    float Minimum() const { return m_minimum; }
    /** @brief 最大値を取得する */
    float Maximum() const { return m_maximum; }
    /** @brief 現在値を設定する */
    void SetValue(float value);
    /** @brief 現在値を取得する */
    float Value() const { return m_value; }
    /** @brief 現在値を0から1へ正規化して取得する */
    float NormalizedValue() const;
    /** @brief スライダーの有効状態を設定する */
    void SetEnabled(bool enabled) { m_enabled = enabled; if (!enabled) m_dragging = false; }
    /** @brief スライダーの有効状態を取得する */
    bool Enabled() const { return m_enabled; }
    /** @brief ドラッグ中か取得する */
    bool IsDragging() const { return m_dragging; }
    /** @brief 値変更時のコールバックを設定する */
    void SetOnValueChanged(std::function<void(float)> callback) { m_onValueChanged = std::move(callback); }

    /** @brief UI入力を処理する、トラック上のクリックとドラッグをサポートする */
    void Update(const UIInputState& input);
    /** @brief 背景トラック、塗りつぶし、つまみを描画する */
    void Render(Renderer& renderer) const;

    ColorF trackColor {0.18f, 0.18f, 0.18f, 1.0f};
    ColorF fillColor {0.18f, 0.55f, 0.90f, 1.0f};
    ColorF handleColor {0.90f, 0.90f, 0.95f, 1.0f};
    ColorF disabledColor {0.30f, 0.30f, 0.30f, 1.0f};
    float handleWidth = 0.03f;

private:
    /** @brief ポインター位置から現在値を設定する */
    void SetValueFromPosition(float x);

    Rect m_bounds;
    float m_minimum = 0.0f;
    float m_maximum = 1.0f;
    float m_value = 0.0f;
    std::function<void(float)> m_onValueChanged;
    bool m_enabled = true;
    bool m_dragging = false;
};
