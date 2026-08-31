#include "RankingScene.h"

#include <cstdio>
#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/UI/Button.h"

namespace {
/**
 * @brief 現在のクライアント領域に対応するUI入力を取得する
 * @return NDC座標へ変換済みのUI入力状態
 */
UIInputState CurrentUIInput() {
    int width = 1280;
    int height = 720;
    if (const HWND window = GetForegroundWindow()) {
        RECT rect {};
        GetClientRect(window, &rect);
        if (rect.right - rect.left > 0) width = rect.right - rect.left;
        if (rect.bottom - rect.top > 0) height = rect.bottom - rect.top;
    }
    return UIInput::Current(width, height);
}
}

/** @brief ランキングシーンを初期化する */
void RankingScene::Initialize() {
    m_rankings = ScoreRepository {}.Load();
    m_returnButton = std::make_unique<Button>(
        Vector2 { 0.34f, 0.09f }, RectAlign::BottomRight, "TITLE", Vector2 { -0.04f, 0.04f });
    m_returnButton->SetOnClick([this]() { changeScene(SceneType::Title); });
}

/** @brief ランキングのボタンへマウス入力を渡す */
void RankingScene::ProcessInput() {
    m_returnButton->Update(CurrentUIInput());
}

/** @brief ランキングシーンを更新する */
void RankingScene::Tick() {
}

/** @brief ランキングシーンが保持するUIを解放する */
void RankingScene::Dispose() {
    m_returnButton.reset();
}

/**
 * @brief 難易度別の上位5件スコアを描画する
 * @param renderer 描画先のレンダラー
 */
void RankingScene::Render(Renderer& renderer) {
    constexpr std::array<const char*, ScoreRepository::DifficultyCount> difficultyNames {{ "EASY", "NORMAL", "HARD" }};
    constexpr std::array<float, ScoreRepository::DifficultyCount> columnPositions {{ -0.62f, 0.0f, 0.62f }};

    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, {0.01f, 0.02f, 0.07f, 1.0f});
    renderer.DrawText("RANKING", TextAlign::Center, 0.052f, {1.0f, 0.78f, 0.18f, 1.0f}, {0.0f, 0.76f});
    for (int difficulty = 0; difficulty < ScoreRepository::DifficultyCount; ++difficulty) {
        const float x = columnPositions[static_cast<size_t>(difficulty)];
        renderer.DrawText(difficultyNames[static_cast<size_t>(difficulty)], TextAlign::Center, 0.026f,
            {0.55f, 0.86f, 1.0f, 1.0f}, {x, 0.54f});
        for (int rank = 0; rank < ScoreRepository::RankCount; ++rank) {
            const int score = m_rankings[static_cast<size_t>(difficulty)][static_cast<size_t>(rank)];
            char line[32];
            if (score > 0) std::snprintf(line, sizeof(line), "%d. %06d", rank + 1, score);
            else std::snprintf(line, sizeof(line), "%d. ------", rank + 1);
            renderer.DrawText(line, TextAlign::Center, 0.019f, {0.92f, 0.92f, 0.96f, 1.0f},
                {x, 0.32f - static_cast<float>(rank) * 0.16f});
        }
    }
    m_returnButton->Render(renderer);
}
