#include "StageDispatch.h"

#include "StageDefinition.h"

#include "../Stage1/Stage1Module.h"
#include "../Stage2/Stage2Module.h"
#include "../Stage3/Stage3Module.h"
#include "../Stage4/Stage4Module.h"
#include "../Stage5/Stage5Module.h"
#include "CityBackgroundModule.h"
#include "../Stage1/Stage1Story.h"
#include "../Stage2/Stage2Story.h"
#include "../Stage3/Stage3Story.h"
#include "../Stage4/Stage4Story.h"
#include "../Stage5/Stage5Story.h"

const SideScrollingShooter::Stage& SideScrollingShooter::StageDispatch::Definition(
    int stageNumber, DifficultyType difficulty) {
    switch (stageNumber) {
    case 1: return Stage1Module::Definition(difficulty);
    case 2: return Stage2Module::Definition(difficulty);
    case 3: return Stage3Module::Definition();
    case 4: return Stage4Module::Definition();
    case 5: return Stage5Module::Definition();
    default: return Stage1Module::Definition(difficulty);
    }
}

BossStory SideScrollingShooter::StageDispatch::Story(int stageNumber) {
    switch (stageNumber) {
    case 2: return ShooterStages::Stage2::Story();
    case 3: return ShooterStages::Stage3::Story();
    case 4: return ShooterStages::Stage4::Story();
    case 5: return ShooterStages::Stage5::Story();
    default: return ShooterStages::Stage1::Story();
    }
}

void SideScrollingShooter::StageDispatch::ResetGimmicks(SideScrollingShooter& shooter) {
    Stage1Module::Reset(shooter);
    Stage2Module::Reset(shooter);
}

void SideScrollingShooter::StageDispatch::ResetScriptState(SideScrollingShooter& shooter) {
    Stage5Module::Reset(shooter);
}

void SideScrollingShooter::StageDispatch::ProcessDebugInput(SideScrollingShooter& shooter) {
    Stage5Module::ProcessDebugInput(shooter);
}

bool SideScrollingShooter::StageDispatch::HandleDebugBossInput(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::StartDebugPhase(shooter, Stage5Phase::EastsourceBattle);
        return true;
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::StartDebugBoss(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::StartDebugBoss(shooter);
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::TickBeforeFrame(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::TickBeforeFrame(shooter);
        break;
    }
}

void SideScrollingShooter::StageDispatch::TickAfterFrame(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::TickAfterFrame(shooter);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::UsesChapterTimeline(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::UsesChapterTimeline(shooter);
    default: return true;
    }
}

bool SideScrollingShooter::StageDispatch::HandleChapterResult(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::HandleChapterResult(shooter);
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::OnChapterStarted(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::OnChapterStarted(shooter);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::TryRestartCheckpoint(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5:
        if (Stage5Module::UsesChapterTimeline(shooter)) return false;
        Stage5Module::RestartCheckpoint(shooter);
        return true;
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::TickWorld(SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 1:
        Stage1Module::TickWorld(shooter);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::HandleBossInteractionAfterTick(
    SideScrollingShooter& shooter, Enemy& boss) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::HandleBossInteractionAfterTick(shooter, boss);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::IsBossSpecialAttackActive(
    const SideScrollingShooter& shooter, const Enemy& boss) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::IsBossSpecialAttackActive(shooter, boss);
    default: return shooter.m_stage->IsBossSpecialAttackActive(shooter, boss);
    }
}

bool SideScrollingShooter::StageDispatch::HitsHazard(const SideScrollingShooter& shooter,
    float x, float y, float z, float radius) {
    switch (shooter.m_stageNumber) {
    case 1: return Stage1Module::HitsHazard(shooter, x, y, z, radius);
    case 2: return Stage2Module::HitsHazard(shooter, x, y, z, radius);
    case 3: return Stage3Module::HitsHazard(shooter, x, y, z, radius);
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::DrawSky(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    switch (shooter.m_stageNumber) {
    case 2:
        Stage2Module::DrawSky(shooter, renderer);
        break;
    case 3:
        Stage3Module::DrawSky(shooter, renderer);
        break;
    case 4:
    case 5:
        CityBackgroundModule::DrawSky(renderer);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::TryDamageStageTarget(
    SideScrollingShooter& shooter, Shot& shot) {
    switch (shooter.m_stageNumber) {
    case 1: return Stage1Module::TryDamageTarget(shooter, shot);
    case 2: return Stage2Module::TryDamageStageTarget(shooter, shot);
    case 5: return Stage5Module::TryDamageStageTarget(shooter, shot);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::TryHitBossPart(
    const SideScrollingShooter& shooter, const Shot& shot,
    const Enemy& boss, BossPart& part) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::TryHitBossPart(shooter, shot, boss, part);
    case 4: return Stage4Module::TryHitBossPart(shooter, shot, boss, part);
    case 5: return Stage5Module::TryHitBossPart(shooter, shot, boss, part);
    default: return shooter.TryHitDefaultBossPart(shot, boss, part);
    }
}

bool SideScrollingShooter::StageDispatch::TryHitBossBody(
    const SideScrollingShooter& shooter, const Shot& shot, const Enemy& boss) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::TryHitBossBody(shooter, shot, boss);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::CanHitBossWhileCollisionDisabled(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 2: return true;
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::FireBossPartBarrage(
    SideScrollingShooter& shooter, const Enemy& boss) {
    switch (shooter.m_stageNumber) {
    case 2:
        Stage2Module::FireBossPartBarrage(shooter, boss);
        break;
    case 4:
        Stage4Module::FireBossPartBarrage(shooter, boss);
        break;
    default:
        shooter.FireBossPartBarrage(boss);
        break;
    }
}

void SideScrollingShooter::StageDispatch::TickSpecialShotBeforeMove(
    SideScrollingShooter& shooter, Shot& shot) {
    switch (shooter.m_stageNumber) {
    case 2:
        Stage2Module::TickSpecialShotBeforeMove(shooter, shot);
        break;
    case 4:
        Stage4Module::TickSpecialShotBeforeMove(shooter, shot);
        break;
    }
}

void SideScrollingShooter::StageDispatch::TickSpecialShotAfterMove(
    SideScrollingShooter& shooter, Shot& shot,
    float previousX, float previousY, float previousZ) {
    switch (shooter.m_stageNumber) {
    case 2:
        Stage2Module::TickSpecialShotAfterMove(shooter, shot, previousY);
        break;
    case 4:
        Stage4Module::TickSpecialShotAfterMove(
            shooter, shot, previousX, previousY, previousZ);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::IsShotCullProtected(
    const SideScrollingShooter& shooter, const Shot& shot) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::IsShotCullProtected(shot);
    case 4: return Stage4Module::IsShotCullProtected(shot);
    default: return false;
    }
}

float SideScrollingShooter::StageDispatch::EnemyShotHitRadius(
    const SideScrollingShooter& shooter, const Shot& shot) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::EnemyShotHitRadius(shot, shooter.IsRailGameplayActive());
    case 4: return Stage4Module::EnemyShotHitRadius(shot, shooter.IsRailGameplayActive());
    default: return shooter.IsRailGameplayActive() ? 0.28f : 0.022f;
    }
}

bool SideScrollingShooter::StageDispatch::TickSpecialDebris(
    SideScrollingShooter& shooter, Debris& debris) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::TickSpecialDebris(shooter, debris);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::HandleBossDefeat(
    SideScrollingShooter& shooter, Enemy& boss) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::HandleBossDefeat(shooter, boss);
    case 5: return Stage5Module::HandleBossDefeat(shooter, boss);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::CanReplacePlayerShot(
    const SideScrollingShooter& shooter, bool enemy) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::CanReplacePlayerShot(enemy);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& enemy, int bossPart) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::SpawnBossDebris(shooter, enemy, bossPart);
    case 4: return Stage4Module::SpawnBossDebris(shooter, enemy, bossPart);
    case 5: return Stage5Module::SpawnBossDebris(shooter, enemy, bossPart);
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::DrawBackground2D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    switch (shooter.m_stageNumber) {
    case 1:
        Stage1Module::DrawBackground2D(shooter, renderer, camera);
        break;
    case 2:
        Stage2Module::DrawBackground2D(shooter, renderer, camera);
        break;
    case 3:
        Stage3Module::DrawBackground2D(shooter, renderer, camera);
        break;
    case 4:
    case 5:
        CityBackgroundModule::DrawBackground2D(shooter, renderer, camera);
        break;
    }
}

void SideScrollingShooter::StageDispatch::DrawBackground3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, float railWeight) {
    switch (shooter.m_stageNumber) {
    case 1:
        Stage1Module::DrawBackground3D(shooter, renderer, camera, railWeight);
        break;
    case 2:
        Stage2Module::DrawBackground3D(shooter, renderer, camera, railWeight);
        break;
    case 3:
        Stage3Module::DrawBackground3D(shooter, renderer, camera, railWeight);
        break;
    case 4:
    case 5:
        CityBackgroundModule::DrawBackground3D(shooter, renderer, camera, railWeight);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float yaw) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::DrawBossModel(shooter, renderer, camera, enemy, yaw);
    case 4: return Stage4Module::DrawBossModel(shooter, renderer, camera, enemy, yaw);
    case 5: return Stage5Module::DrawBossModel(shooter, renderer, camera, enemy);
    default: return false;
    }
}

void SideScrollingShooter::StageDispatch::ApplyCameraCorrection(
    const SideScrollingShooter& shooter, Vector3& railPosition, Vector3& railTarget) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::ApplyCameraCorrection(shooter, railPosition, railTarget);
        break;
    }
}

float SideScrollingShooter::StageDispatch::CameraFarClip(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::CameraFarClip();
    default: return 120.0f;
    }
}

float SideScrollingShooter::StageDispatch::RailGroundY(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 1: return -3.275f;
    default: return -3.65f;
    }
}

bool SideScrollingShooter::StageDispatch::ShouldDrawEnemy(
    const SideScrollingShooter& shooter, const Enemy& enemy) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::ShouldDrawEnemy(shooter, enemy);
    default: return true;
    }
}

void SideScrollingShooter::StageDispatch::DrawStageWorld3D(
    const SideScrollingShooter& shooter, Renderer& renderer, const Camera3D& camera) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::DrawStageWorld3D(shooter, renderer, camera);
        break;
    }
}

void SideScrollingShooter::StageDispatch::DrawOverlay2D(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::DrawOverlay2D(shooter, renderer);
        break;
    }
}

void SideScrollingShooter::StageDispatch::DrawOverlay3D(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    switch (shooter.m_stageNumber) {
    case 5:
        Stage5Module::DrawOverlay3D(shooter, renderer);
        break;
    }
}

bool SideScrollingShooter::StageDispatch::DrawHud(
    const SideScrollingShooter& shooter, Renderer& renderer) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::DrawHud(shooter, renderer);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::DrawSpecialAttackWarning3D(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float size) {
    switch (shooter.m_stageNumber) {
    case 5:
        return Stage5Module::DrawSpecialAttackWarning3D(
            shooter, renderer, camera, enemy, size);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::DrawSpecialShot(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Shot& shot, float yaw) {
    switch (shooter.m_stageNumber) {
    case 2: return Stage2Module::DrawSpecialShot(shooter, renderer, camera, shot, yaw);
    case 4: return Stage4Module::DrawSpecialShot(shooter, renderer, camera, shot, yaw);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::DrawSpecialDebris(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Debris& debris, float railWeight) {
    switch (shooter.m_stageNumber) {
    case 2:
        return Stage2Module::DrawSpecialDebris(
            shooter, renderer, camera, debris, railWeight);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::IsViewLocked(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::IsViewLocked(shooter);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::ShouldAdvanceStageScroll(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::ShouldAdvanceStageScroll(shooter);
    default: return true;
    }
}

bool SideScrollingShooter::StageDispatch::IsPlayerDamageIgnored(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::IsPlayerDamageIgnored(shooter);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::IsGameCleared(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 5: return Stage5Module::IsGameCleared(shooter);
    default: return false;
    }
}

bool SideScrollingShooter::StageDispatch::HasNextStage(
    const SideScrollingShooter& shooter) {
    return shooter.m_stageNumber < 5;
}

void SideScrollingShooter::StageDispatch::TickBossIntroduction(SideScrollingShooter& shooter) {
    if (shooter.m_stageNumber == 1) {
        Stage1Module::TickBossIntroduction(shooter);
    }
}

int SideScrollingShooter::StageDispatch::BossIntroductionFrames(
    const SideScrollingShooter& shooter) {
    switch (shooter.m_stageNumber) {
    case 1: return Stage1Module::BossIntroductionFrames();
    case 2: return Stage2Module::BossIntroductionFrames();
    default: return 1;
    }
}
