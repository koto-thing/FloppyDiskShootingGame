#include "TitleScene.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "../../Infrastructure/ExternalServices/InputService.h"

/**
 * @brief タイトルシーンの初期化処理
 */
void TitleScene::Initialize() {
}

/**
 * @brief タイトルシーンの入力処理
 */
void TitleScene::ProcessInput() {
    // TODO: キー入力によるメニュー選択や、ゲーム本編への遷移要求などをここに実装します
}

/**
 * @brief タイトルシーンの更新処理
 */
void TitleScene::Tick() {
    // Enterキーが押されたら、TestStageに移行する
    if (InputService::IsKeyPressed(VK_RETURN)) {
        changeScene(SceneType::TestStage);
    }
}

/**
 * @brief タイトルシーンの描画処理
 */
void TitleScene::Render(D3D12RenderingService& renderer) {
    // 画面上部に "TITLE" と表示
    renderer.RenderText(
        "FLOPPY DISK SHOOTING GAME",
        { -0.7f, 0.6f },
        0.04f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );
    
    // 画面下部に "PRESS ENTER TO START" と表示
    renderer.RenderText(
        "PRESS ENTER TO START",
        { -0.3f, 0.0f},
        0.02f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );
}
