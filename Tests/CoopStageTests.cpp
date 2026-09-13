#include <cassert>
#include <cstdio>
#include <memory>

#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage1/Stage1Module.h"
#include "../Presentation/Gameplay/Stages/Stage2/Stage2Module.h"
#include "../Presentation/Gameplay/Stages/Stage3/Stage3Module.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5Module.h"

/** @brief ステージ固有の演出と被弾判定の協力プレイ回帰チェック */
struct CoopStageTests {
    /** @brief 実際のステージ処理をGPUなしで検証する @return なし */
    static void Run() {
        using Game = SideScrollingShooter;
        auto game = std::make_unique<Game>();
        auto& g = *game;
        g.m_playerCount = 2;
        g.m_clear = true;
        g.m_players[0].m_playerX = g.m_players[1].m_playerX = 0.0f;

        // Stage1のボス撃破演出で両者が退避する
        Game::Stage1Module::TickBossDefeat(g);
        assert(g.m_players[0].m_playerX < 0.0f);
        assert(g.m_players[1].m_playerX == g.m_players[0].m_playerX);
        g.m_clear = false;

        // 1Pが無敵でも2Pだけに当たるStage2/3レーザーを両視点で検知する
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            g.m_players[0].m_playerX = -1.0f;
            g.m_players[0].m_invincible = 100;
            g.m_players[1].m_playerX = g.m_players[1].m_playerY = 0.0f;
            g.m_players[1].m_playerDestructionTimer = 0;
            g.m_stageNumber = 2;
            g.m_stage = &Game::Stage2Module::Definition(Easy);
            g.m_stage2.boneArchDestroyed = true;
            g.m_stage2.boss.actionAge = 120;
            auto& boss = g.m_enemies[0];
            boss = {};
            boss.active = true;
            boss.phase = 3.0f;
            boss.x = 1.0f;
            boss.z = 40.0f;
            boss.actionZ = Game::PlayerRailZ;
            boss.bossPartHp[Game::BossNose] = 10;
            assert(!Game::Stage2Module::HandleBossInteractionAfterTick(g, boss));
            assert(g.m_players[0].m_playerDestructionTimer == 0);
            assert(g.m_players[1].m_playerDestructionTimer > 0);

            g.m_players[1].m_playerDestructionTimer = 0;
            g.m_stageNumber = 3;
            g.m_stage = &Game::Stage3Module::Definition(Easy);
            boss.phase = 5.0f;
            boss.motionAge = Game::Stage3Module::Phase2SurvivalFrames - 120;
            g.m_stage3.laserTargetInitialized = true;
            g.m_stage3.laserTargetX = g.m_stage3.laserTargetY = 0.0f;
            g.m_stage3.laserTargetZ = rail ? Game::PlayerRailZ : Game::SidePlaneZ;
            Game::Stage3Module::FireBossPartBarrage(g, boss);
            assert(g.m_players[0].m_playerDestructionTimer == 0);
            assert(g.m_players[1].m_playerDestructionTimer > 0);
        }

        // 雲海への遷移は両者のボムを終了し、位置と無敵時間を設定する
        g.m_stageNumber = 5;
        g.m_stage = &Game::Stage5Module::Definition(Easy);
        g.m_stage5.phase = Game::Stage5Phase::TayamaCollapse;
        g.m_stage5.phaseTimer = ShooterStages::Stage5::TayamaCollapseFrames - 1;
        for (auto& player : g.m_players) {
            player.m_playerDestructionTimer = player.m_invincible = 0;
            player.m_bomb.active = true;
            player.m_playerY = 10.0f;
        }
        Game::Stage5Module::TickBeforeFrame(g);
        assert(g.m_stage5.phase == Game::Stage5Phase::CloudSea);
        for (const auto& player : g.m_players) {
            assert(!player.m_bomb.active && player.m_invincible > 0);
            assert(player.m_playerY == 0.0f);
        }
        assert(g.m_players[0].m_playerX != g.m_players[1].m_playerX);

        // 2Pの追尾弾から参照しても同じ龍の節へ命中する
        g.m_stage5.phase = Game::Stage5Phase::TayamaDragonBattle;
        g.m_stage5.tayamaDragonAttack = ShooterStages::Stage5::TayamaDragonAttack::Orbit;
        g.m_stage5.tayamaDragonAttackTimer = ShooterStages::Stage5::TayamaDragonOrbitMoveFrames + 1;
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            Vector3 firstTarget;
            Vector3 secondTarget;
            g.m_activePlayer = 0;
            assert(Game::Stage5Module::GetHomingTarget(g, 3, firstTarget));
            g.m_activePlayer = 1;
            assert(Game::Stage5Module::GetHomingTarget(g, 3, secondTarget));
            assert((firstTarget - secondTarget).LengthSquared() < 0.0001f);
            assert(g.m_activePlayer == 1);
        }

        // 龍の頭部レーザーも1Pの無敵状態と独立して2Pに命中する
        g.m_activePlayer = 0;
        g.m_players[0].m_invincible = 100;
        for (bool rail : {false, true}) {
            g.m_viewMode = g.m_nextViewMode = rail ? Game::ViewMode::Rail3D : Game::ViewMode::Side2D;
            g.m_players[1].m_invincible = g.m_players[1].m_playerDestructionTimer = 0;
            g.m_stage5.tayamaDragonAttack = ShooterStages::Stage5::TayamaDragonAttack::HeadLaser;
            g.m_stage5.tayamaDragonAttackTimer = ShooterStages::Stage5::TayamaHeadLaserWarningFrames - 1;
            g.m_stage5.headLaserArmed = true;
            g.m_activePlayer = 1;
            g.m_stage5.headLaserTarget = g.PlayerWorldPosition();
            g.m_activePlayer = 0;
            Game::Stage5Module::TickBeforeFrame(g);
            assert(g.m_players[0].m_playerDestructionTimer == 0);
            assert(g.m_players[1].m_playerDestructionTimer > 0);
        }
    }
};

/** @brief ステージ固有の協力プレイ回帰チェックを実行する @return なし */
void RunCoopStageTests() {
    CoopStageTests::Run();
    std::puts("CoopStageTests passed");
}
