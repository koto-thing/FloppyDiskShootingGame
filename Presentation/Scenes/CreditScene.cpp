#include "CreditScene.h"

#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Infrastructure/ExternalServices/InputService.h"

CreditScene::~CreditScene() {
    Dispose();
}

/**
 * @brief クレジット用GameObjectとスクロールコンポーネントを生成する
 */
void CreditScene::Initialize() {
    // 左下にタイトルシーンへ戻るボタンを配置する
    m_backButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.35f, 0.10f } },
        "BACK TO TITLE"
    );
    m_backButton->SetOnClick([this]() { changeScene(SceneType::Title); });

    // クレジット全体を移動させるGameObjectを生成する
    m_creditObject = std::make_unique<GameObject>();
    m_creditObject->SetName("CreditContent");

    // GameObjectへクレジットのスクロールコンポーネントを追加する
    m_creditController = m_creditObject->AddComponent<CreditContentController>();
    m_creditController->SetCreditContent(CreditSceneContent::CreateDefault());
    m_creditController->SetScrollSpeed(0.1f);

    // クレジットの先頭を画面下部の外側に配置する
    m_creditObject->SetPosition({ 0.0f, -1.1f, 0.0f });
}

/**
 * @brief クレジットシーンの入力を処理する
 */
void CreditScene::ProcessInput() {
    // 左下のタイトルへ戻るボタンにマウス入力を渡す
    if (m_backButton != nullptr) {
        m_backButton->Update(UIInput::Current(1920, 1080));
    }

    // Escapeキーでタイトル画面へ戻る
    if (InputService::IsKeyPressed(VK_ESCAPE)) {
        changeScene(SceneType::Title);
    }
}

/**
 * @brief クレジット用GameObjectを更新する
 */
void CreditScene::Tick() {
    // 所属コンポーネントのTickによりクレジットを上方向へスクロールする
    if (m_creditObject != nullptr) {
        m_creditObject->Tick();
    }

    // 最後まで流れたらタイトル画面へ戻る
    if (m_creditController != nullptr && m_creditController->IsCreditEnd()) {
        changeScene(SceneType::Title);
    }
}

/**
 * @brief クレジットシーンで生成したオブジェクトを解放する
 */
void CreditScene::Dispose() {
    // GameObjectが所有するコンポーネントを先に終了する
    if (m_creditObject != nullptr) {
        m_creditObject->Dispose();
    }

    m_creditController.reset();
    m_creditObject.reset();
    m_backButton.reset();
}

/**
 * @brief クレジットを現在のスクロール位置で描画する
 * @param renderer 描画コマンドを記録するRenderer
 */
void CreditScene::Render(Renderer& renderer) {
    // 初期化前または終了後は描画しない
    if (m_creditObject == nullptr || m_creditController == nullptr) return;

    // クレジットの描画は専用プレゼンターへ委譲する
    m_creditPresenter.Render(renderer, *m_creditObject, m_creditController->GetCreditContent());

    // 左下のタイトルへ戻るボタンを描画する
    if (m_backButton != nullptr) {
        m_backButton->Render(renderer);
    }
}
