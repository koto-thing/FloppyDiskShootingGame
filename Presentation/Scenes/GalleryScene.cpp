#include "GalleryScene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <windows.h>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Time/Time.h"
#include "../../Engine/UI/Button.h"
#include "../../Infrastructure/Repositories/SettingsRepository.h"
#include "../Gameplay/Models/AircraftModelView.h"
#include "../Gameplay/Models/LegacyBossModelView.h"
#include "../Gameplay/Models/StageEnemyModelView.h"
#include "../Gameplay/Stages/Stage2/Stage2BossModelView.h"
#include "../Gameplay/Stages/Stage3/Stage3BarrierCageView.h"
#include "../Gameplay/Stages/Stage3/Stage3BossModelView.h"
#include "../Gameplay/Stages/Stage3/Stage3FunnelModelView.h"
#include "../Gameplay/Stages/Stage4/Stage4BossModelView.h"
#include "../Gameplay/Stages/Stage5/Stage5ModelView.h"

namespace {
/** @brief ギャラリーUIとカメラ調整に使う展示定義 */
struct ExhibitDefinition {
    GalleryEntry entry;
    const char* name;
    const char* description;
    std::array<const char*, 4> animations;
    int animationCount;
    float modelScale;
    float cameraDistance;
};

constexpr std::array<ExhibitDefinition, 16> Exhibits {{
    {GalleryEntry::Player, "POLICE INTERCEPTOR",
        "A COMPACT POLICE FIGHTER BUILT FOR RAPID INTERCEPTION",
        {"IDLE"}, 1, 3.8f, 15.0f},
    {GalleryEntry::LightEnemy, "LIGHT ENEMY",
        "A LIGHT RAIDER THAT ATTACKS IN LARGE FORMATIONS",
        {"IDLE"}, 1, 3.8f, 15.0f},
    {GalleryEntry::HeavyEnemy, "HEAVY ENEMY",
        "A REINFORCED RAIDER WITH HIGHER ARMOR AND FIREPOWER",
        {"IDLE"}, 1, 3.2f, 15.0f},
    {GalleryEntry::ArmoredEnemy, "ARMORED ENEMY",
        "A HIGH-DURABILITY ATTACK CRAFT USED IN LATER OPERATIONS",
        {"IDLE"}, 1, 3.0f, 15.0f},
    {GalleryEntry::Stage1Boss, "KOTO",
        "A MASSIVE COMMAND FIGHTER WITH SEPARATELY ARMORED SECTIONS",
        {"IDLE"}, 1, 0.32f, 18.0f},
    {GalleryEntry::Stage2Boss, "RYOTA",
        "A LAND BATTLESHIP COMBINED WITH A SAND SUBMARINE",
        {"COMBINED", "TARGET TRACKING", "SEPARATION"}, 3, 0.72f, 22.0f},
    {GalleryEntry::Stage3Boss, "STAGE 3 FLAGSHIP",
        "AN AIRBORNE BATTLESHIP CARRYING MANY GUNS AND FUNNEL PODS",
        {"IDLE", "WEAPON SWEEP", "PODS OPEN", "BARRIER CAGE"}, 4, 0.52f, 23.0f},
    {GalleryEntry::Stage3BarrierFunnel, "BARRIER FUNNEL",
        "A REMOTE UNIT THAT DEPLOYS A PAIR OF BARRIER EMITTERS",
        {"STOWED", "EMITTER DEPLOY"}, 2, 5.0f, 15.0f},
    {GalleryEntry::Stage3ReflectFunnel, "REFLECT FUNNEL",
        "A REMOTE GUN PLATFORM THAT RETURNS SHOTS TOWARD ITS TARGET",
        {"AIM", "FIRE AND RECOIL"}, 2, 5.0f, 15.0f},
    {GalleryEntry::Stage4Boss, "STAGE 4 WAR MACHINE",
        "A LUXURY ARMORED VEHICLE FUSED WITH A SUPER-HEAVY TANK",
        {"IDLE"}, 1, 0.48f, 23.0f},
    {GalleryEntry::Eastsource, "EASTSOURCE",
        "THE DOTONBORI ATTACK CRAFT PILOTED BY THE SYNDICATE HITMAN",
        {"IDLE"}, 1, 0.82f, 19.0f},
    {GalleryEntry::Tayama, "TAYAMA",
        "A CITY TOWER THAT TRANSFORMS INTO AN AERIAL CARRIER",
        {"TOWER", "TRANSFORMATION", "AERIAL CARRIER"}, 3, 0.40f, 22.0f},
    {GalleryEntry::Stage1Enemy, "STAGE 1 RAIDER",
        "A RED INTERCEPTOR DEPLOYED IN THE FIRST OPERATION",
        {"IDLE"}, 1, 3.8f, 15.0f},
    {GalleryEntry::Stage2Enemy, "STAGE 2 GUNSHIP",
        "A BLUE PROPELLER GUNSHIP BUILT FOR DESERT COMBAT",
        {"IDLE"}, 1, 3.5f, 15.0f},
    {GalleryEntry::Stage3Enemy, "STAGE 3 TURRET",
        "A YELLOW AERIAL TURRET CARRYING A FORWARD CANNON",
        {"IDLE"}, 1, 3.3f, 15.0f},
    {GalleryEntry::Stage4Enemy, "STAGE 4 STAR MINE",
        "A GREEN OMNIDIRECTIONAL ASSAULT UNIT",
        {"IDLE"}, 1, 3.2f, 15.0f}
}};

constexpr float LockedColor[4] = {0.025f, 0.030f, 0.040f, 1.0f};

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

/**
 * @brief 循環する添字へ補正する
 * @param value 補正前の添字
 * @param count 要素数
 * @return 0以上count未満の添字
 */
int WrapIndex(int value, int count) {
    return (value % count + count) % count;
}

/**
 * @brief 0から1を往復する補間率を取得する
 * @param time 経過秒
 * @param speed 往復速度
 * @return 0から1の補間率
 */
float PingPong(float time, float speed = 1.0f) {
    return (std::sin(time * speed) + 1.0f) * 0.5f;
}
}

/** @brief 保存済み解放状態と鑑賞カメラを初期化する */
void GalleryScene::Initialize() {
    m_galleryUnlocks = SettingsRepository {}.Load().galleryUnlocks;
    m_camera.SetNearClip(0.1f);
    m_camera.SetFarClip(120.0f);
    m_camera.SetFieldOfView(Math::ToRadians(48.0f));
    m_returnButton = std::make_unique<Button>(
        Vector2 {0.38f, 0.09f}, RectAlign::BottomRight, "BACK TO TITLE", Vector2 {-0.04f, 0.04f});
    m_returnButton->SetOnClick([this]() { changeScene(SceneType::Title); });
    ResetExhibit();
}

/** @brief 展示選択、カメラ、アニメーション操作を受け付ける */
void GalleryScene::ProcessInput() {
    m_returnButton->Update(CurrentUIInput());

    // 左右キーで展示、上下キーで展示内アニメーションを循環選択する
    int exhibitStep = static_cast<int>(Input::GetKeyDown(KeyCode::RightArrow)) -
        static_cast<int>(Input::GetKeyDown(KeyCode::LeftArrow));
    if (exhibitStep != 0) {
        m_exhibitIndex = WrapIndex(m_exhibitIndex + exhibitStep, static_cast<int>(Exhibits.size()));
        ResetExhibit();
    }
    const ExhibitDefinition& exhibit = Exhibits[static_cast<std::size_t>(m_exhibitIndex)];
    int animationStep = static_cast<int>(Input::GetKeyDown(KeyCode::DownArrow)) -
        static_cast<int>(Input::GetKeyDown(KeyCode::UpArrow));
    if (animationStep != 0 && IsUnlocked()) {
        m_animationIndex = WrapIndex(m_animationIndex + animationStep, exhibit.animationCount);
        m_animationTime = 0.0f;
    }

    // Spaceで動作を停止し、Rで現在展示の初期視点へ戻す
    if (Input::GetKeyDown(KeyCode::Space) && IsUnlocked()) m_playing = !m_playing;
    if (Input::GetKeyDown(KeyCode::R)) ResetExhibit();

    // AとDまたは左ドラッグでカメラを周回させる
    const float keyOrbit = static_cast<float>(Input::GetKey(KeyCode::D)) -
        static_cast<float>(Input::GetKey(KeyCode::A));
    m_orbitYaw += keyOrbit * Time::fixedDeltaTime * 1.4f;
    if (Input::GetMouseButton(MouseButton::Left)) {
        const Vector2 delta = Input::GetMouseDelta();
        m_orbitYaw += delta.x * 0.006f;
        m_orbitPitch = (std::clamp)(m_orbitPitch - delta.y * 0.006f, -0.65f, 0.85f);
    }

    // WとSまたはホイールで表示倍率を変更する
    const float keyZoom = static_cast<float>(Input::GetKey(KeyCode::S)) -
        static_cast<float>(Input::GetKey(KeyCode::W));
    m_zoom = (std::clamp)(m_zoom + keyZoom * Time::fixedDeltaTime * 0.8f -
        Input::GetMouseWheelDelta() * 0.10f, 0.55f, 1.75f);
}

/** @brief 再生中アニメーションの時間を更新する */
void GalleryScene::Tick() {
    if (m_playing && IsUnlocked()) m_animationTime += Time::fixedDeltaTime;
}

/** @brief ギャラリーが保持するUIを解放する */
void GalleryScene::Dispose() {
    m_returnButton.reset();
}

/**
 * @brief 選択中の展示モデルと説明UIを描画する
 * @param renderer 描画先Renderer
 */
void GalleryScene::Render(Renderer& renderer) {
    const ExhibitDefinition& exhibit = Exhibits[static_cast<std::size_t>(m_exhibitIndex)];
    const float distance = exhibit.cameraDistance * m_zoom;
    const float horizontalDistance = std::cos(m_orbitPitch) * distance;

    // 選択中モデルを中心とした周回カメラを構成する
    m_camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    m_camera.SetPosition({
        std::sin(m_orbitYaw) * horizontalDistance,
        std::sin(m_orbitPitch) * distance,
        -std::cos(m_orbitYaw) * horizontalDistance
    });
    m_camera.LookAt(Vector3::Zero);
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(m_camera);

    // 展示台をモデルより先に描画する
    const Matrix4x4 viewProjection = m_camera.ProjectionMatrix() * m_camera.ViewMatrix();
    const Matrix4x4 pedestal = Matrix4x4::Translation({0.0f, -4.35f, 0.0f}) *
        Matrix4x4::Scale({10.0f, 0.35f, 10.0f});
    renderer.Draw({PrimitiveShape::Cylinder, viewProjection * pedestal, Vector3::One,
        {0.08f, 0.10f, 0.14f, 1.0f}});
    RenderExhibit(renderer);

    renderer.ResetCamera();
    RenderUi(renderer);
    m_returnButton->Render(renderer);
}

/** @brief 選択中の展示に合わせてアニメーションと視点を初期化する */
void GalleryScene::ResetExhibit() {
    m_animationIndex = 0;
    m_animationTime = 0.0f;
    m_orbitYaw = -0.65f;
    m_orbitPitch = 0.22f;
    m_zoom = 1.0f;
    m_playing = true;
}

/**
 * @brief 選択中の展示が解放済みか判定する
 * @return 解放済みの場合true
 */
bool GalleryScene::IsUnlocked() const {
    const GalleryEntry entry = Exhibits[static_cast<std::size_t>(m_exhibitIndex)].entry;
    return (m_galleryUnlocks & GalleryEntryBit(entry)) != 0u;
}

/**
 * @brief 選択中展示のモデルを描画する
 * @param renderer 描画先Renderer
 */
void GalleryScene::RenderExhibit(Renderer& renderer) const {
    const ExhibitDefinition& exhibit = Exhibits[static_cast<std::size_t>(m_exhibitIndex)];
    const bool unlocked = IsUnlocked();
    const Matrix4x4 viewProjection = m_camera.ProjectionMatrix() * m_camera.ViewMatrix();
    auto drawPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float yaw, float pitch) {
        const float* displayColor = unlocked ? color : LockedColor;
        const Matrix4x4 world = Matrix4x4::Translation(position) *
            Matrix4x4::RotationY(yaw) * Matrix4x4::RotationZ(pitch) * Matrix4x4::Scale(scale);
        renderer.Draw({static_cast<PrimitiveShape>(shape), viewProjection * world, Vector3::One,
            {displayColor[0], displayColor[1], displayColor[2], displayColor[3]}, yaw});
    };
    auto drawMatrixPart = [&](PrimitiveShape shape, const Matrix4x4& world,
        const ColorF& color, auto) {
        const ColorF displayColor = unlocked ? color : ColorF {
            LockedColor[0], LockedColor[1], LockedColor[2], LockedColor[3]};
        renderer.Draw({shape, viewProjection * world, Vector3::One, displayColor});
    };

    // 展示識別子ごとにゲーム本編と同じモデル定義へ鑑賞用状態を渡す
    switch (exhibit.entry) {
    case GalleryEntry::Player:
        AircraftModelView::DrawPlayer({}, 0.0f, exhibit.modelScale, drawPart);
        break;
    case GalleryEntry::LightEnemy:
    case GalleryEntry::HeavyEnemy:
    case GalleryEntry::ArmoredEnemy:
        AircraftModelView::DrawEnemy({}, Math::Pi, exhibit.modelScale, drawPart);
        break;
    case GalleryEntry::Stage1Boss: {
        BossModelTransform transform {{0.0f, 0.1f, 0.0f}};
        transform.yaw = Math::Pi;
        transform.scale = exhibit.modelScale;
        LegacyBossModelView::Draw(transform, drawPart);
        break;
    }
    case GalleryEntry::Stage2Boss: {
        const float separation = m_animationIndex == 2 ? PingPong(m_animationTime, 1.1f) : 0.0f;
        BossModelTransform submarine {{0.0f, -1.35f - separation * 1.4f, 0.0f}};
        submarine.yaw = Math::Pi + separation * Math::HalfPi;
        submarine.scale = exhibit.modelScale;
        BossModelTransform battleship {{0.0f, 1.10f + separation * 2.0f, 0.0f}};
        battleship.yaw = Math::Pi;
        battleship.scale = exhibit.modelScale;
        if (m_animationIndex == 1) {
            battleship.mainGunTracksTarget = true;
            battleship.secondaryGunsTrackTarget = true;
            battleship.aimTarget = {std::sin(m_animationTime) * 7.0f, 2.0f, -8.0f};
            battleship.secondaryAimTarget = {-4.0f, std::cos(m_animationTime) * 3.0f, -7.0f};
        }
        SandSubmarineView::Draw(submarine, drawPart);
        LandBattleshipView::Draw(battleship, drawPart);
        break;
    }
    case GalleryEntry::Stage3Boss: {
        BossModelTransform transform {{0.0f, 1.6f, 0.0f}};
        transform.yaw = Math::Pi;
        transform.scale = exhibit.modelScale;
        Stage3BossModelView::DrawStaticBody(transform, drawPart);
        Stage3BossModelView::DrawGondolaBody(transform, drawPart);
        const float sweep = m_animationIndex == 1 ? std::sin(m_animationTime * 1.3f) * 0.55f : 0.0f;
        const float open = m_animationIndex >= 2 ? PingPong(m_animationTime, 1.2f) : 0.0f;
        for (int i = 0; i < Stage3BossModelView::TopGunCount; ++i) {
            Stage3BossModelView::DrawTopGun(i, transform, {0.0f, sweep, 0.0f}, false, drawPart);
        }
        for (int i = 0; i < Stage3BossModelView::GondolaMachineGunCount; ++i) {
            Stage3BossModelView::DrawGondolaMachineGun(i, transform, {0.0f, -sweep, 0.0f}, drawPart);
        }
        for (int i = 0; i < Stage3BossModelView::HeavyCannonCount; ++i) {
            Stage3BossModelView::DrawHeavyCannon(i, transform, {0.0f, sweep * 0.6f, 0.0f}, drawPart);
        }
        for (int i = 0; i < Stage3BossModelView::MissilePodCount; ++i) {
            Stage3BossModelView::DrawMissilePod(i, transform, open, drawPart);
        }
        for (int i = 0; i < Stage3BossModelView::FunnelPodCount; ++i) {
            Stage3BossModelView::DrawFunnelPod(i, transform, open, drawPart);
        }
        if (m_animationIndex == 3) {
            Stage3BarrierCagePose pose;
            pose.openAmount = open;
            pose.scrollOffset = m_animationTime * 8.0f;
            pose.flicker = PingPong(m_animationTime, 5.0f);
            Stage3BarrierCageView::Draw(transform, pose, drawPart);
        }
        break;
    }
    case GalleryEntry::Stage3BarrierFunnel: {
        BossModelTransform transform {{0.0f, 0.0f, 0.0f}};
        transform.yaw = Math::Pi;
        transform.scale = exhibit.modelScale;
        const float open = m_animationIndex == 1 ? PingPong(m_animationTime, 1.4f) : 0.0f;
        Stage3FunnelModelView::DrawBarrier(transform, open, drawPart);
        break;
    }
    case GalleryEntry::Stage3ReflectFunnel: {
        BossModelTransform transform {{0.0f, 0.0f, 0.0f}};
        transform.yaw = Math::Pi;
        transform.scale = exhibit.modelScale;
        const float yaw = std::sin(m_animationTime * 1.2f) * 0.55f;
        const float pitch = std::cos(m_animationTime * 0.9f) * 0.25f;
        const float recoil = m_animationIndex == 1 ?
            (std::max)(0.0f, std::sin(m_animationTime * 5.0f)) : 0.0f;
        Stage3FunnelModelView::DrawReflectShot(transform, yaw, pitch, recoil, drawPart);
        break;
    }
    case GalleryEntry::Stage4Boss: {
        BossModelTransform transform {{0.0f, -1.8f, 0.0f}};
        transform.yaw = Math::Pi;
        transform.scale = exhibit.modelScale;
        Stage4BossModelView::Draw(transform, drawPart);
        break;
    }
    case GalleryEntry::Eastsource: {
        Stage5ModelTransform transform {{0.0f, 0.0f, 0.0f}, {0.0f, Math::Pi, 0.0f}, exhibit.modelScale};
        EastsourceModelView::VisitParts(transform, {}, drawMatrixPart);
        break;
    }
    case GalleryEntry::Tayama: {
        float progress = 0.0f;
        if (m_animationIndex == 1) progress = PingPong(m_animationTime, 0.7f);
        if (m_animationIndex == 2) progress = 1.0f;
        Stage5ModelTransform transform {{0.0f, -0.1f, 0.0f}, {0.0f, Math::Pi, 0.0f}, exhibit.modelScale};
        TayamaModelView::VisitParts(transform, progress, {}, drawMatrixPart);
        break;
    }
    case GalleryEntry::Stage1Enemy:
    case GalleryEntry::Stage2Enemy:
    case GalleryEntry::Stage3Enemy:
    case GalleryEntry::Stage4Enemy: {
        const int stageNumber = static_cast<int>(exhibit.entry) -
            static_cast<int>(GalleryEntry::Stage1Enemy) + 1;
        auto drawStageEnemyPart = [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
            drawMatrixPart(shape, world, color, 0);
        };
        StageEnemyModelView::Draw(stageNumber, {}, Math::Pi, exhibit.modelScale, drawStageEnemyPart);
        break;
    }
    case GalleryEntry::Count:
        break;
    }
}

/**
 * @brief 展示名、説明、解放状態、操作案内を描画する
 * @param renderer 描画先Renderer
 */
void GalleryScene::RenderUi(Renderer& renderer) const {
    const ExhibitDefinition& exhibit = Exhibits[static_cast<std::size_t>(m_exhibitIndex)];
    const bool unlocked = IsUnlocked();
    char counter[32] {};
    char animationStatus[64] {};
    std::snprintf(counter, sizeof(counter), "%02d / %02d", m_exhibitIndex + 1,
        static_cast<int>(Exhibits.size()));
    std::snprintf(animationStatus, sizeof(animationStatus), "%s / %s",
        exhibit.animations[static_cast<std::size_t>(m_animationIndex)],
        m_playing ? "PLAYING" : "PAUSED");

    // 上部へ展示名、一言説明、展示の進捗を表示する
    renderer.DrawText(unlocked ? exhibit.name : "????????", TextAlign::TopCenter, 0.032f,
        unlocked ? ColorF::White() : ColorF {0.38f, 0.42f, 0.48f, 1.0f}, {0.0f, -0.05f});
    renderer.DrawText(unlocked ? exhibit.description : "UNIDENTIFIED MODEL",
        TextAlign::TopCenter, 0.013f, {0.72f, 0.76f, 0.82f, 1.0f}, {0.0f, -0.16f});
    renderer.DrawText(counter, TextAlign::TopRight, 0.016f,
        {0.65f, 0.72f, 0.82f, 1.0f}, {-0.04f, -0.06f});

    // 未解放展示は画面中央へ解放条件を明示する
    if (!unlocked) {
        renderer.Draw(Rect {{0.0f, 0.0f}, {1.08f, 0.22f}}, {0.01f, 0.015f, 0.025f, 0.86f});
        renderer.DrawText("MODEL LOCKED", TextAlign::Center, 0.030f,
            {1.0f, 0.64f, 0.18f, 1.0f}, {0.0f, 0.035f});
        renderer.DrawText("ENCOUNTER THIS MODEL IN THE GAME TO UNLOCK", TextAlign::Center, 0.012f,
            {0.76f, 0.79f, 0.84f, 1.0f}, {0.0f, -0.045f});
    } else {
        renderer.DrawText(animationStatus, TextAlign::BottomCenter, 0.016f,
            {1.0f, 0.64f, 0.18f, 1.0f}, {0.0f, 0.055f});
    }

    // 操作案内を画面左下へまとめる
    renderer.DrawText("LEFT/RIGHT: MODEL   UP/DOWN: ANIMATION",
        TextAlign::BottomLeft, 0.011f, {0.66f, 0.70f, 0.76f, 1.0f}, {0.04f, 0.135f});
    renderer.DrawText("A/D OR LEFT DRAG: ORBIT   W/S OR WHEEL: ZOOM",
        TextAlign::BottomLeft, 0.010f, {0.62f, 0.66f, 0.72f, 1.0f}, {0.04f, 0.09f});
    renderer.DrawText("SPACE: PLAY/PAUSE   R: RESET",
        TextAlign::BottomLeft, 0.010f, {0.58f, 0.62f, 0.68f, 1.0f}, {0.04f, 0.045f});
}
