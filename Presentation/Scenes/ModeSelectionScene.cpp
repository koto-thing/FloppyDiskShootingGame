#include "ModeSelectionScene.h"

#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"

/** @brief モードセレクトシーンを初期化する */
void ModeSelectionScene::Initialize() {
    // 選択段階を難易度選択へ初期化する
    m_stateController = std::make_unique<ModeSelectionStateController>();

    // 難易度選択ボタンを縦に配置する
    constexpr const char* difficultyLabels[] = { "EASY", "NORMAL", "HARD" };
    constexpr DifficultyType difficulties[] = { Easy, Normal, Hard };
    for (size_t i = 0; i < m_difficultyButtons.size(); ++i) {
        m_difficultyButtons[i] = std::make_unique<Button>(
            Vector2 { 0.40f, 0.12f }, RectAlign::Center,
            difficultyLabels[i], Vector2 { 0.0f, 0.20f - static_cast<float>(i) * 0.20f });
        m_difficultyButtons[i]->SetOnClick([this, difficulty = difficulties[i]]() {
            /** @brief 選択した難易度を共有データへ保存する */
            getData().difficulty = difficulty;
            m_stateController->SetCurrentState(ModeSelectionState::PlayerTypeSelect);
        });
    }

    // プレイヤー機体選択ボタンを縦に配置する
    constexpr const char* playerTypeLabels[] = { "HOMING", "PIERCING", "SPREAD" };
    constexpr PlayerType playerTypes[] = { Homing, Piercing, Spread };
    for (size_t i = 0; i < m_playerTypeButtons.size(); ++i) {
        m_playerTypeButtons[i] = std::make_unique<Button>(
            Vector2 { 0.40f, 0.12f }, RectAlign::Center,
            playerTypeLabels[i], Vector2 { 0.0f, 0.20f - static_cast<float>(i) * 0.20f });
        m_playerTypeButtons[i]->SetOnClick([this, playerType = playerTypes[i]]() {
            // 選択した機体タイプを共有データへ保存してゲームを開始する
            getData().playerType = playerType;
            changeScene(SceneType::Story);
        });
    }

    // 機体選択から難易度選択へ戻るボタンを配置する
    m_backButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.30f, 0.10f } }, "BACK");
    m_backButton->SetOnClick([this]() {
        m_stateController->SetCurrentState(ModeSelectionState::DifficultySelect);
    });
}

/** @brief モードセレクト画面の入力を処理する */
void ModeSelectionScene::ProcessInput() {
    // 現在のクライアント領域からUI入力座標を取得する
    int width = 1280;
    int height = 720;
    if (const HWND hwnd = GetForegroundWindow()) {
        RECT clientRect {};
        if (GetClientRect(hwnd, &clientRect)) {
            width = clientRect.right > 0 ? clientRect.right : width;
            height = clientRect.bottom > 0 ? clientRect.bottom : height;
        }
    }

    UpdateActiveButtons(UIInput::Current(width, height));
}

/** @brief 現在表示中の選択肢だけ入力を受け付ける */
void ModeSelectionScene::UpdateActiveButtons(const UIInputState& inputState) {
    if (m_stateController->GetCurrentState() == ModeSelectionState::DifficultySelect) {
        for (const auto& button : m_difficultyButtons) button->Update(inputState);
        return;
    }

    for (const auto& button : m_playerTypeButtons) button->Update(inputState);
    m_backButton->Update(inputState);
}

/** @brief モードセレクト画面の固定更新を行う */
void ModeSelectionScene::Tick() {
}

/** @brief モードセレクト画面が保持するUIを解放する */
void ModeSelectionScene::Dispose() {
    for (auto& button : m_difficultyButtons) button.reset();
    for (auto& button : m_playerTypeButtons) button.reset();
    m_backButton.reset();
    m_stateController.reset();
}

/** @brief 現在の選択段階に対応するUIを描画する */
void ModeSelectionScene::Render(Renderer& renderer) {
    const bool selectingDifficulty =
        m_stateController->GetCurrentState() == ModeSelectionState::DifficultySelect;

    // 画面上部へ現在の選択内容を表示する
    renderer.DrawText(
        selectingDifficulty ? "SELECT DIFFICULTY" : "SELECT PLAYER TYPE",
        TextAlign::TopCenter,
        0.025f,
        ColorF::White(),
        { 0.0f, -0.35f });

    if (selectingDifficulty) {
        for (const auto& button : m_difficultyButtons) button->Render(renderer);
        return;
    }

    for (const auto& button : m_playerTypeButtons) button->Render(renderer);
    m_backButton->Render(renderer);
}
