#pragma once

namespace ShooterStages::Stage4 {

/** @brief Stage 4ボスの主砲交換を含む戦闘フェーズ */
enum class BossPhase {
    Phase1,
    TransitionToPhase2,
    Phase2,
    TransitionToPhase3,
    Phase3
};

/** @brief 主砲交換中の工程 */
enum class WeaponSwapState {
    None,
    Prepare,
    Unlock,
    Purge,
    WaitingForDrone,
    DroneApproach,
    CarryWeapon,
    AlignWeapon,
    MountWeapon,
    LockWeapon,
    DroneRelease,
    DroneRetreat,
    Complete
};

/** @brief Stage 4ボスの主砲種別 */
enum class MainWeaponType {
    Phase1Cannon,
    SiegeMortar,
    RomanceCannon
};

/** @brief 交換中の主砲描画所有権 */
enum class WeaponVisualState {
    Hidden,
    Detached,
    Carried,
    Attached
};

/** @brief 一回の主砲交換に使用する演出時間と編隊数 */
struct WeaponSwapConfig {
    int droneCount = 3;
    int prepareFrames = 36;
    int unlockFrames = 24;
    int purgeFrames = 40;
    int waitingFrames = 20;
    int approachFrames = 45;
    int carryFrames = 36;
    int alignFrames = 40;
    int mountFrames = 42;
    int lockFrames = 24;
    int releaseFrames = 18;
    int retreatFrames = 52;

    /** @brief 交換開始から完了までの総フレーム数を取得する @return 総フレーム数 */
    constexpr int TotalFrames() const {
        return prepareFrames + unlockFrames + purgeFrames + waitingFrames +
            approachFrames + carryFrames + alignFrames + mountFrames +
            lockFrames + releaseFrames + retreatFrames;
    }
};

/**
 * @brief 搬入主砲に対応する交換設定を取得する
 * @param incomingWeapon 搬入する主砲
 * @return 演出設定
 */
constexpr WeaponSwapConfig SwapConfig(MainWeaponType incomingWeapon) {
    if (incomingWeapon != MainWeaponType::RomanceCannon) return {};
    return {4, 42, 28, 44, 24, 58, 48, 64, 68, 30, 20, 60};
}

/**
 * @brief HPからStage 4固有の三段階攻撃フェーズを取得する
 * @param hp 現在HP
 * @param maxHp 最大HP
 * @return 0から2の攻撃フェーズ番号
 */
constexpr int BossPhaseForHp(int hp, int maxHp) {
    if (maxHp <= 0 || hp * 3 > maxHp * 2) return 0;
    return hp * 3 > maxHp ? 1 : 2;
}

/** @brief Stage 4ボスの主砲交換状態 */
struct State {
    BossPhase phase = BossPhase::Phase1;
    WeaponSwapState swapState = WeaponSwapState::None;
    MainWeaponType currentWeapon = MainWeaponType::Phase1Cannon;
    MainWeaponType outgoingWeapon = MainWeaponType::Phase1Cannon;
    MainWeaponType incomingWeapon = MainWeaponType::Phase1Cannon;
    WeaponVisualState outgoingVisual = WeaponVisualState::Attached;
    WeaponVisualState incomingVisual = WeaponVisualState::Hidden;
    int timer = 0;
    float siegeMortarPitch = 0.91629785f;
    float siegeMortarTargetPitch = 0.91629785f;
    float siegeMortarYaw = 0.0f;
    float siegeMortarTargetYaw = 0.0f;
};

/**
 * @brief 交換状態を次工程へ進めて主砲描画所有権を更新する
 * @param state 更新するStage 4状態
 * @return なし
 */
constexpr void AdvanceWeaponSwap(State& state) {
    state.timer = 0;
    switch (state.swapState) {
    case WeaponSwapState::Prepare:
        state.swapState = WeaponSwapState::Unlock;
        break;
    case WeaponSwapState::Unlock:
        state.outgoingVisual = WeaponVisualState::Detached;
        state.swapState = WeaponSwapState::Purge;
        break;
    case WeaponSwapState::Purge:
        state.outgoingVisual = WeaponVisualState::Hidden;
        state.swapState = WeaponSwapState::WaitingForDrone;
        break;
    case WeaponSwapState::WaitingForDrone:
        state.incomingVisual = WeaponVisualState::Carried;
        state.swapState = WeaponSwapState::DroneApproach;
        break;
    case WeaponSwapState::DroneApproach:
        state.swapState = WeaponSwapState::CarryWeapon;
        break;
    case WeaponSwapState::CarryWeapon:
        state.swapState = WeaponSwapState::AlignWeapon;
        break;
    case WeaponSwapState::AlignWeapon:
        state.swapState = WeaponSwapState::MountWeapon;
        break;
    case WeaponSwapState::MountWeapon:
        state.currentWeapon = state.incomingWeapon;
        state.incomingVisual = WeaponVisualState::Attached;
        state.swapState = WeaponSwapState::LockWeapon;
        break;
    case WeaponSwapState::LockWeapon:
        state.swapState = WeaponSwapState::DroneRelease;
        break;
    case WeaponSwapState::DroneRelease:
        state.swapState = WeaponSwapState::DroneRetreat;
        break;
    case WeaponSwapState::DroneRetreat:
        state.swapState = WeaponSwapState::Complete;
        break;
    case WeaponSwapState::Complete:
        state.phase = state.incomingWeapon == MainWeaponType::SiegeMortar ?
            BossPhase::Phase2 : BossPhase::Phase3;
        state.outgoingVisual = WeaponVisualState::Hidden;
        state.incomingVisual = WeaponVisualState::Attached;
        state.swapState = WeaponSwapState::None;
        break;
    default:
        break;
    }
}

static_assert(SwapConfig(MainWeaponType::SiegeMortar).TotalFrames() == 377);
static_assert(SwapConfig(MainWeaponType::RomanceCannon).TotalFrames() == 486);
static_assert(BossPhaseForHp(496, 496) == 0);
static_assert(BossPhaseForHp(330, 496) == 1);
static_assert(BossPhaseForHp(165, 496) == 2);

/** @brief Stage 4特殊弾の種類 */
enum class ShotKind {
    None,
    Cannonball
};

/** @brief Stage 4特殊弾の状態 */
struct ShotState {
    ShotKind kind = ShotKind::None;
    bool gravity = false;
    bool detonateAtPlayerZ = true;
    float explosionRadius = 0.55f;
};

}
