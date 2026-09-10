#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "Stages/Common/StageDispatch.h"
#include "Stages/Stage2/Stage2Module.h"


#include "SideScrollingShooterEnemies.h"
#include "Stages/Common/StageDefinition.h"
#if defined(_DEBUG)
#include <cassert>
#include "Stages/Stage3/Stage3Module.h"
#include "Stages/Stage5/Stage5Module.h"
#endif

namespace {
constexpr float PlayerHitboxColor[4] = {1.0f, 0.08f, 0.08f, 0.24f};
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
    // 描画専用問い合わせで実際のボス部位判定を最後まで走査する
    Shot query;
    query.hitboxRenderer = &renderer;
    query.hitboxCamera = &camera;
    query.hitboxSideZ = enemy.z;
    if (enemy.type == 2 && (!m_chapterResultActive || enemy.collisionEnabled) &&
        (enemy.collisionEnabled || StageDispatch::CanHitBossWhileCollisionDisabled(*this))) {
        BossPart part = BossNose;
        TryHitBossPart(query, enemy, part);
        StageDispatch::TryHitBossBody(*this, query, enemy);
        StageDispatch::BlocksPlayerShot(*this, query, enemy);
    }

    // 接触判定が無効なボスは共通球の表示対象から除外する
    if (enemy.type == 2 && !enemy.collisionEnabled) return;
    const auto& behavior = enemy.behavior ? *enemy.behavior : EnemyBehaviorForType(enemy.type);
    constexpr float color[4] = {1.0f, 0.0f, 0.0f, 0.24f};

    // 衝突処理と同じ半径を使い、2D判定は軸ごとのワールド倍率を反映する
    const bool rail = IsRailGameplayActive();
    const float diameter = 2.0f * (rail ? behavior.CollisionRadius3D(enemy) : behavior.CollisionRadius(enemy));
    DrawModelPrimitive(renderer, camera, 5, ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z,
        rail ? diameter : diameter * WorldXScale,
        rail ? diameter : diameter * WorldYScale,
        rail ? diameter : diameter * WorldYScale, color);

    // 3Dでは接触範囲とは異なる自機弾の命中半径も表示する
    if (rail && (!m_chapterResultActive || enemy.collisionEnabled)) {
        float radius = behavior.ShotHitRadius3D(enemy);
        if (m_stageNumber == 5 && ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase)) {
            radius *= ShooterStages::Stage5::Part2EnemyScaleMultiplier(RailBlend());
        }
        HitShotSphere(query, ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z, radius);
    }
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

void SideScrollingShooter::Render(Renderer& renderer) const {
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
    // 深度に隠れないオーバーレイとして、接触範囲と命中範囲を最後に描画する
    Camera3D camera;
    if (IsRailRenderActive()) ConfigureRailCamera(camera, renderer);
    else ConfigureSideCamera(camera, renderer);
    renderer.SetPipeline(PipelineId::Object);
    for (const auto& enemy : m_enemies) {
        if (!enemy.active) continue;
        Enemy target = enemy;
        if (!IsRailGameplayActive()) target.z = SidePlaneZ;
        DrawEnemyHitbox(renderer, camera, target);
    }
    Shot query;
    query.hitboxRenderer = &renderer;
    query.hitboxCamera = &camera;
    StageDispatch::HitsHazard(*this, 0.0f, 0.0f, 0.0f, 0.0f, &query);
    if (m_stageNumber == 5) Stage5Module::DrawTargetHitboxes(*this, query);
    if (m_stageNumber == 3) Stage3Module::DrawTargetHitboxes(*this, query);
#endif
    DrawBossNameReveal(renderer);
    DrawMissionBanner(renderer);
    DrawBossWarning(renderer);
    if (m_tutorialMode) DrawTutorialHud(renderer);
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
    if (m_bomb.active) {
        Bomb sideBomb = m_bomb;
        sideBomb.z = SidePlaneZ - 0.5f;
        DrawBomb(renderer, camera, sideBomb);
    }
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
    const bool playerVisible = m_playerDestructionTimer == 0 &&
        (m_tutorialMode || m_invincible == 0 || (m_invincible / 5) % 2 == 0);
    const bool verticalSide = m_stageNumber == 5 &&
        ShooterStages::Stage5::IsPart2RoutePhase(m_stage5.phase);
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        SidePlaneZ, playerVisible, Math::HalfPi, 0.0f,
        verticalSide ? Math::HalfPi : 0.0f);
    if (m_slowMove && playerVisible) {
        // 2D判定の画面比率をワールド寸法へ変換して表示する
        DrawModelPrimitive(renderer, camera, 5, ToWorldX(m_playerX), ToWorldY(m_playerY), SidePlaneZ,
            PlayerHitRadius2D * WorldXScale * 2.0f,
            PlayerHitRadius2D * WorldYScale * 2.0f,
            PlayerHitRadius2D * WorldYScale * 2.0f, PlayerHitboxColor);
    }

    // 半透明の砂粒は船体と弾の描画後に重ねる
    if (m_stageNumber == 2) Stage2Module::DrawSandstorm(*this, renderer, camera);
    renderer.ResetCamera();
    DrawHudBackground(renderer);
    StageDispatch::DrawOverlay2D(*this, renderer);
    DrawPowerUp(renderer, camera, SidePlaneZ);
    DrawTutorialControlHint(renderer, camera, SidePlaneZ);
    DrawViewToggleCooldownHud(renderer, camera, SidePlaneZ);

    // チュートリアル固有HUDだけを描画し、通常ステージ情報との重なりを防ぐ
    if (m_tutorialMode) return;

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    char bombStatus[16];
    const int progress = ChapterProgressPercent();
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    std::snprintf(bombStatus, sizeof(bombStatus), "BOMB %d", m_bombCount);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText(bombStatus, TextAlign::TopCenter, 0.014f, { 0.55f, 0.85f, 1.0f, 1.0f }, { 0.0f, -0.025f });
    renderer.DrawText(HudControlHint(StageDispatch::IsViewLocked(*this)),
        { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });

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
    if (m_bomb.active) DrawBomb(renderer, camera, m_bomb);
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
    const bool cinematic = StageDispatch::IsCinematic(*this);
    const bool playerVisible = m_playerDestructionTimer == 0 &&
        (cinematic || m_tutorialMode || m_invincible == 0 || (m_invincible / 5) % 2 == 0);
    Vector3 playerPosition = IsTayamaBattle() ? gameplayPlayerPosition :
        Vector3 {ToWorldX(m_playerX), ToWorldY(m_playerY),
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
    if (!cinematic && m_slowMove && playerVisible) {
        // 視点遇移中も実際の2D/3D被弾半径に連続して追従する
        const float hitboxWidth = Math::Lerp(
            PlayerHitRadius2D * WorldXScale * 2.0f, PlayerHitRadius3D * 2.0f, railWeight);
        const float hitboxHeight = Math::Lerp(
            PlayerHitRadius2D * WorldYScale * 2.0f, PlayerHitRadius3D * 2.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 5, playerPosition.x, playerPosition.y,
            playerPosition.z,
            hitboxWidth, hitboxHeight, hitboxHeight, PlayerHitboxColor);
    }
    // 遷移中も同じカメラから砂嵐を描画して2D終点と一致させる
    if (m_stageNumber == 2) Stage2Module::DrawSandstorm(*this, renderer, camera);
    renderer.ResetCamera();
    DrawHudBackground(renderer);
    StageDispatch::DrawOverlay3D(*this, renderer, camera);
    if (StageDispatch::IsCinematic(*this)) return;
    const float playerZ = playerPosition.z;
    DrawPowerUp(renderer, camera, playerZ);
    DrawTutorialControlHint(renderer, camera, playerZ);
    DrawViewToggleCooldownHud(renderer, camera, playerZ);

    // チュートリアル固有HUDだけを描画し、通常ステージ情報との重なりを防ぐ
    if (m_tutorialMode) return;

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    char bombStatus[16];
    const int progress = ChapterProgressPercent();
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    std::snprintf(bombStatus, sizeof(bombStatus), "BOMB %d", m_bombCount);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText(bombStatus, TextAlign::TopCenter, 0.014f, { 0.55f, 0.85f, 1.0f, 1.0f }, { 0.0f, -0.025f });
    renderer.DrawText(HudControlHint(StageDispatch::IsViewLocked(*this)),
        { -0.92f, -0.92f }, 0.012f,
        { 0.55f, 0.70f, 0.65f, 1.0f });
    if (m_viewTransitionTimer > 0) {
        renderer.DrawText("CAMERA SHIFT", { -0.16f, -0.02f }, 0.026f,
            { 0.55f, 0.85f, 1.0f, 1.0f });
    }
    DrawBossHud(renderer);
    DrawChapterResult(renderer);
    DrawRestart(renderer);
    DrawBossStory(renderer);
}
