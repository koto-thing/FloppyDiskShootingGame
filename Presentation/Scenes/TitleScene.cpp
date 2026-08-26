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
    // オプションボタン
    m_optionButton = std::make_unique<Button>(
        Rect { { 0.0f, -0.60f }, { 0.35f, 0.10f } },
        "OPTIONS"
    );
    m_optionButton->SetOnClick([this]() { changeScene(SceneType::Option); });
    
    // 左下にクレジットシーンへ移動するボタンを配置する
    m_creditButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.35f, 0.10f } },
        "CREDITS"
    );
    m_creditButton->SetOnClick([this]() { changeScene(SceneType::Credit); });
    

    // 音量調整スライダーの初期化
    // ラベルの右側に適正な余白を確保して配置する
    masterSlider_ = Slider(
        Rect{ { -0.65f, -0.47f }, { 0.55f, 0.04f } },
        0.0f,
        1.0f,
        AudioService::Get().GetMasterVolume()
    );
    masterSlider_.SetOnValueChanged(
        [](float vol) {
            AudioService::Get().SetMasterVolume(vol);
        }
    );

    bgmSlider_ = Slider(
        Rect{ { -0.65f, -0.60f }, { 0.55f, 0.04f } },
        0.0f,
        1.0f,
        AudioService::Get().GetBGMVolume()
    );
    bgmSlider_.SetOnValueChanged(
        [](float vol) {
            AudioService::Get().SetBGMVolume(vol);
        }
    );
    
    seSlider_ = Slider(
        Rect{ { -0.65f, -0.73f }, { 0.55f, 0.04f } },
        0.0f,
        1.0f,
        AudioService::Get().GetSEVolume()
    );
    seSlider_.SetOnValueChanged(
        [](float vol) {
            AudioService::Get().SetSEVolume(vol);
        }
    );
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

    // 左下のクレジットボタンにマウス入力を渡す
    if (m_creditButton != nullptr) {
        m_creditButton->Update(inputState);
    }
    
    // オプションボタンにマウス入力を渡す
    if (m_optionButton != nullptr) {
        m_optionButton->Update(inputState);
    }

    // 音量スライダーの更新
    masterSlider_.Update(inputState);
    bgmSlider_.Update(inputState);
    seSlider_.Update(inputState);
}

/**
 * @brief タイトルシーンの更新処理
 */
void TitleScene::Tick() {
    // Enterキーが押されたら、TestStageに移行する
    if (InputService::IsKeyPressed(VK_RETURN)) {
        AudioService::Get().PlaySE(
            Audio::SfxrPreset::BlipSelect
        );

        changeScene(SceneType::TestStage);
    }
}

void TitleScene::Dispose() {
    m_creditButton.reset();
    m_optionButton.reset();
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
    
    // オプションボタンを描画
    if (m_optionButton != nullptr) {
        m_optionButton->Render(renderer);
    }

    // 音量調整スライダー
    char buf[64];

    // MASTER
    snprintf(
        buf,
        sizeof(buf),
        "MST %3d%%",
        static_cast<int>(
            masterSlider_.Value() * 100.0f + 0.5f
        )
    );

    renderer.DrawText(
        buf,
        { -0.95f, -0.45f },
        0.016f,
        ColorF(0.7f, 0.7f, 0.7f, 0.8f)
    );

    masterSlider_.Render(renderer);

    // BGM
    snprintf(
        buf,
        sizeof(buf),
        "BGM %3d%%",
        static_cast<int>(
            bgmSlider_.Value() * 100.0f + 0.5f
        )
    );

    renderer.DrawText(
        buf,
        { -0.95f, -0.58f },
        0.016f,
        ColorF(0.7f, 0.7f, 0.7f, 0.8f)
    );

    bgmSlider_.Render(renderer);

    // SE
    snprintf(
        buf,
        sizeof(buf),
        "SE  %3d%%",
        static_cast<int>(
            seSlider_.Value() * 100.0f + 0.5f
        )
    );

    renderer.DrawText(
        buf,
        { -0.95f, -0.71f },
        0.016f,
        ColorF(0.7f, 0.7f, 0.7f, 0.8f)
    );

    seSlider_.Render(renderer);
}