#include "../Engine/Graphics/Renderer.h"
#include "../Presentation/Common/SpaceBackground.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

/** @brief 2つの座標が描画用の許容誤差内で一致するか判定する */
bool IsNearlyEqual(const Vector2& left, const Vector2& right) {
    constexpr float kTolerance = 0.0001f;
    return std::abs(left.x - right.x) < kTolerance && std::abs(left.y - right.y) < kTolerance;
}

/**
 * @brief Rendererの記録順を検証するヘッドレスバックエンド
 */
class FakeRenderBackend final : public IRenderBackend {
public:
    std::vector<std::string> events;
    float lastCharacterSpacing = 0.0f;
    Vector2 lastTextPosition {};
    Rect lastRect {};
    PlayerShotVisual lastPlayerShot {};

    void BeginFrame() override { events.emplace_back("begin"); }
    void DrawCircle(const Circle&, const ColorF&) override { events.emplace_back("circle"); }
    void DrawRect(const Rect& rect, const ColorF&) override {
        lastRect = rect;
        events.emplace_back("rect");
    }
    void DrawExplosion(const ExplosionVisual&) override { events.emplace_back("explosion"); }
    /** @brief 弾描画情報を記録する @param shot 弾描画情報 @return なし */
    void DrawPlayerShot(const PlayerShotVisual& shot) override {
        lastPlayerShot = shot;
        events.emplace_back("player-shot");
    }
    void DrawTextCommand(std::string_view, const Vector2& position, float, const ColorF&, float characterSpacing) override {
        lastCharacterSpacing = characterSpacing;
        lastTextPosition = position;
        events.emplace_back("text");
    }
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

void RecordsCharacterSpacing() {
    Renderer renderer;
    renderer.BeginFrame();
    renderer.DrawText("SPACED", {}, 0.1f, ColorF::White(), 0.025f);
    Require(renderer.Command(0).characterSpacing == 0.025f, "Text command must preserve character spacing");
}

void RecordsExplosionCommand() {
    Renderer renderer;
    renderer.BeginFrame();
    renderer.DrawExplosion({Matrix4x4::Identity, 0.5f, 2});
    renderer.DrawExplosion({Matrix4x4::Identity, 0.75f, 3});
    renderer.DrawExplosion({Matrix4x4::Identity, 0.25f, 4});
    Require(renderer.CommandCount() == 3, "Explosions must be recorded as individual commands");
    Require(renderer.Command(0).type == RenderCommand::Type::Explosion,
        "Explosion command must preserve its type");
    Require(renderer.Command(0).explosion.progress == 0.5f,
        "Explosion command must preserve progress");
    Require(renderer.Command(0).explosion.effectType == 2,
        "Explosion command must preserve its effect type");
    Require(renderer.Command(1).explosion.effectType == 3,
        "Engine flame command must preserve its effect type");
    Require(renderer.Command(2).explosion.effectType == 4,
        "Mortar shockwave command must preserve its effect type");
}

/** @brief 弾の深度情報がバックエンドまで保持されることを検証する @return なし */
void PreservesShotDepthTest() {
    FakeRenderBackend backend;
    Renderer renderer(backend);
    renderer.BeginFrame();
    renderer.DrawPlayerShot({{}, {}, 0.0f, 0.0f, 4, 0.75f, true});
    renderer.Flush();
    Require(backend.lastPlayerShot.depth == 0.75f,
        "Shot command must preserve projected depth");
    Require(backend.lastPlayerShot.depthTest,
        "Shot command must preserve depth-test selection");
}

void SendsCharacterSpacingToBackend() {
    FakeRenderBackend backend;
    Renderer renderer(backend);
    renderer.BeginFrame();
    renderer.DrawText("SPACED", {}, 0.1f, ColorF::White(), 0.025f);
    renderer.Flush();
    Require(backend.lastCharacterSpacing == 0.025f, "Renderer must send character spacing to backend");
}

void AlignsTextToScreenPositions() {
    FakeRenderBackend backend;
    Renderer renderer(backend);
    renderer.BeginFrame();
    renderer.DrawText("AB", TextAlign::Center, 0.1f, ColorF::White());
    renderer.Flush();
    Require(IsNearlyEqual(backend.lastTextPosition, {-0.075f, 0.0f}),
            "Center alignment must place the text at the screen center");

    renderer.BeginFrame();
    renderer.DrawText("A", TextAlign::BottomCenter, 0.1f, ColorF::White());
    renderer.Flush();
    Require(IsNearlyEqual(backend.lastTextPosition, {0.0f, -0.9f}),
            "BottomCenter alignment must place the text at the bottom center");
}

void AlignsRectToScreenPositions() {
    FakeRenderBackend backend;
    Renderer renderer(backend);
    renderer.BeginFrame();
    renderer.Draw(Rect{{}, {0.4f, 0.2f}}, RectAlign::Center, ColorF::White());
    renderer.Flush();
    Require(IsNearlyEqual(backend.lastRect.position, {0.0f, 0.0f}),
            "Center alignment must place the rect at the screen center");

    renderer.BeginFrame();
    renderer.Draw(Rect{{0.1f, 0.05f}, {0.4f, 0.2f}}, RectAlign::BottomCenter, ColorF::White());
    renderer.Flush();
    Require(IsNearlyEqual(backend.lastRect.position, {0.1f, -0.75f}),
            "BottomCenter alignment must place the rect at the bottom center with its offset");
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

/**
 * @brief 星空背景が画面全体を覆い、星が時間経過で変化することを検証する
 * @return なし
 */
void SpaceBackgroundCoversScreenAndAnimates() {
    Renderer initialRenderer;
    initialRenderer.BeginFrame();
    SpaceBackground::Render(initialRenderer, 0.0f);

    Require(initialRenderer.CommandCount() ==
            static_cast<std::size_t>(SpaceBackground::StarCount + 1),
        "Space background must record one fill and every star");
    Require(initialRenderer.Command(0).rect.size == Vector2 {2.0f, 2.0f},
        "Space background must cover the whole screen");

    // 全星が余白を含む画面範囲内へ配置されていることを確認する
    for (int index = 0; index < SpaceBackground::StarCount; ++index) {
        const RenderCommand& star = initialRenderer.Command(static_cast<std::size_t>(index + 1));
        Require(star.rect.position.x >= -1.04f && star.rect.position.x <= 1.04f &&
                star.rect.position.y >= -1.04f && star.rect.position.y <= 1.04f,
            "Space background stars must stay inside the covered field");
    }

    Renderer animatedRenderer;
    animatedRenderer.BeginFrame();
    SpaceBackground::Render(animatedRenderer, 1.0f);
    Require(animatedRenderer.Command(1).rect.position.y != initialRenderer.Command(1).rect.position.y &&
            animatedRenderer.Command(1).color.r != initialRenderer.Command(1).color.r,
        "Space background stars must drift and twinkle over time");
}
}

void RunRendererTests() {
    RecordsAndResetsCommands();
    RecordsCharacterSpacing();
    RecordsExplosionCommand();
    PreservesShotDepthTest();
    SendsCharacterSpacingToBackend();
    AlignsTextToScreenPositions();
    AlignsRectToScreenPositions();
    HandlesTextAndOverflow();
    ExecutesOnlyOnFlush();
    EndFrameFlushesAndBackendIsOptional();
    SpaceBackgroundCoversScreenAndAnimates();
}
