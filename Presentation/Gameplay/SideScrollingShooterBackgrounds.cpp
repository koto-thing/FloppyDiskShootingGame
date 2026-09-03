#include "SideScrollingShooter.h"

#include <algorithm>
#include <cstdio>

#include "../../Engine/Graphics/Renderer.h"
#include "Stages/Common/StageDispatch.h"


#include "SideScrollingShooterEnemies.h"
#include "Stages/Common/StageDefinition.h"

namespace {
constexpr float PlayerHitboxColor[4] = {1.0f, 0.08f, 0.08f, 0.24f};
constexpr float PlayerHitRadius2D = 0.050f;
constexpr float PlayerHitRadius3D = 0.38f;
}

void SideScrollingShooter::Render(Renderer& renderer) const {
    // 安定した2D表示では全オブジェクトを同じ奥行きへ固定する
    if (!IsRailRenderActive()) {
        Render2D(renderer);
    } else {
        Render3D(renderer);
    }
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
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        SidePlaneZ, playerVisible, Math::HalfPi);
    if (m_slowMove && playerVisible) {
        // 2D判定の画面比率をワールド寸法へ変換して表示する
        DrawModelPrimitive(renderer, camera, 5, ToWorldX(m_playerX), ToWorldY(m_playerY), SidePlaneZ,
            PlayerHitRadius2D * WorldXScale * 2.0f,
            PlayerHitRadius2D * WorldYScale * 2.0f,
            PlayerHitRadius2D * WorldYScale * 2.0f, PlayerHitboxColor);
    }

    renderer.ResetCamera();
    DrawHudBackground(renderer);
    StageDispatch::DrawOverlay2D(*this, renderer);
    DrawPowerUp(renderer, camera, SidePlaneZ);
    DrawTutorialControlHint(renderer, camera, SidePlaneZ);
    DrawViewToggleCooldownHud(renderer, camera, SidePlaneZ);
    DrawAttackWarnings2D(renderer);

    // チュートリアル固有HUDだけを描画し、通常ステージ情報との重なりを防ぐ
    if (m_tutorialMode) return;

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / m_stage->BossStartDistance() * 100.0f));
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText(StageDispatch::IsViewLocked(*this) ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  BOMB: C  3D MODE LOCKED" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  BOMB: C  MODE: X", { -0.92f, -0.92f }, 0.012f,
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
    const float railWeight = RailBlend();
    const float playerYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
    const float enemyYaw = Math::Lerp(Math::HalfPi, 0.0f, railWeight);
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
    const bool playerVisible = m_playerDestructionTimer == 0 &&
        (m_tutorialMode || m_invincible == 0 || (m_invincible / 5) % 2 == 0);
    if (railWeight > 0.01f && playerVisible) {
        const float groundTopY = StageDispatch::RailGroundY(*this);
        DrawBlobShadow(renderer, camera, ToWorldX(m_playerX),
            Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight), groundTopY,
            1.05f, 0.82f, railWeight * 0.30f);
    }
    DrawPlayerModel(renderer, camera, ToWorldX(m_playerX), ToWorldY(m_playerY),
        Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight),
        playerVisible, playerYaw);
    if (m_slowMove && playerVisible) {
        // 視点遇移中も実際の2D/3D被弾半径に連続して追従する
        const float hitboxWidth = Math::Lerp(
            PlayerHitRadius2D * WorldXScale * 2.0f, PlayerHitRadius3D * 2.0f, railWeight);
        const float hitboxHeight = Math::Lerp(
            PlayerHitRadius2D * WorldYScale * 2.0f, PlayerHitRadius3D * 2.0f, railWeight);
        DrawModelPrimitive(renderer, camera, 5, ToWorldX(m_playerX), ToWorldY(m_playerY),
            Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight),
            hitboxWidth, hitboxHeight, hitboxHeight, PlayerHitboxColor);
    }
    DrawAttackWarnings3D(renderer, camera, railWeight);

    renderer.ResetCamera();
    DrawHudBackground(renderer);
    StageDispatch::DrawOverlay3D(*this, renderer);
    const float playerZ = Math::Lerp(SidePlaneZ, PlayerRailZ, railWeight);
    DrawPowerUp(renderer, camera, playerZ);
    DrawTutorialControlHint(renderer, camera, playerZ);
    DrawViewToggleCooldownHud(renderer, camera, playerZ);

    // チュートリアル固有HUDだけを描画し、通常ステージ情報との重なりを防ぐ
    if (m_tutorialMode) return;

    char stageStatus[48];
    char scoreStatus[32];
    char powerStatus[32];
    char progressStatus[32];
    const int progress = (std::min)(100,
        static_cast<int>(m_scroll / m_stage->BossStartDistance() * 100.0f));
    std::snprintf(stageStatus, sizeof(stageStatus), "STAGE %d/5  CHAPTER %d/3", m_stageNumber, m_chapterNumber);
    std::snprintf(scoreStatus, sizeof(scoreStatus), "SCORE %06d", m_score);
    std::snprintf(powerStatus, sizeof(powerStatus), "POWER %.2f / %.2f", m_power, MaxPower);
    std::snprintf(progressStatus, sizeof(progressStatus), "DIST %03d%%", progress);
    renderer.DrawText(stageStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.025f });
    renderer.DrawText(scoreStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.025f });
    renderer.DrawText(powerStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { -0.48f, -0.085f });
    renderer.DrawText(progressStatus, TextAlign::TopCenter, 0.014f, { 0.75f, 0.95f, 0.85f, 1.0f }, { 0.48f, -0.085f });
    renderer.DrawText(StageDispatch::IsViewLocked(*this) ?
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  BOMB: C  3D MODE LOCKED" :
        "MOVE: ARROWS/WASD  SHOT: Z/SPACE  BOMB: C  MODE: X", { -0.92f, -0.92f }, 0.012f,
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
