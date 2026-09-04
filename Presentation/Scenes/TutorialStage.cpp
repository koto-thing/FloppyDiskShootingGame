#include "TutorialStage.h"

#include <windows.h>

#include "../Gameplay/SideScrollingShooter.h"
#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/UI/UIInput.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"

TutorialStage::TutorialStage() : m_game(std::make_unique<SideScrollingShooter>()) {}
TutorialStage::~TutorialStage() = default;

/** @brief チュートリアル本体と右下のスキップボタンを初期化する */
void TutorialStage::Initialize() {
    m_game->InitializeTutorial(getData().audio, getData().playerType, getData().difficulty);
    m_skipButton = std::make_unique<Button>(
        Vector2 {0.24f, 0.09f}, RectAlign::BottomRight, "SKIP", Vector2 {-0.04f, 0.04f});
    m_skipButton->SetOnClick([this]() { FinishTutorial(); });
    m_nextButton = std::make_unique<Button>(
        Vector2 {0.24f, 0.09f}, RectAlign::BottomRight, "NEXT", Vector2 {-0.32f, 0.04f});
    m_nextButton->SetOnClick([this]() { m_game->NextTutorialStep(); });
}

/** @brief ゲーム操作とスキップボタンへ入力を渡す */
void TutorialStage::ProcessInput() {
    m_game->ProcessInput();
    int width = 1280;
    int height = 720;
    if (const HWND window = GetForegroundWindow()) {
        RECT rect {};
        if (GetClientRect(window, &rect)) {
            width = rect.right > 0 ? rect.right : width;
            height = rect.bottom > 0 ? rect.bottom : height;
        }
    }
    m_skipButton->Update(UIInput::Current(width, height));
    m_nextButton->Update(UIInput::Current(width, height));
}

/** @brief 課題を更新し、全達成時はStage1へ進む */
void TutorialStage::Tick() {
    // UI押下中は射撃や既存弾による課題進行を止めて二重遷移を防ぐ
    if (m_skipButton->IsPressed() || m_nextButton->IsPressed()) return;
    m_game->Tick();
    if (m_game->IsTutorialComplete()) FinishTutorial();
}

/** @brief 初回チュートリアル完了を保存してStage1へ進む */
void TutorialStage::FinishTutorial() {
    GameSettings settings = SettingsRepository {}.Load();
    settings.tutorialCompleted = true;
    SettingsRepository {}.Save(settings);
    changeScene(SceneType::TestStage);
}

/** @brief 保持するUIとゲーム本体を解放する */
void TutorialStage::Dispose() {
    m_skipButton.reset();
    m_nextButton.reset();
    m_game.reset();
}

/** @brief Stage1背景のチュートリアル本体とスキップボタンを描画する */
void TutorialStage::Render(Renderer& renderer) {
    m_game->Render(renderer);
    m_nextButton->Render(renderer);
    m_skipButton->Render(renderer);
}
