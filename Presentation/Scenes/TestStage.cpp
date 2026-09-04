#include "TestStage.h"
#include "../Gameplay/SideScrollingShooter.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"
#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "../../Infrastructure/Repositories/ScoreRepository.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Input/KeyCode.h"
#include "../../Engine/Graphics/Renderer.h"

#include <cstdio>
#include <windows.h>

namespace {
constexpr int FinalClearDisplayFrames = 180;

/**
 * @brief 現在のクライアント領域に対応するUI入力を取得する
 * @return NDC座標へ変換済みのUI入力状態
 */
UIInputState CurrentUIInput() {
    int width = 1280;
    int height = 720;
    if (const HWND window = GetForegroundWindow()) {
        RECT rect {};
        GetClientRect(window, &rect);
        if (rect.right - rect.left > 0) width = rect.right - rect.left;
        if (rect.bottom - rect.top > 0) height = rect.bottom - rect.top;
    }
    return UIInput::Current(width, height);
}
}

TestStage::TestStage() : m_game(std::make_unique<SideScrollingShooter>()) {
}

TestStage::~TestStage() = default;

void TestStage::Initialize() {
    m_game->Initialize(getData().audio, getData().playerType, getData().difficulty);
    m_allClearTimer = 0;
    InitializePauseMenu();
}

void TestStage::ProcessInput() {
    if (Input::GetKeyDown(KeyCode::Escape)) {
        m_pauseMenuOpen = !m_pauseMenuOpen;
        m_optionsOpen = false;
    }

    if (m_pauseMenuOpen) {
        ProcessPauseMenuInput();
        return;
    }

    m_game->ProcessInput();
}

void TestStage::Tick() {
    if (m_pauseMenuOpen) return;
    m_game->Tick();
    if (!m_game->IsAllStagesCleared()) return;

    // 最終クリア表示を見せてからスコアを保存し、エンディングへ遷移する
    if (++m_allClearTimer < FinalClearDisplayFrames) return;
    ScoreRepository {}.Save(getData().difficulty, m_game->Score());
    changeScene(SceneType::Ending);
}

void TestStage::Dispose() {
    m_returnToTitleButton.reset();
    m_openOptionsButton.reset();
    m_closeMenuButton.reset();
    m_backToMenuButton.reset();
    m_retroEffectButton.reset();
    m_game.reset();
}

void TestStage::Render(Renderer& renderer) {
    // ゲーム中の映像設定を次の描画フレームへ反映する
    renderer.SetRetroEffectEnabled(m_retroEffectEnabled);

    // ステージ本体が背景、3Dオブジェクト、UIを一つのRenderer経路へ登録する
    m_game->Render(renderer);
    if (m_pauseMenuOpen) RenderPauseMenu(renderer);
}

void TestStage::InitializePauseMenu() {
    m_returnToTitleButton = std::make_unique<Button>(Vector2 {0.42f, 0.10f}, RectAlign::Center,
        "RETURN TO TITLE", Vector2 {0.0f, 0.12f});
    m_returnToTitleButton->SetClickSound(Button::ClickSound::Cancel);
    m_returnToTitleButton->SetOnClick([this]() { changeScene(SceneType::Title); });

    m_openOptionsButton = std::make_unique<Button>(Vector2 {0.42f, 0.10f}, RectAlign::Center,
        "OPTIONS", Vector2 {0.0f, -0.04f});
    m_openOptionsButton->SetOnClick([this]() { m_optionsOpen = true; });

    m_closeMenuButton = std::make_unique<Button>(Vector2 {0.42f, 0.10f}, RectAlign::Center,
        "CLOSE MENU", Vector2 {0.0f, -0.20f});
    m_closeMenuButton->SetClickSound(Button::ClickSound::Cancel);
    m_closeMenuButton->SetOnClick([this]() { m_pauseMenuOpen = false; });

    m_backToMenuButton = std::make_unique<Button>(Vector2 {0.42f, 0.10f}, RectAlign::Center,
        "BACK TO MENU", Vector2 {0.0f, -0.45f});
    m_backToMenuButton->SetClickSound(Button::ClickSound::Cancel);
    m_backToMenuButton->SetOnClick([this]() { m_optionsOpen = false; });

    // 保存済みのレトロ映像効果設定を切替ボタンへ反映する
    m_retroEffectEnabled = SettingsRepository {}.Load().retroEffectEnabled;
    m_retroEffectButton = std::make_unique<Button>(Vector2 {0.42f, 0.10f}, RectAlign::Center,
        m_retroEffectEnabled ? "RETRO EFFECT  ON" : "RETRO EFFECT OFF", Vector2 {0.0f, -0.30f});
    m_retroEffectButton->SetOnClick([this]() {
        m_retroEffectEnabled = !m_retroEffectEnabled;
        m_retroEffectButton->SetText(m_retroEffectEnabled ? "RETRO EFFECT  ON" : "RETRO EFFECT OFF");

        // 他の設定値を維持したまま切替結果を保存する
        SettingsRepository repository;
        GameSettings settings = repository.Load();
        settings.retroEffectEnabled = m_retroEffectEnabled;
        repository.Save(settings);
    });

    // 既存のオプション画面と同じ音量設定をゲームを止めたまま変更できるようにする
    m_masterVolumeSlider = Slider(Rect {{0.0f, 0.15f}, {0.55f, 0.04f}}, 0.0f, 1.0f,
        AudioService::Get().GetMasterVolume());
    m_masterVolumeSlider.SetOnValueChanged([](float volume) { AudioService::Get().SetMasterVolume(volume); });
    m_bgmVolumeSlider = Slider(Rect {{0.0f, 0.0f}, {0.55f, 0.04f}}, 0.0f, 1.0f,
        AudioService::Get().GetBGMVolume());
    m_bgmVolumeSlider.SetOnValueChanged([](float volume) { AudioService::Get().SetBGMVolume(volume); });
    m_seVolumeSlider = Slider(Rect {{0.0f, -0.15f}, {0.55f, 0.04f}}, 0.0f, 1.0f,
        AudioService::Get().GetSEVolume());
    m_seVolumeSlider.SetOnValueChanged([](float volume) { AudioService::Get().SetSEVolume(volume); });
}

void TestStage::ProcessPauseMenuInput() {
    const UIInputState input = CurrentUIInput();
    if (m_optionsOpen) {
        m_backToMenuButton->Update(input);
        m_masterVolumeSlider.Update(input);
        m_bgmVolumeSlider.Update(input);
        m_seVolumeSlider.Update(input);
        m_retroEffectButton->Update(input);
        return;
    }

    m_returnToTitleButton->Update(input);
    m_openOptionsButton->Update(input);
    m_closeMenuButton->Update(input);
}

void TestStage::RenderPauseMenu(Renderer& renderer) const {
    renderer.Draw(Rect {{0.0f, 0.0f}, {0.62f, 0.72f}}, {0.02f, 0.04f, 0.10f, 0.92f});

    if (!m_optionsOpen) {
        renderer.DrawText("PAUSED", {-0.18f, 0.42f}, 0.04f, ColorF::White());
        m_returnToTitleButton->Render(renderer);
        m_openOptionsButton->Render(renderer);
        m_closeMenuButton->Render(renderer);
        return;
    }

    renderer.DrawText("OPTIONS", {-0.18f, 0.42f}, 0.04f, ColorF::White());
    char text[32];
    snprintf(text, sizeof(text), "MST %3d%%", static_cast<int>(m_masterVolumeSlider.Value() * 100.0f + 0.5f));
    renderer.DrawText(text, {-0.65f, 0.17f}, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));
    snprintf(text, sizeof(text), "BGM %3d%%", static_cast<int>(m_bgmVolumeSlider.Value() * 100.0f + 0.5f));
    renderer.DrawText(text, {-0.65f, 0.02f}, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));
    snprintf(text, sizeof(text), "SE  %3d%%", static_cast<int>(m_seVolumeSlider.Value() * 100.0f + 0.5f));
    renderer.DrawText(text, {-0.65f, -0.13f}, 0.016f, ColorF(0.7f, 0.7f, 0.7f, 0.8f));
    m_masterVolumeSlider.Render(renderer);
    m_bgmVolumeSlider.Render(renderer);
    m_seVolumeSlider.Render(renderer);
    m_retroEffectButton->Render(renderer);
    m_backToMenuButton->Render(renderer);
}
