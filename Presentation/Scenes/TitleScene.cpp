#include "TitleScene.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"
#include <windows.h>
#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Time/Time.h"
#include "../Common/SpaceBackground.h"

#ifdef DrawText
#undef DrawText
#endif

/**
 * @brief タイトルシーンの初期化処理
 */
void TitleScene::Initialize() {
    /** @brief 画面中央の少し上にゲーム開始ボタンを配置する */
    m_startButton = std::make_unique<Button>(
        Vector2 { 0.35f, 0.10f },
        RectAlign::Center,
        "START GAME",
        Vector2 { 0.0f, 0.20f }
    );
    m_startButton->SetOnClick([this]() { changeScene(SceneType::ModeSelection); });

    /** @brief ゲーム開始ボタンの下にギャラリーボタンを配置する */
    m_galleryButton = std::make_unique<Button>(
        Vector2 { 0.35f, 0.10f },
        RectAlign::Center,
        "GALLERY",
        Vector2 { 0.0f, 0.00f }
    );
    m_galleryButton->SetOnClick([this]() { changeScene(SceneType::Gallery); });

    /** @brief モデルテストボタンの下にランキングボタンを配置する */
    m_rankingButton = std::make_unique<Button>(
        Vector2 { 0.35f, 0.10f },
        RectAlign::Center,
        "RANKING",
        Vector2 { 0.0f, -0.20f }
    );
    m_rankingButton->SetOnClick([this]() { changeScene(SceneType::Ranking); });

    /** @brief ランキングボタンの下にオプションボタンを配置する */
    m_optionButton = std::make_unique<Button>(
        Vector2 { 0.35f, 0.10f },
        RectAlign::Center,
        "OPTIONS",
        Vector2 { 0.0f, -0.40f }
    );
    m_optionButton->SetOnClick([this]() { changeScene(SceneType::Option); });

    /** @brief オプションボタンの下にゲーム終了ボタンを配置する */
    m_exitButton = std::make_unique<Button>(
        Vector2 { 0.35f, 0.10f },
        RectAlign::Center,
        "EXIT GAME",
        Vector2 { 0.0f, -0.60f }
    );
    m_exitButton->SetOnClick([]() { PostQuitMessage(0); });
    
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
    HWND hwnd = GetForegroundWindow();

    int w = 1280;
    int h = 720;

    if (hwnd) {
        RECT r;

        GetClientRect(hwnd, &r);

        if (r.right - r.left > 0) {
            w = r.right - r.left;
        }

        if (r.bottom - r.top > 0) {
            h = r.bottom - r.top;
        }
    }

    UIInputState inputState = UIInput::Current(w, h);

    /** @brief ゲーム開始ボタンにマウス入力を渡す */
    if (m_startButton != nullptr) {
        m_startButton->Update(inputState);
    }

    /** @brief ギャラリーボタンにマウス入力を渡す */
    if (m_galleryButton != nullptr) {
        m_galleryButton->Update(inputState);
    }

    /** @brief ランキングボタンにマウス入力を渡す */
    if (m_rankingButton != nullptr) {
        m_rankingButton->Update(inputState);
    }

    /** @brief ゲーム終了ボタンにマウス入力を渡す */
    if (m_exitButton != nullptr) {
        m_exitButton->Update(inputState);
    }

    // 左下のクレジットボタンにマウス入力を渡す
    if (m_creditButton != nullptr) {
        m_creditButton->Update(inputState);
    }
    
    // オプションボタンにマウス入力を渡す
    if (m_optionButton != nullptr) {
        m_optionButton->Update(inputState);
    }

}

/**
 * @brief タイトルシーンの更新処理
 */
void TitleScene::Tick() {
}

void TitleScene::Dispose() {
    m_startButton.reset();
    m_galleryButton.reset();
    m_rankingButton.reset();
    m_exitButton.reset();
    m_creditButton.reset();
    m_optionButton.reset();
}

/**
 * @brief タイトルシーンの描画処理
 */
void TitleScene::Render(Renderer& renderer) {
    // ゆっくり明滅する星空をUIの背面へ描画する
    SpaceBackground::Render(renderer, Time::unscaledTime);

    // 画面上部に "TITLE" と表示
    renderer.DrawText(
        "SPACE YAKUZA",
        { -0.4f, 0.6f },
        0.04f,
        { 1.0f, 1.0f, 1.0f, 1.0f },
        0.005f
    );

    // 左下のクレジットボタンを描画する
    if (m_creditButton != nullptr) {
        m_creditButton->Render(renderer);
    }
    
    // ゲーム開始ボタンを描画する
    if (m_startButton != nullptr) {
        m_startButton->Render(renderer);
    }

    // ギャラリーボタンを描画する
    if (m_galleryButton != nullptr) {
        m_galleryButton->Render(renderer);
    }

    // ランキングボタンを描画する
    if (m_rankingButton != nullptr) {
        m_rankingButton->Render(renderer);
    }

    // オプションボタンを描画する
    if (m_optionButton != nullptr) {
        m_optionButton->Render(renderer);
    }

    // ゲーム終了ボタンを描画する
    if (m_exitButton != nullptr) {
        m_exitButton->Render(renderer);
    }

}
