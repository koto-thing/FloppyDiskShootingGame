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
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
#if defined(SPACEYAKUZA_EDITION_Steam)
#include "../../Infrastructure/ExternalServices/SteamCoopSession.h"
#else
#include "../../Infrastructure/ExternalServices/OnlineCoopSession.h"
#endif
#include "../Gameplay/GameplayRandom.h"
#endif

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
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    // 両PCの初期配置と敵出現に同じ乱数列を使う
    if (getData().onlineGame) GameplayRandom::State = getData().coop->Seed();
#endif
    m_game->Initialize(getData().audio, getData().playerType, getData().difficulty,
        getData().playerCount, getData().secondPlayerType);
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    // 通信プレイの初期入力を適用する
    if (getData().onlineGame) m_game->ApplyNetworkInput({});
#endif
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    // 通信プレイの待機状態を初期化する
    m_networkInput = {};
    m_waitingForPeer = m_peerPaused = false;
#endif
    m_allClearTimer = 0;
    m_pauseMenuOpen = false;
    m_optionsOpen = false;
    InitializePauseMenu();
}

void TestStage::ProcessInput() {
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    if (getData().onlineGame) {
        // 各PCのキーボードまたは1台目のパッドを自分の操作として送る

        // 接続異常やSteamオーバーレイをポーズ状態へ反映する
        if (getData().coop->Failed() || getData().coop->OverlayActive()) m_pauseMenuOpen = true;
        m_closeMenuButton->SetEnabled(!getData().coop->Failed());

        // Escapeでポーズメニューを切り替える
        if (!getData().coop->Failed() && !getData().coop->OverlayActive() && Input::GetKeyDown(KeyCode::Escape)) {
            m_pauseMenuOpen = !m_pauseMenuOpen;
            m_optionsOpen = false;
        }

        // ポーズ中のメニュー入力を処理する
        if (m_pauseMenuOpen && !getData().coop->OverlayActive()) ProcessPauseMenuInput();

        // ポーズ状態または通常操作を通信入力へ変換する
        if (m_pauseMenuOpen) {
            m_networkInput = {CooperativeInput::Pause, 0};
        } else {
            m_networkInput.held =
                (Input::GetKey(KeyCode::LeftArrow) || Input::GetKey(KeyCode::A) ? CooperativeInput::Left : 0) |
                (Input::GetKey(KeyCode::RightArrow) || Input::GetKey(KeyCode::D) ? CooperativeInput::Right : 0) |
                (Input::GetKey(KeyCode::UpArrow) || Input::GetKey(KeyCode::W) ? CooperativeInput::Up : 0) |
                (Input::GetKey(KeyCode::DownArrow) || Input::GetKey(KeyCode::S) ? CooperativeInput::Down : 0) |
                (Input::GetKey(KeyCode::LeftShift) || Input::GetKey(KeyCode::RightShift) ? CooperativeInput::Slow : 0) |
                (Input::GetKey(KeyCode::Z) || Input::GetKey(KeyCode::Space) ? CooperativeInput::Fire : 0);
            m_networkInput.pressed |=
                (Input::GetKeyDown(KeyCode::C) ? CooperativeInput::Bomb : 0) |
                (Input::GetKeyDown(KeyCode::X) ? CooperativeInput::View : 0) |
                (Input::GetKeyDown(KeyCode::Z) || Input::GetKeyDown(KeyCode::Space) ? CooperativeInput::Confirm : 0) |
                (Input::GetKeyDown(KeyCode::R) ? CooperativeInput::Restart : 0);
        }
        return;
    }
#endif
    // 切断中は進行を止め、再接続後も明示的な再開を待つ
    const bool connected = ControllersConnected();
    if (!connected) m_pauseMenuOpen = true;
    m_closeMenuButton->SetEnabled(connected);
    if (connected && (Input::GetKeyDown(KeyCode::Escape) ||
        (getData().playerCount == 2 && Input::GetGamepadKeyDown(1, KeyCode::Escape)))) {
        m_pauseMenuOpen = !m_pauseMenuOpen;
        m_optionsOpen = false;
    }

    if (m_pauseMenuOpen) {
        ProcessPauseMenuInput();
        // 再開フレームは切断前の操作を持ち越さず、現在の入力を読み直す
        if (!m_pauseMenuOpen) m_game->ProcessInput();
        return;
    }

    m_game->ProcessInput();
}

bool TestStage::ControllersConnected() const {
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    if (getData().onlineGame) return !getData().coop->Failed();
#endif
    return getData().playerCount != 2 ||
        (Input::IsGamepadConnected(0) && Input::IsGamepadConnected(1));
}

void TestStage::Tick() {
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    if (getData().onlineGame) {
        // 入力不足時はシミュレーションを進めず、ポーズ中も通信は継続する
        std::array<CooperativeInput, 2> inputs {};

        // 同期済み入力を取得する
        m_waitingForPeer = !getData().coop->Step(m_networkInput, inputs);
        if (m_waitingForPeer) return;

        // 相手のポーズ状態を確認する
        m_peerPaused = (inputs[getData().coop->IsHost() ? 1 : 0].held & CooperativeInput::Pause) != 0;
        if ((inputs[0].held | inputs[1].held) & CooperativeInput::Pause) return;

        // 同期済み入力でゲームを更新する
        m_game->ApplyNetworkInput(inputs);
    } else
#endif
    if (m_pauseMenuOpen) return;
    m_game->Tick();
    if (!m_game->IsAllStagesCleared()) return;

    // 最終クリア表示を見せてからスコアを保存し、エンディングへ遷移する
    if (++m_allClearTimer != FinalClearDisplayFrames) return;
    ScoreRepository {}.Save(getData().difficulty, m_game->Score(), getData().playerCount == 2);
#if defined(SPACEYAKUZA_EDITION_Online)
    // 協力スコアはホストだけ送信して二重登録を防ぐ
    if (!getData().onlineGame || getData().coop->IsHost())
        getData().coop->SubmitScore(static_cast<int>(getData().difficulty), m_game->Score(), getData().playerCount == 2);
#endif
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
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    if (getData().onlineGame && !m_pauseMenuOpen && (m_waitingForPeer || m_peerPaused)) {
        // 相手待ちまたは相手ポーズの状態を表示する
        renderer.DrawText(m_peerPaused ? "FRIEND PAUSED" : "WAITING FOR FRIEND",
            TextAlign::Center, 0.025f, ColorF::White());
    }
#endif
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
    m_closeMenuButton->SetOnClick([this]() { if (ControllersConnected()) m_pauseMenuOpen = false; });

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
        renderer.DrawText("PAUSED", TextAlign::Center, 0.04f, ColorF::White(), {0.0f, 0.42f}, 0.01f);
        if (!ControllersConnected())
            renderer.DrawText(
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
                getData().onlineGame ? getData().coop->Status() :
#endif
                "RECONNECT BOTH CONTROLLERS", TextAlign::Center,
                0.015f, {1.0f, 0.72f, 0.12f, 1.0f}, {0.0f, 0.29f}, 0.002f);
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
