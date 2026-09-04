#include "ModeSelectionScene.h"

#include <cmath>
#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Time/Time.h"
#include "../Common/SpaceBackground.h"
#include "../Gameplay/Models/AircraftModelView.h"

namespace {
constexpr float CharacterSpacing = 0.0035f;
constexpr ColorF PanelColor {0.035f, 0.075f, 0.14f, 0.94f};
constexpr ColorF BorderColor {0.16f, 0.68f, 0.92f, 1.0f};
constexpr ColorF ShotColor {1.0f, 0.82f, 0.18f, 1.0f};

struct PlayerPreviewContent {
    const char* name;
    const char* effect;
    const char* behavior;
};

constexpr PlayerPreviewContent PreviewContents[] = {
    {"HOMING", "LOCKS ON TO THE NEAREST ENEMY", "CURVES TOWARD A MOVING TARGET"},
    {"PIERCING", "PASSES THROUGH MULTIPLE ENEMIES", "FLIES STRAIGHT THROUGH THE FORMATION"},
    {"SPREAD", "FIRES THREE SHOTS AT ONCE", "COVERS A WIDE AREA ABOVE AND BELOW"}
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
    // 選択段階を難易度選択へ初期化する
    m_stateController = std::make_unique<ModeSelectionStateController>();

    // 難易度選択ボタンを縦に配置する
    constexpr const char* difficultyLabels[] = { "EASY", "NORMAL", "HARD" };
    constexpr DifficultyType difficulties[] = { Easy, Normal, Hard };
    for (size_t i = 0; i < m_difficultyButtons.size(); ++i) {
        m_difficultyButtons[i] = std::make_unique<Button>(
            Vector2 { 0.40f, 0.12f }, RectAlign::Center,
            difficultyLabels[i], Vector2 { 0.0f, 0.20f - static_cast<float>(i) * 0.20f });
        m_difficultyButtons[i]->SetOnClick([this, difficulty = difficulties[i]]() {
            /** @brief 選択した難易度を共有データへ保存する */
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
            // 選択した機体タイプを共有データへ保存してゲームを開始する
            getData().playerType = playerType;
            changeScene(SceneType::Story);
        });
    }

    // 左下にタイトルシーンへ戻るボタンを配置する
    m_backButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.30f, 0.10f } }, "BACK");
    m_backButton->SetClickSound(Button::ClickSound::Cancel);
    m_backButton->SetOnClick([this]() { changeScene(SceneType::Title); });
}

/** @brief モードセレクト画面の入力を処理する */
void ModeSelectionScene::ProcessInput() {
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
    m_backButton->Update(inputState);

    if (m_stateController->GetCurrentState() == ModeSelectionState::DifficultySelect) {
        for (const auto& button : m_difficultyButtons) button->Update(inputState);
        return;
    }

    for (const auto& button : m_playerTypeButtons) button->Update(inputState);
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
    for (auto& button : m_difficultyButtons) button.reset();
    for (auto& button : m_playerTypeButtons) button.reset();
    m_backButton.reset();
    m_stateController.reset();
}

/** @brief 現在の選択段階に対応するUIを描画する */
void ModeSelectionScene::Render(Renderer& renderer) {
    const bool selectingDifficulty =
        m_stateController->GetCurrentState() == ModeSelectionState::DifficultySelect;

    // ゆっくり明滅する星空を選択UIの背面へ描画する
    SpaceBackground::Render(renderer, Time::unscaledTime);

    // 画面上部へ現在の選択内容を表示する
    renderer.DrawText(
        selectingDifficulty ? "SELECT DIFFICULTY" : "SELECT PLAYER TYPE",
        TextAlign::TopCenter,
        0.025f,
        ColorF::White(),
        { 0.0f, -0.18f }, CharacterSpacing);

    if (selectingDifficulty) {
        for (const auto& button : m_difficultyButtons) button->Render(renderer);
    } else {
        for (const auto& button : m_playerTypeButtons) button->Render(renderer);

        // 右パネル上部へ効果、下部へ実際の挙動デモを表示する
        const auto& content = PreviewContents[static_cast<size_t>(m_previewPlayerType)];
        DrawPanel(renderer, {0.47f, -0.08f}, {0.49f, 0.63f});
        renderer.DrawText(content.name, {0.05f, 0.43f}, 0.026f, ShotColor, CharacterSpacing);
        renderer.DrawText("EFFECT", {0.05f, 0.32f}, 0.012f, BorderColor, CharacterSpacing);
        renderer.DrawText(content.effect, {0.05f, 0.22f}, 0.012f, ColorF::White(), CharacterSpacing);
        renderer.Draw(Rect {{0.47f, 0.10f}, {0.45f, 0.003f}}, BorderColor);
        renderer.DrawText("BEHAVIOR", {0.05f, 0.01f}, 0.012f, BorderColor, CharacterSpacing);
        renderer.DrawText(content.behavior, {0.05f, -0.48f}, 0.010f, ColorF::White(), CharacterSpacing);
        DrawWeaponDemo(renderer, m_previewPlayerType, Time::unscaledTime);
    }

    m_backButton->Render(renderer);
}
