#include "OnlineLobbyScene.h"
#include "../../Infrastructure/ExternalServices/OnlineCoopSession.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"
#include "../../Engine/Time/Time.h"
#include "../Common/SpaceBackground.h"
#include <windows.h>
#include <algorithm>

/** @brief ロビー操作を初期化する @return なし */
void OnlineLobbyScene::Initialize() {
    auto& session = *getData().coop;

    // 保存設定とロビー画面の基本状態を読み込む
    getData().onlineGame = false;
    m_automatic = SettingsRepository {}.Load().automaticMatchmaking;
    const char* labels[] = {"AUTO MATCH: ON", "FIND PARTNER", "JOIN ROOM", "CANCEL / LEAVE",
        "DIFFICULTY", "MY SHOT", "READY", "START GAME", "BACK TO TITLE"};
    // 基本操作ボタンを配置する
    for (int i = 0; i < 9; ++i) {
        const Vector2 position = i == 8 ? Vector2 {0.0f, -0.84f} :
            Vector2 {i < 4 ? -0.50f : 0.50f, 0.34f - (i % 4) * 0.19f};
        m_buttons[i] = std::make_unique<Button>(Vector2 {0.43f, 0.08f}, RectAlign::Center, labels[i], position);
    }
    // 基本操作ボタンへロビー操作を割り当てる
    m_buttons[0]->SetOnClick([this, &session] {
        session.Leave();
        m_automatic = !m_automatic;
        auto settings = SettingsRepository {}.Load();
        settings.automaticMatchmaking = m_automatic;
        SettingsRepository {}.Save(settings);
        if (m_automatic) session.OpenLobby(true);
    });
    m_buttons[1]->SetOnClick([this, &session] { session.OpenLobby(m_automatic); });
    m_buttons[2]->SetOnClick([this, &session] { session.OpenLobby(false, m_code); });
    m_buttons[3]->SetOnClick([&session] { session.Leave(); });
    m_buttons[4]->SetOnClick([&session] { session.CycleDifficulty(); });
    m_buttons[5]->SetOnClick([&session] { session.CycleShot(); });
    m_buttons[6]->SetOnClick([&session] { session.ToggleReady(); });
    m_buttons[7]->SetOnClick([&session] { session.Start(); });
    m_buttons[8]->SetOnClick([this, &session] { session.Leave(); changeScene(SceneType::Title); });
    // 数字ボタンでもコードを入力できるようにする
    for (int i = 0; i < 6; ++i) {
        m_buttons[9 + i] = std::make_unique<Button>(Vector2 {0.055f, 0.065f}, RectAlign::Center,
            "0", Vector2 {-0.375f + i * 0.15f, -0.54f});
        m_buttons[9 + i]->SetOnClick([this, i] {
            m_code.resize(6, '0');
            m_code[i] = static_cast<char>('0' + (m_code[i] - '0' + 1) % 10);
        });
    }
    // 自動マッチング設定なら初期接続を開始する
    if (m_automatic) session.OpenLobby(true);
}

/** @brief 入力とボタン状態を処理する @return なし */
void OnlineLobbyScene::ProcessInput() {
    auto& session = *getData().coop;

    // Escapeでロビーを終了してタイトルへ戻る
    if (Input::GetKeyDown(KeyCode::Escape)) {
        session.Leave(); changeScene(SceneType::Title); return;
    }

    // 現在のロビー状態から操作可否を決める
    const bool idle = !session.Active() || session.Failed();
    const bool editable = session.InLobby() && !session.Failed() && !session.InGame();
    m_buttons[0]->SetText(m_automatic ? "AUTO MATCH: ON" : "AUTO MATCH: OFF");
    m_buttons[0]->SetEnabled(!session.InGame());
    m_buttons[1]->SetText(m_automatic ? "FIND PARTNER" : "CREATE PRIVATE ROOM");
    m_buttons[1]->SetEnabled(idle);
    m_buttons[2]->SetEnabled(idle && !m_automatic && m_code.size() == 6);
    m_buttons[3]->SetEnabled(session.Active());
    m_buttons[4]->SetEnabled(editable && session.IsHost() && !session.Ready(0) && !session.Ready(1));
    m_buttons[5]->SetEnabled(editable);
    m_buttons[6]->SetEnabled(editable);
    m_buttons[7]->SetEnabled(session.IsHost() && session.CanStart());
    const char* difficulties[] = {"EASY", "NORMAL", "HARD"};
    const char* shots[] = {"HOMING", "PIERCING", "SPREAD"};
    m_buttons[4]->SetText(std::string("DIFFICULTY: ") + difficulties[session.Difficulty()]);
    m_buttons[5]->SetText(std::string("MY SHOT: ") + shots[session.Shot(session.IsHost() ? 0 : 1)]);
    m_buttons[6]->SetText(session.Ready(session.IsHost() ? 0 : 1) ? "CANCEL READY" : "READY");

    // キーボード入力で部屋コードを編集する
    if (idle && !m_automatic) {
        for (int digit = 0; digit < 10; ++digit) {
            if (m_code.size() < 6 && (Input::GetKeyDown(static_cast<KeyCode>(static_cast<int>(KeyCode::Alpha0) + digit)) ||
                Input::GetKeyDown(static_cast<KeyCode>(static_cast<int>(KeyCode::Numpad0) + digit))))
                m_code += static_cast<char>('0' + digit);
        }
        if (Input::GetKeyDown(KeyCode::Backspace) && !m_code.empty()) m_code.pop_back();
    }

    // 部屋コードの数字ボタンを更新する
    for (int i = 0; i < 6; ++i) {
        m_buttons[9 + i]->SetEnabled(idle && !m_automatic);
        m_buttons[9 + i]->SetText(i < static_cast<int>(m_code.size()) ? std::string(1, m_code[i]) : "-");
    }

    // 共通UI入力で各ボタンを更新する
    RECT rect {0, 0, 1280, 720};
    if (const auto window = GetForegroundWindow()) GetClientRect(window, &rect);
    const auto input = UIInput::Current((std::max)(1L, rect.right), (std::max)(1L, rect.bottom));
    for (auto& button : m_buttons) button->Update(input);
}

/** @brief 同期設定でゲームへ移動する @return なし */
void OnlineLobbyScene::Tick() {
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

/** @brief 未開始接続と画面を解放する @return なし */
void OnlineLobbyScene::Dispose() {
    // ゲーム開始前の接続だけ終了する
    if (!getData().coop->InGame()) getData().coop->Leave();

    // ボタンを解放する
    for (auto& button : m_buttons) button.reset();
}

/** @brief 部屋情報と操作を描画する @param renderer 描画先 @return なし */
void OnlineLobbyScene::Render(Renderer& renderer) {
    const auto& session = *getData().coop;

    // 背景と接続状態を描画する
    SpaceBackground::Render(renderer, Time::unscaledTime);
    renderer.DrawText("ONLINE CO-OP", TextAlign::Center, 0.035f, ColorF::White(), {0.0f, 0.86f});
    renderer.DrawText(session.Status(), TextAlign::Center, 0.012f, ColorF::White(), {0.0f, 0.70f});
    const std::string room = session.InLobby() ? "ROOM " + session.RoomCode() + (session.IsHost() ? "  YOU: 1P HOST" : "  YOU: 2P") : "NO ROOM CONNECTED";
    renderer.DrawText(room, TextAlign::Center, 0.018f, ColorF::White(), {0.0f, 0.56f});
    renderer.DrawText(session.Ready(0) ? "1P: READY" : "1P: NOT READY", TextAlign::Center, 0.013f, ColorF::White(), {-0.5f, -0.37f});
    renderer.DrawText(session.Ready(1) ? "2P: READY" : "2P: NOT READY", TextAlign::Center, 0.013f, ColorF::White(), {0.5f, -0.37f});
    renderer.DrawText("ROOM CODE: TYPE 6 DIGITS / BACKSPACE / CLICK DIGITS", TextAlign::Center, 0.010f, ColorF::White(), {0.0f, -0.68f});
    // ロビー操作ボタンを描画する
    for (const auto& button : m_buttons) button->Render(renderer);
}
