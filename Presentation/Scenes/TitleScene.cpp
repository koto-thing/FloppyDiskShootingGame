#include "TitleScene.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"

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

    // Enterキーの押下をフレーム単位で受け取りゲームシーンへ遷移する
    if (Input::GetKeyDown(KeyCode::Enter)) {
        changeScene(SceneType::TestStage);
    }
}

/**
 * @brief タイトルシーンの更新処理
 */
void TitleScene::Tick() {
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
