#include "SteamLobbyScene.h"
#include "../../Application/UseCases/Localization.h"
#include "../../Infrastructure/ExternalServices/SteamCoopSession.h"
#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Time/Time.h"
#include "../Common/SpaceBackground.h"
#include <windows.h>
#include <algorithm>
#include <string>

/** @brief ロビーとボタンを初期化する @return なし */
void SteamLobbyScene::Initialize() {
    auto& session = *getData().coop;

    // ロビー接続と画面の基本状態を初期化する
    getData().onlineGame = false;
    session.OpenLobby();
    const char* labels[] = {"INVITE FRIEND", "DIFFICULTY", "MY SHOT", "READY", "START GAME", "BACK TO TITLE"};
    // 操作ボタンを配置する
    for (int i = 0; i < 6; ++i) {
        m_buttons[i] = std::make_unique<Button>(Vector2 {0.44f, 0.09f}, RectAlign::Center,
            labels[i], Vector2 {0.0f, 0.20f - i * 0.17f});
    }
    // 各ボタンへSteamロビー操作を割り当てる
    m_buttons[0]->SetOnClick([&session] { session.Invite(); });
    m_buttons[1]->SetOnClick([&session] { session.CycleDifficulty(); });
    m_buttons[2]->SetOnClick([&session] { session.CycleShot(); });
    m_buttons[3]->SetOnClick([&session] { session.ToggleReady(); });
    m_buttons[4]->SetOnClick([&session] { session.Start(); });
    m_buttons[5]->SetOnClick([this] { getData().coop->Leave(); changeScene(SceneType::Title); });
}

/** @brief ロビー操作を処理する @return なし */
void SteamLobbyScene::ProcessInput() {
    auto& session = *getData().coop;

    // Steamオーバーレイ表示中はゲーム入力を止める
    if (session.OverlayActive()) return;

    // 現在のロビー状態から操作可否を決める
    const bool editable = session.InLobby() && !session.Failed() && !session.InGame();
    m_buttons[0]->SetEnabled(editable && session.IsHost());
    m_buttons[1]->SetEnabled(editable && session.IsHost() && !session.Ready(0) && !session.Ready(1));
    m_buttons[2]->SetEnabled(editable);
    m_buttons[3]->SetEnabled(editable);
    m_buttons[4]->SetEnabled(session.IsHost() && session.CanStart());
    const char* difficulties[] = {"EASY", "NORMAL", "HARD"};
    const char* shots[] = {"HOMING", "PIERCING", "SPREAD"};
    m_buttons[1]->SetText(std::string(Localization::Text("DIFFICULTY: ")) + Localization::Text(difficulties[(std::clamp)(session.Difficulty(), 0, 2)]));
    m_buttons[2]->SetText(std::string(Localization::Text("MY SHOT: ")) + Localization::Text(shots[(std::clamp)(session.Shot(session.IsHost() ? 0 : 1), 0, 2)]));
    m_buttons[3]->SetText(session.Ready(session.IsHost() ? 0 : 1) ? "CANCEL READY" : "READY");

    // 共通UI入力で各ボタンを更新する
    RECT rect {0, 0, 1280, 720};
    if (const auto window = GetForegroundWindow()) GetClientRect(window, &rect);
    const auto input = UIInput::Current((std::max)(1L, rect.right), (std::max)(1L, rect.bottom));
    for (auto& button : m_buttons) button->Update(input);
}

/** @brief 同じ開始設定でゲームへ遷移する @return なし */
void SteamLobbyScene::Tick() {
    auto& session = *getData().coop;

    // 接続完了時だけゲーム開始設定を共有する
    if (!session.InGame() || session.Failed()) return;

    // 2人プレイ設定を次のシーンへ渡す
    getData().onlineGame = true;
    getData().playerCount = 2;
    getData().difficulty = static_cast<DifficultyType>(session.Difficulty());
    getData().playerType = static_cast<PlayerType>(session.Shot(0));
    getData().secondPlayerType = static_cast<PlayerType>(session.Shot(1));
    changeScene(SceneType::TestStage);
}

/** @brief 未開始ロビーを解放する @return なし */
void SteamLobbyScene::Dispose() {
    // ゲーム開始前のロビーだけ終了する
    if (!getData().coop->InGame()) getData().coop->Leave();

    // ボタンを解放する
    for (auto& button : m_buttons) button.reset();
}

/** @brief 接続状態と準備操作を描画する @param renderer 描画先 @return なし */
void SteamLobbyScene::Render(Renderer& renderer) {
    auto& session = *getData().coop;

    // 背景とロビー状態を描画する
    SpaceBackground::Render(renderer, Time::unscaledTime);
    renderer.DrawText("STEAM ONLINE CO-OP", TextAlign::TopCenter, 0.028f,
        ColorF::White(), {0.0f, -0.16f});
    renderer.DrawText(session.Status(), TextAlign::Center, 0.012f,
        ColorF::White(), {0.0f, 0.60f});
    renderer.DrawText(!session.HasPlayer(0) ? "1P HOST: NOT CONNECTED" : session.Ready(0) ? "1P HOST: READY" : "1P HOST: NOT READY",
        TextAlign::Center, 0.015f, ColorF::White(), {-0.45f, 0.40f});
    renderer.DrawText(!session.HasPlayer(1) ? "2P FRIEND: NOT CONNECTED" : session.Ready(1) ? "2P FRIEND: READY" : "2P FRIEND: NOT READY",
        TextAlign::Center, 0.015f, ColorF::White(), {0.45f, 0.40f});
    // ロビー操作ボタンを描画する
    for (const auto& button : m_buttons) button->Render(renderer);
}
