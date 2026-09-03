#include "StoryScene.h"

#include <algorithm>
#include <cmath>
#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Time/Time.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"

namespace {
constexpr float CrawlSpeed = 0.115f;
constexpr float CrawlStartY = -1.18f;
constexpr float LineSpacing = 0.145f;
constexpr float FadeDuration = 3.0f;
constexpr float WaitDuration = 2.0f;
constexpr float FadeStartY = 0.58f;
constexpr ColorF StoryColor { 1.0f, 0.72f, 0.08f, 1.0f };
constexpr ColorF StarColor { 0.72f, 0.80f, 1.0f, 1.0f };
}

/** @brief ストーリーシーンを初期化する */
void StoryScene::Initialize() {
    m_phase = Phase::Scrolling;
    m_scrollTime = 0.0f;
    m_backgroundTime = 0.0f;
    m_phaseTime = 0.0f;

    // 右下にゲーム開始用のスキップボタンを配置する
    m_skipButton = std::make_unique<Button>(
        Vector2 { 0.24f, 0.09f }, RectAlign::BottomRight, "SKIP", Vector2 { -0.04f, 0.04f });
    m_skipButton->SetOnClick([this]() { changeScene(NextScene()); });
}

/** @brief スキップボタンへマウス入力を渡す */
void StoryScene::ProcessInput() {
    int width = 1280;
    int height = 720;
    if (const HWND hwnd = GetForegroundWindow()) {
        RECT clientRect {};
        if (GetClientRect(hwnd, &clientRect)) {
            width = clientRect.right > 0 ? clientRect.right : width;
            height = clientRect.bottom > 0 ? clientRect.bottom : height;
        }
    }
    m_skipButton->Update(UIInput::Current(width, height));
}

/** @brief スクロール終了後にフェードと待機を順番に進める */
void StoryScene::Tick() {
    // すべての演出段階で星空を流し続ける
    m_backgroundTime += Time::fixedDeltaTime;

    if (m_phase == Phase::Scrolling) {
        m_scrollTime += Time::fixedDeltaTime;
        const float lastLineY = CrawlStartY + m_scrollTime * CrawlSpeed -
            static_cast<float>(CrawlLines().size() - 1) * LineSpacing;
        if (lastLineY >= FadeStartY) {
            m_phase = Phase::Fading;
            m_phaseTime = 0.0f;
        }
        return;
    }

    // フェード中も最後の行が画面上端へ流れ続ける
    if (m_phase == Phase::Fading) m_scrollTime += Time::fixedDeltaTime;
    m_phaseTime += Time::fixedDeltaTime;
    if (m_phase == Phase::Fading && m_phaseTime >= FadeDuration) {
        m_phase = Phase::Waiting;
        m_phaseTime = 0.0f;
    } else if (m_phase == Phase::Waiting && m_phaseTime >= WaitDuration) {
        changeScene(NextScene());
    }
}

/**
 * @brief スクロール終了後に遷移するシーンを取得する
 * @return ゲームプレイシーン
 */
SceneType StoryScene::NextScene() const {
    return SettingsRepository {}.Load().tutorialCompleted ?
        SceneType::TestStage : SceneType::TutorialStage;
}

/**
 * @brief スクロール表示する文章を取得する
 * @return 表示順に並んだストーリー文章
 */
std::span<const char* const> StoryScene::CrawlLines() const {
    return StoryLines;
}

/** @brief ストーリーシーンが保持するUIを解放する */
void StoryScene::Dispose() {
    m_skipButton.reset();
}

/** @brief 星空と遠近スクロールとスキップボタンを描画する */
void StoryScene::Render(Renderer& renderer) {
    RenderStars(renderer);
    if (m_phase != Phase::Waiting) RenderCrawl(renderer);
    m_skipButton->Render(renderer);
}

/** @brief 奥の消失点から星を手前へ加速させて主観的な航行感を表現する */
void StoryScene::RenderStars(Renderer& renderer) const {
    // 画面全域へ均等に配置した遠景星で空白領域を埋める
    for (int index = 0; index < 120; ++index) {
        const unsigned int seed = static_cast<unsigned int>(index * 22695477 + 1);
        const float x = -1.04f + static_cast<float>(seed % 997u) / 997.0f * 2.08f;
        const float initialY = static_cast<float>((seed / 997u) % 991u) / 991.0f * 2.08f;
        const float speed = 0.015f + static_cast<float>(index % 4) * 0.006f;
        float wrappedY = std::fmod(initialY + m_backgroundTime * speed, 2.08f);
        if (wrappedY < 0.0f) wrappedY += 2.08f;
        const float y = wrappedY - 1.04f;
        const float twinkle = 0.42f + 0.18f *
            std::sin(m_backgroundTime * (1.1f + static_cast<float>(index % 5) * 0.17f) +
                static_cast<float>(seed % 31u));
        const float size = index % 11 == 0 ? 0.0030f : 0.0017f;
        renderer.Draw(Rect { { x, y }, { size, size } },
            { StarColor.r * twinkle, StarColor.g * twinkle,
              StarColor.b * twinkle, 0.55f });
    }

    // 消失点から手前へ迫る星を遠景の上へ重ねる
    for (int index = 0; index < 96; ++index) {
        const unsigned int seed = static_cast<unsigned int>(index * 1664525 + 1013904223);
        const int depthLayer = index % 3;
        const float speed = 0.09f + static_cast<float>(depthLayer) * 0.025f;
        const float initialPhase = static_cast<float>((seed / 197u) % 193u) / 193.0f;
        const float phase = std::fmod(initialPhase + m_backgroundTime * speed, 1.0f);
        const float approach = phase * phase;

        // 消失点から上下左右360度の画面外方向へ広げる
        constexpr float horizonY = 0.58f;
        constexpr float TwoPi = 6.283185307f;
        const float angleOffset = static_cast<float>(seed % 101u) / 101.0f;
        const float angle = std::fmod(
            static_cast<float>(index) * 0.618033988f + angleOffset, 1.0f) * TwoPi;
        const float targetX = std::cos(angle) * 1.46f;
        const float targetY = horizonY + std::sin(angle) * 1.62f;
        const float x = targetX * approach;
        const float y = horizonY + (targetY - horizonY) * approach;
        const float brightness = 0.45f + static_cast<float>((seed / 37921u) % 55u) / 100.0f;
        const float width = 0.0012f + approach * 0.0040f;
        const float height = 0.0020f + approach * (0.014f + static_cast<float>(depthLayer) * 0.004f);
        renderer.Draw(Rect { { x, y }, { width, height } },
            { StarColor.r * brightness, StarColor.g * brightness,
              StarColor.b * brightness, 0.25f + approach * 0.75f });
    }
}

/** @brief 上方ほど縮小する行配置で奥へ流れる文章を描画する */
void StoryScene::RenderCrawl(Renderer& renderer) const {
    const float activeScrollTime = m_scrollTime;
    const float fadeAlpha = m_phase == Phase::Fading
        ? (std::clamp)(1.0f - m_phaseTime / FadeDuration, 0.0f, 1.0f)
        : 1.0f;

    const std::span<const char* const> lines = CrawlLines();
    for (size_t index = 0; index < lines.size(); ++index) {
        const float y = CrawlStartY + activeScrollTime * CrawlSpeed -
            static_cast<float>(index) * LineSpacing;
        if (y < -1.08f || y > 0.93f || lines[index][0] == '\0') continue;

        // 消失点へ近づくほど文字と行間の見かけを小さくする
        const float depth = (std::clamp)((1.0f - y) * 0.5f, 0.0f, 1.0f);
        float textSize = 0.010f + depth * 0.020f;
        if (index == 0) textSize *= 1.45f;
        const float horizonFade = (std::clamp)((0.93f - y) / 0.22f, 0.0f, 1.0f);
        renderer.DrawText(
            lines[index], TextAlign::Center, textSize,
            { StoryColor.r, StoryColor.g, StoryColor.b, fadeAlpha * horizonFade },
            { 0.0f, y }, 0.0015f);
    }
}
