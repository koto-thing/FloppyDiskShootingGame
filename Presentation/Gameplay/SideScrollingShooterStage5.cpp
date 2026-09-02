#include "SideScrollingShooter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Infrastructure/ExternalServices/AudioService.h"
#include "SideScrollingShooterShared.h"

namespace {
using SideScrollingShooterShared::Stage5DistantThunder;
using SideScrollingShooterShared::Stage5Thunder;
using SideScrollingShooterShared::Stage5SearchlightDetect;
using SideScrollingShooterShared::Stage5SearchlightLocked;
using SideScrollingShooterShared::Stage5BarrageWarning;
using SideScrollingShooterShared::Stage5EastsourceEntrance;
using SideScrollingShooterShared::Stage5SignalLost;
using SideScrollingShooterShared::Stage5Transformation;
using SideScrollingShooterShared::Stage5WeakpointDestroyed;
using SideScrollingShooterShared::Stage5CoreWarning;
using SideScrollingShooterShared::Stage5ChainExplosion;
using SideScrollingShooterShared::Stage5FinalExplosion;

constexpr int EastsourceIntroFrames = 210;
constexpr int EastsourceFallFrames = 180;

/**
 * @brief 値を一フレームの最大移動量以内で目標へ近づける
 * @param current 現在値
 * @param target 目標値
 * @param maxDelta 最大移動量
 * @return 更新後の値
 */
float MoveTowards(float current, float target, float maxDelta) {
    return current < target ? (std::min)(current + maxDelta, target) :
        (std::max)(current - maxDelta, target);
}

}

#include "SideScrollingShooterEnemies.h"
#include "SideScrollingShooterStages.h"
#include "Stage5ModelView.h"

/**
 * @brief Stage 5専用状態を初期化する
 * @return なし
 */
void SideScrollingShooter::ResetStage5() {
    m_stage5Phase = Stage5Phase::Approach;
    m_stage5Checkpoint = Stage5Checkpoint::Chapter1;
    m_stage5PhaseTimer = 0;
    m_stage5CheckpointPower = m_power;
    m_stage5CheckpointScore = m_score;
    m_stage5CheckpointKills = m_kills;
    m_stage5SoundCooldown = 0;
    m_stage5AttackTimer = 0;
    m_stage5GuardSpawnCooldown = 0;
    m_stage5CoreTargetX = 0.0f;
    m_stage5CoreTargetY = 0.0f;
    m_tayamaTransformation = 0.0f;
    m_searchlights = {};
    m_tayamaWeakpoints = {{
        {TayamaWeakpoint::LeftSearchlight, 180, 180, false, false, 0},
        {TayamaWeakpoint::RightSearchlight, 180, 180, false, false, 0},
        {TayamaWeakpoint::FireControlRadar, 260, 260, false, false, 0},
        {TayamaWeakpoint::LeftLiftEngine, 360, 360, false, false, 0},
        {TayamaWeakpoint::RightLiftEngine, 360, 360, false, false, 0},
        {TayamaWeakpoint::CommandCore, 900, 900, false, false, 0}
    }};
}

/**
 * @brief Stage 5の指定状態からデバッグ開始する
 * @param phase 開始する状態
 * @return なし
 */
void SideScrollingShooter::StartDebugStage5Phase(Stage5Phase phase) {
    StartDebugCheckpoint(5, 3, false);
    m_chapterNumber = 3;
    m_frame = m_stage->ChapterEndFrame(3);
    m_scroll = static_cast<float>(m_frame) * 0.008f;
    m_missionStartTimer = 0;
    m_viewMode = ViewMode::Rail3D;
    m_nextViewMode = ViewMode::Rail3D;
    m_viewTransitionTimer = 0;
    m_viewTransitionProgress = 0.0f;

    // 後半地点は到達済みの弱点を破壊状態へ合わせてから開始する
    if (phase == Stage5Phase::TayamaLiftEngines ||
        phase == Stage5Phase::TayamaCommandCore || phase == Stage5Phase::TayamaCollapse) {
        for (int i = 0; i <= static_cast<int>(TayamaWeakpoint::FireControlRadar); ++i) {
            m_tayamaWeakpoints[i].hp = 0;
            m_tayamaWeakpoints[i].destroyed = true;
        }
    }
    if (phase == Stage5Phase::TayamaCommandCore || phase == Stage5Phase::TayamaCollapse) {
        for (int i = static_cast<int>(TayamaWeakpoint::LeftLiftEngine);
            i <= static_cast<int>(TayamaWeakpoint::RightLiftEngine); ++i) {
            m_tayamaWeakpoints[i].hp = 0;
            m_tayamaWeakpoints[i].destroyed = true;
        }
    }
    if (phase == Stage5Phase::TayamaCollapse) {
        TayamaWeakpointState& core =
            m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::CommandCore)];
        core.hp = 0;
        core.destroyed = true;
    }
    m_tayamaTransformation = phase >= Stage5Phase::TayamaFireControl ? 1.0f : 0.0f;
    StartStage5Phase(phase);
#ifdef _DEBUG
    // デバッグ直行後に各フェーズの演出と当たり判定を確認できる時間を確保する
    m_invincible = (std::max)(m_invincible, 600);
#endif
}

/**
 * @brief 現在状態をStage 5チェックポイントとして保存する
 * @param checkpoint 保存するチェックポイント
 * @return なし
 */
void SideScrollingShooter::SaveStage5Checkpoint(Stage5Checkpoint checkpoint) {
    m_stage5Checkpoint = checkpoint;
    m_stage5CheckpointPower = m_power;
    m_stage5CheckpointScore = m_score;
    m_stage5CheckpointKills = m_kills;
}

/**
 * @brief Stage 5状態を開始する
 * @param phase 開始する状態
 * @param saveCheckpoint 復帰地点として保存する場合true
 * @return なし
 */
void SideScrollingShooter::StartStage5Phase(Stage5Phase phase, bool saveCheckpoint) {
    m_stage5Phase = phase;
    m_stage5PhaseTimer = 0;
    m_stage5AttackTimer = 0;
    m_stage5CoreTargetX = m_playerX;
    m_stage5CoreTargetY = m_playerY;
    m_stage5GuardSpawnCooldown = 0;

    if (phase != Stage5Phase::Approach) RequestViewMode(ViewMode::Rail3D);

    // TAYAMA攻略は専用初期化へ委譲する
    if (phase == Stage5Phase::TayamaFireControl ||
        phase == Stage5Phase::TayamaLiftEngines ||
        phase == Stage5Phase::TayamaCommandCore) {
        StartTayamaPhase(phase, true);
        return;
    }

    if (phase == Stage5Phase::EastsourceIntro) {
        m_shots = {};
        m_enemies = {};
        m_items = {};
        m_bossBattle = true;
        Enemy& eastsource = m_enemies[0];
        m_stage->ConfigureBoss(eastsource, true);
        m_stage->ConfigureBossPartHp(eastsource);
        eastsource.bossPartMaxHp = eastsource.bossPartHp;
        eastsource.x = 1.45f;
        eastsource.y = 0.30f;
        eastsource.z = 66.0f;
        eastsource.collisionEnabled = false;
        m_bossHp = eastsource.hp;
        m_displayBossHp = static_cast<float>(m_bossHp);
        if (saveCheckpoint) SaveStage5Checkpoint(Stage5Checkpoint::Eastsource);
        PlayStage5Cue(Stage5EastsourceEntrance);
        return;
    }
    if (phase == Stage5Phase::EastsourceBattle) {
        StartEastsourceBattle();
        if (saveCheckpoint) SaveStage5Checkpoint(Stage5Checkpoint::Eastsource);
        return;
    }
    if (phase == Stage5Phase::EastsourceFall) {
        m_bossBattle = false;
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
        PlayStage5Cue(Stage5SignalLost);
        return;
    }

    // 壁面区画へ入るたび、その区画固有のライトを初期化する
    if (phase >= Stage5Phase::WallClimbTransition && phase <= Stage5Phase::CarrierTransformation) {
        m_bossBattle = false;
        for (auto& enemy : m_enemies) enemy.active = false;
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
        int lightCount = 0;
        Stage5Checkpoint checkpoint = Stage5Checkpoint::WallClimbLower;
        if (phase == Stage5Phase::WallClimbLower) {
            lightCount = 1;
            m_tayamaTransformation = 0.10f;
        } else if (phase == Stage5Phase::WallClimbMiddle) {
            lightCount = 2;
            checkpoint = Stage5Checkpoint::WallClimbMiddle;
            m_tayamaTransformation = 0.34f;
        } else if (phase == Stage5Phase::WallClimbUpper) {
            lightCount = 3;
            checkpoint = Stage5Checkpoint::WallClimbUpper;
            m_tayamaTransformation = 0.64f;
        } else if (phase == Stage5Phase::RooftopArrival) {
            m_tayamaTransformation = 0.90f;
        } else if (phase == Stage5Phase::CarrierTransformation) {
            m_tayamaTransformation = 0.96f;
            PlayStage5Cue(Stage5Transformation);
        }
        ResetWallSearchlights(lightCount);
        if (saveCheckpoint && lightCount > 0) SaveStage5Checkpoint(checkpoint);
        m_invincible = (std::max)(m_invincible, 60);
        return;
    }

    if (phase == Stage5Phase::TayamaCollapse) {
        m_bossBattle = false;
        m_bossHp = 0;
        m_displayBossHp = 0.0f;
        for (auto& enemy : m_enemies) enemy.active = false;
        for (auto& shot : m_shots) {
            if (shot.enemy) shot.active = false;
        }
        m_invincible = TayamaCollapseFrames + Stage5QuietFlightFrames + 60;
        PlayStage5Cue(Stage5ChainExplosion);
        return;
    }

    if (phase == Stage5Phase::EndingReady) {
        m_clear = true;
        m_clearTimer = 0;
    }
}

/**
 * @brief EASTSOURCE戦を戦闘可能な状態で開始する
 * @return なし
 */
void SideScrollingShooter::StartEastsourceBattle() {
    m_bossBattle = true;
    Enemy& eastsource = m_enemies[0];
    if (!eastsource.active || eastsource.type != Stage::BossEnemy) {
        eastsource = {};
        m_stage->ConfigureBoss(eastsource, true);
        m_stage->ConfigureBossPartHp(eastsource);
        eastsource.bossPartMaxHp = eastsource.bossPartHp;
    }
    eastsource.collisionEnabled = true;
    eastsource.age = 0;
    eastsource.motionAge = 0;
    eastsource.bossPhase = BossNormalPhase1;
    eastsource.x = 0.42f;
    eastsource.y = 0.0f;
    eastsource.z = 45.0f;
    eastsource.baseX = eastsource.x;
    eastsource.baseY = eastsource.y;
    eastsource.baseZ = eastsource.z;
    m_bossHp = eastsource.hp;
    m_displayBossHp = static_cast<float>(m_bossHp);
    m_invincible = (std::max)(m_invincible, 60);
    ResetWallSearchlights(1);
}

/**
 * @brief EASTSOURCEの移動と攻撃を更新する
 * @param eastsource 更新するEASTSOURCE
 * @return なし
 */
void SideScrollingShooter::TickEastsource(Enemy& eastsource) {
    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        const float progress = SmoothStep(static_cast<float>(m_stage5PhaseTimer) / EastsourceIntroFrames);
        eastsource.x = Math::Lerp(1.45f, 0.42f, progress);
        eastsource.y = Math::Lerp(0.30f, 0.0f, progress) + std::sin(progress * Math::Pi * 5.0f) * 0.08f;
        eastsource.z = Math::Lerp(66.0f, 45.0f, progress);
        eastsource.collisionEnabled = false;
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceFall) {
        eastsource.collisionEnabled = false;
        eastsource.x -= 0.004f;
        eastsource.y -= 0.018f + static_cast<float>(m_stage5PhaseTimer) * 0.00008f;
        eastsource.z += 0.08f;
        return;
    }
    if (m_stage5Phase != Stage5Phase::EastsourceBattle) return;

    const bool leftEngine = eastsource.bossPartHp[BossLeftEngine] > 0;
    const bool rightEngine = eastsource.bossPartHp[BossRightEngine] > 0;
    const float engineRate = (static_cast<float>(leftEngine) + static_cast<float>(rightEngine)) * 0.5f;
    const float moveAmplitude = 0.20f + engineRate * 0.38f;
    const float moveSpeed = 0.009f + engineRate * 0.009f;
    eastsource.x = std::sin(static_cast<float>(eastsource.age) * moveSpeed) * moveAmplitude;
    eastsource.y = std::sin(static_cast<float>(eastsource.age) * 0.021f) * 0.42f;
    eastsource.z = 43.0f + std::sin(static_cast<float>(eastsource.age) * 0.013f) * 3.0f;

    const bool nose = eastsource.bossPartHp[BossNose] > 0;
    const int phase = eastsource.bossPhase;
    if (phase == BossNormalPhase2 || phase == BossSpecialPhase2) {
        const int pursuitCycle = eastsource.age % 180;
        const bool fromLeft = (eastsource.age / 180) % 2 == 0;
        const BossPart wingPart = fromLeft ? BossLeftWing : BossRightWing;
        const BossPart enginePart = fromLeft ? BossLeftEngine : BossRightEngine;
        const bool wing = eastsource.bossPartHp[wingPart] > 0;
        const bool engine = eastsource.bossPartHp[enginePart] > 0;
        const float side = fromLeft ? -1.0f : 1.0f;
        const int passEnd = 112 + (engine ? 36 : 52);

        // 索敵中は遠ざかり、予告後だけ画面外から固定方向へ高速再進入する
        if (!wing) {
            eastsource.collisionEnabled = true;
        } else if (pursuitCycle < 82) {
            eastsource.z = Math::Lerp(43.0f, 59.0f,
                SmoothStep(static_cast<float>(pursuitCycle) / 82.0f));
        } else if (pursuitCycle < 112) {
            eastsource.x = side * 1.48f;
            eastsource.z = 59.0f;
            eastsource.collisionEnabled = false;
        } else if (pursuitCycle < passEnd) {
            const float pass = SmoothStep(static_cast<float>(pursuitCycle - 112) / (engine ? 36.0f : 52.0f));
            eastsource.x = Math::Lerp(side * 1.48f, -side * 1.48f, pass);
            eastsource.y = eastsource.attackWarningTargetY;
            eastsource.z = Math::Lerp(31.0f, 19.0f, std::sin(pass * Math::Pi));
            eastsource.collisionEnabled = false;
        } else {
            const float settle = SmoothStep(static_cast<float>(pursuitCycle - passEnd) /
                static_cast<float>(180 - passEnd));
            eastsource.x = Math::Lerp(-side * 1.48f, 0.0f, settle);
            eastsource.z = Math::Lerp(28.0f, 43.0f, settle);
            eastsource.collisionEnabled = settle > 0.55f;
        }
    } else {
        eastsource.collisionEnabled = true;
    }
    if (phase == BossNormalPhase1 || phase == BossSpecialPhase2) {
        const int cycleLength = nose ? 118 : 148;
        const int cycle = eastsource.age % cycleLength;
        const int warningFrames = nose ? 34 : 54;
        if (cycle == 0) {
            const float error = nose ? 0.0f : std::sin(static_cast<float>(eastsource.age) * 0.37f) * 0.24f;
            eastsource.attackWarningTargetX = m_playerX + error;
            eastsource.attackWarningTargetY = m_playerY - error * 0.45f;
            eastsource.attackWarningFrames = warningFrames;
            PlayStage5Cue(Stage5BarrageWarning);
        }
        const int shotCount = nose ? 3 : 1;
        for (int shotIndex = 0; shotIndex < shotCount; ++shotIndex) {
            if (cycle == warningFrames + shotIndex * 10) {
                SpawnEnemyShotAt(eastsource.x, eastsource.y, eastsource.z,
                    eastsource.attackWarningTargetX, eastsource.attackWarningTargetY,
                    PlayerRailZ, 0.72f);
            }
        }
    }

    if (phase == BossSpecialPhase1 || phase == BossSpecialPhase2) {
        const int cycle = eastsource.age % 96;
        if (cycle == 22 && eastsource.bossPartHp[BossLeftWing] > 0) {
            for (int lane = -2; lane <= 2; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(eastsource.x - 0.42f, eastsource.y + 0.12f, eastsource.z,
                    m_playerX + 0.25f, static_cast<float>(lane) * 0.25f, PlayerRailZ, 0.64f);
            }
        }
        if (cycle == 48 && eastsource.bossPartHp[BossRightWing] > 0) {
            for (int lane = -2; lane <= 2; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(eastsource.x + 0.42f, eastsource.y - 0.12f, eastsource.z,
                    m_playerX - 0.25f, static_cast<float>(lane) * 0.25f, PlayerRailZ, 0.64f);
            }
        }
    }

    if (phase == BossNormalPhase2 || phase == BossSpecialPhase2) {
        const int cycle = eastsource.age % 180;
        if (cycle == 92) {
            eastsource.attackWarningTargetX = m_playerX;
            eastsource.attackWarningTargetY = m_playerY;
            eastsource.attackWarningFrames = 30;
        }
        if (cycle >= 120 && cycle < 138) {
            const bool fromLeft = (eastsource.age / 180) % 2 == 0;
            const bool wing = eastsource.bossPartHp[fromLeft ? BossLeftWing : BossRightWing] > 0;
            const bool engine = eastsource.bossPartHp[fromLeft ? BossLeftEngine : BossRightEngine] > 0;
            if (wing && cycle % (engine ? 4 : 7) == 0) {
                const float sourceX = fromLeft ? -1.25f : 1.25f;
                SpawnEnemyShotAt(sourceX, eastsource.attackWarningTargetY, 18.0f,
                    eastsource.attackWarningTargetX, eastsource.attackWarningTargetY,
                    PlayerRailZ, engine ? 0.82f : 0.58f);
            }
        }
    }
}

/**
 * @brief EASTSOURCE撃破後の信号消失演出へ移行する
 * @param eastsource 撃破状態へ移行するEASTSOURCE
 * @return なし
 */
void SideScrollingShooter::DefeatEastsource(Enemy& eastsource) {
    if (m_stage5Phase != Stage5Phase::EastsourceBattle) return;
    eastsource.hp = 0;
    eastsource.collisionEnabled = false;
    m_bossHp = 0;
    m_displayBossHp = 0.0f;
    m_score += 5000;
    const BossPart detachedPart = eastsource.bossPartHp[BossLeftWing] > 0 ? BossLeftWing :
        (eastsource.bossPartHp[BossRightWing] > 0 ? BossRightWing :
            (eastsource.bossPartHp[BossLeftEngine] > 0 ? BossLeftEngine : BossRightEngine));
    SpawnEnemyDebris(eastsource, detachedPart);
    eastsource.bossPartHp[detachedPart] = 0;
    StartStage5Phase(Stage5Phase::EastsourceFall, false);
}

/**
 * @brief 壁面区画のサーチライトを初期化する
 * @param activeCount 有効化するサーチライト数
 * @return なし
 */
void SideScrollingShooter::ResetWallSearchlights(int activeCount) {
    m_searchlights = {};
    for (int i = 0; i < Stage5SearchlightCount; ++i) {
        SearchlightState& light = m_searchlights[i];
        light.beamX = (static_cast<float>(i) - 1.0f) * 0.62f;
        light.beamY = i % 2 == 0 ? 0.34f : -0.28f;
        light.hp = i < activeCount ? 90 : 0;
        light.destroyed = i >= activeCount;
        light.timer = i * 27;
    }
}

/**
 * @brief サーチライトの保存済み地点へ集中砲火を生成する
 * @param light 集中砲火に使用するサーチライト状態
 * @param lightIndex 発射元を決めるサーチライト番号
 * @return なし
 */
void SideScrollingShooter::FireSearchlightVolley(const SearchlightState& light, int lightIndex) {
    float sourceX = (static_cast<float>(lightIndex) - 1.0f) * 0.72f;
    float sourceY = 0.72f - static_cast<float>(lightIndex) * 0.22f;
    float sourceZ = 46.0f;
    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        const TayamaPartGroup group = lightIndex == 0 ?
            TayamaPartGroup::LeftSearchlight : TayamaPartGroup::RightSearchlight;
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
            TayamaTransform(), m_tayamaTransformation, TayamaState(), group);
        if (bounds.valid) {
            sourceX = FromWorldX(bounds.center.x);
            sourceY = FromWorldY(bounds.center.y);
            sourceZ = bounds.center.z;
        }
    }
    for (int bullet = -2; bullet <= 2; ++bullet) {
        const float spread = static_cast<float>(bullet) * 0.065f;
        SpawnEnemyShotAt(sourceX, sourceY, sourceZ,
            light.lockedX + spread, light.lockedY + std::abs(spread) * 0.35f,
            PlayerRailZ, 0.78f);
    }
}

/**
 * @brief 指定数のサーチライトを更新する
 * @param activeCount 更新するライト数
 * @param tayamaWeakpoints TAYAMA弱点と破壊状態を共有する場合true
 * @return なし
 */
void SideScrollingShooter::TickSearchlights(int activeCount, bool tayamaWeakpoints) {
    const bool radarDestroyed = tayamaWeakpoints &&
        m_tayamaWeakpoints[static_cast<int>(TayamaWeakpoint::FireControlRadar)].destroyed;
    const int difficultyOffset = m_difficulty == Easy ? 9 : (m_difficulty == Hard ? -9 : 0);
    const int lockFrames = SearchlightLockFrames + difficultyOffset + (radarDestroyed ? 18 : 0);

    for (int i = 0; i < activeCount; ++i) {
        SearchlightState& light = m_searchlights[i];
        if (tayamaWeakpoints) {
            const TayamaWeakpoint type = i == 0 ? TayamaWeakpoint::LeftSearchlight :
                TayamaWeakpoint::RightSearchlight;
            light.destroyed = m_tayamaWeakpoints[static_cast<int>(type)].destroyed;
        }
        if (light.destroyed) continue;

        const float scanTargetX = std::sin(static_cast<float>(m_stage5PhaseTimer + i * 67) *
            (0.018f + static_cast<float>(i) * 0.002f)) * 0.92f;
        const float scanTargetY = std::sin(static_cast<float>(m_stage5PhaseTimer + i * 43) *
            (0.013f + static_cast<float>(i) * 0.003f)) * 0.66f;
        if (light.phase == SearchlightPhase::Searching || light.phase == SearchlightPhase::Detecting) {
            const float trackingLimit = radarDestroyed ? 0.010f : 0.016f;
            light.beamX = MoveTowards(light.beamX, scanTargetX, trackingLimit);
            light.beamY = MoveTowards(light.beamY, scanTargetY, trackingLimit * 0.78f);
        }

        const bool illuminated = Hit(m_playerX, m_playerY, 0.055f,
            light.beamX, light.beamY, SearchlightDetectionRadius);
        if (light.phase == SearchlightPhase::Searching) {
            if (illuminated) {
                light.phase = SearchlightPhase::Detecting;
                light.detectionFrames = 1;
                PlayStage5Cue(Stage5SearchlightDetect);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Detecting) {
            light.detectionFrames = illuminated ? light.detectionFrames + 1 :
                (std::max)(0, light.detectionFrames - 2);
            if (light.detectionFrames == 0) {
                light.phase = SearchlightPhase::Searching;
            } else if (light.detectionFrames >= lockFrames) {
                light.lockedX = m_playerX;
                light.lockedY = m_playerY;
                light.phase = SearchlightPhase::Locked;
                light.timer = SearchlightWarningFrames;
                PlayStage5Cue(Stage5SearchlightLocked);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Locked) {
            if (--light.timer <= 0) {
                light.phase = SearchlightPhase::Firing;
                light.timer = 0;
                light.volley = 0;
                PlayStage5Cue(Stage5BarrageWarning);
            }
            continue;
        }
        if (light.phase == SearchlightPhase::Firing) {
            if (light.timer % SearchlightVolleyIntervalFrames == 0 &&
                light.volley < SearchlightVolleyCount) {
                FireSearchlightVolley(light, i);
                ++light.volley;
            }
            ++light.timer;
            if (light.volley >= SearchlightVolleyCount &&
                light.timer >= SearchlightVolleyIntervalFrames * SearchlightVolleyCount) {
                light.phase = SearchlightPhase::Cooldown;
                light.timer = 90 + i * 24 + (m_difficulty == Easy ? 30 : 0);
                light.detectionFrames = 0;
            }
            continue;
        }
        if (--light.timer <= 0) light.phase = SearchlightPhase::Searching;
    }
}

/**
 * @brief EASTSOURCEの描画と当たり判定で共有する親Transformを取得する
 * @param eastsource EASTSOURCE本体
 * @return ワールド座標へ変換する親Transform
 */
Stage5ModelTransform SideScrollingShooter::EastsourceTransform(const Enemy& eastsource) const {
    const float fallRoll = m_stage5Phase == Stage5Phase::EastsourceFall ?
        static_cast<float>(m_stage5PhaseTimer) * 0.035f : 0.0f;
    return {{ToWorldX(eastsource.x), ToWorldY(eastsource.y), eastsource.z},
        {0.0f, Math::Lerp(Math::HalfPi, 0.0f, RailBlend()), fallRoll}, 0.72f};
}

/**
 * @brief EASTSOURCEの部位状態をモデルグループへ変換する
 * @param eastsource EASTSOURCE本体
 * @return 描画と当たり判定へ渡すモデル状態
 */
EastsourceModelState SideScrollingShooter::EastsourceState(const Enemy& eastsource) const {
    EastsourceModelState state;
    constexpr EastsourcePartGroup Groups[] = {
        EastsourcePartGroup::Nose,
        EastsourcePartGroup::LeftWing,
        EastsourcePartGroup::RightWing,
        EastsourcePartGroup::LeftEngine,
        EastsourcePartGroup::RightEngine
    };

    // 既存BossPartの固定順をEASTSOURCEのモデルグループへ一度だけ写す
    for (int part = BossNose; part <= BossRightEngine; ++part) {
        const std::size_t group = static_cast<std::size_t>(Groups[part]);
        state.destroyed[group] = eastsource.bossPartHp[part] <= 0;
        const int flash = eastsource.bossPartHitFlashFrames[part];
        state.hitFlash[group] = flash > 0 && (flash / 2) % 2 != 0;
    }
    return state;
}

/**
 * @brief TAYAMAの描画と当たり判定で共有する親Transformを取得する
 * @return 現在の進行に対応する親Transform
 */
Stage5ModelTransform SideScrollingShooter::TayamaTransform() const {
    float y = 3.0f;
    float z = 57.0f;
    float scale = 1.34f;
    float pitch = 0.0f;
    float roll = 0.0f;

    // 通常チャプターでは遠景の都市構造として見せ、後半ほど接近させる
    if (m_stage5Phase == Stage5Phase::Approach) {
        const int chapterLength = m_stage != nullptr ? m_stage->ChapterFrameLength() : ChapterLengthFrames;
        const float chapterProgress = Math::Clamp01(
            static_cast<float>(m_frame - (m_chapterNumber - 1) * chapterLength) /
            static_cast<float>(chapterLength));
        y = -9.0f + static_cast<float>(m_chapterNumber - 1) * 2.4f + chapterProgress * 1.2f;
        z = 88.0f - static_cast<float>(m_chapterNumber - 1) * 6.0f;
        scale = 0.82f + static_cast<float>(m_chapterNumber - 1) * 0.12f;
    } else if (m_stage5Phase <= Stage5Phase::EastsourceFall) {
        const float approach = m_stage5Phase == Stage5Phase::EastsourceIntro ?
            SmoothStep(Math::Clamp01(static_cast<float>(m_stage5PhaseTimer) / EastsourceIntroFrames)) : 1.0f;
        y = Math::Lerp(-3.0f, -1.5f, approach);
        z = Math::Lerp(76.0f, 59.0f, approach);
        scale = Math::Lerp(1.06f, 1.42f, approach);
    } else if (m_stage5Phase <= Stage5Phase::WallClimbUpper) {
        y = -1.5f + m_tayamaTransformation * 4.0f;
        z = 59.0f;
        scale = 1.42f;
    } else if (m_stage5Phase == Stage5Phase::TayamaCollapse) {
        z = 57.0f + static_cast<float>((std::min)(m_stage5PhaseTimer, 450)) * 0.004f;
        scale = 1.34f + static_cast<float>((std::max)(0, m_stage5PhaseTimer - 330)) * 0.0006f;
    }
    if (m_stage5Phase == Stage5Phase::RooftopArrival) {
        const float arrival = SmoothStep(Math::Clamp01(
            static_cast<float>(m_stage5PhaseTimer) / RooftopArrivalFrames));
        pitch = Math::Lerp(0.0f, 0.58f, arrival);
        y = Math::Lerp(2.1f, 0.0f, arrival);
        z = Math::Lerp(59.0f, 57.0f, arrival);
        scale = Math::Lerp(1.42f, 1.08f, arrival);
    } else if (m_stage5Phase >= Stage5Phase::CarrierTransformation) {
        pitch = 0.58f;
        y = 0.0f;
        scale = m_stage5Phase == Stage5Phase::TayamaCollapse ?
            1.08f + static_cast<float>((std::max)(0, m_stage5PhaseTimer - 330)) * 0.0006f : 1.08f;
    }
    if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        y = Math::Lerp(0.0f, 1.8f, SmoothStep(Math::Clamp01(
            static_cast<float>(m_stage5PhaseTimer) / 180.0f)));
    } else if (m_stage5Phase == Stage5Phase::TayamaCollapse) {
        y = 1.8f - static_cast<float>((std::min)(m_stage5PhaseTimer, 450)) * 0.002f;
    }

    // 揚力エンジンの片側破壊を艦体ロールへ反映するが入力軸は回転させない
    if (m_stage5Phase >= Stage5Phase::TayamaLiftEngines) {
        const bool left = m_tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::LeftLiftEngine)].destroyed;
        const bool right = m_tayamaWeakpoints[
            static_cast<int>(TayamaWeakpoint::RightLiftEngine)].destroyed;
        if (left != right) roll = left ? -0.12f : 0.12f;
    }
    return {{0.0f, y, z}, {pitch, 0.0f, roll}, scale};
}

/**
 * @brief 現在の弱点と崩壊状態をTAYAMAモデルグループへ変換する
 * @return 描画と当たり判定へ渡すモデル状態
 */
TayamaModelState SideScrollingShooter::TayamaState() const {
    TayamaModelState state;
    if (m_stage5Phase == Stage5Phase::EndingReady) {
        state.visible.fill(false);
        return state;
    }
    constexpr TayamaPartGroup Groups[] = {
        TayamaPartGroup::LeftSearchlight,
        TayamaPartGroup::RightSearchlight,
        TayamaPartGroup::FireControlRadar,
        TayamaPartGroup::LeftLiftEngine,
        TayamaPartGroup::RightLiftEngine,
        TayamaPartGroup::CommandCore
    };

    // 弱点の破壊と点滅を同名モデルグループへ反映する
    for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        const std::size_t index = static_cast<std::size_t>(weakpoint.type);
        const std::size_t group = static_cast<std::size_t>(Groups[index]);
        state.destroyed[group] = weakpoint.destroyed;
        state.hitFlash[group] = weakpoint.hitFlashFrames > 0 &&
            (weakpoint.hitFlashFrames / 2) % 2 != 0;
    }
    state.visible[static_cast<std::size_t>(TayamaPartGroup::CommandCore)] =
        m_stage5Phase >= Stage5Phase::TayamaCommandCore;
    if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        state.visible[static_cast<std::size_t>(TayamaPartGroup::ArmorPanel)] = false;
    }

    if (m_stage5Phase != Stage5Phase::TayamaCollapse) return state;

    // 大型構造はDebrisへ分解せずグループ単位のTransformで画面内崩壊させる
    const float deckFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 180) / 270.0f);
    const float bridgeFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 300) / 190.0f);
    const float engineFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 60) / 180.0f);
    const float armorFall = Math::Clamp01(static_cast<float>(m_stage5PhaseTimer - 210) / 210.0f);
    auto SetOffset = [&](TayamaPartGroup group, const Vector3& position,
        const Vector3& rotation, float scale) {
        state.collapseOffsets[static_cast<std::size_t>(group)] =
            {position, rotation, {scale, scale, scale}};
    };
    SetOffset(TayamaPartGroup::LeftFlightDeck,
        {-deckFall * 12.0f, -deckFall * 13.0f, deckFall * 3.0f},
        {deckFall * 0.18f, 0.0f, deckFall * 1.05f}, 1.0f - deckFall * 0.22f);
    SetOffset(TayamaPartGroup::RightFlightDeck,
        {deckFall * 12.0f, -deckFall * 14.0f, deckFall * 2.0f},
        {-deckFall * 0.16f, 0.0f, -deckFall * 1.12f}, 1.0f - deckFall * 0.22f);
    SetOffset(TayamaPartGroup::Bridge,
        {bridgeFall * 2.0f, -bridgeFall * 10.0f, -bridgeFall * 2.0f},
        {bridgeFall * 0.32f, bridgeFall * 0.24f, bridgeFall * 0.82f}, 1.0f - bridgeFall * 0.30f);
    SetOffset(TayamaPartGroup::LeftLiftEngine,
        {-engineFall * 3.0f, -engineFall * 16.0f, engineFall * 2.0f},
        {0.0f, engineFall * 0.35f, engineFall * 0.55f}, 1.0f - engineFall * 0.35f);
    SetOffset(TayamaPartGroup::RightLiftEngine,
        {engineFall * 3.0f, -engineFall * 16.0f, engineFall * 2.0f},
        {0.0f, -engineFall * 0.35f, -engineFall * 0.55f}, 1.0f - engineFall * 0.35f);
    SetOffset(TayamaPartGroup::ArmorPanel,
        {0.0f, -armorFall * 9.0f, armorFall * 4.0f},
        {armorFall * 0.24f, armorFall * 0.45f, 0.0f}, 1.0f - armorFall * 0.32f);
    SetOffset(TayamaPartGroup::Hangar,
        {0.0f, -armorFall * 11.0f, -armorFall * 3.0f},
        {-armorFall * 0.22f, -armorFall * 0.30f, armorFall * 0.20f}, 1.0f - armorFall * 0.34f);

    // 最終爆発後は全グループを消し、60フレームの静かな飛行だけを残す
    if (m_stage5PhaseTimer >= TayamaCollapseFrames) state.visible.fill(false);
    return state;
}

/**
 * @brief 自機弾を壁面サーチライトへ適用する
 * @param shot 判定する自機弾
 * @return ライトへ命中した場合true
 */
bool SideScrollingShooter::TryDamageWallSearchlight(Shot& shot) {
    if (m_stage5Phase < Stage5Phase::WallClimbLower ||
        m_stage5Phase > Stage5Phase::WallClimbUpper) return false;
    const int activeCount = m_stage5Phase == Stage5Phase::WallClimbLower ? 1 :
        (m_stage5Phase == Stage5Phase::WallClimbMiddle ? 2 : 3);

    // 描画する壁面ライト基部と同じ固定配置へ線分判定する
    for (int index = 0; index < activeCount; ++index) {
        SearchlightState& light = m_searchlights[index];
        if (light.destroyed) continue;
        const float sourceX = (static_cast<float>(index) - 1.0f) * 0.72f;
        const float sourceY = 0.72f - static_cast<float>(index) * 0.22f;
        if (!Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz,
            ToWorldX(shot.x), ToWorldY(shot.y), shot.z, shot.hitRadius * WorldXScale,
            ToWorldX(sourceX), ToWorldY(sourceY), 46.0f, 0.72f)) continue;

        if (!shot.piercing) shot.active = false;
        light.hp -= shot.damage;
        SpawnExplosion(sourceX, sourceY, 46.0f, light.hp <= 0);
        if (light.hp <= 0) {
            light.hp = 0;
            light.destroyed = true;
            light.phase = SearchlightPhase::Cooldown;
            m_score += 300;
            PlayStage5Cue(Stage5WeakpointDestroyed);
        } else {
            PlayHitSound();
        }
        return true;
    }
    return false;
}

/**
 * @brief 自機弾をTAYAMAの有効弱点へ適用する
 * @param shot 判定する自機弾
 * @return TAYAMAへ命中した場合true
 */
bool SideScrollingShooter::TryDamageTayama(Shot& shot) {
    if (m_stage5Phase < Stage5Phase::TayamaFireControl ||
        m_stage5Phase > Stage5Phase::TayamaCommandCore) return false;
    const Stage5ModelTransform transform = TayamaTransform();
    const TayamaModelState modelState = TayamaState();
    constexpr TayamaPartGroup Groups[] = {
        TayamaPartGroup::LeftSearchlight,
        TayamaPartGroup::RightSearchlight,
        TayamaPartGroup::FireControlRadar,
        TayamaPartGroup::LeftLiftEngine,
        TayamaPartGroup::RightLiftEngine,
        TayamaPartGroup::CommandCore
    };
    const Vector3 shotStart {ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy), shot.z - shot.vz};
    const Vector3 shotEnd {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};

    // 現フェーズの弱点だけを先に判定し、後続フェーズへの先行ダメージを防ぐ
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (!weakpoint.active || weakpoint.destroyed) continue;
        const TayamaPartGroup group = Groups[static_cast<std::size_t>(weakpoint.type)];
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(
            transform, m_tayamaTransformation, modelState, group);
        if (!bounds.valid || !Hit3DSegment(shotStart.x, shotStart.y, shotStart.z,
            shotEnd.x, shotEnd.y, shotEnd.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;

        if (!shot.piercing) shot.active = false;
        weakpoint.hp -= shot.damage;
        weakpoint.hitFlashFrames = BossPartHitFlashFrames;
        SpawnExplosion(FromWorldX(bounds.center.x), FromWorldY(bounds.center.y), bounds.center.z,
            weakpoint.hp <= 0);
        if (weakpoint.hp <= 0) {
            weakpoint.hp = 0;
            weakpoint.destroyed = true;
            m_score += weakpoint.type == TayamaWeakpoint::CommandCore ? 5000 : 750;
            if (weakpoint.type == TayamaWeakpoint::LeftSearchlight) m_searchlights[0].destroyed = true;
            if (weakpoint.type == TayamaWeakpoint::RightSearchlight) m_searchlights[1].destroyed = true;
            PlayStage5Cue(Stage5WeakpointDestroyed);
        } else {
            PlayHitSound();
        }

        bool phaseComplete = true;
        for (const TayamaWeakpointState& current : m_tayamaWeakpoints) {
            if (IsTayamaWeakpointActiveForPhase(current.type, m_stage5Phase) && !current.destroyed) {
                phaseComplete = false;
                break;
            }
        }
        if (phaseComplete) {
            if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
                StartTayamaPhase(Stage5Phase::TayamaLiftEngines);
            } else if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
                StartTayamaPhase(Stage5Phase::TayamaCommandCore);
            } else {
                StartStage5Phase(Stage5Phase::TayamaCollapse, false);
            }
        } else {
            UpdateTayamaBossHp();
        }
        return true;
    }

    // 無効な装甲への命中は小さな着弾だけを出し、HPとスコアを変えない
    for (int group = 0; group < static_cast<int>(TayamaPartGroup::Count); ++group) {
        const Stage5GroupBounds bounds = TayamaModelView::GroupBounds(transform,
            m_tayamaTransformation, modelState, static_cast<TayamaPartGroup>(group));
        if (!bounds.valid || !Hit3DSegment(shotStart.x, shotStart.y, shotStart.z,
            shotEnd.x, shotEnd.y, shotEnd.z, shot.hitRadius * WorldXScale,
            bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius)) continue;
        if (!shot.piercing) shot.active = false;
        SpawnExplosion(shot.x, shot.y, shot.z);
        PlayHitSound();
        return true;
    }
    return false;
}

/**
 * @brief 現在フェーズで有効なTAYAMA弱点HP合計をHUDへ反映する
 * @return なし
 */
void SideScrollingShooter::UpdateTayamaBossHp() {
    int currentHp = 0;
    int currentMaxHp = 0;
    for (const TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (!IsTayamaWeakpointActiveForPhase(weakpoint.type, m_stage5Phase)) continue;
        currentHp += (std::max)(0, weakpoint.hp);
        currentMaxHp += weakpoint.maxHp;
    }
    m_bossHp = currentHp;
    if (m_displayBossHp <= 0.0f || m_displayBossHp > static_cast<float>(currentMaxHp)) {
        m_displayBossHp = static_cast<float>(currentHp);
    }
}

/**
 * @brief TAYAMAの攻略フェーズを開始する
 * @param phase 開始する攻略状態
 * @param resetCurrentHp 現在フェーズのHPを初期値へ戻す場合true
 * @return なし
 */
void SideScrollingShooter::StartTayamaPhase(Stage5Phase phase, bool resetCurrentHp) {
    m_stage5Phase = phase;
    m_stage5PhaseTimer = 0;
    m_stage5AttackTimer = 0;
    m_stage5CoreTargetX = m_playerX;
    m_stage5CoreTargetY = m_playerY;
    m_bossBattle = false;
    m_tayamaTransformation = 1.0f;
    for (auto& enemy : m_enemies) enemy.active = false;
    for (auto& shot : m_shots) {
        if (shot.enemy) shot.active = false;
    }

    // 前フェーズの破壊結果を維持し、現在フェーズだけを有効化する
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        weakpoint.active = IsTayamaWeakpointActiveForPhase(weakpoint.type, phase) && !weakpoint.destroyed;
        if (weakpoint.active && resetCurrentHp) weakpoint.hp = weakpoint.maxHp;
        weakpoint.hitFlashFrames = 0;
    }
    if (phase == Stage5Phase::TayamaFireControl) {
        ResetWallSearchlights(2);
        SaveStage5Checkpoint(Stage5Checkpoint::TayamaFireControl);
    } else if (phase == Stage5Phase::TayamaLiftEngines) {
        SaveStage5Checkpoint(Stage5Checkpoint::TayamaLiftEngines);
    } else {
        SaveStage5Checkpoint(Stage5Checkpoint::TayamaCommandCore);
        PlayStage5Cue(Stage5CoreWarning);
    }
    UpdateTayamaBossHp();
    m_displayBossHp = static_cast<float>(m_bossHp);
    m_invincible = (std::max)(m_invincible, 75);
}

/**
 * @brief TAYAMA戦の更新処理
 * @return なし
 */
void SideScrollingShooter::TickTayama() {
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        if (weakpoint.hitFlashFrames > 0) --weakpoint.hitFlashFrames;
    }
    ++m_stage5AttackTimer;
    m_stage5GuardSpawnCooldown = (std::max)(0, m_stage5GuardSpawnCooldown - 1);

    if (m_stage5Phase == Stage5Phase::TayamaFireControl) {
        TickSearchlights(2, true);
        if (m_stage5AttackTimer % 120 == 72) {
            for (int lane = -3; lane <= 3; ++lane) {
                if (lane == 0) continue;
                SpawnEnemyShotAt(static_cast<float>(lane) * 0.18f, 0.62f, 58.0f,
                    m_playerX + static_cast<float>(lane) * 0.10f,
                    m_playerY + static_cast<float>(lane) * 0.09f, PlayerRailZ, 0.60f);
            }
        }
        if (m_stage5GuardSpawnCooldown == 0 && m_stage5PhaseTimer > 120) {
            SpawnEnemy(Stage::ArmoredEnemy, 1.16f, (m_stage5PhaseTimer / 240) % 2 == 0 ? -0.72f : 0.72f,
                0.58f, 60.0f);
            m_stage5GuardSpawnCooldown = 300;
        }
        const int sweepCycle = m_stage5AttackTimer % 210;
        if (sweepCycle == 0) {
            m_stage5CoreTargetY = m_playerY;
            PlayStage5Cue(Stage5BarrageWarning);
        }
        if (sweepCycle == 36) {
            for (int lane = -4; lane <= 4; ++lane) {
                SpawnEnemyShotAt(static_cast<float>(lane) * 0.27f, 0.70f, 58.0f,
                    static_cast<float>(lane) * 0.27f, m_stage5CoreTargetY,
                    PlayerRailZ, 0.76f);
            }
        }
        return;
    }

    if (m_stage5Phase == Stage5Phase::TayamaLiftEngines) {
        const int cycle = m_stage5AttackTimer % 132;
        if (cycle == 0) {
            m_stage5CoreTargetX = m_playerX;
            m_stage5CoreTargetY = m_playerY;
            PlayStage5Cue(Stage5BarrageWarning);
        }
        if (cycle == 32) {
            for (int engine = 0; engine < 2; ++engine) {
                const TayamaWeakpoint type = engine == 0 ?
                    TayamaWeakpoint::LeftLiftEngine : TayamaWeakpoint::RightLiftEngine;
                if (m_tayamaWeakpoints[static_cast<int>(type)].destroyed) continue;
                const float side = engine == 0 ? -1.0f : 1.0f;
                for (int lane = -2; lane <= 2; ++lane) {
                    SpawnEnemyShotAt(side * 0.72f, -0.42f, 55.0f,
                        m_stage5CoreTargetX + static_cast<float>(lane) * 0.13f,
                        m_stage5CoreTargetY, PlayerRailZ, 0.72f);
                }
            }
        }
        if (m_stage5GuardSpawnCooldown == 0 && m_stage5PhaseTimer > 150) {
            SpawnEnemy(Stage::StraightShooterEnemy, 1.16f,
                (m_stage5PhaseTimer / 360) % 2 == 0 ? -0.58f : 0.58f, 0.54f, 59.0f);
            m_stage5GuardSpawnCooldown = 360;
        }
        return;
    }

    if (m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        const int cycle = m_stage5AttackTimer % 180;
        if (cycle == 0) {
            m_stage5CoreTargetX = m_playerX;
            m_stage5CoreTargetY = m_playerY;
            PlayStage5Cue(Stage5CoreWarning);
        }
        if (cycle == 42 || cycle == 52 || cycle == 62) {
            SpawnEnemyShotAt(0.0f, 0.35f, 55.0f,
                m_stage5CoreTargetX, m_stage5CoreTargetY, PlayerRailZ, 0.92f);
        }
        if (cycle == 104) {
            for (int ray = 0; ray < 12; ++ray) {
                const float angle = static_cast<float>(ray) * Math::TwoPi / 12.0f;
                SpawnEnemyShotAt(0.0f, 0.35f, 55.0f,
                    m_playerX + std::cos(angle) * 0.72f,
                    m_playerY + std::sin(angle) * 0.56f, PlayerRailZ, 0.66f);
            }
        }
        if (cycle == 138) {
            for (int lane = -4; lane <= 4; ++lane) {
                SpawnEnemyShotAt(static_cast<float>(lane) * 0.22f, -0.62f, 54.0f,
                    static_cast<float>(lane) * 0.22f,
                    -0.52f + std::abs(static_cast<float>(lane)) * 0.10f,
                    PlayerRailZ, 0.58f);
            }
        }
    }
}

/**
 * @brief Stage 5専用シーケンスを更新する
 * @return なし
 */
void SideScrollingShooter::TickStage5() {
    ++m_stage5PhaseTimer;

    if (m_stage5Phase == Stage5Phase::EastsourceIntro) {
        if (m_stage5PhaseTimer == 58) {
            SpawnExplosion(0.78f, 0.22f, 61.0f, true);
            constexpr float PanelColor[] = {0.20f, 0.22f, 0.30f, 1.0f};
            for (int i = 0; i < 8; ++i) {
                SpawnDebrisPiece(5.2f + static_cast<float>(i) * 0.28f, 1.0f + static_cast<float>(i % 3),
                    61.0f, 0.03f + static_cast<float>(i) * 0.004f, 0.02f,
                    -0.04f - static_cast<float>(i % 2) * 0.02f, 0.0f, 0.08f,
                    1, 0.8f, 0.35f, 0.16f, PanelColor, 120, 90, false);
            }
        }
        if (m_stage5PhaseTimer >= EastsourceIntroFrames) {
            StartStage5Phase(Stage5Phase::EastsourceBattle);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceBattle) {
        const Enemy& eastsource = m_enemies[0];
        const int phase = eastsource.bossPhase;
        const int pursuitCycle = eastsource.age % 180;
        if ((phase == BossNormalPhase2 || phase == BossSpecialPhase2) && pursuitCycle < 90) {
            TickSearchlights(1, false);
        }
        if (pursuitCycle == 90) ResetWallSearchlights(1);
        return;
    }
    if (m_stage5Phase == Stage5Phase::EastsourceFall) {
        if (m_stage5PhaseTimer >= EastsourceFallFrames) {
            m_enemies[0].active = false;
            StartStage5Phase(Stage5Phase::WallClimbTransition, false);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbTransition) {
        m_tayamaTransformation = Math::Lerp(0.0f, 0.10f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbTransitionFrames));
        if (m_stage5PhaseTimer >= WallClimbTransitionFrames) {
            StartStage5Phase(Stage5Phase::WallClimbLower);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbLower) {
        m_tayamaTransformation = Math::Lerp(0.10f, 0.34f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbLowerFrames));
        TickSearchlights(1, false);
        if (m_stage5PhaseTimer >= WallClimbLowerFrames) {
            StartStage5Phase(Stage5Phase::WallClimbMiddle);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbMiddle) {
        m_tayamaTransformation = Math::Lerp(0.34f, 0.64f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbMiddleFrames));
        TickSearchlights(2, false);
        if (m_stage5GuardSpawnCooldown-- <= 0) {
            SpawnEnemy(Stage::ArmoredEnemy, 1.16f,
                (m_stage5PhaseTimer / 180) % 2 == 0 ? -0.76f : 0.76f, 0.42f, 58.0f);
            m_stage5GuardSpawnCooldown = 210;
        }
        if (m_stage5PhaseTimer >= WallClimbMiddleFrames) {
            StartStage5Phase(Stage5Phase::WallClimbUpper);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::WallClimbUpper) {
        m_tayamaTransformation = Math::Lerp(0.64f, 0.90f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / WallClimbUpperFrames));
        TickSearchlights(3, false);
        if (m_stage5GuardSpawnCooldown-- <= 0) {
            SpawnEnemy(Stage::StraightShooterEnemy, 1.16f,
                (m_stage5PhaseTimer / 150) % 2 == 0 ? -0.82f : 0.82f, -0.34f, 60.0f);
            m_stage5GuardSpawnCooldown = 180;
        }
        if (m_stage5PhaseTimer >= WallClimbUpperFrames) {
            StartStage5Phase(Stage5Phase::RooftopArrival, false);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::RooftopArrival) {
        m_tayamaTransformation = Math::Lerp(0.90f, 0.96f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / RooftopArrivalFrames));
        if (m_stage5PhaseTimer >= RooftopArrivalFrames) {
            StartStage5Phase(Stage5Phase::CarrierTransformation, false);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::CarrierTransformation) {
        m_tayamaTransformation = Math::Lerp(0.96f, 1.0f,
            SmoothStep(static_cast<float>(m_stage5PhaseTimer) / CarrierTransformationFrames));
        if (m_stage5PhaseTimer >= CarrierTransformationFrames) {
            StartTayamaPhase(Stage5Phase::TayamaFireControl);
        }
        return;
    }
    if (m_stage5Phase == Stage5Phase::TayamaFireControl ||
        m_stage5Phase == Stage5Phase::TayamaLiftEngines ||
        m_stage5Phase == Stage5Phase::TayamaCommandCore) {
        TickTayama();
        return;
    }
    if (m_stage5Phase == Stage5Phase::TayamaCollapse) {
        // 艦尾から艦首へ連鎖させ、最終フレームまで画面内で輪郭を崩す
        if (m_stage5PhaseTimer < TayamaCollapseFrames && m_stage5PhaseTimer % 36 == 0) {
            const int burst = m_stage5PhaseTimer / 36;
            SpawnExplosion(-0.85f + static_cast<float>((burst * 7) % 17) * 0.10f,
                -0.34f + static_cast<float>((burst * 5) % 9) * 0.09f, 54.0f, true);
            PlayStage5Cue(Stage5ChainExplosion);
        }
        if (m_stage5PhaseTimer == 450) PlayStage5Cue(Stage5FinalExplosion);
        if (m_stage5PhaseTimer >= TayamaCollapseFrames + Stage5QuietFlightFrames) {
            StartStage5Phase(Stage5Phase::EndingReady, false);
        }
    }
}

/**
 * @brief 現在のStage 5チェックポイントへ復帰する
 * @return なし
 */
void SideScrollingShooter::RestartStage5Checkpoint() {
    ++m_chapterRetryCounts[2];
    m_shots = {};
    m_enemies = {};
    m_items = {};
    m_explosions = {};
    m_debris = {};
    m_power = m_stage5CheckpointPower;
    m_score = m_stage5CheckpointScore;
    m_kills = m_stage5CheckpointKills;
    m_playerX = -0.72f;
    m_playerY = 0.0f;
    m_viewMode = ViewMode::Rail3D;
    m_nextViewMode = ViewMode::Rail3D;
    m_viewTransitionTimer = 0;
    m_viewTransitionProgress = 0.0f;
    m_invincible = 120;
    m_restartTimer = RestartDisplayFrames;

    if (m_stage5Checkpoint == Stage5Checkpoint::Eastsource) {
        StartStage5Phase(Stage5Phase::EastsourceBattle, false);
        return;
    }
    if (m_stage5Checkpoint == Stage5Checkpoint::WallClimbLower ||
        m_stage5Checkpoint == Stage5Checkpoint::WallClimbMiddle ||
        m_stage5Checkpoint == Stage5Checkpoint::WallClimbUpper) {
        const Stage5Phase phase = m_stage5Checkpoint == Stage5Checkpoint::WallClimbLower ?
            Stage5Phase::WallClimbLower :
            (m_stage5Checkpoint == Stage5Checkpoint::WallClimbMiddle ?
                Stage5Phase::WallClimbMiddle : Stage5Phase::WallClimbUpper);
        StartStage5Phase(phase, false);
        return;
    }

    // TAYAMAは前フェーズを破壊済みとし、現在フェーズのHPだけを戻す
    const Stage5Phase phase = m_stage5Checkpoint == Stage5Checkpoint::TayamaFireControl ?
        Stage5Phase::TayamaFireControl :
        (m_stage5Checkpoint == Stage5Checkpoint::TayamaLiftEngines ?
            Stage5Phase::TayamaLiftEngines : Stage5Phase::TayamaCommandCore);
    for (TayamaWeakpointState& weakpoint : m_tayamaWeakpoints) {
        const bool previousPhase =
            (phase >= Stage5Phase::TayamaLiftEngines &&
                static_cast<int>(weakpoint.type) <= static_cast<int>(TayamaWeakpoint::FireControlRadar)) ||
            (phase >= Stage5Phase::TayamaCommandCore &&
                (weakpoint.type == TayamaWeakpoint::LeftLiftEngine ||
                    weakpoint.type == TayamaWeakpoint::RightLiftEngine));
        weakpoint.destroyed = previousPhase;
        weakpoint.hp = previousPhase ? 0 : weakpoint.maxHp;
    }
    StartTayamaPhase(phase, true);
}

/**
 * @brief Stage 5用の効果音をクールダウン付きで再生する
 * @param cue 効果音種別
 * @return なし
 */
void SideScrollingShooter::PlayStage5Cue(int cue) {
    if (!m_audio || m_stage5SoundCooldown > 0) return;
    switch (cue) {
    case Stage5DistantThunder:
        m_audio->PlayMMLSE("t90 o2 l8 v7 c r g");
        m_stage5SoundCooldown = 90;
        break;
    case Stage5Thunder:
        m_audio->PlayMMLSE("t180 o2 l32 v13 c>c<g c");
        m_stage5SoundCooldown = 45;
        break;
    case Stage5SearchlightDetect:
        m_audio->PlayMMLSE("t220 o6 l32 v8 c r c");
        m_stage5SoundCooldown = 18;
        break;
    case Stage5SearchlightLocked:
        m_audio->PlayMMLSE("t240 o6 l16 v11 c>g");
        m_stage5SoundCooldown = 24;
        break;
    case Stage5BarrageWarning:
        m_audio->PlayMMLSE("t180 o4 l32 v10 c c c");
        m_stage5SoundCooldown = 18;
        break;
    case Stage5EastsourceEntrance:
        m_audio->PlayMMLSE("t200 o3 l16 v13 c g > c g");
        m_stage5SoundCooldown = 60;
        break;
    case Stage5SignalLost:
        m_audio->PlayMMLSE("t140 o5 l32 v8 g f e c");
        m_stage5SoundCooldown = 60;
        break;
    case Stage5Transformation:
        m_audio->PlayMMLSE("t110 o2 l16 v12 c d e g");
        m_stage5SoundCooldown = 75;
        break;
    case Stage5WeakpointDestroyed:
        m_audio->PlaySE(Audio::SfxrPreset::Explosion);
        m_stage5SoundCooldown = 20;
        break;
    case Stage5CoreWarning:
        m_audio->PlayMMLSE("t240 o3 l16 v12 c > c < c > c");
        m_stage5SoundCooldown = 36;
        break;
    case Stage5FinalExplosion:
        m_audio->PlayMMLSE("t80 o1 l2 v15 c g c");
        m_stage5SoundCooldown = 90;
        break;
    default:
        m_audio->PlaySE(Audio::SfxrPreset::Explosion);
        m_stage5SoundCooldown = 16;
        break;
    }
}
