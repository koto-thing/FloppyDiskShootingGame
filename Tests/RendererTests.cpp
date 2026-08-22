#include "../Engine/Graphics/Renderer.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

/**
 * @brief Rendererの記録順を検証するヘッドレスバックエンド
 */
class FakeRenderBackend final : public IRenderBackend {
public:
    std::vector<std::string> events;

    void BeginFrame() override { events.emplace_back("begin"); }
    void DrawCircle(const Circle&, const ColorF&) override { events.emplace_back("circle"); }
    void DrawRect(const Rect&, const ColorF&) override { events.emplace_back("rect"); }
    void DrawTextCommand(std::string_view, const Vector2&, float, const ColorF&) override { events.emplace_back("text"); }
    void SetPipeline(PipelineId) override { events.emplace_back("pipeline"); }
    void EndFrame() override { events.emplace_back("end"); }
};

void RecordsAndResetsCommands() {
    Renderer renderer;
    renderer.BeginFrame();
    renderer.SetPipeline(PipelineId::Object);
    renderer.Draw(Circle{{1.0f, 2.0f}, 3.0f}, ColorF::Red());
    renderer.Draw(Rect{{-1.0f, -2.0f}, {4.0f, 5.0f}}, ColorF::White());
    renderer.DrawText("SCORE", {10.0f, 20.0f}, 24.0f, ColorF::White());
    Require(renderer.CommandCount() == 4, "Renderer must preserve command count");
    Require(renderer.Command(0).type == RenderCommand::Type::Pipeline, "Pipeline command must preserve order");
    Require(renderer.Command(1).circle.radius == 3.0f, "Circle command must preserve geometry");
    Require(renderer.Command(2).rect.size.x == 4.0f, "Rect command must preserve geometry");
    Require(renderer.Command(3).textLength == 5, "Text command must preserve text length");
    renderer.EndFrame();

    renderer.BeginFrame();
    Require(renderer.CommandCount() == 0, "BeginFrame must clear previous commands");
}

void HandlesTextAndOverflow() {
    Renderer renderer;
    renderer.BeginFrame();
    renderer.DrawText(std::string(512, 'x'), {}, 1.0f, ColorF::White());
    Require(renderer.Command(0).textLength == renderer.Command(0).text.size() - 1, "Text must be truncated safely");
    Require(renderer.Command(0).text[renderer.Command(0).textLength] == static_cast<char>(0), "Text must be null terminated");

    for (std::size_t index = renderer.CommandCount(); index < Renderer::MaxCommands + 32; ++index) {
        renderer.Draw(Rect{}, ColorF::Black());
    }
    Require(renderer.CommandCount() == Renderer::MaxCommands, "Renderer must enforce fixed command capacity");
    Require(renderer.DroppedCommandCount() == 32, "Renderer must count dropped commands");
    Require(renderer.HasOverflowed(), "Renderer must expose overflow state");

    renderer.BeginFrame();
    Require(renderer.DroppedCommandCount() == 0, "BeginFrame must reset dropped command count");
    Require(!renderer.HasOverflowed(), "BeginFrame must reset overflow state");
}

void ExecutesOnlyOnFlush() {
    FakeRenderBackend backend;
    Renderer renderer(backend);
    renderer.BeginFrame();
    renderer.SetPipeline(PipelineId::Object);
    renderer.Draw(Circle{}, ColorF::Red());
    renderer.Draw(Rect{}, ColorF::White());
    renderer.DrawText("text", {}, 1.0f, ColorF::White());
    Require(backend.events.size() == 1 && backend.events[0] == "begin", "Commands must not execute before Flush");

    renderer.Flush();
    Require(backend.events == std::vector<std::string>{"begin", "pipeline", "circle", "rect", "text"},
            "Flush must preserve command order");
    renderer.Flush();
    Require(backend.events.size() == 5, "Flush must not execute commands twice");

    renderer.EndFrame();
    Require(backend.events.size() == 6 && backend.events.back() == "end", "EndFrame must finish the backend frame");
}

void EndFrameFlushesAndBackendIsOptional() {
    FakeRenderBackend backend;
    Renderer renderer(backend);
    renderer.BeginFrame();
    renderer.DrawText("deferred", {}, 1.0f, ColorF::White());
    renderer.EndFrame();
    Require(backend.events == std::vector<std::string>{"begin", "text", "end"},
            "EndFrame must flush unexecuted commands before ending");

    Renderer headless;
    headless.BeginFrame();
    headless.Draw(Rect{}, ColorF::White());
    headless.Flush();
    headless.EndFrame();
    Require(headless.CommandCount() == 1, "Headless Renderer must safely record commands");
}
}

void RunRendererTests() {
    RecordsAndResetsCommands();
    HandlesTextAndOverflow();
    ExecutesOnlyOnFlush();
    EndFrameFlushesAndBackendIsOptional();
}
