#include "OptionScene.h"

#include <cstdio>
#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"

void OptionScene::Initialize() {
    // タイトルに戻るボタン
    m_backToTitleButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.35f, 0.10f } },
        "BACK TO TITLE"
    );
    m_backToTitleButton->SetOnClick([this]() { changeScene(SceneType::Title); });

    // 保存済みのレトロ映像効果設定を切替ボタンへ反映する
    m_retroEffectEnabled = SettingsRepository().Load().retroEffectEnabled;
    m_retroEffectButton = std::make_unique<Button>(
        Rect {{0.0f, -0.36f}, {0.55f, 0.10f}},
        m_retroEffectEnabled ? "RETRO EFFECT  ON" : "RETRO EFFECT OFF");
    m_retroEffectButton->SetOnClick([this]() {
        m_retroEffectEnabled = !m_retroEffectEnabled;
        m_retroEffectButton->SetText(m_retroEffectEnabled ? "RETRO EFFECT  ON" : "RETRO EFFECT OFF");
    });
    
    // マスター音量
    m_masterVolumeSlider = Slider(
        Rect { { 0.0f, 0.15f }, { 0.55f, 0.04f } },
        0.0f,
        1.0f,
        AudioService::Get().GetMasterVolume()
    );
    m_masterVolumeSlider.SetOnValueChanged(
        [](float vol) {
            AudioService::Get().SetMasterVolume(vol);
        }
    );
    
    // BGM音量
    m_bgmVolumeSlider = Slider(
        Rect { { 0.0f, 0.0f }, { 0.55f, 0.04f } },
        0.0f,
        1.0f,
        AudioService::Get().GetBGMVolume()
    );
    m_bgmVolumeSlider.SetOnValueChanged(
        [](float vol) {
            AudioService::Get().SetBGMVolume(vol);
        }
    );
    
    // SE音量
    m_seVolumeSlider = Slider(
        Rect { { 0.0f, -0.15f }, { 0.55f, 0.04f} },
        0.0f, 
        1.0f,
        AudioService::Get().GetSEVolume()
    );
    m_seVolumeSlider.SetOnValueChanged(
        [](float vol) {
            AudioService::Get().SetSEVolume(vol);
        }
    );
}

void OptionScene::ProcessInput() {
    HWND hwnd = GetForegroundWindow();
    
    int w = 1280;
    int h = 720;
    
    if (hwnd) {
        RECT rect;
        
        GetClientRect(hwnd, &rect);
        if (rect.right - rect.left > 0) {
            w = rect.right - rect.left;
        }
        if (rect.bottom - rect.top > 0) {
            h = rect.bottom - rect.top;
        }
    }
    
    UIInputState inputState = UIInput::Current(w, h);
    
    
    if (m_backToTitleButton != nullptr) {
        m_backToTitleButton->Update(inputState);
    }
    if (m_retroEffectButton != nullptr) m_retroEffectButton->Update(inputState);
    
    m_masterVolumeSlider.Update(inputState);
    m_bgmVolumeSlider.Update(inputState);
    m_seVolumeSlider.Update(inputState);
}

void OptionScene::Tick() {
    
}

void OptionScene::Dispose() {
    // 現在の音量設定をランキングと同じ永続データ領域へ保存する
    const AudioService& audio = AudioService::Get();
    SettingsRepository().Save({
        audio.GetMasterVolume(),
        audio.GetBGMVolume(),
        audio.GetSEVolume(),
        m_retroEffectEnabled
    });
    m_backToTitleButton.reset();
    m_retroEffectButton.reset();
}

void OptionScene::Render(Renderer& renderer) {
    // 切替結果を次の描画フレームからバックエンドへ反映する
    renderer.SetRetroEffectEnabled(m_retroEffectEnabled);
    // オプション画面の見出しを描画する
    renderer.DrawText(
        "OPTIONS",
        { -0.18f, 0.65f },
        0.04f,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );

    // 各音量の現在値をラベルとして描画する
    char buf[64];

    snprintf(buf, sizeof(buf), "MST %3d%%", static_cast<int>(m_masterVolumeSlider.Value() * 100.0f + 0.5f));
    renderer.DrawText(buf, { -0.65f, 0.17f }, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));

    snprintf(buf, sizeof(buf), "BGM %3d%%", static_cast<int>(m_bgmVolumeSlider.Value() * 100.0f + 0.5f));
    renderer.DrawText(buf, { -0.65f, 0.02f }, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));

    snprintf(buf, sizeof(buf), "SE  %3d%%", static_cast<int>(m_seVolumeSlider.Value() * 100.0f + 0.5f));
    renderer.DrawText(buf, { -0.65f, -0.13f }, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));

    // 音量スライダーを描画する
    m_masterVolumeSlider.Render(renderer);
    m_bgmVolumeSlider.Render(renderer);
    m_seVolumeSlider.Render(renderer);

    if (m_retroEffectButton != nullptr) m_retroEffectButton->Render(renderer);

    // タイトルへ戻るボタンを描画する
    if (m_backToTitleButton != nullptr) {
        m_backToTitleButton->Render(renderer);
    }
}
