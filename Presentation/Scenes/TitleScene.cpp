#include "TitleScene.h"
#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <string>
#include "../../Engine/Graphics/Renderer.h"
#include "../../Infrastructure/ExternalServices/InputService.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"

#ifdef DrawText
#undef DrawText
#endif

/**
 * @brief タイトルシーンの初期化処理
 */
void TitleScene::Initialize() {
    // 左下にクレジットシーンへ移動するボタンを配置する
    m_creditButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.35f, 0.10f } },
        "CREDITS"
    );
    m_creditButton->SetOnClick([this]() { changeScene(SceneType::Credit); });
}

/**
 * @brief タイトルシーンの入力処理
 */
void TitleScene::ProcessInput() {
    // 左下のクレジットボタンにマウス入力を渡す
    if (m_creditButton != nullptr) {
        m_creditButton->Update(UIInput::Current(1920, 1080));
    }
}

/**
 * @brief タイトルシーンの更新処理
 */
void TitleScene::Tick() {
    // Enterキーが押されたら、TestStageに移行する
    if (InputService::IsKeyPressed(VK_RETURN)) {
        AudioService::Get().PlaySE(Audio::SfxrPreset::BlipSelect);
        changeScene(SceneType::TestStage);
    }
}

void TitleScene::Dispose() {
    m_creditButton.reset();
}

/**
 * @brief タイトルシーンの描画処理
 */
void TitleScene::Render(Renderer& renderer) {
    // 画面上部に "TITLE" と表示
    renderer.DrawText(
        "FLOPPY DISK SHOOTING GAME",
        { -0.7f, 0.6f },
        0.04f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );
    
    // 画面下部に "PRESS ENTER TO START" と表示
    renderer.DrawText(
        "PRESS ENTER TO START",
        { -0.3f, 0.0f},
        0.02f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );

    // 左下のクレジットボタンを描画する
    if (m_creditButton != nullptr) {
        m_creditButton->Render(renderer);
    }
}
