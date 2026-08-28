#include "Renderer.h"

#include <algorithm>
#include <cstring>

RenderCommand* Renderer::TryAppend(RenderCommand::Type type) {
    if (m_commandCount >= MaxCommands) {
        ++m_droppedCommandCount;
        m_overflowed = true;
        return nullptr;
    }
    RenderCommand& command = m_commands[m_commandCount++];
    command = {};
    command.type = type;
    return &command;
}

void Renderer::BeginFrame() {
    m_commandCount = 0;
    m_droppedCommandCount = 0;
    m_overflowed = false;
    m_flushed = false;
    if (m_backend != nullptr) m_backend->BeginFrame();
}

void Renderer::Draw(const Circle& circle, const ColorF& color) {
    RenderCommand* command = TryAppend(RenderCommand::Type::Circle);
    if (command == nullptr) return;
    command->circle = circle;
    command->color = color;
}

void Renderer::Draw(const Rect& rect, const ColorF& color) {
    RenderCommand* command = TryAppend(RenderCommand::Type::Rect);
    if (command == nullptr) return;
    command->rect = rect;
    command->color = color;
}

void Renderer::Draw(const Primitive3D& primitive) {
    RenderCommand* command = TryAppend(RenderCommand::Type::Primitive3D);
    if (command == nullptr) return;
    command->primitive = primitive;
}

void Renderer::DrawText(std::string_view text, const Vector2& position, float size, const ColorF& color,
                        float characterSpacing) {
    RenderCommand* command = TryAppend(RenderCommand::Type::Text);
    if (command == nullptr) return;
    command->position = position;
    command->size = size;
    command->characterSpacing = characterSpacing;
    command->color = color;
    command->textLength = std::min(text.size(), command->text.size() - 1);
    std::memcpy(command->text.data(), text.data(), command->textLength);
    command->text[command->textLength] = '\0';

}

void Renderer::Draw(const Rect& rect, RectAlign alignment, const ColorF& color) {
    /** @brief 配置基準から描画用の中心座標と半サイズを計算して既存の矩形描画へ渡す */
    const Rect bounds = CreateAlignedRect(rect.size * 2.0f, alignment, rect.position);
    Draw(Rect{bounds.Center(), bounds.size * 0.5f}, color);
}

void Renderer::DrawText(std::string_view text, TextAlign alignment, float size, const ColorF& color,
                        const Vector2& offset, float characterSpacing) {
    /** @brief 配置基準から先頭文字の中心座標を計算して既存の文字描画へ渡す */
    DrawText(text, CalculateTextPosition(text, alignment, size, characterSpacing) + offset, size, color,
             characterSpacing);
}

Rect Renderer::CreateAlignedRect(const Vector2& size, RectAlign alignment, const Vector2& offset) {
    /** @brief 矩形のサイズと配置基準から画面内の左下座標を計算する */
    Vector2 position = {-1.0f, 1.0f - size.y};
    switch (alignment) {
    case RectAlign::TopCenter:
    case RectAlign::Center:
    case RectAlign::BottomCenter:
        position.x = -size.x * 0.5f;
        break;
    case RectAlign::TopRight:
    case RectAlign::CenterRight:
    case RectAlign::BottomRight:
        position.x = 1.0f - size.x;
        break;
    default:
        break;
    }

    switch (alignment) {
    case RectAlign::CenterLeft:
    case RectAlign::Center:
    case RectAlign::CenterRight:
        position.y = -size.y * 0.5f;
        break;
    case RectAlign::BottomLeft:
    case RectAlign::BottomCenter:
    case RectAlign::BottomRight:
        position.y = -1.0f;
        break;
    default:
        break;
    }

    return {position + offset, size};
}

Vector2 Renderer::CalculateTextPosition(std::string_view text, TextAlign alignment, float size,
                                        float characterSpacing) const {
    /** @brief 改行を考慮して最長行の文字数と行数を求める */
    std::size_t longestLineLength = 0;
    std::size_t currentLineLength = 0;
    std::size_t lineCount = 1;
    for (const char character : text) {
        if (character == '\n') {
            longestLineLength = std::max(longestLineLength, currentLineLength);
            currentLineLength = 0;
            ++lineCount;
        } else {
            ++currentLineLength;
        }
    }
    longestLineLength = std::max(longestLineLength, currentLineLength);

    /** @brief 文字列の幅と高さから各方向の先頭文字位置を決定する */
    const float glyphHalfWidth = size * AspectRatio();
    const float characterAdvance = size * 1.5f + characterSpacing;
    const float lineWidth = longestLineLength == 0 ? 0.0f :
        (static_cast<float>(longestLineLength - 1) * characterAdvance) + glyphHalfWidth * 2.0f;
    const float textHeight = static_cast<float>(lineCount) * size * 2.0f;

    float startX = -1.0f + glyphHalfWidth;
    float startY = 1.0f - size;
    switch (alignment) {
    case TextAlign::TopCenter:
    case TextAlign::Center:
    case TextAlign::BottomCenter:
        startX = -lineWidth * 0.5f + glyphHalfWidth;
        break;
    case TextAlign::TopRight:
    case TextAlign::CenterRight:
    case TextAlign::BottomRight:
        startX = 1.0f - lineWidth + glyphHalfWidth;
        break;
    default:
        break;
    }

    switch (alignment) {
    case TextAlign::CenterLeft:
    case TextAlign::Center:
    case TextAlign::CenterRight:
        startY = textHeight * 0.5f - size;
        break;
    case TextAlign::BottomLeft:
    case TextAlign::BottomCenter:
    case TextAlign::BottomRight:
        startY = -1.0f + textHeight - size;
        break;
    default:
        break;
    }

    return {startX, startY};
}

void Renderer::SetPipeline(PipelineId pipeline) {
    RenderCommand* command = TryAppend(RenderCommand::Type::Pipeline);
    if (command == nullptr) return;
    command->pipeline = pipeline;
}

void Renderer::SetCamera(const Camera2D& camera) {
    RenderCommand* command = TryAppend(RenderCommand::Type::SetCamera);
    if (command == nullptr) return;
    command->cameraMatrices = camera.Matrices(); command->viewport = camera.GetViewport();
}

void Renderer::SetCamera(const Camera3D& camera) {
    RenderCommand* command = TryAppend(RenderCommand::Type::SetCamera);
    if (command == nullptr) return;
    command->cameraMatrices = camera.Matrices(); command->viewport = camera.GetViewport();
}

void Renderer::ResetCamera() { TryAppend(RenderCommand::Type::ResetCamera); }

void Renderer::Flush() {
    if (m_flushed) return;
    m_flushed = true;
    if (m_backend == nullptr) return;

    for (std::size_t index = 0; index < m_commandCount; ++index) {
        const RenderCommand& command = m_commands[index];
        switch (command.type) {
        case RenderCommand::Type::Circle:
            m_backend->DrawCircle(command.circle, command.color);
            break;
        case RenderCommand::Type::Rect:
            m_backend->DrawRect(command.rect, command.color);
            break;
        case RenderCommand::Type::Primitive3D:
            m_backend->DrawPrimitive3D(command.primitive);
            break;
        case RenderCommand::Type::Text:
            m_backend->DrawTextCommand(std::string_view(command.text.data(), command.textLength),
                                command.position, command.size, command.color, command.characterSpacing);
            break;
        case RenderCommand::Type::Pipeline:
            m_backend->SetPipeline(command.pipeline);
            break;
        case RenderCommand::Type::SetCamera:
            m_backend->SetCamera(command.cameraMatrices, command.viewport);
            break;
        case RenderCommand::Type::ResetCamera:
            m_backend->ResetCamera();
            break;
        }
    }
}

void Renderer::EndFrame() {
    Flush();
    if (m_backend != nullptr) m_backend->EndFrame();
}
