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
