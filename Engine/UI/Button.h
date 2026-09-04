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
    enum class ClickSound { Confirm, Cancel };
    using ClickSoundHandler = std::function<void(ClickSound)>;

    explicit Button(Rect bounds = {}, std::string text = {});
    Button(Vector2 size, RectAlign alignment, std::string text = {}, Vector2 offset = Vector2::Zero);

    void SetBounds(const Rect& bounds) { m_bounds = bounds; }
    /** @brief 画面内の配置基準とサイズからボタン境界を設定する */
    void SetBounds(const Vector2& size, RectAlign alignment, const Vector2& offset = Vector2::Zero) {
        m_bounds = Renderer::CreateAlignedRect(size, alignment, offset);
    }
    const Rect& Bounds() const { return m_bounds; }
    void SetText(std::string text) { m_text = std::move(text); }
    const std::string& Text() const { return m_text; }
    void SetEnabled(bool enabled) { m_enabled = enabled; if (!enabled) m_pressed = false; }
    bool Enabled() const { return m_enabled; }
    bool IsHovered() const { return m_hovered; }
    bool IsPressed() const { return m_pressed; }
    void SetOnClick(std::function<void()> callback) { m_onClick = std::move(callback); }
    /** @brief クリック時に再生する効果音の種類を設定する
     * @param sound 効果音の種類
     */
    void SetClickSound(ClickSound sound) { m_clickSound = sound; }
    /** @brief 全ボタン共通の効果音再生処理を設定する
     * @param handler 効果音の種類を受け取る再生処理
     */
    static void SetClickSoundHandler(ClickSoundHandler handler) { s_clickSoundHandler = std::move(handler); }

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
    float characterSpacing = 0.002f;

private:
    Rect m_bounds;
    std::string m_text;
    std::function<void()> m_onClick;
    ClickSound m_clickSound = ClickSound::Confirm;
    inline static ClickSoundHandler s_clickSoundHandler;
    bool m_enabled = true;
    bool m_hovered = false;
    bool m_pressed = false;
};
