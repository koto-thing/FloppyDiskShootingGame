#include "SideScrollingShooter.h"
#include "../../Application/UseCases/Localization.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "Stages/Common/StageDispatch.h"
#include "Stages/Stage2/Stage2Module.h"
#include "Models/StageEnemyModelView.h"


#include "SideScrollingShooterEnemies.h"
#include "Stages/Common/StageDefinition.h"
#if defined(_DEBUG)
#include <cassert>
#include "Stages/Stage3/Stage3Module.h"
#include "Stages/Stage5/Stage5Module.h"
#endif

namespace {
constexpr float PlayerHitboxColor[4] = {1.0f, 0.08f, 0.08f, 0.65f};
constexpr float PlayerHitRadius2D = 0.050f;
constexpr float PlayerHitRadius3D = 0.38f;

/**
 * @brief 接続中の入力機器に対応したHUD操作案内を取得する
 * @param viewLocked 視点切替が禁止されている場合はtrue
 * @return HUDへ描画する操作案内
 */
const char* HudControlHint(bool viewLocked) {
    if (Input::IsSwitch2ProConnected()) {
        return viewLocked ?
            "MOVE: L STICK/DPAD  SHOT: A/R/ZR  3D MODE LOCKED  BOMB: Y  MENU: +" :
            "MOVE: L STICK/DPAD  SHOT: A/R/ZR  MODE: X  BOMB: Y  MENU: +";
    }
    if (Input::IsGamepadConnected()) {
        return viewLocked ?
            "MOVE: L STICK/DPAD  SHOT: A/RB/RT  3D MODE LOCKED  BOMB: Y  MENU: START" :
            "MOVE: L STICK/DPAD  SHOT: A/RB/RT  MODE: X  BOMB: Y  MENU: START";
    }
    return viewLocked ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  3D MODE LOCKED  BOMB: C  MENU: ESC" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  MODE: X  BOMB: C  MENU: ESC";
}
}

#if defined(_DEBUG)
void SideScrollingShooter::DrawHitboxEllipsoid(
    const Shot& query, const Vector3& center, const Vector3& radii) {
    // 半径をプリミティブの直径へ変換して判定形状を表示する
    assert(query.hitboxRenderer && query.hitboxCamera);
    constexpr float color[4] = {1.0f, 0.0f, 0.0f, 0.24f};
    DrawModelPrimitive(*query.hitboxRenderer, *query.hitboxCamera, 5,
        center.x, center.y, center.z, radii.x * 2.0f, radii.y * 2.0f, radii.z * 2.0f, color);
}

void SideScrollingShooter::DrawEnemyHitbox(
    Renderer& renderer, const Camera3D& camera, const Enemy& enemy) const {
    if (enemy.type == 2 && !enemy.collisionEnabled &&
        !StageDispatch::CanHitBossWhileCollisionDisabled(*this)) return;
    // 自機弾が実際に判定する描画形状を重ねる
    Shot query;
    query.hitboxRenderer = &renderer;
    query.hitboxCamera = &camera;
    HitEnemyModel(query, enemy);
}
#endif

bool SideScrollingShooter::HitShotCircle(const Shot& shot, float x, float y, float radius) {
#if defined(_DEBUG)
    // 2Dの円を軸ごとのワールド倍率で描画し、命中による途中終了を防ぐ
    if (shot.hitboxRenderer && shot.hitboxCamera) {
        constexpr float color[4] = {1.0f, 0.0f, 0.0f, 0.24f};
        DrawModelPrimitive(*shot.hitboxRenderer, *shot.hitboxCamera, 5,
            ToWorldX(x), ToWorldY(y), shot.hitboxSideZ,
            radius * WorldXScale * 2.0f, radius * WorldYScale * 2.0f,
            radius * WorldYScale * 2.0f, color);
        return false;
    }
#endif
    // ボムは描画と同じワールド半径で線分を掃引し、太いレーザー全体を判定する
    if (shot.bomb) return Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), 0.0f,
        ToWorldX(shot.x), ToWorldY(shot.y), 0.0f, shot.hitRadius * WorldXScale,
        ToWorldX(x), ToWorldY(y), 0.0f, radius * WorldXScale);
    // 通常の弾は従来と同じ円判定を使用する
    return Hit(shot.x, shot.y, shot.hitRadius, x, y, radius);
}

bool SideScrollingShooter::HitShotSphere(const Shot& shot, float x, float y, float z, float radius) {
#if defined(_DEBUG)
    // 3Dの判定球を描画し、命中による途中終了を防ぐ
    if (shot.hitboxRenderer && shot.hitboxCamera) {
        constexpr float color[4] = {1.0f, 0.0f, 0.0f, 0.24f};
        DrawModelPrimitive(*shot.hitboxRenderer, *shot.hitboxCamera, 5,
            x, y, z, radius * 2.0f, radius * 2.0f, radius * 2.0f, color);
        return false;
    }
#endif
    // 通常の弾は従来と同じ移動線分判定を使用する
    return Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
        ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale, x, y, z, radius);
}

void SideScrollingShooter::Render(Renderer& renderer, int localPlayer) const {
    // 描画側だけで自機を選び、同期済みの戦闘状態には持ち込まない
    m_renderPlayer = m_networkGame && localPlayer == 1 && m_playerCount == 2 ? 1 : 0;
    // 安定した2D表示では全オブジェクトを同じ奥行きへ固定する
    if (!IsRailRenderActive()) {
        Render2D(renderer);
    } else {
        Render3D(renderer);
    }
#if defined(_DEBUG)
    // 初回描画で通常の命中判定と、状態を変えず全形状を描く問い合わせを自己検証する
    static const bool hitboxCheck = [] {
        Shot shot;
        assert(HitShotCircle(shot, 0.0f, 0.0f, 0.1f));
        assert(!HitShotCircle(shot, 1.0f, 0.0f, 0.1f));
        shot.z = 2.0f;
        shot.vz = 4.0f;
        assert(HitShotSphere(shot, 0.0f, 0.0f, 0.0f, 0.1f));
        assert(!HitShotSphere(shot, 1.0f, 0.0f, 0.0f, 0.1f));
        Renderer checkRenderer;
        Camera3D checkCamera;
        shot.hitboxRenderer = &checkRenderer;
        shot.hitboxCamera = &checkCamera;
        assert(!HitShotCircle(shot, 0.0f, 0.0f, 0.1f));
        assert(!HitShotSphere(shot, 0.0f, 0.0f, 0.0f, 0.1f));
        assert(checkRenderer.CommandCount() == 2);

        // 両視点で全関節が列挙され、破壊済み・水没中・ボス戦中は表示されないことを確認する
        auto hazardCheck = std::make_unique<SideScrollingShooter>();
        for (const ViewMode mode : {ViewMode::Side2D, ViewMode::Rail3D}) {
            hazardCheck->m_viewMode = mode;
            hazardCheck->m_stageNumber = 2;
            hazardCheck->m_stage2.boneArchDestroyed = false;
            checkRenderer.BeginFrame();
            assert(!StageDispatch::HitsHazard(*hazardCheck, 0, 0, 0, 0, &shot));
            assert(checkRenderer.CommandCount() == 13);
            hazardCheck->m_stage2.boneArchDestroyed = true;
            checkRenderer.BeginFrame();
            assert(!StageDispatch::HitsHazard(*hazardCheck, 0, 0, 0, 0, &shot));
            assert(checkRenderer.CommandCount() == 0);
            hazardCheck->m_stageNumber = 3;
            hazardCheck->m_frame = 0;
            hazardCheck->m_bossBattle = false;
            assert(!StageDispatch::HitsHazard(*hazardCheck, 0, 0, 0, 0, &shot));
            assert(checkRenderer.CommandCount() == 0);
            hazardCheck->m_frame = 120;
            assert(!StageDispatch::HitsHazard(*hazardCheck, 0, 0, 0, 0, &shot));
            assert(checkRenderer.CommandCount() > 1);
            hazardCheck->m_bossBattle = true;
            checkRenderer.BeginFrame();
            assert(!StageDispatch::HitsHazard(*hazardCheck, 0, 0, 0, 0, &shot));
            assert(checkRenderer.CommandCount() == 0);
        }
        return true;
    }();
    (void)hitboxCheck;
    if (m_showHitboxes) {
        // 深度に隠れないオーバーレイとして、接触範囲と命中範囲を最後に描画する
        Camera3D camera;
        if (IsRailRenderActive()) ConfigureRailCamera(camera, renderer);
        else ConfigureSideCamera(camera, renderer);
        renderer.SetPipeline(PipelineId::Object);
        for (const auto& enemy : m_enemies) {
            if (!enemy.active) continue;
            DrawEnemyHitbox(renderer, camera, enemy);
        }
        Shot query;
        query.hitboxRenderer = &renderer;
        query.hitboxCamera = &camera;
        StageDispatch::HitsHazard(*this, 0.0f, 0.0f, 0.0f, 0.0f, &query);
        if (m_stageNumber == 5) Stage5Module::DrawTargetHitboxes(*this, query);
        if (m_stageNumber == 3) Stage3Module::DrawTargetHitboxes(*this, query);
    }
#endif
    DrawBossNameReveal(renderer);
    DrawMissionBanner(renderer);
    DrawBossWarning(renderer);
    if (m_tutorialMode) DrawTutorialHud(renderer);
}

/** @brief 両視点共通の基板風ステータスと操作案内を描画する @param renderer 描画先 @return なし */
void SideScrollingShooter::DrawArcadeHud(Renderer& renderer) const {
    constexpr ColorF Gold {1.0f, 0.78f, 0.25f, 1.0f};
    constexpr ColorF White {0.90f, 0.96f, 0.90f, 1.0f};
    constexpr ColorF Empty {0.10f, 0.16f, 0.20f, 1.0f};
    constexpr ColorF Muted {0.55f, 0.70f, 0.72f, 1.0f};

    // 中央の上段にステージ、中段に共有スコアを配置する
    char text[Localization::BufferSize(96)];
    std::snprintf(text, sizeof(text), Localization::Text("STAGE %d/5  CHAPTER %d/3"), m_stageNumber, m_chapterNumber);
    renderer.DrawText(text, TextAlign::Center, 0.011f, Gold, {0.0f, 0.968f}, 0.001f);
    std::snprintf(text, sizeof(text), Localization::Text("SCORE %06d"), m_score);
    renderer.DrawText(text, TextAlign::Center, 0.011f, White, {0.0f, 0.915f}, 0.001f);

    // 左右の枠を固定し、未参加の2Pは空のゲージで表示する
    for (int player = 0; player < 2; ++player) {
        constexpr float width = 0.54f;
        const float left = player == 0 ? -0.96f : 0.42f;
        const bool active = player < m_playerCount;
        const auto& state = m_players[player];
        const ColorF color = !active ? Muted : player == 0 ? ColorF {0.30f, 0.80f, 1.0f, 1.0f} :
            ColorF {0.40f, 1.0f, 0.55f, 1.0f};
        renderer.DrawText(player == 0 ? "1P" : "2P", TextAlign::Center, 0.012f,
            color, {left + width * 0.5f, 0.968f}, 0.001f);

        // 16分割のパワーゲージは端数も正確に満たす
        renderer.DrawText("POWER", TextAlign::Center, 0.010f, color,
            {left + width * 0.12f, 0.919f}, 0.0f);
        const float cellWidth = width * 0.70f / 16.0f;
        for (int cell = 0; cell < 16; ++cell) {
            const float half = cellWidth * 0.40f;
            const float center = left + width * 0.29f + (cell + 0.5f) * cellWidth;
            const float fill = active ? Math::Clamp01(state.m_power / MaxPower * 16.0f - cell) : 0.0f;
            renderer.Draw(Rect {{center, 0.919f}, {half, 0.009f}}, Empty);
            if (fill > 0.0f) renderer.Draw(Rect {{center - half * (1.0f - fill), 0.919f},
                {half * fill, 0.009f}}, state.m_power >= MaxPower ? Gold : color);
        }

        // ボムは3つのドット絵記号で残数と消費済みスロットを区別する
        renderer.DrawText("BOMB", TextAlign::Center, 0.010f, color,
            {left + width * 0.12f, 0.872f}, 0.001f);
        for (int bomb = 0; bomb < MaxBombCount; ++bomb) {
            const ColorF ink = active && bomb < state.m_bombCount ? Gold : Empty;
            const float center = left + width * (0.36f + bomb * 0.23f);
            renderer.Draw(Rect {{center, 0.861f}, {0.010f, 0.007f}}, ink);
            renderer.Draw(Rect {{center, 0.861f}, {0.006f, 0.010f}}, ink);
            renderer.Draw(Rect {{center + 0.004f, 0.874f}, {0.003f, 0.002f}}, ink);
        }
    }

    // 中央下段の航路上で自機マークをゴールフラッグへ進める
    const int progress = ChapterProgressPercent();
    renderer.DrawText("DIST", TextAlign::Center, 0.009f, Gold, {-0.33f, 0.868f}, 0.0f);
    renderer.Draw(Rect {{0.045f, 0.853f}, {0.285f, 0.0015f}}, Muted);
    for (int tick = 0; tick <= 8; ++tick) {
        renderer.Draw(Rect {{-0.24f + tick * 0.07125f, 0.857f}, {0.001f, 0.004f}}, Muted);
    }
    const float shipX = -0.24f + 0.57f * Math::Clamp01(progress / 100.0f);
    renderer.Draw(Rect {{shipX, 0.867f}, {0.018f, 0.005f}}, White);
    renderer.Draw(Rect {{shipX - 0.005f, 0.867f}, {0.007f, 0.014f}}, Gold);
    renderer.Draw(Rect {{shipX + 0.021f, 0.867f}, {0.004f, 0.003f}}, White);
    renderer.Draw(Rect {{0.36f, 0.872f}, {0.0015f, 0.022f}}, White);
    for (int row = 0; row < 2; ++row) for (int column = 0; column < 3; ++column) {
        renderer.Draw(Rect {{0.366f + column * 0.009f, 0.890f - row * 0.009f},
            {0.0045f, 0.0045f}}, (row + column) % 2 == 0 ? White : Empty);
    }
    renderer.DrawText(m_networkGame ? "ONLINE CO-OP  SHARED SCORE  VIEW: HOST" :
        m_playerCount == 2 ? "LOCAL CO-OP  SHARED SCORE  VIEW: 1P  PAUSE: START/+" :
        HudControlHint(StageDispatch::IsViewLocked(*this)), TextAlign::Center, 0.010f,
        Muted, {0.0f, -0.935f}, 0.0f);
}

/** @brief 戦闘更新や音声再生を行わず通常の2D背景と鑑賞用の飛行機を描画する @param renderer 描画先 @param stage ステージ番号 @param frame 経過フレーム @return なし */
void SideScrollingShooter::RenderBackgroundPreview(Renderer& renderer, int stage, int frame) {
    // 鑑賞用の時間とステージだけを設定し、敵生成や保存処理へ進めない
    m_stageNumber = (std::clamp)(stage, 1, 5);
    m_frame = (std::max)(0, frame);
    m_scroll = static_cast<float>(m_frame) * 0.008f;
    renderer.ResetCamera();
    StageDispatch::DrawSky(*this, renderer);
    Camera3D camera;
    ConfigureSideCamera(camera, renderer);
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);
    StageDispatch::DrawBackground2D(*this, renderer, camera);
    // 戦闘用の敵を生成せず、既存の飛行機モデルを滑らかな周回軌道に置く
    const Matrix4x4 viewProjection = camera.ProjectionMatrix() * camera.ViewMatrix();
    for (int i = 0; i < 4; ++i) {
        const float phase = static_cast<float>(m_frame % 21600) * (Math::Pi / 10800.0f) *
            (5 + i) + i * Math::HalfPi;
        const Vector3 position {24.0f * std::sin(phase),
            2.8f + i * 0.65f + 1.3f * std::sin(phase * 2.0f),
            SidePlaneZ + 3.0f + 2.0f * std::cos(phase)};
        const float yaw = std::atan2(24.0f * std::cos(phase), -2.0f * std::sin(phase));
        StageEnemyModelView::Draw(1 + i % 2, position, yaw, 0.65f + i * 0.08f,
            [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                renderer.Draw({shape, viewProjection * world, Vector3::One, color});
            });
    }
    renderer.ResetCamera();
}

void SideScrollingShooter::Render2D(Renderer& renderer) const {
    StageDispatch::DrawSky(*this, renderer);

    Camera3D camera;
    ConfigureSideCamera(camera, renderer);
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    StageDispatch::DrawBackground2D(*this, renderer, camera);

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        Enemy sideEnemy = enemy;
        sideEnemy.z = SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f);
        DrawEnemyModel(renderer, camera, sideEnemy, Math::HalfPi);
    }
    DrawLinkedEnemyLasers(renderer, camera, 0.0f);
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot sideShot = shot;
        sideShot.z = SidePlaneZ + (shot.enemy ? 1.0f : -0.4f);
        DrawShotModel(renderer, camera, sideShot, Math::HalfPi);
    }
    ForEachPlayer([&] {
        if (Player().m_bomb.active) {
            DrawBomb(renderer, camera, Player().m_bomb);
        }
    });
    for (const auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        Explosion sideExplosion = explosion;
        sideExplosion.z = SidePlaneZ - 0.8f;
        DrawExplosion(renderer, camera, sideExplosion);
    }
    for (const auto& debris : m_debris) {
        if (!debris.active) continue;
        Debris sideDebris = debris;
        sideDebris.z = SidePlaneZ - 0.6f;
        DrawDebris(renderer, camera, sideDebris, 0.0f);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        Item sideItem = item;
        sideItem.z = SidePlaneZ - 0.2f;
        DrawItemModel(renderer, camera, sideItem, 0.0f);
    }
    ForEachPlayer([&] {
        const bool playerVisible = Player().m_playerDestructionTimer == 0 &&
            (m_tutorialMode || Player().m_invincible == 0 || (Player().m_invincible / 5) % 2 == 0);
        const bool verticalSide = m_stageNumber == 5 &&
            ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase);
        DrawPlayerModel(renderer, camera, ToWorldX(Player().m_playerX), ToWorldY(Player().m_playerY),
            SidePlaneZ, playerVisible, Math::HalfPi, 0.0f,
            verticalSide ? Math::HalfPi : 0.0f);
    });
    // 半透明の砂粒は船体と弾の描画後に重ねる
    if (m_stageNumber == 2) Stage2Module::DrawSandstorm(*this, renderer, camera);

    // 全自機の描画後に深度テストなしで被弾判定を重ね、機体内部でも表示する
    renderer.SetPipeline(PipelineId::Object);
    ForEachPlayer([&] {
        const bool playerVisible = Player().m_playerDestructionTimer == 0 &&
            (m_tutorialMode || Player().m_invincible == 0 || (Player().m_invincible / 5) % 2 == 0);
        if (Player().m_slowMove && playerVisible) {
            // 2D判定の画面比率をワールド寸法へ変換して表示する
            DrawModelPrimitive(renderer, camera, 5, ToWorldX(Player().m_playerX), ToWorldY(Player().m_playerY), SidePlaneZ,
                PlayerHitRadius2D * WorldXScale * 2.0f,
                PlayerHitRadius2D * WorldYScale * 2.0f,
                PlayerHitRadius2D * WorldYScale * 2.0f, PlayerHitboxColor);
        }

    });
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.ResetCamera();
    DrawHudBackground(renderer);
    StageDispatch::DrawOverlay2D(*this, renderer);
    ForEachPlayer([&] { DrawPowerUp(renderer, camera, SidePlaneZ); });
    DrawTutorialControlHint(renderer, camera, SidePlaneZ);
    DrawViewToggleCooldownHud(renderer, camera, SidePlaneZ);

    // チュートリアル固有HUDだけを描画し、通常ステージ情報との重なりを防ぐ
    if (m_tutorialMode) return;

    DrawArcadeHud(renderer);

    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
}

void SideScrollingShooter::Render3D(Renderer& renderer) const {
    StageDispatch::DrawSky(*this, renderer);

    Camera3D camera;
    ConfigureRailCamera(camera, renderer);
    const float railWeight = IsTayamaBattle() ? 1.0f : RailBlend();
    const Vector3 gameplayPlayerPosition = PlayerWorldPosition();
    const float playerYaw = IsTayamaBattle() ?
        std::atan2(-gameplayPlayerPosition.x,
            ShooterStages::Stage5::TayamaArenaCenterZ - gameplayPlayerPosition.z) :
        Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    const float enemyYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    const bool verticalSide = m_stageNumber == 5 &&
        ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase);
    const float playerRoll = verticalSide ?
        Math::Lerp(Math::HalfPi, 0.0f, railWeight) : 0.0f;
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(camera);

    StageDispatch::DrawBackground3D(*this, renderer, camera, railWeight);

    StageDispatch::DrawStageWorld3D(*this, renderer, camera);

    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        if (!StageDispatch::ShouldDrawEnemy(*this, enemy)) continue;
        Enemy drawEnemy = enemy;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? enemy.transitionSideX :
            (exitingRail ? enemy.x : ToSideXFromRailZ(enemy.z));
        const float sideY = enteringRail ? enemy.transitionSideY : enemy.y;
        drawEnemy.x = Math::Lerp(sideX, exitingRail ? enemy.transitionSideX : enemy.x, railWeight);
        drawEnemy.y = Math::Lerp(sideY, exitingRail ? enemy.transitionSideY : enemy.y, railWeight);
        const float railZ = exitingRail ? enemy.transitionRailZ : enemy.z;
        drawEnemy.z = Math::Lerp(SidePlaneZ + (enemy.type == 2 ? 2.2f : 1.5f), railZ, railWeight);
        if (enemy.type != 2) {
            const float groundTopY = StageDispatch::RailGroundY(*this);
            const float minimumRailY = FromWorldY(groundTopY + 0.32f);
            drawEnemy.y = Math::Lerp(drawEnemy.y, (std::max)(drawEnemy.y, minimumRailY), railWeight);
        }

        // レール3Dへ入るほど機体直下の影を表示する
        if (railWeight > 0.01f) {
            const float groundTopY = StageDispatch::RailGroundY(*this);
            const bool isBoss = enemy.type == 2;
            DrawBlobShadow(renderer, camera, ToWorldX(drawEnemy.x), drawEnemy.z, groundTopY,
                isBoss ? 2.4f : 0.72f, isBoss ? 2.0f : 0.58f,
                railWeight * (isBoss ? 0.34f : 0.26f));
        }
        DrawEnemyModel(renderer, camera, drawEnemy, enemyYaw);
    }
    DrawLinkedEnemyLasers(renderer, camera, railWeight);
    for (const auto& shot : m_shots) {
        if (!shot.active) continue;
        Shot drawShot = shot;
        const bool enteringRail = m_viewTransitionTimer > 0 && m_nextViewMode == ViewMode::Rail3D;
        const bool exitingRail = m_viewTransitionTimer > 0 && m_viewMode == ViewMode::Rail3D;
        const float sideX = enteringRail ? shot.transitionSideX : shot.x;
        const float sideY = enteringRail ? shot.transitionSideY : shot.y;
        drawShot.x = Math::Lerp(sideX, exitingRail ? shot.transitionSideX : shot.x, railWeight);
        drawShot.y = Math::Lerp(sideY, shot.y, railWeight);
        drawShot.z = Math::Lerp(SidePlaneZ + (shot.enemy ? 1.0f : -0.4f), shot.z, railWeight);
        DrawShotModel(renderer, camera, drawShot, shot.enemy ? enemyYaw : playerYaw);
    }
    ForEachPlayer([&] { if (Player().m_bomb.active) DrawBomb(renderer, camera, Player().m_bomb); });
    for (const auto& explosion : m_explosions) {
        if (!explosion.active) continue;
        // 2Dではレール変換済みの奥行きで船体背後へ隠れないよう前景面へ寄せる
        Explosion drawExplosion = explosion;
        drawExplosion.z = Math::Lerp(SidePlaneZ - 0.4f, explosion.z, railWeight);
        DrawExplosion(renderer, camera, drawExplosion);
    }
    for (const auto& debris : m_debris) {
        if (!debris.active) continue;
        DrawDebris(renderer, camera, debris, railWeight);
    }
    for (const auto& item : m_items) {
        if (!item.active) continue;
        DrawItemModel(renderer, camera, item, 0.0f);
    }
    ForEachPlayer([&] {
        const bool cinematic = StageDispatch::IsCinematic(*this);
        const Vector3 gameplayPlayerPosition = PlayerWorldPosition();
        const float playerYaw = IsTayamaBattle() ? std::atan2(-gameplayPlayerPosition.x,
            ShooterStages::Stage5::TayamaArenaCenterZ - gameplayPlayerPosition.z) :
            Math::Lerp(Math::HalfPi, 0.0f, railWeight);
        const bool playerVisible = Player().m_playerDestructionTimer == 0 &&
            (cinematic || m_tutorialMode || Player().m_invincible == 0 || (Player().m_invincible / 5) % 2 == 0);
        Vector3 playerPosition = IsTayamaBattle() ? gameplayPlayerPosition :
            Vector3 {ToWorldX(Player().m_playerX), ToWorldY(Player().m_playerY),
                Math::Lerp(SidePlaneZ, PlayerRailDepth(), railWeight)};
        float playerPitch = 0.0f;
        StageDispatch::ApplyPlayerRenderCorrection(*this, playerPosition, playerPitch);
        if (!cinematic && railWeight > 0.01f && playerVisible) {
            const float groundTopY = StageDispatch::RailGroundY(*this);
            DrawBlobShadow(renderer, camera, playerPosition.x,
                playerPosition.z, groundTopY,
                1.05f, 0.82f, railWeight * 0.30f);
        }
        DrawPlayerModel(renderer, camera, playerPosition.x, playerPosition.y,
            playerPosition.z, playerVisible, playerYaw, playerPitch, playerRoll);
    });
    // 遷移中も同じカメラから砂嵐を描画して2D終点と一致させる
    if (m_stageNumber == 2) Stage2Module::DrawSandstorm(*this, renderer, camera);

    // 全自機の描画後に深度テストなしで被弾判定を重ね、機体内部でも表示する
    renderer.SetPipeline(PipelineId::Object);
    ForEachPlayer([&] {
        const bool cinematic = StageDispatch::IsCinematic(*this);
        const bool playerVisible = Player().m_playerDestructionTimer == 0 &&
            (cinematic || m_tutorialMode || Player().m_invincible == 0 || (Player().m_invincible / 5) % 2 == 0);
        if (!cinematic && Player().m_slowMove && playerVisible) {
            Vector3 playerPosition = IsTayamaBattle() ? PlayerWorldPosition() :
                Vector3 {ToWorldX(Player().m_playerX), ToWorldY(Player().m_playerY),
                    Math::Lerp(SidePlaneZ, PlayerRailDepth(), railWeight)};
            float playerPitch = 0.0f;
            StageDispatch::ApplyPlayerRenderCorrection(*this, playerPosition, playerPitch);
            // 視点遇移中も実際の2D/3D被弾半径に連続して追従する
            const float hitboxWidth = Math::Lerp(
                PlayerHitRadius2D * WorldXScale * 2.0f, PlayerHitRadius3D * 2.0f, railWeight);
            const float hitboxHeight = Math::Lerp(
                PlayerHitRadius2D * WorldYScale * 2.0f, PlayerHitRadius3D * 2.0f, railWeight);
            DrawModelPrimitive(renderer, camera, 5, playerPosition.x, playerPosition.y,
                playerPosition.z,
                hitboxWidth, hitboxHeight, hitboxHeight, PlayerHitboxColor);
        }
    });
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.ResetCamera();
    DrawHudBackground(renderer);
    StageDispatch::DrawOverlay3D(*this, renderer, camera);
    if (StageDispatch::IsCinematic(*this)) return;
    ForEachPlayer([&] {
        if (!m_networkGame || m_activePlayer == m_renderPlayer) DrawReticle(renderer, camera);
    });
    const float playerZ = IsTayamaBattle() ? PlayerWorldPosition().z : Math::Lerp(SidePlaneZ, PlayerRailDepth(), railWeight);
    ForEachPlayer([&] { DrawPowerUp(renderer, camera, IsTayamaBattle() ? PlayerWorldPosition().z : playerZ); });
    DrawTutorialControlHint(renderer, camera, playerZ);
    DrawViewToggleCooldownHud(renderer, camera, playerZ);

    // チュートリアル固有HUDだけを描画し、通常ステージ情報との重なりを防ぐ
    if (m_tutorialMode) return;

    DrawArcadeHud(renderer);
    if (m_viewTransitionTimer > 0) {
#if defined(SPACEYAKUZA_EDITION_Online) || defined(SPACEYAKUZA_EDITION_Steam)
        // 視点切り替えの案内も言語によらず画面中央へ配置する
        renderer.DrawText("CAMERA SHIFT", TextAlign::Center, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f }, {0.0f, -0.02f});
#else
        renderer.DrawText("CAMERA SHIFT", { -0.16f, -0.02f }, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f });
#endif
    }
    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
}
