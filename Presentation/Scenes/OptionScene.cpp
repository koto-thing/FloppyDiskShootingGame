#include "OptionScene.h"

#include "../../Infrastructure/ExternalServices/AudioService.h"

void OptionScene::Initialize() {
    // タイトルに戻るボタン
    m_backToTitleButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.35f, 0.10f } },
        "BACK TO TITLE"
    );
    m_backToTitleButton->SetOnClick([this]() { changeScene(SceneType::Title); });
    
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
    
    m_masterVolumeSlider.Update(inputState);
    m_bgmVolumeSlider.Update(inputState);
    m_seVolumeSlider.Update(inputState);
}

void OptionScene::Tick() {
    
}

void OptionScene::Dispose() {
    m_backToTitleButton.reset();
}

void OptionScene::Render(Renderer& renderer) {
    if (m_backToTitleButton != nullptr) {
        m_backToTitleButton->Render(renderer);
    }
}