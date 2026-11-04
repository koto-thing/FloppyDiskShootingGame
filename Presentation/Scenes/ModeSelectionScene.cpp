#include "ModeSelectionScene.h"

#include <algorithm>
#include <cmath>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Time/Time.h"
#include "../Common/SpaceBackground.h"
#include "../Gameplay/Models/AircraftModelView.h"
#include "../Gameplay/SideScrollingShooter.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"

namespace {
constexpr float CharacterSpacing = 0.0035f;
constexpr ColorF PanelColor {0.035f, 0.075f, 0.14f, 0.94f};
constexpr ColorF BorderColor {0.16f, 0.68f, 0.92f, 1.0f};
constexpr ColorF ShotColor {1.0f, 0.82f, 0.18f, 1.0f};

constexpr const char* ShotNames[] = {"HOMING", "PIERCING", "SPREAD"};

/** @brief 図解の局所座標を画面とワールドへ写す描画文脈 */
struct PreviewView {
    Renderer& renderer;
    Camera3D camera;
    Vector2 center;
    float scale;
    float time;

    /**
     * @brief 局所座標をNDC座標へ変換する
     * @param local 図解の局所座標
     * @return NDC座標
     */
    Vector2 Screen(const Vector2& local) const {
        return {center.x + local.x * scale, center.y + local.y * scale};
    }

    /**
     * @brief 局所座標をデモ用カメラのワールド座標へ変換する
     * @param local 図解の局所座標
     * @param z ワールドZ、小さいほど手前
     * @return ワールド座標
     */
    Vector3 World(const Vector2& local, float z = 0.0f) const {
        const Vector2 screen = Screen(local);
        return {screen.x * camera.GetViewport().AspectRatio(), screen.y, z};
    }

    /**
     * @brief ワールド行列付きの3Dプリミティブを描画する
     * @param shape 形状
     * @param world ワールド行列
     * @param color 色
     * @return なし
     */
    void DrawPart(PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) const {
        renderer.Draw({shape, camera.ProjectionMatrix() * camera.ViewMatrix() * world,
            Vector3::One, color, 0.0f});
    }
};

/**
 * @brief 本編と同じ正面カメラを持つ描画文脈を開始する
 * @param renderer 描画先
 * @param center 図解の中心
 * @param scale 図解の拡大率
 * @param time アニメーション時刻
 * @return 描画文脈
 */
PreviewView BeginPreview(Renderer& renderer, const Vector2& center, float scale, float time) {
    Camera3D camera;
    camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    camera.SetProjectionMode(ProjectionMode::Orthographic);
    camera.SetOrthographicHeight(2.0f);
    camera.SetPosition({0.0f, 0.0f, -5.0f});
    camera.LookAt(Vector3::Zero);
    renderer.SetCamera(camera);
    return {renderer, camera, center, scale, time};
}

/**
 * @brief 共有モデルで右向きの自機または左向きの敵機を描画する
 * @param view 描画文脈
 * @param local 機体中心
 * @param player 自機ならtrue、敵機ならfalse
 * @param modelScale 機体だけに掛ける追加の拡大率
 * @return なし
 */
void DrawPreviewAircraft(const PreviewView& view, const Vector2& local, bool player, float modelScale = 1.0f) {
    auto drawPart = [&](int shape, const Vector3& partPosition, const Vector3& partScale,
        const float color[4], float partYaw, float pitch) {
        view.DrawPart(PrimitiveShapeFromLegacyIndex(shape), Matrix4x4::Translation(partPosition) *
            Matrix4x4::RotationY(partYaw) * Matrix4x4::RotationZ(pitch) * Matrix4x4::Scale(partScale),
            {color[0], color[1], color[2], color[3]});
    };
    if (player) AircraftModelView::DrawPlayer(view.World(local), Math::HalfPi, 0.22f * view.scale * modelScale, drawPart);
    else AircraftModelView::DrawEnemy(view.World(local), -Math::HalfPi, 0.18f * view.scale * modelScale, drawPart);
}

/**
 * @brief 本編の爆発シェーダーで命中を描画する
 * @param view 描画文脈
 * @param local 爆発中心
 * @param size 爆発の基準サイズ
 * @param age 0から1の進行度、範囲外は描画しない
 * @return なし
 */
void DrawPreviewBurst(const PreviewView& view, const Vector2& local, float size, float age) {
    if (age < 0.0f || age >= 1.0f) return;
    const float extent = size * view.scale * (0.5f + age);
    view.renderer.DrawExplosion({view.camera.ProjectionMatrix() * view.camera.ViewMatrix() *
        Matrix4x4::Translation(view.World(local, -0.5f)) * Matrix4x4::Scale({extent, extent, 1.0f}), age});
}

/**
 * @brief 本編のプロシージャル弾を描画する
 * @param view 描画文脈
 * @param local 弾の中心
 * @param size 弾の半サイズ
 * @param direction 進行方向
 * @param type 弾の見た目の種別
 * @param opacity 不透明度
 * @return なし
 */
void DrawPreviewShot(const PreviewView& view, const Vector2& local, const Vector2& size,
    float direction, int type, float opacity = 1.0f) {
    view.renderer.DrawPlayerShot({view.Screen(local), size * view.scale, direction,
        view.time * 60.0f, type, 0.0f, false, opacity});
}

/**
 * @brief ボムの発動で消える敵弾の列を描画する
 * @param view 描画文脈
 * @param progress ボム図解のループ進行度
 * @param clearAt 敵弾が消える進行度
 * @return なし
 */
void DrawClearedEnemyShots(const PreviewView& view, float progress, float clearAt) {
    constexpr int ShotCount = 5;
    for (int i = 0; i < ShotCount; ++i) {
        const Vector2 position {0.20f - (std::min)(progress, clearAt) * 0.80f, -0.14f + i * 0.07f};
        if (progress < clearAt) DrawPreviewShot(view, position, {0.038f, 0.020f}, Math::Pi, 4);
        else DrawPreviewBurst(view, position, 0.06f, (progress - clearAt) / 0.10f);
    }
}

/**
 * @brief 動く標的へ曲がるホーミング弾と照準を描画する
 * @param view 描画文脈
 * @return なし
 */
void DrawHomingShotDemo(const PreviewView& view) {
    constexpr float HitAt = 0.86f;
    const float progress = std::fmod(view.time * 0.38f, 1.0f);
    const Vector2 start {-0.25f, -0.10f};
    const Vector2 target {0.32f, 0.04f + std::sin(view.time * 1.6f) * 0.10f};
    auto path = [&](float amount) {
        return Vector2 {Math::Lerp(start.x, target.x, amount),
            Math::Lerp(start.y, target.y, amount) + std::sin(amount * Math::Pi) * 0.12f};
    };

    // 機体と命中の炎を描画する
    DrawPreviewAircraft(view, {-0.35f, start.y}, true);
    DrawPreviewAircraft(view, target, false);
    DrawPreviewBurst(view, target, 0.10f, (progress - HitAt) / (1.0f - HitAt));

    // 飛んできた軌跡を残し、弾頭を進行方向へ向ける
    const float travel = Math::Clamp01(progress / HitAt);
    constexpr int TrailCount = 10;
    for (int i = 0; i < TrailCount; ++i) {
        const float amount = travel * i / TrailCount;
        const Vector2 from = path(amount), to = path(amount + 0.02f);
        DrawPreviewShot(view, from, {0.012f, 0.005f}, std::atan2(to.y - from.y, to.x - from.x), 0,
            0.15f + 0.55f * i / TrailCount);
    }
    if (progress < HitAt) {
        const Vector2 head = path(travel), next = path(travel + 0.02f);
        DrawPreviewShot(view, head, {0.040f, 0.020f}, std::atan2(next.y - head.y, next.x - head.x),
            static_cast<int>(Homing));
    }
    view.renderer.ResetCamera();

    // 標的へ収束する四隅の照準を画面上で正方形に保つ
    const float halfY = Math::Lerp(0.13f, 0.08f, Math::Clamp01(progress * 2.0f)) * view.scale;
    const float halfX = halfY / view.renderer.AspectRatio();
    const float thickness = 0.004f * view.scale;
    const float arm = 0.022f * view.scale;
    const Vector2 reticle = view.Screen(target);
    for (const float signX : {-1.0f, 1.0f}) for (const float signY : {-1.0f, 1.0f}) {
        const Vector2 corner {reticle.x + signX * halfX, reticle.y + signY * halfY};
        view.renderer.Draw(Rect {{corner.x - signX * arm * 0.5f, corner.y}, {arm * 0.5f, thickness}}, ShotColor);
        view.renderer.Draw(Rect {{corner.x, corner.y - signY * arm}, {thickness * 0.6f, arm}}, ShotColor);
    }
}

/**
 * @brief 一直線に敵列を貫く貫通弾を描画する
 * @param view 描画文脈
 * @return なし
 */
void DrawPiercingShotDemo(const PreviewView& view) {
    constexpr float EnemyX[] = {-0.01f, 0.18f, 0.37f};
    constexpr float StartX = -0.25f;
    const float progress = std::fmod(view.time * 0.38f, 1.0f);
    const float shotX = Math::Lerp(StartX, 0.40f, progress);

    // 弾が通過した順に各敵機へ命中の炎を出す
    DrawPreviewAircraft(view, {-0.35f, 0.0f}, true);
    for (const float x : EnemyX) {
        DrawPreviewAircraft(view, {x, 0.0f}, false);
        DrawPreviewBurst(view, {x, 0.0f}, 0.09f, (shotX - x) / 0.16f);
    }
    DrawPreviewShot(view, {shotX, 0.0f}, {0.060f, 0.026f}, 0.0f, static_cast<int>(Piercing));
    view.renderer.ResetCamera();

    // 通過済みの直線を薄く残して貫通を強調する
    const Vector2 from = view.Screen({StartX, 0.0f}), to = view.Screen({shotX, 0.0f});
    view.renderer.Draw(Rect {(from + to) * 0.5f, {(to.x - from.x) * 0.5f, 0.004f * view.scale}},
        {BorderColor.r, BorderColor.g, BorderColor.b, 0.45f});
}

/**
 * @brief 距離で弾と威力が小さくなる散弾を近距離と遠距離で交互に描画する
 * @param view 描画文脈
 * @return なし
 */
void DrawSpreadShotDemo(const PreviewView& view) {
    constexpr float StartX = -0.25f;
    constexpr float FarX = 0.33f;
    constexpr float HitAt = 0.72f;
    constexpr float FanSlope = 0.27f;
    constexpr float PlayerY = 0.03f;
    const float cycle = view.time * 0.38f;
    const float progress = std::fmod(cycle, 1.0f);
    const bool nearTarget = static_cast<int>(cycle) % 2 == 0;
    const float enemyX = nearTarget ? -0.04f : FarX;
    const float shotX = Math::Lerp(StartX, enemyX, Math::Clamp01(progress / HitAt));
    const float falloff = (shotX - StartX) / (FarX - StartX);

    // 近距離は大きな炎、遠距離は小さな炎で威力差を見せる
    DrawPreviewAircraft(view, {-0.35f, PlayerY}, true);
    DrawPreviewAircraft(view, {enemyX, PlayerY}, false);
    DrawPreviewBurst(view, {enemyX, PlayerY}, nearTarget ? 0.17f : 0.05f, (progress - HitAt) / (1.0f - HitAt));

    // 本編と同じく飛距離に応じて三方向の弾を最大1/3まで縮める
    if (progress < HitAt) {
        const float size = Math::Lerp(1.0f, 1.0f / 3.0f, falloff);
        for (int lane = -1; lane <= 1; ++lane) {
            DrawPreviewShot(view, {shotX, PlayerY + lane * (shotX - StartX) * FanSlope},
                {0.040f * size, 0.020f * size}, std::atan(lane * FanSlope), static_cast<int>(Spread));
        }
    }
    view.renderer.ResetCamera();

    // 距離と威力の関係を先細りのくさびで示し、弾の到達点まで明るくする
    constexpr int SliceCount = 16;
    const float sliceWidth = (FarX - StartX) / SliceCount;
    for (int i = 0; i < SliceCount; ++i) {
        const float x = StartX + sliceWidth * (i + 0.5f);
        const float height = Math::Lerp(0.026f, 0.007f, static_cast<float>(i) / (SliceCount - 1));
        view.renderer.Draw(Rect {view.Screen({x, -0.14f - height}),
            {sliceWidth * 0.40f * view.scale, height * view.scale}},
            {ShotColor.r, ShotColor.g, ShotColor.b, x <= shotX ? 1.0f : 0.25f});
    }
}

/**
 * @brief 打ち上げ、落下、追尾の順に飛ぶミサイル十発のボムを描画する
 * @param view 描画文脈
 * @return なし
 */
void DrawHomingBombDemo(const PreviewView& view) {
    constexpr Vector2 Targets[] = {{0.30f, 0.12f}, {0.36f, -0.02f}, {0.30f, -0.16f}};
    constexpr float FlightLength = 0.62f;
    const float progress = std::fmod(view.time * 0.25f, 1.0f);
    const Vector2 player {-0.33f, -0.02f};
    DrawPreviewAircraft(view, player, true);
    for (const Vector2& target : Targets) DrawPreviewAircraft(view, target, false);
    DrawClearedEnemyShots(view, progress, 0.22f);

    for (int i = 0; i < 10; ++i) {
        const float age = (progress - (0.06f + 0.018f * i)) / FlightLength;
        if (age < 0.0f) continue;
        const Vector2& target = Targets[i % 3];
        DrawPreviewBurst(view, target, 0.09f, (age - 1.0f) / 0.25f);
        if (age >= 1.0f) continue;

        // 上昇、落下、標的への旋回を3次ベジェで一本の軌道にする
        const Vector2 lift {-0.34f + 0.035f * i, 0.55f};
        const Vector2 fall {-0.20f + 0.030f * i, -0.55f};
        auto path = [&](float amount) {
            const float rest = 1.0f - amount;
            return player * (rest * rest * rest) + lift * (3.0f * rest * rest * amount) +
                fall * (3.0f * rest * amount * amount) + target * (amount * amount * amount);
        };
        const Vector3 center = view.World(path(age));
        const Vector3 ahead = view.World(path((std::min)(age + 0.02f, 1.0f))) - center;
        const Vector3 direction = ahead.Normalized();
        const float size = 0.026f * view.scale;

        // 本編のミサイルと同じ構成を縮小し、噴射炎を後方へ伸ばす
        const Matrix4x4 transform = Matrix4x4::Translation(center) *
            Matrix4x4::RotationZ(std::atan2(direction.y, direction.x)) * Matrix4x4::Scale(Vector3::One * size);
        view.DrawPart(PrimitiveShape::Sphere, transform * Matrix4x4::Scale({2.6f, 1.1f, 1.1f}), {0.60f, 0.72f, 0.82f, 1.0f});
        view.DrawPart(PrimitiveShape::Sphere, transform * Matrix4x4::Translation({0.85f, 0.0f, 0.0f}) *
            Matrix4x4::Scale({1.0f, 0.85f, 0.85f}), {1.0f, 0.30f, 0.08f, 1.0f});
        view.DrawPart(PrimitiveShape::Box, transform * Matrix4x4::Translation({-0.8f, 0.0f, 0.0f}) *
            Matrix4x4::Scale({0.65f, 1.7f, 0.15f}), {0.10f, 0.22f, 0.35f, 1.0f});
        SideScrollingShooter::DrawRailgunBeamBetween(view.renderer, view.camera,
            center - direction * (4.5f * size), center - direction * size, 0.60f * size, 0.25f, 0);
    }
    view.renderer.ResetCamera();
}

/**
 * @brief 機首から前方を薙ぎ払う極太レーザーのボムを描画する
 * @param view 描画文脈
 * @return なし
 */
void DrawLaserBombDemo(const PreviewView& view) {
    constexpr Vector2 Enemies[] = {{0.02f, 0.05f}, {0.20f, -0.05f}, {0.36f, 0.03f}};
    constexpr float BeamOn = 0.20f;
    constexpr float BeamOff = 0.85f;
    const float progress = std::fmod(view.time * 0.25f, 1.0f);
    const Vector2 player {-0.35f, 0.0f};
    DrawPreviewAircraft(view, player, true);
    for (const Vector2& enemy : Enemies) DrawPreviewAircraft(view, enemy, false);
    DrawClearedEnemyShots(view, progress, BeamOn);

    // 発射と収束を短くフェードさせ、照射中は敵機へ炎を出し続ける
    const float fade = Math::Clamp01((std::min)((progress - BeamOn), (BeamOff - progress)) / 0.05f);
    if (fade > 0.0f) {
        const float width = 0.15f * view.scale * fade;
        const float pulse = 0.5f + 0.5f * std::sin(view.time * 21.0f);
        const Vector3 start = view.World(player, -0.3f) +
            Vector3 {AircraftModelView::NoseTipZ * 0.22f * view.scale, 0.0f, 0.0f};
        const Vector3 end = view.World({0.48f, player.y}, -0.3f);
        SideScrollingShooter::DrawRailgunBeamBetween(view.renderer, view.camera, start, end, width * 1.4f, pulse, 4);
        SideScrollingShooter::DrawRailgunBeamBetween(view.renderer, view.camera, start, end, width, pulse, 5);
        for (int i = 0; i < 3; ++i)
            DrawPreviewBurst(view, Enemies[i], 0.08f, std::fmod(view.time * 2.5f + i * 0.33f, 1.0f));
    }
    view.renderer.ResetCamera();
}

/**
 * @brief 三回の被弾で割れるシールドのボムを描画する
 * @param view 描画文脈
 * @return なし
 */
void DrawShieldBombDemo(const PreviewView& view) {
    constexpr float HitTimes[] = {0.32f, 0.50f, 0.68f};
    constexpr float ShieldOn = 0.08f;
    constexpr float Radius = 0.19f;
    constexpr float Flight = 0.16f;
    const float progress = std::fmod(view.time * 0.25f, 1.0f);
    const Vector2 player {-0.20f, 0.0f};
    const Vector2 enemy {0.36f, 0.0f};
    const bool shieldUp = progress >= ShieldOn;
    int hits = 0;
    for (const float hitTime : HitTimes) hits += progress >= hitTime ? 1 : 0;

    // 機首までシールドへ収まるよう自機だけ小さく描画する
    DrawPreviewAircraft(view, player, true, 0.65f);
    DrawPreviewAircraft(view, enemy, false);
    if (shieldUp && hits < 3) {
        SideScrollingShooter::DrawBombShield(view.renderer, view.camera, view.World(player),
            Radius * view.scale, static_cast<int>(view.time * 60.0f) % 360, hits, 0.0035f * view.scale);
    }

    // 敵弾がシールド表面へ届くたびに炎を出し、被弾回数を増やす
    const float contactX = player.x + Radius / view.renderer.AspectRatio();
    for (int i = 0; i < 3; ++i) {
        const float y = (i - 1) * 0.06f;
        const float flight = (progress - (HitTimes[i] - Flight)) / Flight;
        if (flight >= 0.0f && flight < 1.0f)
            DrawPreviewShot(view, {Math::Lerp(enemy.x - 0.08f, contactX, flight), y}, {0.038f, 0.020f}, Math::Pi, 4);
        DrawPreviewBurst(view, {contactX, y}, 0.07f, (progress - HitTimes[i]) / 0.08f);
    }
    view.renderer.ResetCamera();

    // シールド下へ残り耐久を三つのマスで表示する
    const float aspect = view.renderer.AspectRatio();
    for (int i = 0; i < 3; ++i) {
        const bool alive = shieldUp && i >= hits;
        view.renderer.Draw(Rect {view.Screen({player.x + (i - 1) * 0.05f, -0.215f}),
            {0.012f / aspect * view.scale, 0.012f * view.scale}},
            alive ? ColorF {0.25f, 1.0f, 0.85f, 1.0f} : ColorF {0.25f, 1.0f, 0.85f, 0.18f});
    }
}

/**
 * @brief 選択中の機体のショットとボムを図解として描画する
 * @param renderer 描画先
 * @param playerType 図解対象の機体タイプ
 * @param time アニメーション時刻
 * @param shotCenter ショット図解の中心
 * @param bombCenter ボム図解の中心
 * @param scale 図解の拡大率
 * @return なし
 */
void DrawWeaponPreview(Renderer& renderer, PlayerType playerType, float time,
    const Vector2& shotCenter, const Vector2& bombCenter, float scale) {
    const PreviewView shot = BeginPreview(renderer, shotCenter, scale, time);
    if (playerType == Piercing) DrawPiercingShotDemo(shot);
    else if (playerType == Spread) DrawSpreadShotDemo(shot);
    else DrawHomingShotDemo(shot);

    const PreviewView bomb = BeginPreview(renderer, bombCenter, scale, time);
    if (playerType == Piercing) DrawLaserBombDemo(bomb);
    else if (playerType == Spread) DrawShieldBombDemo(bomb);
    else DrawHomingBombDemo(bomb);
}

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

}

/** @brief モードセレクトシーンを初期化する */
void ModeSelectionScene::Initialize() {
    // 人数を選び直してから各モードの開始条件を確認する
    m_stateController = std::make_unique<ModeSelectionStateController>();
    getData().playerCount = 1;
    getData().startStage = 1;
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
    constexpr PlayerType playerTypes[] = { Homing, Piercing, Spread };
    for (size_t i = 0; i < m_playerTypeButtons.size(); ++i) {
        m_playerTypeButtons[i] = std::make_unique<Button>(
            Vector2 { 0.40f, 0.12f }, RectAlign::Center,
            ShotNames[i], Vector2 { -0.62f, 0.20f - static_cast<float>(i) * 0.20f });
        m_playerTypeButtons[i]->SetOnClick([this, playerType = playerTypes[i]]() {
            // シングルプレイは機体選択後にステージ選択へ進む
            getData().playerType = playerType;
            m_stateController->SetCurrentState(ModeSelectionState::StageSelect);
        });
    }

    // 到達済みのステージだけ選択できるボタンを縦に配置する
    constexpr const char* stageLabels[] = {"STAGE 1", "STAGE 2", "STAGE 3", "STAGE 4", "STAGE 5"};
    const int unlockedStage = SettingsRepository {}.Load().unlockedStage;
    for (int stage = 1; stage <= static_cast<int>(m_stageButtons.size()); ++stage) {
        auto& button = m_stageButtons[static_cast<size_t>(stage - 1)];
        button = std::make_unique<Button>(Vector2 {0.40f, 0.12f}, RectAlign::Center,
            stageLabels[stage - 1], Vector2 {0.0f, 0.40f - static_cast<float>(stage - 1) * 0.20f});
        button->SetEnabled(stage <= unlockedStage);
        button->SetOnClick([this, stage] {
            // STAGE 1だけ導入のストーリーと初回チュートリアルを通す
            getData().startStage = stage;
            changeScene(stage == 1 ? SceneType::Story : SceneType::TestStage);
        });
    }

    // 左下にタイトルシーンへ戻るボタンを配置する
    m_backButton = std::make_unique<Button>(
        Rect { { -0.95f, -0.90f }, { 0.30f, 0.10f } }, "BACK");
    m_backButton->SetClickSound(Button::ClickSound::Cancel);
    m_backButton->SetOnClick([this]() {
        const auto state = m_stateController->GetCurrentState();
        if (state == ModeSelectionState::PlayerCountSelect) changeScene(SceneType::Title);
        else if (state == ModeSelectionState::StageSelect)
            m_stateController->SetCurrentState(ModeSelectionState::PlayerTypeSelect);
        else m_stateController->SetCurrentState(state == ModeSelectionState::PlayerTypeSelect ?
            ModeSelectionState::DifficultySelect : ModeSelectionState::PlayerCountSelect);
    });
}

/** @brief モードセレクト画面の入力を処理する */
void ModeSelectionScene::ProcessInput() {
    // 2人用の個別入力は共有マウス入力と分離する
    const auto state = m_stateController->GetCurrentState();
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

    UpdateActiveButtons(UIInput::Current());
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
    if (state == ModeSelectionState::StageSelect) {
        for (const auto& button : m_stageButtons) button->Update(inputState);
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
    for (auto& button : m_stageButtons) button.reset();
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
        state == ModeSelectionState::StageSelect ? "SELECT STAGE" :
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
    } else if (state == ModeSelectionState::StageSelect) {
        for (const auto& button : m_stageButtons) button->Render(renderer);
    } else if (selectingDifficulty) {
        for (const auto& button : m_difficultyButtons) button->Render(renderer);
    } else if (getData().playerCount == 2) {
        RenderCooperativeSelection(renderer);
    } else {
        for (const auto& button : m_playerTypeButtons) button->Render(renderer);

        // 右パネルは機体名と見出しだけを文字にし、上段へショット、下段へボムを図解する
        DrawPanel(renderer, {0.47f, -0.08f}, {0.49f, 0.63f});
        renderer.DrawText(ShotNames[static_cast<size_t>(m_previewPlayerType)], TextAlign::Center,
            0.026f, ShotColor, {0.47f, 0.44f}, CharacterSpacing);
        renderer.DrawText("SHOT", TextAlign::Center, 0.012f, BorderColor, {0.47f, 0.33f}, CharacterSpacing);
        renderer.Draw(Rect {{0.47f, -0.12f}, {0.45f, 0.003f}}, BorderColor);
        renderer.DrawText("BOMB", TextAlign::Center, 0.012f, ShotColor, {0.47f, -0.18f}, CharacterSpacing);
        DrawWeaponPreview(renderer, m_previewPlayerType, Time::unscaledTime,
            {0.47f, 0.09f}, {0.47f, -0.45f}, 1.0f);
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
            renderer.DrawText(ShotNames[shot], TextAlign::Center,
                0.020f, ColorF::White(), {x, y}, CharacterSpacing);
        }
        // 協力プレイでも各自が選んだ機体のショットとボムを左右に並べて図解する
        renderer.DrawText("SHOT", TextAlign::Center, 0.010f, BorderColor, {x - 0.225f, -0.29f}, 0.001f);
        renderer.DrawText("BOMB", TextAlign::Center, 0.010f, ShotColor, {x + 0.225f, -0.29f}, 0.001f);
        renderer.DrawText(m_stateController->IsPlayerReady(player) ? "READY" : "PRESS FIRE TO CONFIRM",
            TextAlign::Center, 0.014f, playerColors[player], {x, -0.65f}, CharacterSpacing);
        DrawWeaponPreview(renderer, static_cast<PlayerType>(selected), Time::unscaledTime,
            {x - 0.225f, -0.44f}, {x + 0.225f, -0.44f}, 0.48f);
    }
    renderer.DrawText(assigning ? "CONFIRM ON EACH CONTROLLER" : "UP / DOWN: SELECT    FIRE: READY    PAUSE: CANCEL",
        TextAlign::Center, 0.012f, ColorF::White(), {0.0f, -0.77f}, CharacterSpacing);
}
