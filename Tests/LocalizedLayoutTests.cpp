#include "../Application/UseCases/Localization.h"
#include "../Application/UseCases/SceneManager.h"
#include "../Engine/Graphics/Renderer.h"
#include "../Engine/Graphics/Utf8Text.h"
#include "../Engine/Input/Input.h"
#include "../Presentation/Scenes/ModeSelectionScene.h"
#include "../Presentation/Scenes/OptionScene.h"
#include "../Presentation/Scenes/TestStage.h"
#include "../Presentation/Gameplay/SideScrollingShooter.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>

using Scenes = SceneManager<SceneType, SceneSharedData>;

/** @brief 既存の入力テスト境界から実際のポーズ画面を開く */
class InputTestAccess {
public:
    /** @brief 実入力を移動せずゲーム内ポインターを配置する @param point NDC座標 @return なし */
    static void MovePointer(Vector2 point) {
        int width = 1280, height = 720;
        RECT rect {};
        if (const HWND window = GetForegroundWindow(); window && GetClientRect(window, &rect)) {
            if (rect.right > rect.left) width = rect.right - rect.left;
            if (rect.bottom > rect.top) height = rect.bottom - rect.top;
        }
        Input::m_mousePosition = {(point.x + 1.0f) * width * 0.5f, (1.0f - point.y) * height * 0.5f};
    }

    /** @brief ゲーム内だけでボタンをクリックする @param scenes 実行中のシーン @param point NDC座標 @return なし */
    static void Click(Scenes& scenes, Vector2 point) {
        Input::BeginFrame();
        MovePointer(point);
        Input::SetMouseButtonState(MouseButton::Left, true);
        scenes.ProcessInput();
        Input::BeginFrame();
        Input::SetMouseButtonState(MouseButton::Left, false);
        scenes.ProcessInput();
        Input::CancelNativeInputState();
    }

    /** @brief ポーズ設定へ移動する入力をゲーム内だけへ送る @param scenes 実行中のシーン @return なし */
    static void OpenPauseOptions(Scenes& scenes) {
        // OSへキーやクリックを送らず、ゲーム内の入力状態だけを更新する
        Input::CancelNativeInputState();
        Input::BeginFrame();
        Input::SetNativeKeyState(VK_ESCAPE, true);
        scenes.ProcessInput();
        Input::BeginFrame();
        Input::SetNativeKeyState(VK_ESCAPE, false);

        // シーンと同じクライアント寸法で設定ボタンの中心をクリックする
        Click(scenes, {0.0f, -0.04f});
    }
};

/** @brief GPUなしで各画面比率を再現する描画バックエンド */
class LayoutBackend final : public IRenderBackend {
public:
    float aspect = 1.0f;
    /** @brief フレーム開始を省略する @return なし */
    void BeginFrame() override {}
    /** @brief 円描画を省略する @param circle 形状 @param color 色 @return なし */
    void DrawCircle(const Circle& circle, const ColorF& color) override {}
    /** @brief 矩形描画を省略する @param rect 形状 @param color 色 @return なし */
    void DrawRect(const Rect& rect, const ColorF& color) override {}
    /** @brief 文字描画を省略する @param text 文字列 @param position 座標 @param size サイズ @param color 色 @param spacing 字間 @return なし */
    void DrawTextCommand(std::string_view text, const Vector2& position, float size,
                         const ColorF& color, float spacing) override {}
    /** @brief パイプライン変更を省略する @param pipeline 描画系 @return なし */
    void SetPipeline(PipelineId pipeline) override {}
    /** @brief フレーム終了を省略する @return なし */
    void EndFrame() override {}
    /** @brief テスト対象の比率を返す @return 横幅と高さの比率 */
    float AspectRatio() const override { return aspect; }
    /** @brief テスト対象の横幅を返す @return ピクセル幅 */
    int Width() const override { return static_cast<int>(720 * aspect); }
    /** @brief テスト対象の高さを返す @return ピクセル高さ */
    int Height() const override { return 720; }
};

/** @brief 実画面の設定見出しが中央に収まることを検査する @param scenes 実行中のシーン @param renderer 描画先 @param y 見出しの高さ @return なし */
static void CheckOptionsHeading(Scenes& scenes, Renderer& renderer, float y) {
    renderer.BeginFrame();
    scenes.Render(renderer);
    assert(!renderer.HasOverflowed());
    int headings = 0;
    for (std::size_t i = 0; i < renderer.CommandCount(); ++i) {
        const auto& text = renderer.Command(i);
        if (text.type != RenderCommand::Type::Text ||
            std::string_view(text.text.data()) != Localization::Translate("OPTIONS") ||
            std::abs(text.position.y - y) > 0.0001f) continue;
        const auto metrics = Utf8Text::Measure(text.text.data(), text.size,
                                               text.characterSpacing, renderer.AspectRatio());
        const float left = text.position.x - metrics.firstGlyphOffset;
        assert(std::abs(left + metrics.width * 0.5f) < 0.0001f);
        assert(left >= -1.0f && left + metrics.width <= 1.0f);
        ++headings;
    }
    assert(headings == 1);
}

/** @brief 3機体の翻訳済み説明がパネル中央へ収まることを検査する @param scenes 実行中のシーン @param renderer 描画先 @return なし */
static void CheckModePreview(Scenes& scenes, Renderer& renderer) {
    scenes.Initialize(SceneType::ModeSelection);
    InputTestAccess::Click(scenes, {0.0f, 0.16f});
    InputTestAccess::Click(scenes, {0.0f, 0.20f});
    for (int shot = 0; shot < 3; ++shot) {
        InputTestAccess::MovePointer({-0.62f, 0.20f - shot * 0.20f});
        scenes.ProcessInput();
        renderer.BeginFrame();
        scenes.Render(renderer);
        int labels = 0;
        for (std::size_t i = 0; i < renderer.CommandCount(); ++i) {
            const auto& text = renderer.Command(i);
            if (text.type != RenderCommand::Type::Text || text.position.x < 0.0f ||
                text.position.y > 0.5f || text.position.y < -0.7f) continue;
            const auto metrics = Utf8Text::Measure(text.text.data(), text.size, text.characterSpacing);
            const float left = text.position.x - metrics.firstGlyphOffset;
            assert(std::abs(left + metrics.width * 0.5f - 0.47f) < 0.0001f);
            assert(left >= -0.02f && left + metrics.width <= 0.96f);
            ++labels;
        }
        assert(labels == 7);
    }
}

/** @brief 翻訳済みHUDの画面内配置とラベル同士の非重複を検証する */
struct HudLayoutTests {
    /** @brief 現在言語の1P/2P表示を検査する @param renderer 対象比率の描画先 @return なし */
    static void Check(Renderer& renderer) {
        auto game = std::make_unique<SideScrollingShooter>();
        for (int players : {1, 2}) {
            game->m_playerCount = players;
            game->m_score = 2147483647;
            renderer.BeginFrame();
            game->DrawArcadeHud(renderer);
            assert(!renderer.HasOverflowed());
            for (size_t i = 0; i < renderer.CommandCount(); ++i) {
                const auto& text = renderer.Command(i);
                if (text.type != RenderCommand::Type::Text) continue;
                const auto metrics = Utf8Text::Measure(text.text.data(), text.size, text.characterSpacing, renderer.AspectRatio());
                const float left = text.position.x - metrics.firstGlyphOffset;
                assert(left >= -0.98f && left + metrics.width <= 0.98f);
                assert(std::abs(text.position.y) - metrics.height * 0.5f >= 0.84f);
                assert(std::abs(text.position.y) + metrics.height * 0.5f <= 1.0f);
                // 同じ帯内で文字の外接矩形が重ならないことを確認する
                for (size_t j = i + 1; j < renderer.CommandCount(); ++j) {
                    const auto& other = renderer.Command(j);
                    if (other.type != RenderCommand::Type::Text) continue;
                    const auto bounds = Utf8Text::Measure(other.text.data(), other.size, other.characterSpacing, renderer.AspectRatio());
                    const float otherLeft = other.position.x - bounds.firstGlyphOffset;
                    const bool separate = std::abs(text.position.y - other.position.y) >= (metrics.height + bounds.height) * 0.5f ||
                        left + metrics.width <= otherLeft || otherLeft + bounds.width <= left;
                    if (!separate) std::fprintf(stderr, "HUD overlap: %s / %s (aspect %.3f, players %d)\n",
                        text.text.data(), other.text.data(), renderer.AspectRatio(), players);
                    assert(separate);
                }
            }
        }
    }
};

/** @brief 7言語と3画面比率で通常設定とポーズ設定とHUDの配置を検査する @return 成功時0 */
int main() {
    // 設定保存をプロセス固有の一時領域へ隔離して既存プレイデータを保護する
    const auto testRoot = std::filesystem::temp_directory_path() /
        (L"SpaceYakuzaLayout-" + std::to_wstring(GetCurrentProcessId()));
    assert(SetEnvironmentVariableW(L"LOCALAPPDATA", testRoot.c_str()));
    LayoutBackend backend;
    Renderer renderer(backend);
    renderer.SetTextTranslator(&Localization::Translate);
    Scenes scenes;
    scenes.AddScene<OptionScene>(SceneType::Option);
    scenes.AddScene<TestStage>(SceneType::TestStage);
    scenes.AddScene<ModeSelectionScene>(SceneType::ModeSelection);
    for (int language = 0; language < static_cast<int>(Localization::Language::Count); ++language) {
        Localization::SetLanguage(static_cast<Localization::Language>(language));
        for (const float aspect : {16.0f / 9.0f, 4.0f / 3.0f, 21.0f / 9.0f}) {
            backend.aspect = aspect;
            HudLayoutTests::Check(renderer);
            scenes.Initialize(SceneType::Option);
            CheckOptionsHeading(scenes, renderer, 0.65f);
            scenes.Initialize(SceneType::TestStage);
            InputTestAccess::OpenPauseOptions(scenes);
            CheckOptionsHeading(scenes, renderer, 0.42f);
            CheckModePreview(scenes, renderer);
        }
    }
    scenes.Dispose();
    // 削除対象が一時領域内であることを確認する
    assert(std::filesystem::equivalent(testRoot.parent_path(), std::filesystem::temp_directory_path()));
    std::filesystem::remove_all(testRoot);
    std::puts("Localized layout passed: 7 languages, 3 aspect ratios, 1P/2P HUD, both settings screens, 3 shot previews");
}
