#include "Stage4Module.h"

#include <algorithm>
#include <cmath>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../../../Engine/Input/Input.h"
#include "../../../../Engine/Input/KeyCode.h"

#include "Stage4BossModelView.h"
#include "Stage4EnemySheet.h"
#include "Stage4EnemySheetEasy.h"
#include "Stage4EnemySheetHard.h"
#include "Stage4EnemySheetNormal.h"
#include "Stage4WeaponDroneView.h"
#include "../../GameplayRandom.h"

using ShooterStages::Stage4::ShotKind;
using Stage4Logic = ShooterStages::Stage4::State;
using Stage4BossPhase = ShooterStages::Stage4::BossPhase;
using Stage4Weapon = ShooterStages::Stage4::MainWeaponType;
using Stage4WeaponVisual = ShooterStages::Stage4::WeaponVisualState;
using Stage4SwapState = ShooterStages::Stage4::WeaponSwapState;
using Stage4SwapConfig = ShooterStages::Stage4::WeaponSwapConfig;

namespace {

constexpr float Stage4BossScale = 1.00f;
constexpr float Stage4ModelYawOffset = -Math::HalfPi;
constexpr float Stage4TrackWheelRadius = 0.74f;
constexpr float Stage4CannonballSpeed = 0.56f;
constexpr float Stage4CannonballGravity = 0.00135f;
constexpr float Stage4SiegeMortarPitchRate = Math::Pi * 1.4f / 180.0f;
constexpr float Stage4SiegeMortarYawRate = Math::Pi * 1.8f / 180.0f;
constexpr float Stage4CannonMuzzleDistance = 4.47f;
constexpr float Stage4CannonballSideRadius = 0.075f;
constexpr float Stage4GiantCannonballSideRadius = 0.150f;
constexpr float Stage4GiantExplosionRadius = 0.65f;
constexpr float Stage4RailGroundGameY = -0.829545f;
constexpr float Stage4BodyHitRadius = 3.35f;
constexpr int Stage4MainCannonRecoilFrames = 30;
constexpr Vector3 Stage4MainCannonLocal {-5.75f, 3.55f, 0.0f};
constexpr Vector3 Stage4MainCannonPivotLocal {-3.25f, 3.55f, 0.0f};
constexpr Vector3 Stage4BodyHitLocal[] = {
    {-4.70f, 0.35f, -7.40f}, {-4.70f, 0.35f, 0.0f}, {-4.70f, 0.35f, 7.40f},
    {0.00f, 0.35f, -7.40f}, {0.00f, 0.35f, 0.0f}, {0.00f, 0.35f, 7.40f},
    {4.70f, 0.50f, -7.40f}, {4.70f, 0.50f, 0.0f}, {4.70f, 0.50f, 7.40f},
    {-1.35f, 2.55f, -4.40f}, {-1.35f, 2.55f, 0.0f}, {-1.35f, 2.55f, 4.40f}
};
constexpr Vector3 Stage4SecondaryGunLocal[] = {
    {-3.40f, 3.76f, -3.45f}, {-3.40f, 3.76f, 3.45f},
    {-4.40f, 3.08f, -7.65f}, {-4.40f, 3.08f, 7.65f},
    {3.10f, 3.00f, -7.15f}, {3.10f, 3.00f, 7.15f}
};
constexpr float Stage4MainCannonHitRadius = 1.45f;
constexpr float Stage4SecondaryGunHitRadius = 0.82f;

/** @brief 表示モード別の迫撃砲攻撃設定 */
struct Stage4SiegeMortarConfig {
    float normalSpeed;
    float giantSpeed;
    float minPitch;
    float maxPitch;
    float minYaw;
    float maxYaw;
};

constexpr Stage4SiegeMortarConfig Stage4SiegeMortarSideConfig {
    0.20f, 0.35f,
    Stage4BossModelView::SiegeMortarMinPitch,
    Stage4BossModelView::SiegeMortar2DMaxPitch,
    0.0f, 0.0f
};
constexpr Stage4SiegeMortarConfig Stage4SiegeMortarRailConfig {
    0.7f, 0.80f,
    Stage4BossModelView::SiegeMortarMinPitch,
    Stage4BossModelView::SiegeMortar3DMaxPitch,
    -Math::Pi * 10.0f / 180.0f,
    Math::Pi * 10.0f / 180.0f
};

/** @brief 交換工程に対応する継続フレーム数を取得する @param state 工程 @param config 演出設定 @return 継続フレーム数 */
int SwapStateFrames(Stage4SwapState state, const Stage4SwapConfig& config) {
    switch (state) {
    case Stage4SwapState::Prepare: return config.prepareFrames;
    case Stage4SwapState::Unlock: return config.unlockFrames;
    case Stage4SwapState::Purge: return config.purgeFrames;
    case Stage4SwapState::WaitingForDrone: return config.waitingFrames;
    case Stage4SwapState::DroneApproach: return config.approachFrames;
    case Stage4SwapState::CarryWeapon: return config.carryFrames;
    case Stage4SwapState::AlignWeapon: return config.alignFrames;
    case Stage4SwapState::MountWeapon: return config.mountFrames;
    case Stage4SwapState::LockWeapon: return config.lockFrames;
    case Stage4SwapState::DroneRelease: return config.releaseFrames;
    case Stage4SwapState::DroneRetreat: return config.retreatFrames;
    default: return 0;
    }
}

/** @brief 論理主砲種別を描画主砲種別へ変換する @param weapon 論理主砲種別 @return 描画主砲種別 */
Stage4MainWeaponType ViewWeaponType(Stage4Weapon weapon) {
    switch (weapon) {
    case Stage4Weapon::SiegeMortar: return Stage4MainWeaponType::SiegeMortar;
    case Stage4Weapon::RomanceCannon: return Stage4MainWeaponType::RomanceCannon;
    default: return Stage4MainWeaponType::Phase1Cannon;
    }
}

/** @brief 表示モードに対応する迫撃砲設定を取得する @param railMode 3D表示中か @return 迫撃砲設定 */
constexpr Stage4SiegeMortarConfig SiegeMortarConfig(bool railMode) {
    return railMode ? Stage4SiegeMortarRailConfig : Stage4SiegeMortarSideConfig;
}

/** @brief 論理主砲に対応する現在姿勢を取得する @param state Stage 4状態 @param weapon 論理主砲種別 @return 主砲姿勢 */
Stage4MainWeaponPose WeaponPose(const Stage4Logic& state, Stage4Weapon weapon, bool railMode) {
    const Stage4MainWeaponType weaponType = ViewWeaponType(weapon);
    Stage4MainWeaponPose pose = Stage4BossModelView::DefaultMainWeaponPose(weaponType);
    if (weapon == Stage4Weapon::SiegeMortar) {
        const Stage4SiegeMortarConfig config = SiegeMortarConfig(railMode);
        pose.barrelPitch = (std::clamp)(state.siegeMortarPitch,
            config.minPitch, config.maxPitch);
        pose.localYaw = railMode ? (std::clamp)(state.siegeMortarYaw,
            config.minYaw, config.maxYaw) : 0.0f;
    }
    return pose;
}

/** @brief 迫撃砲の姿勢から発射速度を作る @param baseYaw 車体基準Yaw @param yawOffset 主砲左右角 @param pitch 主砲仰角 @param speed 発射速度 @return ワールド弾速 */
Vector3 SiegeMortarVelocity(float baseYaw, float yawOffset, float pitch, float speed) {
    const float horizontalSpeed = speed * std::cos(pitch);
    const float yaw = baseYaw + yawOffset;
    return {
        -horizontalSpeed * std::cos(yaw),
        speed * std::sin(pitch),
        horizontalSpeed * std::sin(yaw)
    };
}

/** @brief 親Transformのローカル方向へ位置をずらす @param transform 基準Transform @param offset ローカル移動量 @return 移動後Transform */
BossModelTransform OffsetTransform(const BossModelTransform& transform, const Vector3& offset) {
    BossModelTransform result = transform;
    const float cosine = std::cos(transform.yaw);
    const float sine = std::sin(transform.yaw);
    result.position += Vector3 {
        offset.x * cosine + offset.z * sine,
        offset.y,
        -offset.x * sine + offset.z * cosine
    } * transform.scale;
    return result;
}

/** @brief 0から1の工程進捗を取得する @param state 交換状態 @param config 演出設定 @return 補間用進捗 */
float SwapProgress(const Stage4Logic& state, const Stage4SwapConfig& config) {
    const int frames = SwapStateFrames(state.swapState, config);
    return frames > 0 ? Math::Clamp01(static_cast<float>(state.timer) /
        static_cast<float>(frames)) : 1.0f;
}

static_assert(sizeof(Stage4SecondaryGunLocal) / sizeof(Stage4SecondaryGunLocal[0]) == 6);
static_assert(sizeof(Stage4BodyHitLocal) / sizeof(Stage4BodyHitLocal[0]) == 12);

}

void SideScrollingShooter::Stage4Module::Reset(SideScrollingShooter& shooter) {
    shooter.m_stage4 = {};
}

void SideScrollingShooter::Stage4Module::ProcessDebugInput(SideScrollingShooter& shooter) {
#ifdef _DEBUG
    // F6とF7から二種類の交換演出をHP操作なしで開始する
    if (Input::GetKeyDown(KeyCode::F6)) {
        StartDebugWeaponSwap(shooter, Stage4Weapon::SiegeMortar);
    }
    if (Input::GetKeyDown(KeyCode::F7)) {
        StartDebugWeaponSwap(shooter, Stage4Weapon::RomanceCannon);
    }
#else
    (void)shooter;
#endif
}

bool SideScrollingShooter::Stage4Module::TickWeaponSwap(
    SideScrollingShooter& shooter, Enemy& boss) {
    Stage4Logic& state = shooter.m_stage4;

    // 被弾で先行更新された攻撃フェーズから必要な交換を開始する
    if (state.swapState == Stage4SwapState::None) {
        if (boss.bossPhase == BossSpecialPhase1 && state.phase == Stage4BossPhase::Phase1) {
            BeginWeaponSwap(shooter, boss, Stage4Weapon::SiegeMortar);
        } else if (boss.bossPhase >= BossNormalPhase2 &&
            state.phase == Stage4BossPhase::Phase2) {
            BeginWeaponSwap(shooter, boss, Stage4Weapon::RomanceCannon);
        }
    }
    if (state.swapState == Stage4SwapState::None) return false;

    // 工程ごとの設定時間が過ぎたらタイマーをリセットして次へ進む
    const Stage4SwapConfig config = ShooterStages::Stage4::SwapConfig(state.incomingWeapon);
    if (++state.timer >= SwapStateFrames(state.swapState, config)) {
        AdvanceWeaponSwap(shooter);
        if (state.swapState == Stage4SwapState::Complete) AdvanceWeaponSwap(shooter);
    }
    return true;
}

bool SideScrollingShooter::Stage4Module::IsWeaponSwapActive(
    const SideScrollingShooter& shooter) {
    return shooter.m_stage4.swapState != Stage4SwapState::None;
}

void SideScrollingShooter::Stage4Module::TickSiegeMortarAim(SideScrollingShooter& shooter) {
    Stage4Logic& state = shooter.m_stage4;
    if (state.currentWeapon != Stage4Weapon::SiegeMortar) return;
    const bool railMode = shooter.IsRailGameplayActive();
    const Stage4SiegeMortarConfig config = SiegeMortarConfig(railMode);

    // 射撃後に決まった目標仰角へ一定速度で砲身を動かす
    state.siegeMortarPitch = (std::clamp)(state.siegeMortarPitch,
        config.minPitch, config.maxPitch);
    state.siegeMortarTargetPitch = (std::clamp)(state.siegeMortarTargetPitch,
        config.minPitch, config.maxPitch);
    const float delta = state.siegeMortarTargetPitch - state.siegeMortarPitch;
    if (std::abs(delta) <= Stage4SiegeMortarPitchRate) {
        state.siegeMortarPitch = state.siegeMortarTargetPitch;
    } else {
        state.siegeMortarPitch += delta > 0.0f ?
            Stage4SiegeMortarPitchRate : -Stage4SiegeMortarPitchRate;
    }

    // 3D表示中だけ正面基準の左右旋回を行う
    state.siegeMortarYaw = (std::clamp)(state.siegeMortarYaw,
        config.minYaw, config.maxYaw);
    state.siegeMortarTargetYaw = railMode ? (std::clamp)(state.siegeMortarTargetYaw,
        config.minYaw, config.maxYaw) : 0.0f;
    const float yawDelta = state.siegeMortarTargetYaw - state.siegeMortarYaw;
    if (std::abs(yawDelta) <= Stage4SiegeMortarYawRate) {
        state.siegeMortarYaw = state.siegeMortarTargetYaw;
    } else {
        state.siegeMortarYaw += yawDelta > 0.0f ?
            Stage4SiegeMortarYawRate : -Stage4SiegeMortarYawRate;
    }
}

bool SideScrollingShooter::Stage4Module::HandleBossPhaseAfterDamage(
    SideScrollingShooter& shooter, Enemy& boss) {
    Stage4Logic& state = shooter.m_stage4;

    // 交換中は次の閾値判定と撃破を保留して演出の連続スキップを防ぐ
    if (IsWeaponSwapActive(shooter)) {
        const int minimumHp = state.incomingWeapon == Stage4Weapon::SiegeMortar ?
            boss.maxHp / 3 + 1 : 1;
        boss.hp = (std::max)(boss.hp, minimumHp);
        shooter.m_bossHp = boss.hp;
        return false;
    }

    // 一度の大ダメージでも一段階だけ進めて必ず各交換演出を通す
    const int requestedPhase = shooter.m_stage->BossPhaseForHp(boss.hp, boss.maxHp);
    if (requestedPhase > boss.bossPhase && boss.bossPhase < BossNormalPhase2) {
        ++boss.bossPhase;
        const Stage4Weapon incoming = boss.bossPhase == BossSpecialPhase1 ?
            Stage4Weapon::SiegeMortar : Stage4Weapon::RomanceCannon;
        BeginWeaponSwap(shooter, boss, incoming);
        const int minimumHp = incoming == Stage4Weapon::SiegeMortar ?
            boss.maxHp / 3 + 1 : 1;
        boss.hp = (std::max)(boss.hp, minimumHp);
        shooter.m_bossHp = boss.hp;
        return false;
    }
    return boss.hp <= 0;
}

void SideScrollingShooter::Stage4Module::BeginWeaponSwap(
    SideScrollingShooter& shooter, Enemy& boss, Stage4Weapon incomingWeapon) {
    Stage4Logic& state = shooter.m_stage4;
    state.outgoingWeapon = state.currentWeapon;
    state.incomingWeapon = incomingWeapon;
    state.outgoingVisual = Stage4WeaponVisual::Attached;
    state.incomingVisual = Stage4WeaponVisual::Hidden;
    state.phase = incomingWeapon == Stage4Weapon::SiegeMortar ?
        Stage4BossPhase::TransitionToPhase2 : Stage4BossPhase::TransitionToPhase3;
    state.swapState = Stage4SwapState::Prepare;
    state.timer = 0;
    boss.phase = 0.0f;
    boss.recoilAge = 0;

    // 交換開始時に敵弾を消して演出と主砲なし状態を読みやすくする
    for (auto& shot : shooter.m_shots) {
        if (shot.enemy) shot.active = false;
    }
}

void SideScrollingShooter::Stage4Module::AdvanceWeaponSwap(SideScrollingShooter& shooter) {
    Stage4Logic& state = shooter.m_stage4;
    const bool mountedRomance = state.swapState == Stage4SwapState::MountWeapon &&
        state.incomingWeapon == Stage4Weapon::RomanceCannon;

    // 状態側の共通所有権遷移を使い装着瞬間だけ画面振動を加える
    ShooterStages::Stage4::AdvanceWeaponSwap(state);
    if (mountedRomance) {
        shooter.m_screenShakeIntensity = 0.045f;
        shooter.m_screenShakeFrames = 18;
        shooter.m_screenShakeDurationFrames = 18;
    }
}

void SideScrollingShooter::Stage4Module::StartDebugWeaponSwap(
    SideScrollingShooter& shooter, Stage4Weapon incomingWeapon) {
    // ボス戦外からの入力は既存ボス直行処理で必要な戦闘状態を作る
    if (shooter.m_stageNumber != 4 || !shooter.m_bossBattle ||
        !shooter.m_enemies[0].active || shooter.m_enemies[0].type != 2) {
        shooter.StartDebugCheckpoint(4, 3, true);
    }

    Enemy& boss = shooter.m_enemies[0];
    shooter.m_stage4 = {};
    if (incomingWeapon == Stage4Weapon::RomanceCannon) {
        shooter.m_stage4.phase = Stage4BossPhase::Phase2;
        shooter.m_stage4.currentWeapon = Stage4Weapon::SiegeMortar;
        shooter.m_stage4.outgoingWeapon = Stage4Weapon::SiegeMortar;
        boss.bossPhase = BossSpecialPhase1;
        boss.hp = boss.maxHp / 3;
    } else {
        boss.bossPhase = BossNormalPhase1;
        boss.hp = boss.maxHp * 2 / 3;
    }
    ++boss.bossPhase;
    shooter.m_bossHp = boss.hp;
    shooter.m_displayBossHp = static_cast<float>(boss.hp);
    BeginWeaponSwap(shooter, boss, incomingWeapon);
}

const SideScrollingShooter::Stage& SideScrollingShooter::Stage4Module::Definition(
    DifficultyType difficulty) {
    static const Stage4EnemySheetEasy easyStage;
    static const Stage4EnemySheetNormal normalStage;
    static const Stage4EnemySheetHard hardStage;
    switch (difficulty) {
    case Hard: return hardStage;
    case Normal: return normalStage;
    default: return easyStage;
    }
}

bool SideScrollingShooter::Stage4Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float yaw) {
    if (enemy.type != 2) return false;

    // Stage2と同じ親Transform経由で専用モデルを描画する
    const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
    const Vector3 aimTarget {
        ToWorldX(enemy.turretAimX),
        ToWorldY(enemy.turretAimY),
        Math::Lerp(SidePlaneZ, enemy.turretAimZ, railWeight)
    };
    BossModelTransform transform {
        {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z},
        aimTarget, yaw + Stage4ModelYawOffset, Stage4BossScale,
        enemy.bossPartHp[BossNose] > 0, true
    };
    const Stage4Logic& swap = shooter.m_stage4;
    const Stage4SwapConfig swapConfig = ShooterStages::Stage4::SwapConfig(swap.incomingWeapon);

    // Romance Cannon装着時だけ車体全体を沈ませ重量を受ける動きを作る
    if (swap.currentWeapon == Stage4Weapon::RomanceCannon &&
        swap.swapState == Stage4SwapState::LockWeapon) {
        const float progress = SwapProgress(swap, swapConfig);
        transform.position.y -= std::sin(progress * Math::Pi) * 0.18f;
    }
    Stage4BossModelState state;
    state.mainCannon = false;
    state.mainCannonHit = enemy.bossPartHitFlashFrames[BossNose] > 0 &&
        (enemy.bossPartHitFlashFrames[BossNose] / 2) % 2 != 0;
    for (int i = 0; i < 6; ++i) {
        const int part = BossFunnelHatch0 + i;
        state.secondaryGuns[i] = enemy.bossPartHp[part] > 0;
        state.secondaryGunsHit[i] = enemy.bossPartHitFlashFrames[part] > 0 &&
            (enemy.bossPartHitFlashFrames[part] / 2) % 2 != 0;
    }
    BossModelTransform aimedTransform = transform;
    aimedTransform.secondaryAimTarget = aimTarget;
    aimedTransform.secondaryGunsTrackTarget = true;
    aimedTransform.mainGunTracksTarget = swap.currentWeapon == Stage4Weapon::Phase1Cannon &&
        swap.swapState == Stage4SwapState::None && enemy.phase <= 0.0f;
    aimedTransform.trackRoll = (shooter.IsRailGameplayActive() ?
        enemy.z - enemy.baseZ : ToWorldX(enemy.x - enemy.baseX)) /
        Stage4TrackWheelRadius;

    // Stage4BossModelViewの出力を既存Primitive描画へ接続する
    auto DrawBossPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float partYaw, float partPitch) {
        DrawModelPrimitive(renderer, camera, static_cast<PrimitiveShape>(shape),
            position.x, position.y, position.z,
            scale.x, scale.y, scale.z, color, partYaw, partPitch);
    };
    Stage4BossModelView::DrawBody(aimedTransform, DrawBossPart, state);

    // 装着主砲またはパージ中の旧主砲を独立Transformで描画する
    const BossModelTransform mount = Stage4BossModelView::MainWeaponMount(aimedTransform);
    if (swap.outgoingVisual != Stage4WeaponVisual::Hidden) {
        BossModelTransform outgoingTransform = mount;
        if (swap.swapState == Stage4SwapState::Unlock) {
            const float progress = SmoothStep(SwapProgress(swap, swapConfig));
            outgoingTransform = OffsetTransform(outgoingTransform,
                {0.0f, progress * 0.35f + std::sin(swap.timer * 0.85f) * 0.045f, 0.0f});
        } else if (swap.outgoingVisual == Stage4WeaponVisual::Detached) {
            const float progress = SwapProgress(swap, swapConfig);
            const float travel = progress * progress;
            const Vector3 purgeOffset = swap.outgoingWeapon == Stage4Weapon::Phase1Cannon ?
                Vector3 {-10.0f * travel, 0.35f + 4.8f * travel, 0.0f} :
                Vector3 {7.0f * travel, 0.35f + 7.2f * travel, 0.0f};
            outgoingTransform = OffsetTransform(outgoingTransform, purgeOffset);
        }
        const Stage4MainWeaponType outgoingType = ViewWeaponType(swap.outgoingWeapon);
        const Stage4MainWeaponPose outgoingPose =
            WeaponPose(swap, swap.outgoingWeapon, shooter.IsRailGameplayActive());
        Stage4BossModelView::DrawMainWeapon(outgoingType, outgoingTransform,
            outgoingPose, DrawBossPart, state.mainCannonHit);

        if (swap.swapState == Stage4SwapState::Purge) {
            // 旧主砲下面の補助エンジンへ既存の噴射煙と炎を重ねる
            const Vector3 nozzle = Stage4BossModelView::MainWeaponPointWorldPosition(
                outgoingTransform, outgoingPose,
                Stage4BossModelView::PurgeThrusterLocalPosition(outgoingType));
            const float scale = outgoingTransform.scale;
            const Matrix4x4 smokeWorld = Matrix4x4::Translation(
                nozzle + Vector3 {0.0f, -1.65f * scale, 0.0f}) *
                Matrix4x4::Scale({0.52f * scale, 1.10f * scale, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * smokeWorld,
                static_cast<float>(shooter.m_frame) / 12.0f, 1});
            const Matrix4x4 flameWorld = Matrix4x4::Translation(
                nozzle + Vector3 {0.0f, -0.82f * scale, 0.0f}) *
                Matrix4x4::Scale({0.38f * scale, 0.90f * scale, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * flameWorld,
                static_cast<float>(shooter.m_frame) / 12.0f, 3});
        }
    }

    // 搬入主砲はSpawn、Approach、Carry、Align、Mountの各点を滑らかに結ぶ
    BossModelTransform incomingTransform = mount;
    if (swap.incomingVisual != Stage4WeaponVisual::Hidden) {
        const bool romance = swap.incomingWeapon == Stage4Weapon::RomanceCannon;
        const Vector3 spawnOffset = romance ? Vector3 {15.0f, 13.0f, 0.0f} :
            Vector3 {11.0f, 10.0f, 0.0f};
        const Vector3 approachOffset = romance ? Vector3 {8.0f, 8.5f, 0.0f} :
            Vector3 {6.0f, 7.0f, 0.0f};
        const Vector3 carryOffset = romance ? Vector3 {3.0f, 6.0f, 0.0f} :
            Vector3 {2.0f, 5.0f, 0.0f};
        const Vector3 alignOffset {0.0f, romance ? 3.2f : 2.5f, 0.0f};
        const float progress = SmoothStep(SwapProgress(swap, swapConfig));
        Vector3 offset {};
        switch (swap.swapState) {
        case Stage4SwapState::DroneApproach:
            offset = Math::Lerp(spawnOffset, approachOffset, progress);
            break;
        case Stage4SwapState::CarryWeapon:
            offset = Math::Lerp(approachOffset, carryOffset, progress);
            break;
        case Stage4SwapState::AlignWeapon:
            offset = Math::Lerp(carryOffset, alignOffset, progress);
            break;
        case Stage4SwapState::MountWeapon:
            offset = Math::Lerp(alignOffset, Vector3::Zero, progress);
            break;
        default:
            break;
        }
        if (romance && swap.incomingVisual == Stage4WeaponVisual::Carried) {
            offset.y += std::sin(static_cast<float>(swap.timer) * 0.12f) * 0.08f;
        }
        incomingTransform = OffsetTransform(incomingTransform, offset);
        const Stage4MainWeaponType incomingType = ViewWeaponType(swap.incomingWeapon);
        const Stage4MainWeaponPose incomingPose =
            WeaponPose(swap, swap.incomingWeapon, shooter.IsRailGameplayActive());
        Stage4BossModelView::DrawMainWeapon(
            incomingType, incomingTransform, incomingPose, DrawBossPart);

        // 主砲側CarryPointへ各ドローンのLiftPointを一致させて編隊を作る
        if (swap.swapState >= Stage4SwapState::DroneApproach &&
            swap.swapState <= Stage4SwapState::DroneRetreat) {
            Stage4WeaponDronePose dronePose;
            dronePose.leftShoulderPitch = 0.12f;
            dronePose.rightShoulderPitch = 0.12f;
            dronePose.leftElbowPitch = -0.18f;
            dronePose.rightElbowPitch = -0.18f;
            if (swap.swapState == Stage4SwapState::DroneApproach) {
                dronePose.leftClampOpen = 1.0f - progress;
                dronePose.rightClampOpen = 1.0f - progress;
                dronePose.thrusterTilt = -0.18f * (1.0f - progress);
            } else if (swap.swapState == Stage4SwapState::DroneRelease) {
                dronePose.leftClampOpen = progress;
                dronePose.rightClampOpen = progress;
            } else if (swap.swapState == Stage4SwapState::DroneRetreat) {
                dronePose.leftClampOpen = 1.0f;
                dronePose.rightClampOpen = 1.0f;
                dronePose.leftShoulderPitch = -0.28f * progress;
                dronePose.rightShoulderPitch = -0.28f * progress;
                dronePose.thrusterTilt = 0.32f * progress;
            }
            for (int index = 0; index < swapConfig.droneCount; ++index) {
                const Vector3 carryPoint = Stage4BossModelView::MainWeaponPointWorldPosition(
                    incomingTransform, incomingPose,
                    Stage4BossModelView::CarryPointLocalPosition(incomingType, index));
                BossModelTransform droneTransform = incomingTransform;
                droneTransform.scale = Stage4BossScale * 0.92f;
                droneTransform = Stage4WeaponDroneView::PlaceLiftPointAt(
                    droneTransform, dronePose, carryPoint);
                if (swap.swapState == Stage4SwapState::DroneRetreat) {
                    const float side = index % 2 == 0 ? -1.0f : 1.0f;
                    droneTransform = OffsetTransform(droneTransform,
                        {5.0f * progress, 8.0f * progress, side * 2.5f * progress});
                }
                Stage4WeaponDroneView::Draw(droneTransform, dronePose, DrawBossPart);
            }
        }
    }
    return true;
}

bool SideScrollingShooter::Stage4Module::TryHitBossPart(
    const SideScrollingShooter& shooter, const Shot& shot,
    const Enemy& boss, BossPart& part) {
    if (boss.type != 2) return false;

    // 主砲身をBossNose枠として判定する
    if (boss.bossPartHp[BossNose] > 0) {
        const Vector3 world = LocalToWorld(shooter, boss, BossPartLocalPosition(BossNose));
        const bool hit = shooter.IsRailGameplayActive() ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy),
                shot.z - shot.vz, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, world.x, world.y, world.z,
                Stage4MainCannonHitRadius) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y),
                Stage4MainCannonHitRadius / WorldXScale);
        if (hit) {
            part = BossNose;
            return true;
        }
    }

    // 副砲6基をBossFunnelHatch0からBossFunnelHatch5の枠として判定する
    for (int gun = 0; gun < 6; ++gun) {
        const BossPart candidate = static_cast<BossPart>(BossFunnelHatch0 + gun);
        if (boss.bossPartHp[candidate] <= 0) continue;
        const Vector3 world = LocalToWorld(shooter, boss, BossPartLocalPosition(candidate));
        const bool hit = shooter.IsRailGameplayActive() ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy),
                shot.z - shot.vz, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, world.x, world.y, world.z,
                Stage4SecondaryGunHitRadius) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y),
                Stage4SecondaryGunHitRadius / WorldXScale);
        if (!hit) continue;
        part = candidate;
        return true;
    }
    return false;
}

void SideScrollingShooter::Stage4Module::FireBossPartBarrage(
    SideScrollingShooter& shooter, Enemy& boss) {
    if (boss.type != 2 || IsWeaponSwapActive(shooter)) return;

    // 生存中の主砲身と副砲6基から、現在の自機位置へ直接撃つ
    for (int partIndex = 0; partIndex < BossPartCount; ++partIndex) {
        const BossPart part = static_cast<BossPart>(partIndex);
        if (boss.bossPartHp[part] <= 0) continue;
        if (part != BossNose && SecondaryGunIndex(part) < 0) continue;

        if (part == BossNose) {
            SpawnMainCannonball(shooter, boss);
            boss.recoilAge = Stage4MainCannonRecoilFrames;
            continue;
        }

        const Vector3 world = LocalToWorld(shooter, boss, BossPartLocalPosition(part));
        const int bulletCount = shooter.m_stage->BossPartBulletCount(
            part, static_cast<BossPhase>(boss.bossPhase), shooter.IsRailGameplayActive());
        for (int index = 0; index < bulletCount; ++index) {
            const float spread = static_cast<float>(index) -
                static_cast<float>(bulletCount - 1) * 0.5f;
            const float sourceX = FromWorldX(world.x);
            const float sourceY = FromWorldY(world.y);
            const float targetY = shooter.m_playerY + spread * 0.08f;
            if (shooter.IsRailGameplayActive()) {
                const float dx = ToWorldX(shooter.m_playerX) - world.x;
                const float dy = ToWorldY(targetY) - world.y;
                const float dz = PlayerRailZ - world.z;
                const float length = (std::max)(
                    0.001f, std::sqrt(dx * dx + dy * dy + dz * dz));
                const float speed = boss.behavior != nullptr ?
                    boss.behavior->RailAimedShotSpeed() : 0.62f;
                shooter.SpawnShotDirect(sourceX, sourceY, world.z,
                    FromWorldX(dx / length * speed),
                    FromWorldY(dy / length * speed),
                    dz / length * speed, true, index, bulletCount);
            } else {
                const float dx = shooter.m_playerX - sourceX;
                const float dy = targetY - sourceY;
                const float length = (std::max)(
                    0.001f, std::sqrt(dx * dx + dy * dy));
                const float speed = boss.behavior != nullptr ?
                    boss.behavior->AimedShotSpeed() : 0.018f;
                shooter.SpawnShot(sourceX, sourceY,
                    dx / length * speed, dy / length * speed, true, world.z,
                    boss.behavior != nullptr ? boss.behavior->RailAimedShotSpeed() : 0.62f);
            }
        }
    }
}

bool SideScrollingShooter::Stage4Module::HitsHazard(
    const SideScrollingShooter& shooter, float x, float y, float z, float radius) {
    for (const Enemy& enemy : shooter.m_enemies) {
        if (!enemy.active || enemy.type != 2 || !enemy.collisionEnabled) continue;

        // 車体と砲塔の主要な塊を描画用ローカル座標と同じ基準で判定する
        for (const Vector3& local : Stage4BodyHitLocal) {
            const Vector3 world = LocalToWorld(shooter, enemy, local);
            const bool hit = shooter.IsRailGameplayActive() ?
                Hit3D(ToWorldX(x), ToWorldY(y), z, radius * WorldXScale,
                    world.x, world.y, world.z, Stage4BodyHitRadius) :
                Hit(x, y, radius, FromWorldX(world.x), FromWorldY(world.y),
                    Stage4BodyHitRadius / WorldXScale);
            if (hit) return true;
        }
    }
    return false;
}

void SideScrollingShooter::Stage4Module::TickSpecialShotBeforeMove(
    SideScrollingShooter& shooter, Shot& shot) {
    (void)shooter;
    if (!shot.enemy || shot.stage4.kind != ShotKind::Cannonball) return;

    // 重力付き砲撃だけ毎フレーム落下速度を増やす
    if (shot.stage4.gravity) shot.vy -= Stage4CannonballGravity;
    ++shot.age;
}

void SideScrollingShooter::Stage4Module::TickSpecialShotAfterMove(
    SideScrollingShooter& shooter, Shot& shot,
    float previousX, float previousY, float previousZ) {
    if (!shot.enemy || shot.stage4.kind != ShotKind::Cannonball) return;

    // 3D中は自機レーン到達を優先しつつ、手前で地面や端に当たる場合はそちらを着弾扱いにする
    const float groundY = shooter.IsRailGameplayActive() ?
        Stage4RailGroundGameY : Side2DPlayerMinY;
    const bool hitGround = previousY > groundY && shot.y <= groundY;
    const bool hitEdge = shooter.IsRailGameplayActive() ?
        (shot.z <= 0.0f || shot.z >= 72.0f ||
            std::abs(shot.x) >= 1.2f ||
            (!shot.stage4.gravity && std::abs(shot.y) >= 1.24f)) :
        (shot.x <= Side2DPlayerMinX || shot.x >= Side2DPlayerMaxX ||
            (!shot.stage4.gravity && shot.y >= Side2DPlayerMaxY));
    const bool hitPlayerZ = shot.stage4.detonateAtPlayerZ && shooter.IsRailGameplayActive() &&
        ((previousZ <= PlayerRailZ && shot.z >= PlayerRailZ) ||
            (previousZ >= PlayerRailZ && shot.z <= PlayerRailZ));
    if (!hitGround && !hitEdge && !hitPlayerZ) return;

    bool impactAtPlayerZ = false;
    float impactZ = shot.z;
    if (hitPlayerZ) {
        const auto crossRatio = [](float previous, float current, float target) {
            const float travel = current - previous;
            return std::abs(travel) <= 0.0001f ? 0.0f : (target - previous) / travel;
        };
        const auto useEarliest = [](float& current, float candidate) {
            if (candidate >= 0.0f && candidate < current) current = candidate;
        };
        const float zRatio = crossRatio(previousZ, shot.z, PlayerRailZ);
        float obstacleRatio = 2.0f;
        if (hitGround) useEarliest(obstacleRatio, crossRatio(previousY, shot.y, groundY));
        if (shot.z <= 0.0f) useEarliest(obstacleRatio, crossRatio(previousZ, shot.z, 0.0f));
        if (shot.z >= 72.0f) useEarliest(obstacleRatio, crossRatio(previousZ, shot.z, 72.0f));
        if (shot.x <= -1.2f) useEarliest(obstacleRatio, crossRatio(previousX, shot.x, -1.2f));
        if (shot.x >= 1.2f) useEarliest(obstacleRatio, crossRatio(previousX, shot.x, 1.2f));
        if (shot.y <= -1.24f) useEarliest(obstacleRatio, crossRatio(previousY, shot.y, -1.24f));
        if (shot.y >= 1.24f) useEarliest(obstacleRatio, crossRatio(previousY, shot.y, 1.24f));
        if (zRatio <= obstacleRatio) {
            impactAtPlayerZ = true;
            impactZ = PlayerRailZ;
        }
    }

    shooter.SpawnMortarExplosion(
        shot.x, hitGround && !impactAtPlayerZ ? groundY : shot.y,
        impactZ, shot.stage4.explosionRadius);
    shot.active = false;
}

bool SideScrollingShooter::Stage4Module::IsShotCullProtected(const Shot& shot) {
    return shot.stage4.kind == ShotKind::Cannonball;
}

float SideScrollingShooter::Stage4Module::EnemyShotHitRadius(
    const Shot& shot, bool railMode) {
    if (shot.stage4.kind != ShotKind::Cannonball) {
        return railMode ? 0.28f : 0.022f;
    }
    return railMode ? shot.hitRadius * WorldXScale : shot.hitRadius;
}

bool SideScrollingShooter::Stage4Module::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& boss, int bossPart) {
    if (boss.type != 2) return false;
    const BossPart part = static_cast<BossPart>(bossPart);
    if (part != BossNose && SecondaryGunIndex(part) < 0) return false;

    constexpr float ArmorBlack[] = {0.060f, 0.065f, 0.075f, 1.0f};
    constexpr float HighlightBlack[] = {0.10f, 0.10f, 0.12f, 1.0f};
    const float yaw = ModelYaw(shooter);
    int pieceNumber = 0;
    auto AddPiece = [&](int shape, const Vector3& local, const Vector3& scale,
        const float color[4]) {
        const Vector3 world = LocalToWorld(shooter, boss, local);
        const float side = pieceNumber++ % 2 == 0 ? -1.0f : 1.0f;
        shooter.SpawnDebrisPiece(world.x, world.y, world.z,
            side * 0.035f, 0.028f + side * 0.006f, -0.025f,
            yaw, 0.08f + side * 0.03f,
            shape, scale.x * Stage4BossScale, scale.y * Stage4BossScale,
            scale.z * Stage4BossScale, color);
    };

    // 破壊された砲の主要プリミティブだけを飛散させる
    if (part == BossNose) {
        AddPiece(2, {-4.85f, 3.55f, 0.0f}, {2.35f, 0.92f, 0.92f}, HighlightBlack);
        AddPiece(2, {-6.80f, 3.55f, 0.0f}, {1.65f, 0.76f, 0.76f}, ArmorBlack);
        AddPiece(2, {-7.78f, 3.55f, 0.0f}, {0.42f, 1.16f, 1.16f}, ArmorBlack);
        return true;
    }

    const int gun = SecondaryGunIndex(part);
    const Vector3& local = Stage4SecondaryGunLocal[gun];
    AddPiece(1, {local.x + 0.55f, local.y, local.z}, {0.88f, 0.42f, 0.68f}, ArmorBlack);
    AddPiece(2, local, {1.25f, 0.24f, 0.24f}, HighlightBlack);
    return true;
}

bool SideScrollingShooter::Stage4Module::DrawSpecialShot(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Shot& shot, float yaw) {
    (void)shooter;
    if (!shot.enemy || shot.stage4.kind != ShotKind::Cannonball) return false;

    // 主砲弾は通常敵弾より大きな鉄球として描画する
    constexpr float ShellColor[] = {0.16f, 0.16f, 0.17f, 1.0f};
    constexpr float HotCoreColor[] = {0.85f, 0.18f, 0.035f, 1.0f};
    const float radius = shot.hitRadius * WorldXScale;
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        radius, radius, radius, ShellColor, yaw + shot.age * 0.12f);
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z - 0.02f,
        radius * 0.36f, radius * 0.36f, radius * 0.36f, HotCoreColor,
        yaw + shot.age * 0.18f);
    return true;
}

float SideScrollingShooter::Stage4Module::ModelYaw(const SideScrollingShooter& shooter) {
    return (shooter.IsRailGameplayActive() ? 0.0f : Math::HalfPi) + Stage4ModelYawOffset;
}

Vector3 SideScrollingShooter::Stage4Module::LocalToWorld(
    const SideScrollingShooter& shooter, const Enemy& boss, const Vector3& local) {
    const float yaw = ModelYaw(shooter);
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    return {
        ToWorldX(boss.x) + (local.x * cosine + local.z * sine) * Stage4BossScale,
        ToWorldY(boss.y) + local.y * Stage4BossScale,
        boss.z + (-local.x * sine + local.z * cosine) * Stage4BossScale
    };
}

int SideScrollingShooter::Stage4Module::SecondaryGunIndex(BossPart part) {
    const int index = static_cast<int>(part) - BossFunnelHatch0;
    return index >= 0 && index < 6 ? index : -1;
}

Vector3 SideScrollingShooter::Stage4Module::BossPartLocalPosition(BossPart part) {
    const int secondaryGun = SecondaryGunIndex(part);
    if (secondaryGun >= 0) return Stage4SecondaryGunLocal[secondaryGun];
    return Stage4MainCannonLocal;
}

void SideScrollingShooter::Stage4Module::SpawnMainCannonball(
    SideScrollingShooter& shooter, const Enemy& boss) {
    // 現在装着中の主砲Transformと砲口APIから射出位置を決める
    const Stage4Weapon logicalWeapon = shooter.m_stage4.currentWeapon;
    const Stage4MainWeaponType weaponType = ViewWeaponType(logicalWeapon);
    const Vector3 aimTarget {
        ToWorldX(boss.turretAimX),
        ToWorldY(boss.turretAimY),
        shooter.IsRailGameplayActive() ? boss.turretAimZ : boss.z
    };
    Vector3 muzzle;
    Stage4MainWeaponPose weaponPose;
    if (logicalWeapon == Stage4Weapon::Phase1Cannon) {
        const Vector3 pivot = LocalToWorld(shooter, boss, Stage4MainCannonPivotLocal);
        const Vector3 aimDelta = aimTarget - pivot;
        const float aimLength = (std::max)(0.001f, std::sqrt(
            aimDelta.x * aimDelta.x + aimDelta.y * aimDelta.y + aimDelta.z * aimDelta.z));
        muzzle = pivot + aimDelta / aimLength * Stage4CannonMuzzleDistance;
    } else {
        BossModelTransform tankTransform;
        tankTransform.position = {ToWorldX(boss.x), ToWorldY(boss.y), boss.z};
        tankTransform.yaw = ModelYaw(shooter);
        tankTransform.scale = Stage4BossScale;
        const BossModelTransform weaponTransform =
            Stage4BossModelView::MainWeaponMount(tankTransform);
        weaponPose =
            WeaponPose(shooter.m_stage4, logicalWeapon, shooter.IsRailGameplayActive());
        muzzle = Stage4BossModelView::MainWeaponPointWorldPosition(weaponTransform, weaponPose,
            Stage4BossModelView::MainWeaponMuzzleLocalPosition(weaponType, weaponPose));
    }
    if (logicalWeapon == Stage4Weapon::SiegeMortar) {
        SpawnSiegeMortarBarrage(shooter, boss, muzzle, weaponPose);
        ChooseNextSiegeMortarAim(shooter);
        return;
    }

    const Vector3 delta = aimTarget - muzzle;
    const float length = (std::max)(0.001f,
        std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z));
    const Vector3 direction = delta / length;
    SpawnCannonballShot(shooter, muzzle, direction * Stage4CannonballSpeed,
        Stage4CannonballSideRadius, 0.55f, false, true, 2);
}

void SideScrollingShooter::Stage4Module::SpawnSiegeMortarBarrage(
    SideScrollingShooter& shooter, const Enemy& boss,
    const Vector3& muzzle, const Stage4MainWeaponPose& pose) {
    (void)boss;
    constexpr float Spread[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
    const bool railMode = shooter.IsRailGameplayActive();
    const Stage4SiegeMortarConfig config = SiegeMortarConfig(railMode);
    const float pitch = (std::clamp)(pose.barrelPitch, config.minPitch, config.maxPitch);
    const float yaw = railMode ? (std::clamp)(pose.localYaw, config.minYaw, config.maxYaw) : 0.0f;
    const auto launchArc = [&](float targetOffset, float speed) {
        const float spreadYaw = railMode ? targetOffset * Math::Pi * 4.0f / 180.0f : 0.0f;
        const float spreadPitch = railMode ? 0.0f : targetOffset * Math::Pi * 2.0f / 180.0f;
        Vector3 velocity = SiegeMortarVelocity(ModelYaw(shooter), yaw + spreadYaw,
            (std::clamp)(pitch + spreadPitch, config.minPitch, config.maxPitch), speed);
        if (!railMode) velocity.z = 0.0f;
        return velocity;
    };

    // 中央の特大砲丸を先に生成し、周囲へPhase1砲丸サイズを散らす
    SpawnCannonballShot(shooter, muzzle,
        launchArc(0.0f, config.giantSpeed),
        Stage4GiantCannonballSideRadius, Stage4GiantExplosionRadius, true, true, 3);
    for (float spread : Spread) {
        SpawnCannonballShot(shooter, muzzle,
            launchArc(spread, config.normalSpeed + std::abs(spread) * 0.025f),
            Stage4CannonballSideRadius, 0.55f, true, true, 2);
    }
}

void SideScrollingShooter::Stage4Module::ChooseNextSiegeMortarAim(
    SideScrollingShooter& shooter) {
    const bool railMode = shooter.IsRailGameplayActive();
    const Stage4SiegeMortarConfig config = SiegeMortarConfig(railMode);
    shooter.m_stage4.siegeMortarTargetPitch = GameplayRandom::Range(
        config.minPitch, config.maxPitch);
    shooter.m_stage4.siegeMortarTargetYaw = railMode ?
        GameplayRandom::Range(config.minYaw, config.maxYaw) : 0.0f;
}

bool SideScrollingShooter::Stage4Module::SpawnCannonballShot(
    SideScrollingShooter& shooter, const Vector3& muzzle, const Vector3& velocity,
    float sideRadius, float explosionRadius,
    bool gravity, bool detonateAtPlayerZ, int damage) {
    for (auto& shot : shooter.m_shots) {
        if (shot.active) continue;
        shot = {};
        shot.x = FromWorldX(muzzle.x);
        shot.y = FromWorldY(muzzle.y);
        shot.z = muzzle.z;
        shot.transitionSideX = shot.x;
        shot.transitionSideY = shot.y;
        shot.vx = FromWorldX(velocity.x);
        shot.vy = FromWorldY(velocity.y);
        shot.vz = shooter.IsRailGameplayActive() ? velocity.z : 0.0f;
        shot.hitRadius = sideRadius;
        shot.damage = damage;
        shot.enemy = true;
        shot.stage4.kind = ShotKind::Cannonball;
        shot.stage4.gravity = gravity;
        shot.stage4.detonateAtPlayerZ = detonateAtPlayerZ;
        shot.stage4.explosionRadius = explosionRadius;
        shot.active = true;
        return true;
    }
    return false;
}
