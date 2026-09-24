#include "ModeSelectionScene.h"

#include <cmath>
#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Time/Time.h"
#include "../Common/SpaceBackground.h"
#include "../Gameplay/Models/AircraftModelView.h"
#include "../../Domain/ValueObjects/ResumeCode.h"

namespace {
constexpr float CharacterSpacing = 0.0035f;
constexpr ColorF PanelColor {0.035f, 0.075f, 0.14f, 0.94f};
constexpr ColorF BorderColor {0.16f, 0.68f, 0.92f, 1.0f};
constexpr ColorF ShotColor {1.0f, 0.82f, 0.18f, 1.0f};

struct PlayerPreviewContent {
    const char* name;
    const char* effect;
    const char* behavior;
    const char* bomb[2];
};

constexpr PlayerPreviewContent PreviewContents[] = {
    {"HOMING", "LOCKS ON TO THE NEAREST ENEMY", "CURVES TOWARD A MOVING TARGET",
        {"BOMB: 10 HEAVY HOMING MISSILES", "LAUNCH, FALL, SEEK / CLEAR SHOTS"}},
    {"PIERCING", "PASSES THROUGH MULTIPLE ENEMIES", "FLIES STRAIGHT THROUGH THE FORMATION",
        {"BOMB: NOSE-MOUNTED MEGA LASER", "WIDE FORWARD BEAM / CLEARS SHOTS"}},
    {"SPREAD", "HIGH DAMAGE AT CLOSE RANGE", "DAMAGE FALLS OFF WITH DISTANCE",
        {"BOMB: ONE-HIT SHIELD", "STAYS ACTIVE UNTIL HIT"}}
};

/**
 * @brief 中心座標と半サイズで枠線付き矩形を描画する
 * @param renderer 描画先
 * @param center 矩形中心
 * @param halfSize 矩形の半サイズ
 * @return なし
 */
void DrawPanel(Renderer& renderer, const Vector2& center, const Vector2& halfSize) {
    constexpr float Border = 0.006f;
    renderer.Draw(Rect {center, halfSize}, PanelColor);
    renderer.Draw(Rect {{center.x, center.y + halfSize.y}, {halfSize.x, Border}}, BorderColor);
    renderer.Draw(Rect {{center.x, center.y - halfSize.y}, {halfSize.x, Border}}, BorderColor);
    renderer.Draw(Rect {{center.x - halfSize.x, center.y}, {Border, halfSize.y}}, BorderColor);
    renderer.Draw(Rect {{center.x + halfSize.x, center.y}, {Border, halfSize.y}}, BorderColor);
}

/**
 * @brief 本編と同じ共有モデルでデモ用機体を描画する
 * @param renderer 描画先
 * @param camera デモ用カメラ
 * @param position 機体中心
 * @param player 自機ならtrue、敵機ならfalse
 * @param facesRight 右向きならtrue
 * @return なし
 */
void DrawDemoAircraft(Renderer& renderer, const Camera3D& camera, const Vector2& position,
    bool player, bool facesRight) {
    const float yaw = facesRight ? Math::HalfPi : -Math::HalfPi;
    const Vector3 worldPosition {position.x * camera.GetViewport().AspectRatio(), position.y, 0.0f};
    auto drawPart = [&](int shape, const Vector3& partPosition, const Vector3& partScale,
        const float color[4], float partYaw, float pitch) {
        const Matrix4x4 world = Matrix4x4::Translation(partPosition) *
            Matrix4x4::RotationY(partYaw) * Matrix4x4::RotationZ(pitch) * Matrix4x4::Scale(partScale);
        renderer.Draw({PrimitiveShapeFromLegacyIndex(shape),
            camera.ProjectionMatrix() * camera.ViewMatrix() * world, Vector3::One,
            {color[0], color[1], color[2], color[3]}, partYaw});
    };
    if (player) AircraftModelView::DrawPlayer(worldPosition, yaw, 0.22f, drawPart);
    else AircraftModelView::DrawEnemy(worldPosition, yaw, 0.18f, drawPart);
}

/**
 * @brief 選択中の機体タイプに対応する特殊弾デモを描画する
 * @param renderer 描画先
 * @param playerType デモ対象の機体タイプ
 * @param elapsedTime アニメーション時刻
 * @return なし
 */
void DrawWeaponDemo(Renderer& renderer, PlayerType playerType, float elapsedTime) {
    const float progress = std::fmod(elapsedTime * 0.38f, 1.0f);
    const float shotX = 0.18f + progress * 0.58f;

    // 本編と同じ機体モデルを画面正面のカメラで描画する
    Camera3D camera;
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Orthographic);
    camera.SetOrthographicHeight(2.0f);
    camera.SetPosition({0.0f, 0.0f, -5.0f});
    camera.LookAt(Vector3::Zero);
    renderer.SetCamera(camera);

    // 左側へ自機、右側へ標的を配置する
    DrawDemoAircraft(renderer, camera, {0.10f, -0.20f}, true, true);
    if (playerType == Piercing) {
        DrawDemoAircraft(renderer, camera, {0.46f, -0.20f}, false, false);
        DrawDemoAircraft(renderer, camera, {0.67f, -0.20f}, false, false);
        DrawDemoAircraft(renderer, camera, {0.85f, -0.20f}, false, false);
        renderer.ResetCamera();
        renderer.DrawPlayerShot({{shotX, -0.20f}, {0.060f, 0.026f}, 0.0f, elapsedTime * 60.0f,
            static_cast<int>(Piercing)});
        return;
    }

    if (playerType == Spread) {
        DrawDemoAircraft(renderer, camera, {0.79f, -0.20f}, false, false);
        DrawDemoAircraft(renderer, camera, {0.77f, -0.36f}, false, false);
        DrawDemoAircraft(renderer, camera, {0.77f, -0.04f}, false, false);
        renderer.ResetCamera();
        for (int lane = -1; lane <= 1; ++lane) {
            const float direction = static_cast<float>(lane) * Math::ToRadians(24.0f);
            renderer.DrawPlayerShot({{shotX, -0.20f + lane * progress * 0.18f}, {0.040f, 0.020f},
                direction, elapsedTime * 60.0f, static_cast<int>(Spread)});
        }
        return;
    }

    const float targetY = -0.08f + std::sin(elapsedTime * 1.6f) * 0.10f;
    DrawDemoAircraft(renderer, camera, {0.80f, targetY}, false, false);
    renderer.ResetCamera();
    const float curvedY = -0.20f + (targetY + 0.20f) * progress + std::sin(progress * 3.14159265f) * 0.10f;
    renderer.DrawPlayerShot({{shotX, curvedY}, {0.040f, 0.020f},
        std::atan2(targetY - curvedY, 0.80f - shotX), elapsedTime * 60.0f, static_cast<int>(Homing)});
}
}

/** @brief モードセレクトシーンを初期化する */
void ModeSelectionScene::Initialize() {
    // 人数を選び直してから各モードの開始条件を確認する
    m_stateController = std::make_unique<ModeSelectionStateController>();
    getData().playerCount = 1;
    getData().resumeCode.clear();
    m_resumeCode.clear();
    m_invalidResumeCode = false;
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    constexpr const char* playerCountLabels[] = {"1 PLAYER", "2 PLAYERS ONLINE"};
#else
    constexpr const char* playerCountLabels[] = {"1 PLAYER", "2 PLAYERS LOCAL"};
#endif
    for (int player = 0; player < 2; ++player) {
        m_playerCountButtons[player] = std::make_unique<Button>(
            Vector2 {0.48f, 0.12f}, RectAlign::Center,
            playerCountLabels[player], Vector2 {0.0f, 0.16f - player * 0.24f});
        m_playerCountButtons[player]->SetOnClick([this, count = player + 1]() {
            getData().playerCount = count;
            m_stateController = std::make_unique<ModeSelectionStateController>();
            m_stateController->SetCurrentState(count == 2 ?
                ModeSelectionState::ControllerSelect : ModeSelectionState::DifficultySelect);
        });
    }

#if defined(SPACEYAKUZA_EDITION_Steam)
    // Steam版の2人用は実機2台の割り当てを通らず招待ロビーへ進む
    getData().onlineGame = false;
    m_playerCountButtons[1]->SetOnClick([this] { changeScene(SceneType::SteamLobby); });
#elif defined(SPACEYAKUZA_EDITION_Online)
    getData().onlineGame = false;
    m_playerCountButtons[1]->SetOnClick([this] { changeScene(SceneType::OnlineLobby); });
#endif

    // 難易度選択ボタンを縦に配置する
    constexpr const char* difficultyLabels[] = { "EASY", "NORMAL", "HARD" };
    constexpr DifficultyType difficulties[] = { Easy, Normal, Hard };
    for (size_t i = 0; i < m_difficultyButtons.size(); ++i) {
        m_difficultyButtons[i] = std::make_unique<Button>(
            Vector2 { 0.40f, 0.12f }, RectAlign::Center,
            difficultyLabels[i], Vector2 { 0.0f, 0.20f - static_cast<float>(i) * 0.20f });
        m_difficultyButtons[i]->SetOnClick([this, difficulty = difficulties[i]]() {
            // 選択した難易度を共有データへ保存する
            getData().difficulty = difficulty;
            m_stateController->SetCurrentState(ModeSelectionState::PlayerTypeSelect);
        });
    }

    // プレイヤー機体選択ボタンを縦に配置する
    constexpr const char* playerTypeLabels[] = { "HOMING", "PIERCING", "SPREAD" };
    constexpr PlayerType playerTypes[] = { Homing, Piercing, Spread };
    for (size_t i = 0; i < m_playerTypeButtons.size(); ++i) {
        m_playerTypeButtons[i] = std::make_unique<Button>(
            Vector2 { 0.40f, 0.12f }, RectAlign::Center,
            playerTypeLabels[i], Vector2 { -0.62f, 0.20f - static_cast<float>(i) * 0.20f });
        m_playerTypeButtons[i]->SetOnClick([this, playerType = playerTypes[i]]() {
            // シングルプレイは機体選択後に再開コード入力へ進む
            getData().playerType = playerType;
            m_stateController->SetCurrentState(ModeSelectionState::ResumeCodeInput);
        });
    }

    // 空欄での通常開始とコードでの再開を分ける
    m_startButton = std::make_unique<Button>(Vector2 {0.46f, 0.10f}, RectAlign::Center,
        "START FROM BEGINNING", Vector2 {0.0f, -0.36f});
    m_startButton->SetOnClick([this] {
        getData().resumeCode.clear();
        changeScene(SceneType::Story);
    });
    m_resumeButton = std::make_unique<Button>(Vector2 {0.46f, 0.10f}, RectAlign::Center,
        "START FROM CODE", Vector2 {0.0f, -0.12f});
    m_resumeButton->SetOnClick([this] {
        ResumeCode code;
        m_invalidResumeCode = !ResumeCode::Parse(m_resumeCode, code);
        if (m_invalidResumeCode) return;
        getData().resumeCode = m_resumeCode;
        getData().difficulty = code.difficulty;
        getData().playerType = code.player;
        changeScene(SceneType::TestStage);
    });

    // 左下にタイトルシーンへ戻るボタンを配置する
    m_backButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.30f, 0.10f } }, "BACK");
    m_backButton->SetClickSound(Button::ClickSound::Cancel);
    m_backButton->SetOnClick([this]() {
        const auto state = m_stateController->GetCurrentState();
        if (state == ModeSelectionState::PlayerCountSelect) changeScene(SceneType::Title);
        else if (state == ModeSelectionState::ResumeCodeInput)
            m_stateController->SetCurrentState(ModeSelectionState::PlayerTypeSelect);
        else m_stateController->SetCurrentState(state == ModeSelectionState::PlayerTypeSelect ?
            ModeSelectionState::DifficultySelect : ModeSelectionState::PlayerCountSelect);
    });
}

/** @brief モードセレクト画面の入力を処理する */
void ModeSelectionScene::ProcessInput() {
    // 2人用の個別入力は共有マウス入力と分離する
    const auto state = m_stateController->GetCurrentState();
    if (state == ModeSelectionState::ResumeCodeInput) {
        // パッドの移動キーを文字に変換せず、キーボードとテンキーで入力する
        const auto append = [&](KeyCode key, char character) {
            if (Input::GetKeyDown(key) && !Input::GetGamepadKeyDown(0, key) &&
                m_resumeCode.size() < ResumeCode::MaxLength) {
                m_resumeCode += character;
                m_invalidResumeCode = false;
            }
        };
        for (int i = 0; i < 26; ++i) append(static_cast<KeyCode>(static_cast<int>(KeyCode::A) + i), static_cast<char>('A' + i));
        for (int i = 0; i < 10; ++i) {
            append(static_cast<KeyCode>(static_cast<int>(KeyCode::Alpha0) + i), static_cast<char>('0' + i));
            append(static_cast<KeyCode>(static_cast<int>(KeyCode::Numpad0) + i), static_cast<char>('0' + i));
        }
        append(KeyCode::Minus, '-');
        append(KeyCode::NumpadSubtract, '-');
        if (Input::GetKeyDown(KeyCode::Backspace) && !m_resumeCode.empty()) {
            m_resumeCode.pop_back();
            m_invalidResumeCode = false;
        }
    }
    if (getData().playerCount == 2 && state != ModeSelectionState::PlayerCountSelect) {
        UpdateCooperativeInput();
        if (m_stateController->ArePlayersReady()) {
            getData().playerType = m_stateController->GetPlayerType(0);
            getData().secondPlayerType = m_stateController->GetPlayerType(1);
            changeScene(SceneType::Story);
            return;
        }
        // 決定の同じフレームを次の選択段階へ持ち越さない
        if (state != m_stateController->GetCurrentState()) return;
    }

    // 現在のクライアント領域からUI入力座標を取得する
    int width = 1280;
    int height = 720;
    if (const HWND hwnd = GetForegroundWindow()) {
        RECT clientRect {};
        if (GetClientRect(hwnd, &clientRect)) {
            width = clientRect.right > 0 ? clientRect.right : width;
            height = clientRect.bottom > 0 ? clientRect.bottom : height;
        }
    }

    UpdateActiveButtons(UIInput::Current(width, height));
    UpdatePlayerPreview();
}

/** @brief 現在表示中の選択肢だけ入力を受け付ける */
void ModeSelectionScene::UpdateActiveButtons(const UIInputState& inputState) {
    const auto state = m_stateController->GetCurrentState();
    // 個別選択の決定ボタンを共有カーソルのBACKクリックとして扱わない
    UIInputState backInput = inputState;
    if (getData().playerCount == 2 &&
        (state == ModeSelectionState::ControllerSelect || state == ModeSelectionState::PlayerTypeSelect) &&
        (Input::GetGamepadKey(0, KeyCode::Space) || Input::GetGamepadKeyUp(0, KeyCode::Space))) {
        backInput.primaryDown = backInput.primaryPressed = backInput.primaryReleased = false;
    }
    m_backButton->Update(backInput);
    if (state != m_stateController->GetCurrentState()) return;

    if (state == ModeSelectionState::PlayerCountSelect) {
        for (const auto& button : m_playerCountButtons) button->Update(inputState);
        return;
    }
    if (state == ModeSelectionState::DifficultySelect) {
        for (const auto& button : m_difficultyButtons) button->Update(inputState);
        return;
    }
    if (state == ModeSelectionState::ResumeCodeInput) {
        m_resumeButton->Update(inputState);
        m_startButton->Update(inputState);
        return;
    }

    // 2人用のショット選択は各自のコントローラーだけが変更できる
    if (getData().playerCount == 2 || state != ModeSelectionState::PlayerTypeSelect) return;
    for (const auto& button : m_playerTypeButtons) button->Update(inputState);
}

void ModeSelectionScene::UpdateCooperativeInput() {
    for (int player = 0; player < 2; ++player) {
        const bool previous = Input::GetGamepadKeyDown(player, KeyCode::UpArrow) ||
            Input::GetGamepadKeyDown(player, KeyCode::W);
        const bool next = Input::GetGamepadKeyDown(player, KeyCode::DownArrow) ||
            Input::GetGamepadKeyDown(player, KeyCode::S);
        m_stateController->UpdatePlayerInput(player, Input::IsGamepadConnected(player),
            static_cast<int>(next) - static_cast<int>(previous),
            Input::GetGamepadKeyDown(player, KeyCode::Space),
            Input::GetGamepadKeyDown(player, KeyCode::Escape));
    }
}

void ModeSelectionScene::UpdatePlayerPreview() {
    if (m_stateController->GetCurrentState() != ModeSelectionState::PlayerTypeSelect) return;

    // ホバーが外れた時は直前に確認していた機体の説明を維持する
    constexpr PlayerType playerTypes[] = { Homing, Piercing, Spread };
    for (size_t i = 0; i < m_playerTypeButtons.size(); ++i) {
        if (m_playerTypeButtons[i]->IsHovered()) m_previewPlayerType = playerTypes[i];
    }
}

/** @brief モードセレクト画面の固定更新を行う */
void ModeSelectionScene::Tick() {
}

/** @brief モードセレクト画面が保持するUIを解放する */
void ModeSelectionScene::Dispose() {
    for (auto& button : m_playerCountButtons) button.reset();
    for (auto& button : m_difficultyButtons) button.reset();
    for (auto& button : m_playerTypeButtons) button.reset();
    m_backButton.reset();
    m_resumeButton.reset();
    m_startButton.reset();
    m_stateController.reset();
}

/** @brief 現在の選択段階に対応するUIを描画する */
void ModeSelectionScene::Render(Renderer& renderer) {
    const auto state = m_stateController->GetCurrentState();
    const bool selectingDifficulty =
        state == ModeSelectionState::DifficultySelect;

    // ゆっくり明滅する星空を選択UIの背面へ描画する
    SpaceBackground::Render(renderer, Time::unscaledTime);

    // 画面上部へ現在の選択内容を表示する
    renderer.DrawText(
        state == ModeSelectionState::PlayerCountSelect ? "SELECT PLAYERS" :
        state == ModeSelectionState::ControllerSelect ? "ASSIGN CONTROLLERS" :
        state == ModeSelectionState::ResumeCodeInput ? "RESUME CODE" :
        selectingDifficulty ? "SELECT DIFFICULTY" : "SELECT SHOT TYPE",
        TextAlign::TopCenter,
        0.025f,
        ColorF::White(),
        { 0.0f, -0.18f }, CharacterSpacing);

    if (state == ModeSelectionState::PlayerCountSelect) {
        for (const auto& button : m_playerCountButtons) button->Render(renderer);
#if defined(SPACEYAKUZA_EDITION_Steam)
        renderer.DrawText("ONLINE CO-OP WITH A STEAM FRIEND", TextAlign::Center,
#else
        renderer.DrawText("LOCAL CO-OP REQUIRES TWO CONTROLLERS", TextAlign::Center,
#endif
            0.014f, ColorF::White(), {0.0f, -0.40f}, CharacterSpacing);
    } else if (state == ModeSelectionState::ResumeCodeInput) {
        DrawPanel(renderer, {0.0f, 0.36f}, {0.92f, 0.12f});
        renderer.DrawText(m_resumeCode.empty() ? "TYPE CODE HERE" : m_resumeCode,
            TextAlign::Center, 0.012f, ShotColor, {0.0f, 0.36f}, 0.001f);
        renderer.DrawText("KEYBOARD: A-Z / 0-9 / -    BACKSPACE: DELETE", TextAlign::Center,
            0.012f, ColorF::White(), {0.0f, 0.16f}, 0.001f);
        renderer.DrawText(m_invalidResumeCode ? "INVALID CODE - CHECK ALL CHARACTERS" :
            "CODE OVERRIDES DIFFICULTY AND SHOT TYPE", TextAlign::Center,
            0.012f, m_invalidResumeCode ? ColorF {1.0f, 0.35f, 0.25f, 1.0f} : ColorF::White(),
            {0.0f, 0.02f}, 0.001f);
        m_resumeButton->Render(renderer);
        m_startButton->Render(renderer);
    } else if (selectingDifficulty) {
        for (const auto& button : m_difficultyButtons) button->Render(renderer);
    } else if (getData().playerCount == 2) {
        RenderCooperativeSelection(renderer);
    } else {
        for (const auto& button : m_playerTypeButtons) button->Render(renderer);

        // 右パネル上部へ効果、下部へ実際の挙動デモを表示する
        const auto& content = PreviewContents[static_cast<size_t>(m_previewPlayerType)];
        DrawPanel(renderer, {0.47f, -0.08f}, {0.49f, 0.63f});
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
        // 翻訳で説明が長くなっても右側のパネル幅に収める
        renderer.DrawText(content.name, TextAlign::Center, 0.026f, ShotColor, {0.47f, 0.43f}, CharacterSpacing);
        renderer.DrawText("EFFECT", TextAlign::Center, 0.012f, BorderColor, {0.47f, 0.32f}, CharacterSpacing);
        renderer.DrawText(content.effect, TextAlign::Center, 0.012f, ColorF::White(), {0.47f, 0.22f}, CharacterSpacing);
        renderer.Draw(Rect {{0.47f, 0.10f}, {0.45f, 0.003f}}, BorderColor);
        renderer.DrawText("BEHAVIOR", TextAlign::Center, 0.012f, BorderColor, {0.47f, 0.01f}, CharacterSpacing);
        renderer.DrawText(content.behavior, TextAlign::Center, 0.010f, ColorF::White(), {0.47f, -0.45f}, CharacterSpacing);
        for (int line = 0; line < 2; ++line)
            renderer.DrawText(content.bomb[line], TextAlign::Center,
                0.010f, line == 0 ? ShotColor : ColorF::White(), {0.47f, -0.56f - line * 0.09f}, CharacterSpacing);
#else
        renderer.DrawText(content.name, {0.05f, 0.43f}, 0.026f, ShotColor, CharacterSpacing);
        renderer.DrawText("EFFECT", {0.05f, 0.32f}, 0.012f, BorderColor, CharacterSpacing);
        renderer.DrawText(content.effect, {0.05f, 0.22f}, 0.012f, ColorF::White(), CharacterSpacing);
        renderer.Draw(Rect {{0.47f, 0.10f}, {0.45f, 0.003f}}, BorderColor);
        renderer.DrawText("BEHAVIOR", {0.05f, 0.01f}, 0.012f, BorderColor, CharacterSpacing);
        renderer.DrawText(content.behavior, {0.05f, -0.45f}, 0.010f, ColorF::White(), CharacterSpacing);
        // 自機紹介の末尾に機体固有のボムと防御性能を示す
        for (int line = 0; line < 2; ++line)
            renderer.DrawText(content.bomb[line], {0.05f, -0.56f - line * 0.09f},
                0.010f, line == 0 ? ShotColor : ColorF::White(), CharacterSpacing);
#endif
        DrawWeaponDemo(renderer, m_previewPlayerType, Time::unscaledTime);
    }

    m_backButton->Render(renderer);
}

void ModeSelectionScene::RenderCooperativeSelection(Renderer& renderer) const {
    const bool assigning = m_stateController->GetCurrentState() == ModeSelectionState::ControllerSelect;
    constexpr ColorF playerColors[] = {{0.18f, 0.58f, 1.0f, 1.0f}, {0.20f, 0.95f, 0.38f, 1.0f}};

    // コントローラー番号と担当色を並べ、各自の決定状態を表示する
    for (int player = 0; player < 2; ++player) {
        const float x = player == 0 ? -0.50f : 0.50f;
        DrawPanel(renderer, {x, -0.07f}, {0.45f, 0.66f});
        renderer.DrawText(player == 0 ? "1P BLUE" : "2P GREEN", TextAlign::Center,
            0.025f, playerColors[player], {x, 0.47f}, CharacterSpacing);
        if (assigning) {
            const char* status = !Input::IsGamepadConnected(player) ? "CONNECT CONTROLLER" :
                m_stateController->IsControllerJoined(player) ? "READY" : "PRESS FIRE TO JOIN";
            renderer.DrawText(status, TextAlign::Center, 0.018f, ColorF::White(), {x, 0.06f}, CharacterSpacing);
            continue;
        }

        // 同じショットを選択できる独立した一覧を描画する
        const int selected = static_cast<int>(m_stateController->GetPlayerType(player));
        for (int shot = 0; shot < 3; ++shot) {
            const float y = 0.24f - shot * 0.20f;
            if (shot == selected)
                renderer.Draw(Rect {{x, y}, {0.32f, 0.074f}}, playerColors[player]);
            renderer.DrawText(PreviewContents[shot].name, TextAlign::Center,
                0.020f, ColorF::White(), {x, y}, CharacterSpacing);
        }
        renderer.DrawText(PreviewContents[selected].effect, TextAlign::Center,
            0.010f, ColorF::White(), {x, -0.34f}, 0.001f);
        // 協力プレイでも各自が選んだ機体のボムを確認できる
        for (int line = 0; line < 2; ++line)
            renderer.DrawText(PreviewContents[selected].bomb[line], TextAlign::Center,
                0.010f, line == 0 ? ShotColor : ColorF::White(), {x, -0.44f - line * 0.09f}, 0.001f);
        renderer.DrawText(m_stateController->IsPlayerReady(player) ? "READY" : "PRESS FIRE TO CONFIRM",
            TextAlign::Center, 0.014f, playerColors[player], {x, -0.65f}, CharacterSpacing);
    }
    renderer.DrawText(assigning ? "CONFIRM ON EACH CONTROLLER" : "UP / DOWN: SELECT    FIRE: READY    PAUSE: CANCEL",
        TextAlign::Center, 0.012f, ColorF::White(), {0.0f, -0.77f}, CharacterSpacing);
}
