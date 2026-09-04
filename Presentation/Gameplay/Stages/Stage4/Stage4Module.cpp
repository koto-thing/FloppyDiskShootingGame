#include "Stage4Module.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../../../../Engine/Graphics/Renderer.h"
#include "../../../../Engine/Input/Input.h"
#include "../../../../Engine/Input/KeyCode.h"
#include "../../../../Infrastructure/ExternalServices/AudioService.h"

#include "Stage4BossModelView.h"
#include "Stage4EnemySheet.h"
#include "Stage4EnemySheetEasy.h"
#include "Stage4EnemySheetHard.h"
#include "Stage4EnemySheetNormal.h"
#include "Stage4WeaponDroneView.h"
#include "../Common/CityBackgroundModule.h"
#include "../../GameplayRandom.h"
#include "../../Voices/VoiceDpcmDecoder.h"

using ShooterStages::Stage4::ShotKind;
using ShooterStages::Stage4::CrossedRangeEdge;
using Stage4Logic = ShooterStages::Stage4::State;
using Stage4BossPhase = ShooterStages::Stage4::BossPhase;
using Stage4Weapon = ShooterStages::Stage4::MainWeaponType;
using Stage4WeaponVisual = ShooterStages::Stage4::WeaponVisualState;
using Stage4SwapState = ShooterStages::Stage4::WeaponSwapState;
using Stage4SwapConfig = ShooterStages::Stage4::WeaponSwapConfig;

namespace {

// Boss4の攻撃の調整値
constexpr float Stage4BossScale = 1.00f;
constexpr float Stage4ModelYawOffset = -Math::HalfPi;
constexpr float Stage4TrackWheelRadius = 0.74f;
constexpr float Stage4CannonballSpeed = 0.7f;
constexpr float Stage4CannonballGravity = 0.00135f;
constexpr float Stage4SiegeMortarPitchRate = Math::Pi * 1.4f / 180.0f;
constexpr float Stage4SiegeMortarYawRate = Math::Pi * 1.8f / 180.0f;
constexpr float Stage4CannonMuzzleDistance = 4.47f;
constexpr float Stage4CannonballSideRadius = 0.075f;
constexpr float Stage4GiantCannonballSideRadius = 0.150f;
constexpr float Stage4GiantExplosionRadius = 0.65f;
constexpr float Stage4RomanceCannonballSpeed = 1.00f;
constexpr float Stage4RomanceCannonballSideRadius = 0.320f;
constexpr float Stage4RomanceSideExplosionRadius = 1.85f;
constexpr float Stage4RomanceRailExplosionRadius = 1.15f;
constexpr float Stage4RomanceSideExplosionX = -1.5f;
constexpr float Stage4RomanceCannonballGravityScale = 0.95f;
constexpr float Stage4RailGroundGameY = -0.829545f;
constexpr float Stage4BodyHitRadius = 3.35f;
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
constexpr float Stage4SecondaryShotSpeed = 0.19f;
constexpr float Stage4SecondaryMissileLaunchVelocity = 0.09f;
constexpr int Stage4AimedBurstCount = 5;
constexpr int Stage4SpreadShotCount = 8;
constexpr int Stage4SpreadAttackInterval = 240;
constexpr int Stage4SpreadAttackStartFrame = 40;
constexpr int Stage4SpreadShotInterval = 5;
constexpr int Stage4AimedAttackInterval = 180;
constexpr int Stage4AimedAttackStartFrame = 100;
constexpr int Stage4AimedShotInterval = 6;
constexpr int Stage4WeaponSwapReturnFrames = 42;
constexpr int Stage4BossDefeatSequenceFrames = 360;
constexpr int Stage4BossDefeatChargeStartFrame = 72;
constexpr int Stage4BossDefeatBackfireFrame = 150;
constexpr int Stage4BossDefeatExplosionFrames = 48;

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

/** @brief Stage3機銃と同じ決定的な散射量を取得する @param seed 散射Seed @return -1から1の散射量 */
constexpr float SecondaryGunSpread(std::uint32_t seed) {
    seed ^= seed >> 16;
    seed *= 0x7FEB352Du;
    seed ^= seed >> 15;
    return static_cast<float>(seed & 1023u) / 511.5f - 1.0f;
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

/** @brief Phase1主砲の自機追尾方向を可動仰角内に丸める @param delta 支点から目標への差分 @param baseYaw 車体正面Yaw @param tracksYaw Yaw追尾する場合true @return 砲身方向 */
Vector3 Phase1CannonDirection(const Vector3& delta, float baseYaw, bool tracksYaw) {
    const float forwardX = -std::cos(baseYaw);
    const float forwardZ = std::sin(baseYaw);
    const float horizontal = (std::max)(0.001f, tracksYaw ?
        std::sqrt(delta.x * delta.x + delta.z * delta.z) :
        std::abs(delta.x * forwardX + delta.z * forwardZ));
    const float length = (std::max)(0.001f,
        std::sqrt(horizontal * horizontal + delta.y * delta.y));
    const float yaw = tracksYaw ? std::atan2(delta.z, -delta.x) : baseYaw;
    const float elevation = (std::clamp)(std::asin(delta.y / length),
        Stage4BossModelView::Phase1CannonMinElevation,
        Stage4BossModelView::Phase1CannonMaxElevation);
    const float pitchCosine = std::cos(elevation);
    return {
        -std::cos(yaw) * pitchCosine,
        std::sin(elevation),
        std::sin(yaw) * pitchCosine
    };
}

/** @brief Phase1主砲の発射速度を取得する @param direction 砲身方向 @param railMode 3D表示中か @return 発射速度 */
Vector3 Phase1CannonVelocity(const Vector3& direction, bool railMode) {
    if (!railMode || std::abs(direction.z) <= 0.0001f) {
        return direction * Stage4CannonballSpeed;
    }

    // 3D中は奥行き方向の進行速度を通常主砲の基準速度に合わせる
    return direction * (Stage4CannonballSpeed / std::abs(direction.z));
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
static_assert(Stage4AimedBurstCount == 5);
static_assert(Stage4SpreadAttackStartFrame +
    (Stage4SpreadShotCount - 1) * Stage4SpreadShotInterval < Stage4SpreadAttackInterval);
static_assert(Stage4AimedAttackStartFrame +
    (Stage4AimedBurstCount - 1) * Stage4AimedShotInterval < Stage4AimedAttackInterval);
static_assert(SecondaryGunSpread(1u) >= -1.0f && SecondaryGunSpread(1u) <= 1.0f);

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

    // 交換前に車体と主砲姿勢を基準へ戻してから既存交換演出へ進める
    if (state.swapState == Stage4SwapState::ReturnToAnchor) {
        const float progress = SmoothStep(Math::Clamp01(
            static_cast<float>(++state.timer) / static_cast<float>(Stage4WeaponSwapReturnFrames)));
        boss.x = Math::Lerp(state.returnStartX, boss.baseX, progress);
        boss.y = Math::Lerp(state.returnStartY, boss.baseY, progress);
        boss.z = shooter.IsRailGameplayActive() ?
            Math::Lerp(state.returnStartZ, boss.baseZ, progress) : ToRailZFromSideX(boss.x);
        boss.turretAimX = Math::Lerp(state.returnStartAimX, state.returnTargetAimX, progress);
        boss.turretAimY = Math::Lerp(state.returnStartAimY, state.returnTargetAimY, progress);
        boss.turretAimZ = Math::Lerp(state.returnStartAimZ, state.returnTargetAimZ, progress);
        state.siegeMortarPitch = Math::Lerp(state.returnStartSiegeMortarPitch,
            Stage4BossModelView::SiegeMortarDefaultPitch, progress);
        state.siegeMortarTargetPitch = state.siegeMortarPitch;
        state.siegeMortarYaw = Math::Lerp(state.returnStartSiegeMortarYaw, 0.0f, progress);
        state.siegeMortarTargetYaw = state.siegeMortarYaw;
        if (state.timer >= Stage4WeaponSwapReturnFrames) {
            boss.x = boss.baseX;
            boss.y = boss.baseY;
            boss.z = shooter.IsRailGameplayActive() ? boss.baseZ : ToRailZFromSideX(boss.x);
            boss.turretAimX = state.returnTargetAimX;
            boss.turretAimY = state.returnTargetAimY;
            boss.turretAimZ = state.returnTargetAimZ;
            ShooterStages::Stage4::AdvanceWeaponSwap(state);
        }
        return true;
    }

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

void SideScrollingShooter::Stage4Module::TickBossIntroduction(
    SideScrollingShooter& shooter) {
    if (shooter.m_bossIntroductionPhase != BossIntroductionPhase::Entrance ||
        !shooter.m_enemies[0].active) return;

    // 道路奥から車列を押し退けながら既存の戦闘基準位置へ進入する
    Enemy& boss = shooter.m_enemies[0];
    const float rate = SmoothStep(ShooterStages::Stage4::EntranceRate(
        shooter.m_bossIntroductionTimer, ShooterStages::Stage4::BossApproachFrames));
    if (shooter.IsRailGameplayActive()) {
        boss.x = 0.0f;
        boss.y = -0.5f;
        boss.z = Math::Lerp(112.0f, 48.0f, rate);
    } else {
        boss.x = Math::Lerp(3.25f, 1.80f, rate);
        boss.y = -0.80f;
        boss.z = ToRailZFromSideX(boss.x);
    }
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

bool SideScrollingShooter::Stage4Module::HandleBossDefeat(
    SideScrollingShooter& shooter, Enemy& boss) {
    if (!boss.active) return false;

    // 半壊した最終主砲を残し敵弾だけ消して専用撃破演出へ移る
    for (auto& shot : shooter.m_shots) {
        if (shot.enemy) shot.active = false;
    }
    shooter.SpawnExplosion(boss.x - 0.22f, boss.y + 0.20f, boss.z, true);
    shooter.SpawnEnemyDebris(boss, BossNose);
    shooter.UnlockGallery(GalleryEntry::Stage4Boss);
    shooter.UnlockGallery(GalleryEntry::Stage4WeaponDrone);
    shooter.m_bossHp = 0;
    shooter.m_score += 5000;
    shooter.m_clear = true;
    shooter.m_clearTimer = Stage4BossDefeatSequenceFrames;
    PlayDefeatVoice(shooter);
    return true;
}

void SideScrollingShooter::Stage4Module::PlayDefeatVoice(
    SideScrollingShooter& shooter) {
    if (!shooter.m_audio) return;

    // 二種類のBOTAMOCHI撃破音声を一度だけPCMへ復号してランダムに選ぶ
    static const auto botaVoice =
        VoiceCodec::DecodeForAudioService(VoiceSamples::botamochiDeathBota);
    static const auto mochiVoice =
        VoiceCodec::DecodeForAudioService(VoiceSamples::botamochiDeathMochi);
    shooter.m_audio->PlaySE(GameplayRandom::Range(0.0f, 1.0f) < 0.5f ?
        botaVoice : mochiVoice);
}

void SideScrollingShooter::Stage4Module::TickBossDefeat(SideScrollingShooter& shooter) {
    if (!shooter.m_clear || !shooter.m_enemies[0].active) return;
    Enemy& boss = shooter.m_enemies[0];
    const int age = Stage4BossDefeatSequenceFrames - shooter.m_clearTimer;

    // 再点火中は車体を震わせ、砲尾の暴発時に全身を粉砕する
    if (age >= Stage4BossDefeatChargeStartFrame && age < Stage4BossDefeatBackfireFrame) {
        const float strength = Math::Clamp01(static_cast<float>(age -
            Stage4BossDefeatChargeStartFrame) /
            static_cast<float>(Stage4BossDefeatBackfireFrame -
                Stage4BossDefeatChargeStartFrame));
        boss.x = boss.baseX + std::sin(static_cast<float>(age) * 1.7f) * 0.010f * strength;
        boss.y = boss.baseY + std::sin(static_cast<float>(age) * 2.3f) * 0.008f * strength;
        shooter.m_screenShakeIntensity = 0.010f + strength * 0.025f;
        shooter.m_screenShakeFrames = 2;
        shooter.m_screenShakeDurationFrames = 2;
    }
    if (age != Stage4BossDefeatBackfireFrame) return;

    shooter.SpawnExplosion(boss.x, boss.y, boss.z, true);
    for (int burst = 0; burst < 5; ++burst) {
        shooter.SpawnExplosion(boss.x + (burst % 2 == 0 ? -0.18f : 0.18f),
            boss.y + 0.10f - static_cast<float>(burst) * 0.06f,
            boss.z + static_cast<float>(burst - 2) * 1.7f, true);
    }
    shooter.SpawnEnemyDebris(boss);
    shooter.SpawnPowerItem(boss.x, boss.y, boss.z, 1.00f);
    shooter.m_screenShakeIntensity = 0.085f;
    shooter.m_screenShakeFrames = 32;
    shooter.m_screenShakeDurationFrames = 32;
    boss.active = false;
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
    state.swapState = Stage4SwapState::ReturnToAnchor;
    state.timer = 0;
    boss.phase = 0.0f;
    boss.recoilAge = 0;
    boss.recoilType = 0;
    state.returnStartX = boss.x;
    state.returnStartY = boss.y;
    state.returnStartZ = boss.z;
    state.returnStartAimX = boss.turretAimX;
    state.returnStartAimY = boss.turretAimY;
    state.returnStartAimZ = boss.turretAimZ;
    state.returnStartSiegeMortarPitch = state.siegeMortarPitch;
    state.returnStartSiegeMortarYaw = state.siegeMortarYaw;
    state.returnTargetAimX = boss.baseX;
    state.returnTargetAimY = boss.baseY;
    state.returnTargetAimZ = shooter.IsRailGameplayActive() ?
        boss.baseZ : ToRailZFromSideX(boss.baseX);
    if (state.outgoingWeapon == Stage4Weapon::Phase1Cannon) {
        BossModelTransform transform;
        transform.position = {
            ToWorldX(boss.baseX), ToWorldY(boss.baseY),
            shooter.IsRailGameplayActive() ? boss.baseZ : ToRailZFromSideX(boss.baseX)
        };
        transform.yaw = ModelYaw(shooter);
        transform.scale = Stage4BossScale;
        const Vector3 pivot = OffsetTransform(transform, Stage4MainCannonPivotLocal).position;
        const Vector3 aim = pivot + Vector3 {
            -std::cos(transform.yaw), 0.0f, std::sin(transform.yaw)
        } * Stage4CannonMuzzleDistance;
        state.returnTargetAimX = FromWorldX(aim.x);
        state.returnTargetAimY = FromWorldY(aim.y);
        state.returnTargetAimZ = aim.z;
    }

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
        enemy.bossPartHp[MainCannonPart(shooter.m_stage4.currentWeapon)] > 0, true
    };
    const Stage4Logic& swap = shooter.m_stage4;
    const BossPart currentMainCannonPart = MainCannonPart(swap.currentWeapon);
    const Stage4SwapConfig swapConfig = ShooterStages::Stage4::SwapConfig(swap.incomingWeapon);

    // Romance Cannon装着時だけ車体全体を沈ませ重量を受ける動きを作る
    if (swap.currentWeapon == Stage4Weapon::RomanceCannon &&
        swap.swapState == Stage4SwapState::LockWeapon) {
        const float progress = SwapProgress(swap, swapConfig);
        transform.position.y -= std::sin(progress * Math::Pi) * 0.18f;
    }
    Stage4BossModelState state;
    state.mainCannon = false;
    state.mainCannonHit = enemy.bossPartHitFlashFrames[currentMainCannonPart] > 0 &&
        (enemy.bossPartHitFlashFrames[currentMainCannonPart] / 2) % 2 != 0;
    for (int i = 0; i < 6; ++i) {
        const int part = BossFunnelHatch0 + i;
        state.secondaryGuns[i] = enemy.bossPartHp[part] > 0;
        state.secondaryGunsHit[i] = enemy.bossPartHitFlashFrames[part] > 0 &&
            (enemy.bossPartHitFlashFrames[part] / 2) % 2 != 0;
        state.secondaryGunTracksTarget[i] = true;
        if (i >= 4) {
            const Vector3 mount = LocalToWorld(shooter, enemy, Stage4SecondaryGunLocal[i]);
            state.secondaryGunAimTargets[i] = mount + Vector3 {0.0f, 8.0f, 0.0f};
        } else if (i >= 2) {
            const float sweep = std::sin(static_cast<float>(enemy.age) * 0.035f +
                static_cast<float>(i) * Math::Pi) * 3.2f;
            state.secondaryGunAimTargets[i] = aimTarget + Vector3 {0.0f, sweep, 0.0f};
        } else {
            state.secondaryGunAimTargets[i] = aimTarget;
        }
    }
    const bool phase1MainCannonActive = swap.currentWeapon == Stage4Weapon::Phase1Cannon &&
        (swap.swapState == Stage4SwapState::None ||
            swap.swapState == Stage4SwapState::ReturnToAnchor);
    const Vector3 rushAimTarget {
            ToWorldX(swap.rushAimX),
            ToWorldY(swap.rushAimY),
            Math::Lerp(SidePlaneZ, swap.rushAimZ, railWeight)
    };
    const Vector3 rushOffset {
        ToWorldX(enemy.x - enemy.actionX),
        ToWorldY(enemy.y - enemy.actionY),
        enemy.z - enemy.actionZ
    };
    const Vector3 mainAimTarget = phase1MainCannonActive && enemy.phase > 0.0f ?
        rushAimTarget + rushOffset : aimTarget;
    BossModelTransform aimedTransform = transform;
    aimedTransform.aimTarget = mainAimTarget;
    aimedTransform.secondaryAimTarget = aimTarget;
    aimedTransform.secondaryGunsTrackTarget = true;
    aimedTransform.mainGunTracksTarget = phase1MainCannonActive;
    aimedTransform.mainGunTracksYaw = shooter.IsRailGameplayActive();
    aimedTransform.mainGunMinElevation = Stage4BossModelView::Phase1CannonMinElevation;
    aimedTransform.mainGunMaxElevation = Stage4BossModelView::Phase1CannonMaxElevation;
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

    // 撃破後は砲身を失った砲尾が無理に再点火し、暴発直前まで発光する
    if (shooter.m_clear) {
        const int age = Stage4BossDefeatSequenceFrames - shooter.m_clearTimer;
        Stage4MainWeaponPose damagedPose = WeaponPose(
            swap, Stage4Weapon::RomanceCannon, shooter.IsRailGameplayActive());
        if (age >= Stage4BossDefeatChargeStartFrame) {
            const float charge = Math::Clamp01(static_cast<float>(age -
                Stage4BossDefeatChargeStartFrame) /
                static_cast<float>(Stage4BossDefeatBackfireFrame -
                    Stage4BossDefeatChargeStartFrame));
            damagedPose.barrelPitch += std::sin(static_cast<float>(age) * 0.32f) *
                (0.02f + charge * 0.07f);
            const BossModelTransform damagedMount =
                Stage4BossModelView::MainWeaponMount(aimedTransform);
            const Matrix4x4 chargeWorld = Matrix4x4::Translation(
                OffsetTransform(damagedMount, {2.8f, 1.65f, 0.0f}).position) *
                Matrix4x4::Scale({0.65f + charge * 1.65f,
                    0.65f + charge * 1.65f, 1.0f});
            renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * chargeWorld,
                static_cast<float>(age) / 10.0f, 3});
        }
        Stage4BossModelView::DrawDamagedRomanceCannon(
            Stage4BossModelView::MainWeaponMount(aimedTransform), damagedPose,
            DrawBossPart, age < Stage4BossDefeatExplosionFrames && (age / 3) % 2 != 0);
        return true;
    }

    // 装着主砲またはパージ中の旧主砲を独立Transformで描画する
    const BossModelTransform mount = Stage4BossModelView::MainWeaponMount(aimedTransform);
    const BossPart outgoingMainCannonPart = MainCannonPart(swap.outgoingWeapon);
    if (swap.outgoingVisual != Stage4WeaponVisual::Hidden &&
        enemy.bossPartHp[outgoingMainCannonPart] > 0) {
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
            outgoingPose, DrawBossPart,
            enemy.bossPartHitFlashFrames[outgoingMainCannonPart] > 0 &&
            (enemy.bossPartHitFlashFrames[outgoingMainCannonPart] / 2) % 2 != 0);

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
    const BossPart incomingMainCannonPart = MainCannonPart(swap.incomingWeapon);
    if (swap.incomingVisual != Stage4WeaponVisual::Hidden &&
        enemy.bossPartHp[incomingMainCannonPart] > 0) {
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
            incomingType, incomingTransform, incomingPose, DrawBossPart,
            enemy.bossPartHitFlashFrames[incomingMainCannonPart] > 0 &&
            (enemy.bossPartHitFlashFrames[incomingMainCannonPart] / 2) % 2 != 0);

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

    // 主砲交換中は主砲へのダメージを無効化し、副砲だけを破壊可能にする
    const BossPart mainCannonPart = MainCannonPart(shooter.m_stage4.currentWeapon);
    if (!IsWeaponSwapActive(shooter) && boss.bossPartHp[mainCannonPart] > 0) {
        const Vector3 world = LocalToWorld(shooter, boss, BossPartLocalPosition(mainCannonPart));
        const bool hit = shooter.IsRailGameplayActive() ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy),
                shot.z - shot.vz, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, world.x, world.y, world.z,
                Stage4MainCannonHitRadius) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y),
                Stage4MainCannonHitRadius / WorldXScale);
        if (hit) {
            part = mainCannonPart;
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

bool SideScrollingShooter::Stage4Module::BlocksPlayerShot(
    const SideScrollingShooter& shooter, const Shot& shot, const Enemy& boss) {
    if (!IsWeaponSwapActive(shooter) || boss.type != 2) return false;

    // 副砲判定を通過した弾が交換中の車体へ当たった場合はHPを減らさず遮断する
    for (const Vector3& local : Stage4BodyHitLocal) {
        const Vector3 world = LocalToWorld(shooter, boss, local);
        const bool hit = shooter.IsRailGameplayActive() ?
            Hit3DSegment(ToWorldX(shot.x - shot.vx), ToWorldY(shot.y - shot.vy),
                shot.z - shot.vz, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
                shot.hitRadius * WorldXScale, world.x, world.y, world.z,
                Stage4BodyHitRadius) :
            Hit(shot.x, shot.y, shot.hitRadius,
                FromWorldX(world.x), FromWorldY(world.y),
                Stage4BodyHitRadius / WorldXScale);
        if (hit) return true;
    }
    return false;
}

void SideScrollingShooter::Stage4Module::FireBossPartBarrage(
    SideScrollingShooter& shooter, Enemy& boss) {
    if (boss.type != 2 || IsWeaponSwapActive(shooter)) return;

    // 主砲周期では現在装着中の主砲だけを発射する
    const BossPart mainCannonPart = MainCannonPart(shooter.m_stage4.currentWeapon);
    if (boss.bossPartHp[mainCannonPart] > 0) {
        SpawnMainCannonball(shooter, boss);
        boss.recoilType = shooter.m_stage4.currentWeapon == Stage4Weapon::RomanceCannon ? 1 : 0;
        boss.recoilAge = Stage4EnemySheet::MainCannonRecoilFramesForWeapon(
            shooter.m_stage4.currentWeapon);
    }
}

void SideScrollingShooter::Stage4Module::TickSecondaryGunAttacks(
    SideScrollingShooter& shooter, const Enemy& boss) {
    if (boss.type != 2) return;

    // 突進開始フレームも含め、突進中はミサイル以外の副砲を停止する
    const bool rushActive = Stage4EnemySheet::IsRushAttackActive(boss);
    const int spreadFrame = boss.age % Stage4SpreadAttackInterval;
    const int aimedFrame = boss.age % Stage4AimedAttackInterval;
    const int mainAttackInterval = shooter.m_stage->BossAttackInterval(
        static_cast<BossPhase>(boss.bossPhase));
    const bool firesMissile = mainAttackInterval > 0 &&
        boss.age % mainAttackInterval == 0;
    const bool firesSpread = !rushActive &&
        spreadFrame >= Stage4SpreadAttackStartFrame &&
        spreadFrame <= Stage4SpreadAttackStartFrame +
            (Stage4SpreadShotCount - 1) * Stage4SpreadShotInterval &&
        (spreadFrame - Stage4SpreadAttackStartFrame) % Stage4SpreadShotInterval == 0;
    const bool firesAimed = !rushActive &&
        aimedFrame >= Stage4AimedAttackStartFrame &&
        aimedFrame <= Stage4AimedAttackStartFrame +
            (Stage4AimedBurstCount - 1) * Stage4AimedShotInterval &&
        (aimedFrame - Stage4AimedAttackStartFrame) % Stage4AimedShotInterval == 0;
    if (!firesMissile && !firesSpread && !firesAimed) return;

    const Vector3 player {
        ToWorldX(shooter.m_playerX), ToWorldY(shooter.m_playerY),
        shooter.IsRailGameplayActive() ? PlayerRailZ : SidePlaneZ
    };
    auto DirectionTo = [&](const Vector3& source, const Vector3& target) {
        Vector3 delta = target - source;
        if (!shooter.IsRailGameplayActive()) delta.z = 0.0f;
        return delta / (std::max)(0.001f, delta.Length());
    };
    auto SpawnBullet = [&](const Vector3& source, const Vector3& direction,
        float hitRadius, int damage) {
        for (int shotIndex = 0; shotIndex < shooter.ActiveShotCapacity(); ++shotIndex) {
            auto& shot = shooter.m_shots[shotIndex];
            if (shot.active) continue;
            shot = {};
            shot.x = FromWorldX(source.x);
            shot.y = FromWorldY(source.y);
            shot.z = source.z;
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            shot.vx = FromWorldX(direction.x * Stage4SecondaryShotSpeed);
            shot.vy = FromWorldY(direction.y * Stage4SecondaryShotSpeed);
            shot.vz = direction.z * Stage4SecondaryShotSpeed;
            shot.hitRadius = hitRadius;
            shot.damage = damage;
            shot.enemy = true;
            shot.active = true;
            return true;
        }
        return false;
    };

    bool fired = false;
    for (int gun = 0; gun < 6; ++gun) {
        const BossPart part = static_cast<BossPart>(BossFunnelHatch0 + gun);
        if (boss.bossPartHp[part] <= 0) continue;
        const Vector3 source = LocalToWorld(shooter, boss, BossPartLocalPosition(part));

        // 中央二基は独立周期の一回につき自機方向へ一発ずつ撃つ
        if (gun < 2) {
            if (firesAimed) {
                fired |= SpawnBullet(source, DirectionTo(source, player), 0.022f, 1);
            }
            continue;
        }

        // 手前二基はバースト中に一発ずつStage3と同じ散射式でばらまく
        if (gun < 4) {
            if (!firesSpread) continue;
            const Vector3 aimed = DirectionTo(source, player);
            const std::uint32_t seed = static_cast<std::uint32_t>(boss.age) * 17u +
                static_cast<std::uint32_t>(gun) * 131u;
            Vector3 direction = aimed;
            direction.x += SecondaryGunSpread(seed) * 0.62f;
            direction.y += SecondaryGunSpread(seed + 53u) * 0.48f;
            if (shooter.IsRailGameplayActive()) {
                direction.z += SecondaryGunSpread(seed + 101u) * 0.10f;
            }
            direction = direction / (std::max)(0.001f, direction.Length());
            fired |= SpawnBullet(source, direction, 0.022f, 1);
            continue;
        }

        // 後方二基は主砲と同じ周期でStage3と同じ遅延点火追尾ミサイルを撃つ
        if (!firesMissile) continue;
        for (int shotIndex = 0; shotIndex < shooter.ActiveShotCapacity(); ++shotIndex) {
            auto& shot = shooter.m_shots[shotIndex];
            if (shot.active) continue;
            shot = {};
            shot.x = FromWorldX(source.x);
            shot.y = FromWorldY(source.y);
            shot.z = source.z;
            shot.transitionSideX = shot.x;
            shot.transitionSideY = shot.y;
            shot.vy = Stage4SecondaryMissileLaunchVelocity;
            shot.hitRadius = 0.055f;
            shot.damage = 2;
            shot.enemy = true;
            shot.stage2.kind = ShooterStages::Stage2::ShotKind::Funnel;
            shot.stage2.delayedEngine = true;
            shot.active = true;
            shooter.PlayMissileLaunchSound();
            fired = true;
            break;
        }
    }
    if (fired) shooter.PlayEnemyShotSound();
}

bool SideScrollingShooter::Stage4Module::HitsHazard(
    const SideScrollingShooter& shooter, float x, float y, float z, float radius) {
    if (CityBackgroundModule::HitsTruck(shooter, x, y, z, radius)) return true;

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
    if (shot.stage4.gravity) {
        shot.vy -= Stage4CannonballGravity * shot.stage4.gravityScale;
    }
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
        (CrossedRangeEdge(previousZ, shot.z, 0.0f, 72.0f) ||
            CrossedRangeEdge(previousX, shot.x, -1.2f, 1.2f) ||
            (!shot.stage4.gravity &&
                CrossedRangeEdge(previousY, shot.y, -1.24f, 1.24f))) :
        (CrossedRangeEdge(previousX, shot.x, Side2DPlayerMinX, Side2DPlayerMaxX) ||
            (!shot.stage4.gravity &&
                CrossedRangeEdge(previousY, shot.y, groundY, Side2DPlayerMaxY)));
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
        !shooter.IsRailGameplayActive() && shot.stage4.fixedSideExplosionX ?
            shot.stage4.sideExplosionX : shot.x,
        hitGround && !impactAtPlayerZ ? groundY : shot.y,
        impactZ, shot.stage4.explosionRadius);
    shot.active = false;
}

bool SideScrollingShooter::Stage4Module::IsShotCullProtected(const Shot& shot) {
    return shot.stage4.kind == ShotKind::Cannonball ||
        (shot.stage2.kind == ShooterStages::Stage2::ShotKind::Funnel &&
            shot.stage2.delayedEngine);
}

float SideScrollingShooter::Stage4Module::EnemyShotHitRadius(
    const Shot& shot, bool railMode) {
    if (shot.stage4.kind != ShotKind::Cannonball) {
        if (shot.stage2.kind == ShooterStages::Stage2::ShotKind::Funnel) {
            return railMode ? 0.42f : 0.055f;
        }
        return railMode ? 0.28f : 0.022f;
    }
    return railMode ? shot.hitRadius * WorldXScale : shot.hitRadius;
}

bool SideScrollingShooter::Stage4Module::SpawnBossDebris(
    SideScrollingShooter& shooter, const Enemy& boss, int bossPart) {
    if (boss.type != 2) return false;
    const BossPart part = static_cast<BossPart>(bossPart);
    if (bossPart >= 0 && !IsMainCannonPart(part) && SecondaryGunIndex(part) < 0) return false;

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

    // 最終暴発では幅広い車体を小片へ分け、四方へ粉砕された輪郭を作る
    if (bossPart < 0) {
        constexpr Vector3 BodyPieces[] = {
            {-5.5f, 0.1f, -7.0f}, {-5.5f, 0.3f, 0.0f}, {-5.5f, 0.1f, 7.0f},
            {0.0f, 0.5f, -7.2f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.5f, 7.2f},
            {5.5f, 0.2f, -7.0f}, {5.5f, 0.4f, 0.0f}, {5.5f, 0.2f, 7.0f},
            {-1.5f, 3.0f, -4.3f}, {-1.5f, 3.2f, 0.0f}, {-1.5f, 3.0f, 4.3f}
        };
        for (int index = 0; index < 12; ++index) {
            const float narrow = index % 3 == 1 ? 1.9f : 2.6f;
            AddPiece(index % 4 == 0 ? 3 : 1, BodyPieces[index],
                {narrow, 0.8f + static_cast<float>(index % 2) * 0.5f, 2.2f},
                index % 3 == 0 ? HighlightBlack : ArmorBlack);
        }
        return true;
    }

    // 破壊された砲の主要プリミティブだけを飛散させる
    if (IsMainCannonPart(part)) {
        if (shooter.m_stage4.currentWeapon == Stage4Weapon::RomanceCannon) {
            AddPiece(2, {-4.2f, 5.0f, 0.0f}, {3.2f, 1.55f, 1.55f}, HighlightBlack);
            AddPiece(2, {-7.0f, 5.0f, 0.0f}, {2.6f, 1.35f, 1.35f}, ArmorBlack);
            AddPiece(2, {-9.5f, 5.0f, 0.0f}, {2.1f, 1.15f, 1.15f}, ArmorBlack);
            AddPiece(2, {-11.2f, 5.0f, 0.0f}, {0.7f, 2.3f, 2.3f}, HighlightBlack);
        } else {
            AddPiece(2, {-4.85f, 3.55f, 0.0f}, {2.35f, 0.92f, 0.92f}, HighlightBlack);
            AddPiece(2, {-6.80f, 3.55f, 0.0f}, {1.65f, 0.76f, 0.76f}, ArmorBlack);
            AddPiece(2, {-7.78f, 3.55f, 0.0f}, {0.42f, 1.16f, 1.16f}, ArmorBlack);
        }
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

    // 主砲弾は通常敵弾より大きな黄金色の砲丸として描画する
    constexpr float ShellColor[] = {0.95f, 0.62f, 0.08f, 1.0f};
    constexpr float HotCoreColor[] = {1.0f, 0.90f, 0.34f, 1.0f};
    const float radius = shot.hitRadius * WorldXScale;
    const Vector3 center {ToWorldX(shot.x), ToWorldY(shot.y), shot.z};

    // 既存の黄金色加算エフェクトを砲弾の背面へ重ねて脈動する光輪を作る
    const float glowScale = radius *
        (1.85f + std::sin(static_cast<float>(shot.age) * 0.16f) * 0.12f);
    const Matrix4x4 glowWorld = Matrix4x4::Translation(center) *
        Matrix4x4::Scale({glowScale, glowScale, 1.0f});
    renderer.DrawExplosion({camera.ProjectionMatrix() * camera.ViewMatrix() * glowWorld,
        0.08f, 0});

    // 発光の手前へ砲弾本体と高温の芯を描画する
    DrawModelPrimitive(renderer, camera, 2, center.x, center.y, center.z,
        radius, radius, radius, ShellColor, yaw + shot.age * 0.12f);
    DrawModelPrimitive(renderer, camera, 2, center.x, center.y, center.z - 0.02f,
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

SideScrollingShooter::BossPart SideScrollingShooter::Stage4Module::MainCannonPart(
    ShooterStages::Stage4::MainWeaponType weapon) {
    switch (weapon) {
    case Stage4Weapon::SiegeMortar: return BossLeftWing;
    case Stage4Weapon::RomanceCannon: return BossRightWing;
    default: return BossNose;
    }
}

bool SideScrollingShooter::Stage4Module::IsMainCannonPart(BossPart part) {
    return part == BossNose || part == BossLeftWing || part == BossRightWing;
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
    Vector3 phase1Direction;
    if (logicalWeapon == Stage4Weapon::Phase1Cannon) {
        const Vector3 pivot = LocalToWorld(shooter, boss, Stage4MainCannonPivotLocal);
        const float yaw = ModelYaw(shooter);
        phase1Direction = Phase1CannonDirection(aimTarget - pivot, yaw,
            shooter.IsRailGameplayActive());
        muzzle = pivot + phase1Direction * Stage4CannonMuzzleDistance;
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
    if (logicalWeapon == Stage4Weapon::RomanceCannon) {
        SpawnRomanceCannonShot(shooter, muzzle, weaponPose);
        return;
    }

    SpawnCannonballShot(shooter, muzzle,
        Phase1CannonVelocity(phase1Direction, shooter.IsRailGameplayActive()),
        Stage4CannonballSideRadius, 0.55f, false, true, 2);
}

void SideScrollingShooter::Stage4Module::SpawnRomanceCannonShot(
    SideScrollingShooter& shooter, const Vector3& muzzle,
    const Stage4MainWeaponPose& pose) {
    const bool railMode = shooter.IsRailGameplayActive();
    Vector3 velocity = SiegeMortarVelocity(ModelYaw(shooter), pose.localYaw,
        pose.barrelPitch, Stage4RomanceCannonballSpeed);
    if (!railMode) velocity.z = 0.0f;

    // 画面縦幅を覆う超巨大爆発として扱う
    const float explosionRadius = railMode ?
        Stage4RomanceRailExplosionRadius : Stage4RomanceSideExplosionRadius;
    SpawnCannonballShot(shooter, muzzle, velocity,
        Stage4RomanceCannonballSideRadius, explosionRadius,
        true, true, 4, true, Stage4RomanceSideExplosionX,
        Stage4RomanceCannonballGravityScale);
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
    bool gravity, bool detonateAtPlayerZ, int damage,
    bool fixedSideExplosionX, float sideExplosionX, float gravityScale) {
    for (int shotIndex = 0; shotIndex < shooter.ActiveShotCapacity(); ++shotIndex) {
        auto& shot = shooter.m_shots[shotIndex];
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
        shot.stage4.fixedSideExplosionX = fixedSideExplosionX;
        shot.stage4.gravityScale = gravityScale;
        shot.stage4.explosionRadius = explosionRadius;
        shot.stage4.sideExplosionX = sideExplosionX;
        shot.active = true;
        return true;
    }
    return false;
}
