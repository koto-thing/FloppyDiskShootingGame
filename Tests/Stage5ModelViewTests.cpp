#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "../Presentation/Gameplay/SideScrollingShooter.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5ModelView.h"
#include "../Presentation/Gameplay/Stages/Stage5/Stage5CityModelView.h"
#include "../Presentation/Gameplay/Stages/Stage5/WallSecurityDroneModelView.h"

/**
 * @brief Stage 5の状態順序と共有モデルTransformを検証する
 * @return なし
 */
void RunStage5ModelViewTests() {
    using Phase = SideScrollingShooter::Stage5Phase;
    using Weakpoint = SideScrollingShooter::TayamaWeakpoint;
    static_assert(TayamaPartGroupCount == ShooterStages::Stage5::TayamaCollisionGroupCount);
    assert(ShooterStages::Stage5::State {}.tayamaCollisionBoundsFrame == -1);

    // 反射ファンネルは第2形態開始5秒後から5秒間隔で3基を射出する
    static_assert(ShooterStages::Stage5::TayamaReflectFunnelCount == 3);
    static_assert(!ShooterStages::Stage5::IsTayamaReflectFunnelLaunchFrame(0));
    static_assert(!ShooterStages::Stage5::IsTayamaReflectFunnelLaunchFrame(
        ShooterStages::Stage5::TayamaReflectFunnelLaunchIntervalFrames - 1));
    static_assert(ShooterStages::Stage5::IsTayamaReflectFunnelLaunchFrame(
        ShooterStages::Stage5::TayamaReflectFunnelLaunchIntervalFrames));

    // 第2形態の突進は予告後だけ接触判定を持ち、復帰完了後に通常位置へ戻る
    constexpr int rushStart = ShooterStages::Stage5::TayamaDragonRushStartFrame;
    constexpr int rushActive = rushStart +
        ShooterStages::Stage5::TayamaDragonRushWarningFrames;
    constexpr int rushRecoveryEnd = rushActive +
        ShooterStages::Stage5::TayamaDragonRushActiveFrames +
        ShooterStages::Stage5::TayamaDragonRushRecoveryFrames;
    assert(ShooterStages::Stage5::TayamaDragonRushProgress(rushStart + 30) < 0.0f);
    assert(ShooterStages::Stage5::IsTayamaDragonRushActive(rushActive));
    assert(ShooterStages::Stage5::TayamaDragonRushProgress(rushActive +
        ShooterStages::Stage5::TayamaDragonRushActiveFrames - 1) == 1.0f);
    assert(ShooterStages::Stage5::TayamaDragonRushProgress(rushRecoveryEnd) == 0.0f);
    const float rushWindupStep = std::abs(
        ShooterStages::Stage5::TayamaDragonRushProgress(rushActive) -
        ShooterStages::Stage5::TayamaDragonRushProgress(rushActive - 1));
    const float rushRecoveryStep = std::abs(
        ShooterStages::Stage5::TayamaDragonRushProgress(rushRecoveryEnd) -
        ShooterStages::Stage5::TayamaDragonRushProgress(rushRecoveryEnd - 1));
    assert(rushWindupStep < 0.001f);
    assert(rushRecoveryStep < 0.001f);

    // 薙ぎ払い、胴体弾幕、レーザーは開始と終了で補間値を連続させる
    assert(ShooterStages::Stage5::TayamaDragonSweepProgress(0) == 0.0f);
    assert(ShooterStages::Stage5::TayamaDragonSweepProgress(
        ShooterStages::Stage5::TayamaDragonSweepWarningFrames) == 1.0f);
    assert(ShooterStages::Stage5::TayamaDragonSweepProgress(
        ShooterStages::Stage5::TayamaDragonSweepWarningFrames +
        ShooterStages::Stage5::TayamaDragonSweepActiveFrames +
        ShooterStages::Stage5::TayamaDragonSweepRecoveryFrames) == 0.0f);
    assert(ShooterStages::Stage5::TayamaDragonBarrageGlow(
        ShooterStages::Stage5::TayamaDragonBarrageFireFrame) == 1.0f);
    assert(ShooterStages::Stage5::TayamaDragonLaserWidthProgress(
        ShooterStages::Stage5::TayamaHeadLaserWarningFrames) == 0.0f);
    assert(ShooterStages::Stage5::TayamaDragonLaserWidthProgress(
        ShooterStages::Stage5::TayamaHeadLaserWarningFrames +
        ShooterStages::Stage5::TayamaHeadLaserFadeFrames) == 1.0f);

    // 旋回は突進後の専用区間だけ有効になり、一周して終了する
    constexpr int orbitStart = rushStart +
        ShooterStages::Stage5::TayamaDragonOrbitStartFrame;
    assert(ShooterStages::Stage5::TayamaDragonOrbitFrame(orbitStart) == 0);
    assert(ShooterStages::Stage5::IsTayamaDragonOrbitActive(orbitStart));
    assert(std::fabs(ShooterStages::Stage5::TayamaDragonOrbitAngle(orbitStart +
        ShooterStages::Stage5::TayamaDragonOrbitFrames / 2) - Math::Pi) < 0.0001f);
    assert(!ShooterStages::Stage5::IsTayamaDragonOrbitActive(orbitStart +
        ShooterStages::Stage5::TayamaDragonOrbitFrames));
    constexpr int romanceStart = ShooterStages::Stage5::TayamaDragonRomanceCannonIntervalFrames;
    static_assert(ShooterStages::Stage5::TayamaDragonRomanceCannonFrame(
        romanceStart - 1) == -1);
    static_assert(ShooterStages::Stage5::TayamaDragonRomanceCannonFrame(romanceStart) == 0);
    static_assert(ShooterStages::Stage5::TayamaDragonHeadSeparationRate(
        romanceStart + ShooterStages::Stage5::TayamaDragonRomanceCannonMoveFrames) == 1.0f);

    // 正規遷移を許可し、TAYAMA攻略順の飛び越しを拒否する
    assert(SideScrollingShooter::IsValidStage5Transition(
        Phase::EastsourceBattle, Phase::EastsourceFall));
    assert(SideScrollingShooter::IsValidStage5Transition(
        Phase::TayamaCommandCore, Phase::TayamaCollapse));
    assert(!SideScrollingShooter::IsValidStage5Transition(
        Phase::TayamaFireControl, Phase::TayamaCommandCore));
    assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
        Weakpoint::FireControlRadar, Phase::TayamaFireControl));
    assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
        Weakpoint::CommandCore, Phase::TayamaFireControl));

    // 正面、右側、背面の周回座標が同じ半径でTAYAMAを囲むことを確認する
    const Vector2 front = SideScrollingShooter::TayamaOrbitXZ(0.0f, 0.0f);
    const Vector2 right = SideScrollingShooter::TayamaOrbitXZ(Math::HalfPi, 0.0f);
    const Vector2 back = SideScrollingShooter::TayamaOrbitXZ(Math::Pi, 0.0f);
    assert(std::abs(front.x) < 0.0001f);
    assert(std::abs(front.y - (ShooterStages::Stage5::TayamaArenaCenterZ -
        ShooterStages::Stage5::TayamaOrbitRadius)) < 0.0001f);
    assert(std::abs(right.x - ShooterStages::Stage5::TayamaOrbitRadius) < 0.0001f);
    assert(std::abs(right.y - ShooterStages::Stage5::TayamaArenaCenterZ) < 0.0001f);
    assert(std::abs(back.x) < 0.0001f);
    assert(std::abs(back.y - (ShooterStages::Stage5::TayamaArenaCenterZ +
        ShooterStages::Stage5::TayamaOrbitRadius)) < 0.0001f);
    assert(ShooterStages::Stage5::IsTayamaBattlePhase(Phase::TayamaFireControl));
    assert(ShooterStages::Stage5::IsTayamaBattlePhase(Phase::TayamaLiftEngines));
    assert(ShooterStages::Stage5::IsTayamaBattlePhase(Phase::TayamaCommandCore));
    assert(!ShooterStages::Stage5::IsTayamaBattlePhase(Phase::CarrierTransformation));
    assert(ShooterStages::Stage5::ShouldResetTayamaPlayer(
        Phase::TayamaFireControl, false));
    assert(ShooterStages::Stage5::ShouldResetTayamaPlayer(
        Phase::TayamaLiftEngines, true));
    assert(ShooterStages::Stage5::ShouldResetTayamaPlayer(
        Phase::TayamaCommandCore, true));
    assert(!ShooterStages::Stage5::ShouldResetTayamaPlayer(
        Phase::TayamaLiftEngines, false));
    assert(ShooterStages::Stage5::TayamaCameraFarClip >
        ShooterStages::Stage5::TayamaOrbitRadius * 2.0f);
    assert(ShooterStages::Stage5::IsTayamaStompRange(
        ShooterStages::Stage5::TayamaStompTriggerMaxWorldY));
    assert(!ShooterStages::Stage5::IsTayamaStompRange(
        ShooterStages::Stage5::TayamaStompTriggerMaxWorldY + 0.01f));
    assert(ShooterStages::Stage5::TayamaStompLiftOffset(
        ShooterStages::Stage5::TayamaStompRaiseFrames) ==
        ShooterStages::Stage5::TayamaStompLiftLocalY);
    assert(ShooterStages::Stage5::TayamaStompLiftOffset(
        ShooterStages::Stage5::TayamaStompImpactFrame) == 0.0f);

    // 第一形態の初期周回位置をTAYAMAの両目中央の正面へ揃える
    const Stage5ModelTransform battleTransform {
        {0.0f, TayamaModelView::GroundedRootY(
            ShooterStages::Stage5::RooftopSurfaceY,
            ShooterStages::Stage5::TayamaBossScale),
            ShooterStages::Stage5::TayamaArenaCenterZ},
        {}, ShooterStages::Stage5::TayamaBossScale
    };
    const Vector3 battleEye = TayamaModelView::EyeWorldCenter(battleTransform);
    const Vector3 battleStart {front.x, battleEye.y, front.y};
    const Vector3 battleBack {back.x, battleEye.y, back.y};
    const Vector3 battleRight {right.x, battleEye.y, right.y};
    assert(std::abs(battleStart.y - battleEye.y) < 0.0001f);
    assert(TayamaModelView::IsInFrontOfHead(battleTransform, battleStart, 0.9999f));
    assert(TayamaModelView::IsBehindHead(battleTransform, battleBack, 0.9999f));
    assert(!TayamaModelView::IsBehindHead(battleTransform, battleStart, 0.0f));
    assert(!TayamaModelView::IsBehindHead(battleTransform, battleRight, 0.1f));

    // 踏みつけ中は選択した脚部だけが上がり、反対側は接地位置を保つ
    TayamaModelState groundedState;
    TayamaModelState raisedState;
    raisedState.collapseOffsets[static_cast<std::size_t>(
        TayamaPartGroup::LeftLiftEngine)].position.y =
        ShooterStages::Stage5::TayamaStompLiftOffset(
            ShooterStages::Stage5::TayamaStompRaiseFrames);
    const Stage5GroupBounds groundedLeft = TayamaModelView::GroupBounds(
        battleTransform, 1.0f, groundedState, TayamaPartGroup::LeftLiftEngine);
    const Stage5GroupBounds raisedLeft = TayamaModelView::GroupBounds(
        battleTransform, 1.0f, raisedState, TayamaPartGroup::LeftLiftEngine);
    const Stage5GroupBounds groundedRight = TayamaModelView::GroupBounds(
        battleTransform, 1.0f, groundedState, TayamaPartGroup::RightLiftEngine);
    const Stage5GroupBounds raisedRight = TayamaModelView::GroupBounds(
        battleTransform, 1.0f, raisedState, TayamaPartGroup::RightLiftEngine);
    assert(raisedLeft.center.y > groundedLeft.center.y + 20.0f);
    assert(std::abs(raisedRight.center.y - groundedRight.center.y) < 0.0001f);

    // 発射元の内側接触だけを除外し、外へ出た後は同じ部位も判定する
    std::uint16_t ignoredBossGroups = 1;
    assert(!ShooterStages::Stage5::CanBossShotHitGroup(
        ignoredBossGroups, 1, true));
    assert(ignoredBossGroups == 1);
    assert(!ShooterStages::Stage5::CanBossShotHitGroup(
        ignoredBossGroups, 1, false));
    assert(ignoredBossGroups == 0);
    assert(ShooterStages::Stage5::CanBossShotHitGroup(
        ignoredBossGroups, 1, true));
    assert(ShooterStages::Stage5::CanBossShotHitGroup(
        ignoredBossGroups, 2, true));

    // EASTSOURCE撃破後は暗転で場面を接続し、接近後だけ外壁上昇を進める
    assert(ShooterStages::Stage5::IsCinematicPhase(Phase::EastsourceFall));
    assert(ShooterStages::Stage5::IsCinematicPhase(Phase::WallClimbTransition));
    assert(ShooterStages::Stage5::IsCinematicPhase(Phase::CarrierTransformation));
    assert(ShooterStages::Stage5::IsValidTransition(
        Phase::WallClimbTransition, Phase::WallClimbLower));
    assert(!ShooterStages::Stage5::IsValidTransition(
        Phase::WallClimbTransition, Phase::RooftopArrival));
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::EastsourceFall, ShooterStages::Stage5::EastsourceFallFrames) == 1.0f);
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::WallClimbTransition, 0) == 1.0f);
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::WallClimbTransition, ShooterStages::Stage5::WallClimbFadeFrames) == 0.0f);
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::WallClimbLower, 0) == 1.0f);
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::WallClimbLower, ShooterStages::Stage5::WallClimbFadeFrames) == 0.0f);
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::WallClimbUpper, ShooterStages::Stage5::WallClimbUpperFrames) == 1.0f);
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        Phase::RooftopArrival, 0) == 1.0f);
    // カメラは到着前半で斜め上を保ち、変形終端で戦闘位置へ到達する
    assert(ShooterStages::Stage5::RooftopCameraFrontProgress(
        ShooterStages::Stage5::RooftopArrivalFrames / 2) == 0.0f);
    assert(ShooterStages::Stage5::RooftopCameraFrontProgress(
        ShooterStages::Stage5::RooftopArrivalFrames) == 1.0f);
    assert(ShooterStages::Stage5::CarrierCameraBattleProgress(
        ShooterStages::Stage5::CarrierTransformationFrames -
            ShooterStages::Stage5::CarrierCameraMoveFrames) == 0.0f);
    assert(ShooterStages::Stage5::CarrierCameraBattleProgress(
        ShooterStages::Stage5::CarrierTransformationFrames) == 1.0f);
    assert(ShooterStages::Stage5::WallApproachProgress(
        ShooterStages::Stage5::WallClimbFadeFrames) == 0.0f);
    assert(ShooterStages::Stage5::WallApproachProgress(
        ShooterStages::Stage5::WallClimbApproachFrames) == 1.0f);
    assert(ShooterStages::Stage5::WallClimbProgress(
        ShooterStages::Stage5::WallClimbApproachFrames) == 0.0f);
    assert(ShooterStages::Stage5::WallClimbProgress(
        ShooterStages::Stage5::WallClimbTransitionFrames) == 1.0f);
    assert(ShooterStages::Stage5::EastsourceFlyAwayProgress(0) == 0.0f);
    assert(ShooterStages::Stage5::EastsourceFlyAwayProgress(
        ShooterStages::Stage5::EastsourceFallFrames) == 1.0f);
    assert(ShooterStages::Stage5::PandDBuildingWidth == 144.0f);
    assert(ShooterStages::Stage5::PandDBuildingWindowColumns == 6);
    assert(ShooterStages::Stage5::WallClimbHeight <
        ShooterStages::Stage5::PandDBuildingHeight * 0.5f);
    assert(ShooterStages::Stage5::IsRooftopPhase(Phase::RooftopArrival));
    assert(ShooterStages::Stage5::IsRooftopPhase(Phase::CarrierTransformation));
    assert(!ShooterStages::Stage5::IsRooftopPhase(Phase::WallClimbUpper));
    assert(ShooterStages::Stage5::RooftopStormCloudCount == 192);
    assert(ShooterStages::Stage5::TayamaBossScale >
        ShooterStages::Stage5::PandDBuildingCapScale);
    const Vector3 tayamaWorldSize = TayamaModelView::TowerSize *
        ShooterStages::Stage5::TayamaBossScale;
    assert(std::abs(tayamaWorldSize.x - 115.2f) < 0.001f);
    assert(std::abs(tayamaWorldSize.y - 152.6144f) < 0.001f);
    assert(std::abs(tayamaWorldSize.z - 71.2f) < 0.001f);
    assert(ShooterStages::Stage5::TayamaFrontDistanceScale >
        ShooterStages::Stage5::TayamaOverheadDistanceScale);
    assert(std::abs(TayamaModelView::GroundedRootY(
        ShooterStages::Stage5::RooftopSurfaceY,
        ShooterStages::Stage5::TayamaBossScale) +
        TayamaModelView::TowerBoundsMin.y * ShooterStages::Stage5::TayamaBossScale -
        ShooterStages::Stage5::RooftopSurfaceY) < 0.0001f);
    assert(ShooterStages::Stage5::WallWaveHash(0, 0) !=
        ShooterStages::Stage5::WallWaveHash(1, 0));
    assert(ShooterStages::Stage5::IsPart2RoutePhase(
        ShooterStages::Stage5::Phase::WallClimbLower));
    assert(ShooterStages::Stage5::IsPart2RoutePhase(
        ShooterStages::Stage5::Phase::WallClimbUpper));
    assert(!ShooterStages::Stage5::IsPart2RoutePhase(
        ShooterStages::Stage5::Phase::WallClimbTransition));
    assert(SideScrollingShooter::UsesVerticalPlayerShots(5, Phase::WallClimbMiddle));
    assert(!SideScrollingShooter::UsesVerticalPlayerShots(4, Phase::WallClimbMiddle));
    assert(!SideScrollingShooter::UsesVerticalPlayerShots(5, Phase::RooftopArrival));
    assert(ShooterStages::Stage5::Part2ChapterNumber(
        ShooterStages::Stage5::Phase::WallClimbLower) == 1);
    assert(ShooterStages::Stage5::Part2ChapterNumber(
        ShooterStages::Stage5::Phase::WallClimbMiddle) == 2);
    assert(ShooterStages::Stage5::Part2ChapterNumber(
        ShooterStages::Stage5::Phase::WallClimbUpper) == 3);
    assert(ShooterStages::Stage5::WallClimbLowerFrames == 900);
    assert(ShooterStages::Stage5::WallClimbMiddleFrames == 900);
    assert(ShooterStages::Stage5::WallClimbUpperFrames == 900);
    assert(ShooterStages::Stage5::Part2RouteElapsedFrames(
        ShooterStages::Stage5::Phase::WallClimbMiddle, 0) ==
        ShooterStages::Stage5::WallClimbLowerFrames);
    assert(ShooterStages::Stage5::Part2RouteElapsedFrames(
        ShooterStages::Stage5::Phase::WallClimbUpper, 0) ==
        ShooterStages::Stage5::WallClimbLowerFrames +
        ShooterStages::Stage5::WallClimbMiddleFrames);
    assert(ShooterStages::Stage5::Part2RailEnemyEntryY > 60.0f);
    assert(ShooterStages::Stage5::Part2RailEnemyFallSpeed == 0.16f);
    assert(ShooterStages::Stage5::Part2RailPlayerMinY == 0.80f);
    assert(ShooterStages::Stage5::Part2RailPlayerMaxY == 16.0f);
    assert(ShooterStages::Stage5::Part2RailShotMinY <
        ShooterStages::Stage5::Part2RailPlayerMinY);
    assert(ShooterStages::Stage5::Part2RailShotMaxY >
        ShooterStages::Stage5::Part2RailPlayerMaxY);
    assert(ShooterStages::Stage5::Part2EnemyScaleMultiplier(0.0f) == 1.0f);
    assert(ShooterStages::Stage5::Part2EnemyScaleMultiplier(1.0f) == 2.0f);
    assert(ShooterStages::Stage5::Part2RailDroneAimY(-1.0f) >
        ShooterStages::Stage5::Part2RailPlayerMinY);
    assert(ShooterStages::Stage5::Part2RailDroneAimY(1.0f) <
        ShooterStages::Stage5::Part2RailPlayerMaxY);
    assert(ShooterStages::Stage5::Part2RailDroneEntryY >
        ShooterStages::Stage5::Part2RailDroneBaseY +
        ShooterStages::Stage5::Part2RailDroneBaseStep * 2.0f);
    assert(ShooterStages::Stage5::Part2RailDroneEntryProgress(0) == 0.0f);
    assert(ShooterStages::Stage5::Part2RailDroneEntryProgress(
        ShooterStages::Stage5::Part2RailDroneEntryFrames / 2) == 0.5f);
    assert(ShooterStages::Stage5::Part2RailDroneEntryProgress(
        ShooterStages::Stage5::Part2RailDroneEntryFrames) == 1.0f);
    constexpr float SideEnemyExitY = -1.87f;
    const float railHalfwayY = ShooterStages::Stage5::RemapPart2EnemyY(
        (ShooterStages::Stage5::Part2SideEnemyEntryY + SideEnemyExitY) * 0.5f,
        ShooterStages::Stage5::Part2SideEnemyEntryY, SideEnemyExitY,
        ShooterStages::Stage5::Part2RailEnemyEntryY,
        ShooterStages::Stage5::Part2RailEnemyExitY);
    assert(std::abs(railHalfwayY -
        (ShooterStages::Stage5::Part2RailEnemyEntryY +
            ShooterStages::Stage5::Part2RailEnemyExitY) * 0.5f) < 0.0001f);
    assert(std::abs(ShooterStages::Stage5::RemapPart2EnemyY(railHalfwayY,
        ShooterStages::Stage5::Part2RailEnemyEntryY,
        ShooterStages::Stage5::Part2RailEnemyExitY,
        ShooterStages::Stage5::Part2SideEnemyEntryY, SideEnemyExitY) -
        (ShooterStages::Stage5::Part2SideEnemyEntryY + SideEnemyExitY) * 0.5f) < 0.0001f);
    const float railDroneMiddleY = ShooterStages::Stage5::RemapPart2DroneBaseY(
        ShooterStages::Stage5::Part2SideDroneBaseY +
            ShooterStages::Stage5::Part2SideDroneBaseStep,
        ShooterStages::Stage5::Part2SideDroneBaseY,
        ShooterStages::Stage5::Part2SideDroneBaseStep,
        ShooterStages::Stage5::Part2RailDroneBaseY,
        ShooterStages::Stage5::Part2RailDroneBaseStep);
    assert(std::abs(railDroneMiddleY - ShooterStages::Stage5::Part2RailDroneBaseY -
        ShooterStages::Stage5::Part2RailDroneBaseStep) < 0.0001f);
    assert(ShooterStages::Stage5::Part2RailDroneBaseY >
        ShooterStages::Stage5::Part2RailPlayerMinY);
    assert(ShooterStages::Stage5::Part2RailEnemyPlaneZ > 0.0f);
    assert(ShooterStages::Stage5::Part2RailEnemyExitY < 0.0f);
    assert(!ShooterStages::Stage5::IsPart2PlayerFlyingAway(
        ShooterStages::Stage5::Phase::WallClimbUpper,
        ShooterStages::Stage5::WallClimbUpperFrames -
            ShooterStages::Stage5::WallClimbExitFadeFrames -
            ShooterStages::Stage5::Part2PlayerFlyAwayFrames - 1));
    assert(ShooterStages::Stage5::IsPart2PlayerFlyingAway(
        ShooterStages::Stage5::Phase::WallClimbUpper,
        ShooterStages::Stage5::WallClimbUpperFrames -
            ShooterStages::Stage5::WallClimbExitFadeFrames -
            ShooterStages::Stage5::Part2PlayerFlyAwayFrames));
    assert(ShooterStages::Stage5::Part2EnemyExitY(10.0f, true) <
        ShooterStages::Stage5::Part2EnemyExitY(10.0f, false));
    assert(ShooterStages::Stage5::CinematicFadeAlpha(
        ShooterStages::Stage5::Phase::WallClimbUpper,
        ShooterStages::Stage5::WallClimbUpperFrames -
            ShooterStages::Stage5::WallClimbExitFadeFrames - 1) == 0.0f);
    assert(ShooterStages::Stage5::Part2SideSceneryFallSpeed == 0.64f);
    assert(ShooterStages::Stage5::Part2RailSceneryFallSpeed == 0.96f);
    assert(ShooterStages::Stage5::Part2SideItemFallSpeed > 0.0f);
    assert(ShooterStages::Stage5::Part2RailItemFallSpeed >
        ShooterStages::Stage5::Part2SideItemFallSpeed);

    // 壁面警備ドローンはポインター接触時だけ45フレーム中に9発連射する
    assert(ShooterStages::Stage5::DroneSearchlightTouches(
        0.0f, 0.0f, 0.10f, 0.0f, 0.055f));
    assert(!ShooterStages::Stage5::DroneSearchlightTouches(
        0.0f, 0.0f, 0.20f, 0.0f, 0.055f));
    int droneBurstShots = 0;
    for (int remaining = ShooterStages::Stage5::DroneMachineGunBurstFrames;
        remaining > 0; --remaining) {
        if (ShooterStages::Stage5::IsDroneMachineGunFireFrame(remaining)) {
            ++droneBurstShots;
        }
    }
    assert(droneBurstShots == 9);

    // 壁面接触パッドは伸長に応じて壁面側へ移動する
    constexpr Vector3 retractedContact =
        WallSecurityDroneModelView::WallContactLocalPosition(-1.0f, 1.0f, 0.0f);
    constexpr Vector3 extendedContact =
        WallSecurityDroneModelView::WallContactLocalPosition(-1.0f, 1.0f, 1.0f);
    static_assert(retractedContact.x == -0.31f && retractedContact.y == 0.20f);
    static_assert(extendedContact.z > retractedContact.z);

    // EASTSOURCEは表示列挙と境界計算が同じ26パーツを使用する
    const Stage5ModelTransform eastsourceTransform {{2.0f, 3.0f, 40.0f}, {}, 0.72f};
    EastsourceModelState eastsourceState;
    int eastsourceParts = 0;
    EastsourceModelView::VisitParts(eastsourceTransform, eastsourceState,
        [&](PrimitiveShape, const Matrix4x4&, const ColorF&, EastsourcePartGroup) {
            ++eastsourceParts;
        });
    assert(eastsourceParts == static_cast<int>(EastsourceModelView::PrimitiveCount));
    assert(EastsourceModelView::GroupBounds(eastsourceTransform, eastsourceState,
        EastsourcePartGroup::Nose).valid);
    eastsourceState.destroyed[static_cast<std::size_t>(EastsourcePartGroup::Nose)] = true;
    assert(!EastsourceModelView::GroupBounds(eastsourceTransform, eastsourceState,
        EastsourcePartGroup::Nose).valid);

    // TAYAMAは同じパーツ群を補間し、崩壊Offsetも同じ境界へ反映する
    const Stage5ModelTransform tayamaTransform {{0.0f, 0.0f, 57.0f}, {}, 1.08f};
    TayamaModelState tayamaState;
    int tayamaParts = 0;
    TayamaModelView::VisitParts(tayamaTransform, 1.0f, tayamaState,
        [&](PrimitiveShape, const Matrix4x4&, const ColorF&, TayamaPartGroup) {
            ++tayamaParts;
        });
    assert(tayamaParts == static_cast<int>(TayamaModelView::PrimitiveCount));

    // 両腕は肩を通る水平X軸を中心に同じ時計回り方向へ回る
    Vector3 restingShoulder;
    Vector3 restingTip;
    Vector3 sweptShoulder;
    Vector3 sweptTip;
    Vector3 rightRestingShoulder;
    Vector3 rightRestingTip;
    Vector3 rightSweptShoulder;
    Vector3 rightSweptTip;
    TayamaModelView::ArmWorldSegment(
        tayamaTransform, true, 0.0f, restingShoulder, restingTip);
    TayamaModelView::ArmWorldSegment(
        tayamaTransform, true, Math::HalfPi, sweptShoulder, sweptTip);
    TayamaModelView::ArmWorldSegment(
        tayamaTransform, false, 0.0f, rightRestingShoulder, rightRestingTip);
    TayamaModelView::ArmWorldSegment(
        tayamaTransform, false, Math::HalfPi, rightSweptShoulder, rightSweptTip);
    assert((sweptShoulder - restingShoulder).LengthSquared() < 0.0001f);
    assert((rightSweptShoulder - rightRestingShoulder).LengthSquared() < 0.0001f);
    assert(std::abs(sweptTip.x - restingTip.x) < 0.0001f);
    assert(std::abs(rightSweptTip.x - rightRestingTip.x) < 0.0001f);
    assert(sweptTip.z < restingTip.z);
    assert(rightSweptTip.z < rightRestingTip.z);

    // 第一形態の眉毛は髪と同じ暗色、第二形態の髪と眉毛は発光金色へ切り替える
    constexpr ColorF Phase1Eyebrow = TayamaModelView::HeadPartColor(9);
    constexpr ColorF Phase1Hair = TayamaModelView::HeadPartColor(14);
    constexpr ColorF Phase2Eyebrow = TayamaModelView::HeadPartColor(9, true);
    constexpr ColorF Phase2Hair = TayamaModelView::HeadPartColor(14, true);
    static_assert(Phase1Eyebrow.r == TayamaModelView::Dark.r &&
        Phase1Eyebrow.g == TayamaModelView::Dark.g &&
        Phase1Eyebrow.b == TayamaModelView::Dark.b);
    static_assert(Phase1Hair.r == TayamaModelView::Hull.r &&
        Phase1Hair.g == TayamaModelView::Hull.g &&
        Phase1Hair.b == TayamaModelView::Hull.b);
    static_assert(Phase2Eyebrow.r == TayamaModelView::HeadGold.r &&
        Phase2Eyebrow.g == TayamaModelView::HeadGold.g &&
        Phase2Eyebrow.b == TayamaModelView::HeadGold.b);
    static_assert(Phase2Hair.r == TayamaModelView::HeadGold.r &&
        Phase2Hair.g == TayamaModelView::HeadGold.g &&
        Phase2Hair.b == TayamaModelView::HeadGold.b);

    // 第一形態の顔を従来の3分の1にし、レーザーの発射点を両目へ揃える
    static_assert(TayamaModelView::HeadScale(0.0f) == 1.0f);
    static_assert(TayamaModelView::HeadScale(1.0f) == 1.0f);
    static_assert(TayamaModelView::HeadRemovalOrder.size() ==
        ShooterStages::Stage5::TayamaDragonHeadPartCount);
    static_assert(TayamaModelView::IsHeadPartVisibleAfterRemoval(0, 33));
    static_assert(!TayamaModelView::IsHeadPartVisibleAfterRemoval(0, 34));
    static_assert(!TayamaModelView::IsHeadPartVisibleAfterRemoval(28, 1));
    std::array<Vector3, 2> tayamaEyeCenters {};
    int tayamaHeadPart = 0;
    float tayamaFaceWidth = 0.0f;
    TayamaModelView::VisitParts(tayamaTransform, 1.0f, tayamaState,
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&, TayamaPartGroup group) {
            if (group != TayamaPartGroup::Bridge ||
                tayamaHeadPart >= static_cast<int>(TayamaModelView::HeadParts.size())) return;
            if (tayamaHeadPart == 0) {
                tayamaFaceWidth = world.TransformVector(Vector3::Right).Length();
            }
            if (tayamaHeadPart == 11 || tayamaHeadPart == 12) {
                tayamaEyeCenters[static_cast<std::size_t>(tayamaHeadPart - 11)] =
                    world.TransformPoint(Vector3::Zero);
            }
            ++tayamaHeadPart;
        });
    assert(std::abs(tayamaFaceWidth - 4.0f * TayamaModelView::MechaHeadScale *
        tayamaTransform.scale) < 0.0001f);
    const std::array<Vector3, 2> laserEyes =
        TayamaModelView::EyeWorldPositions(tayamaTransform);
    assert((laserEyes[0] - tayamaEyeCenters[0]).LengthSquared() < 0.0001f);
    assert((laserEyes[1] - tayamaEyeCenters[1]).LengthSquared() < 0.0001f);
    assert((TayamaModelView::EyeWorldCenter(tayamaTransform) -
        (laserEyes[0] + laserEyes[1]) * 0.5f).LengthSquared() < 0.0001f);

    // 第一形態のサーチライト灯体は支点を保ったまま照射目標へ正面を向ける
    const Vector3 searchlightTarget {12.0f, -6.0f, -24.0f};
    const Vector2 searchlightAim = TayamaModelView::SearchlightAimRotation(
        true, searchlightTarget);
    const Vector3 searchlightForward =
        (Matrix4x4::RotationY(searchlightAim.x) *
            Matrix4x4::RotationX(searchlightAim.y))
            .TransformVector({0.0f, 0.0f, -1.0f}).Normalized();
    const Vector3 expectedSearchlightForward =
        (searchlightTarget - TayamaModelView::LeftSearchlightPivot).Normalized();
    assert(Vector3::Dot(searchlightForward, expectedSearchlightForward) > 0.9999f);

    // 第2形態の頭部背面は龍の球形首装甲より正面へ配置する
    constexpr float DragonJointDiameter = (3.35f + 1.8f) *
        TayamaModelView::DragonJointDiameterScale;
    constexpr float DragonHeadScale = 0.58f;
    constexpr float DragonHeadOffset = TayamaModelView::DragonHeadForwardOffset(
        DragonJointDiameter, DragonHeadScale);
    static_assert(DragonHeadOffset - TayamaModelView::HeadRearExtent *
        TayamaModelView::DragonHeadScale * DragonHeadScale >=
        DragonJointDiameter * 0.5f);

    const auto allBounds = TayamaModelView::AllGroupBounds(tayamaTransform, 1.0f, tayamaState);
    for (std::size_t group = 0; group < TayamaPartGroupCount; ++group) {
        const Stage5GroupBounds individual = TayamaModelView::GroupBounds(
            tayamaTransform, 1.0f, tayamaState, static_cast<TayamaPartGroup>(group));
        assert(allBounds[group].valid == individual.valid);
        assert((allBounds[group].center - individual.center).LengthSquared() < 0.0001f);
        assert(std::abs(allBounds[group].radius - individual.radius) < 0.0001f);
    }
    const Stage5GroupBounds before = TayamaModelView::GroupBounds(tayamaTransform, 1.0f,
        tayamaState, TayamaPartGroup::LeftFlightDeck);
    tayamaState.collapseOffsets[static_cast<std::size_t>(TayamaPartGroup::LeftFlightDeck)] =
        {{-8.0f, -4.0f, 2.0f}, {0.0f, 0.0f, 0.6f}, Vector3::One};
    const Stage5GroupBounds after = TayamaModelView::GroupBounds(tayamaTransform, 1.0f,
        tayamaState, TayamaPartGroup::LeftFlightDeck);
    assert(before.valid && after.valid);
    assert((after.center - before.center).LengthSquared() > 1.0f);

    // PANDD会建造物は元の縦横比を保ち超巨大ビルの屋上へ接地する
    constexpr float BuildingBottomY = -3.65f +
        ShooterStages::Stage5::PandDBuildingHeight;
    constexpr Vector3 BuildingSize = TayamaModelView::TowerSize *
        ShooterStages::Stage5::PandDBuildingCapScale;
    const float buildingLimit = (std::numeric_limits<float>::max)();
    std::array<Vector3, 2> buildingBounds {{
        {buildingLimit, buildingLimit, buildingLimit},
        {-buildingLimit, -buildingLimit, -buildingLimit}
    }};
    const Matrix4x4 buildingRoot = TayamaModelView::BuildingRoot(
        0.0f, BuildingBottomY, 45.0f, BuildingSize);
    TayamaModelView::VisitParts(buildingRoot, 0.0f, TayamaModelState {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&, TayamaPartGroup) {
            for (float x : {-0.5f, 0.5f}) {
                for (float y : {-0.5f, 0.5f}) {
                    for (float z : {-0.5f, 0.5f}) {
                        const Vector3 point = world.TransformPoint({x, y, z});
                        buildingBounds[0].x = (std::min)(buildingBounds[0].x, point.x);
                        buildingBounds[0].y = (std::min)(buildingBounds[0].y, point.y);
                        buildingBounds[0].z = (std::min)(buildingBounds[0].z, point.z);
                        buildingBounds[1].x = (std::max)(buildingBounds[1].x, point.x);
                        buildingBounds[1].y = (std::max)(buildingBounds[1].y, point.y);
                        buildingBounds[1].z = (std::max)(buildingBounds[1].z, point.z);
                    }
                }
            }
        });
    const Vector3 fittedBuildingSize = buildingBounds[1] - buildingBounds[0];
    assert(std::abs(buildingBounds[0].y - BuildingBottomY) < 0.01f);
    assert(std::abs(fittedBuildingSize.x - BuildingSize.x) < 0.01f);
    assert(std::abs(fittedBuildingSize.y - BuildingSize.y) < 0.01f);
    assert(std::abs(fittedBuildingSize.z - BuildingSize.z) < 0.01f);

    // 列挙行列の単位立方体外形からFBX基準寸法とY=0接地を検証する
    const auto ModelBounds = [](Stage5BuildingType type) {
        const float limit = (std::numeric_limits<float>::max)();
        std::array<Vector3, 2> bounds {{
            {limit, limit, limit}, {-limit, -limit, -limit}
        }};
        Stage5CityModelView::VisitBuilding(type, Stage5ModelTransform {},
            [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
                for (float x : {-0.5f, 0.5f}) {
                    for (float y : {-0.5f, 0.5f}) {
                        for (float z : {-0.5f, 0.5f}) {
                            const Vector3 point = world.TransformPoint({x, y, z});
                            bounds[0].x = (std::min)(bounds[0].x, point.x);
                            bounds[0].y = (std::min)(bounds[0].y, point.y);
                            bounds[0].z = (std::min)(bounds[0].z, point.z);
                            bounds[1].x = (std::max)(bounds[1].x, point.x);
                            bounds[1].y = (std::max)(bounds[1].y, point.y);
                            bounds[1].z = (std::max)(bounds[1].z, point.z);
                        }
                    }
                }
            });
        return bounds;
    };
    constexpr std::array<Vector3, 7> ExpectedModelSizes {{
        {4.40f, 7.743f, 4.40f},
        {4.40f, 15.33f, 4.40f},
        {4.40f, 9.83f, 4.40f},
        {4.62f, 11.40f, 3.168f},
        {3.36f, 7.92f, 2.304f},
        {4.62f, 11.40f, 3.168f},
        {3.370353f, 7.44f, 2.304f}
    }};

    // 全FBX近似モデルが指定Primitive予算内で列挙されることを確認する
    for (int building = 0; building < static_cast<int>(Stage5BuildingType::Count); ++building) {
        const auto type = static_cast<Stage5BuildingType>(building);
        int primitiveCount = 0;
        Stage5CityModelView::VisitBuilding(type, Stage5ModelTransform {},
            [&](PrimitiveShape, const Matrix4x4&, const ColorF&) { ++primitiveCount; });
        assert(primitiveCount == static_cast<int>(Stage5CityModelView::PrimitiveCount(type)));
        if (type == Stage5BuildingType::Tower) assert(primitiveCount >= 18 && primitiveCount <= 28);
        if (type == Stage5BuildingType::TowerSub) assert(primitiveCount >= 8 && primitiveCount <= 16);
        if (type == Stage5BuildingType::TowerSubSmall) assert(primitiveCount >= 8 && primitiveCount <= 14);
        if (type == Stage5BuildingType::BuildingLeft ||
            type == Stage5BuildingType::BuildingRight) {
            assert(primitiveCount >= 16 && primitiveCount <= 22);
        }
        if (type == Stage5BuildingType::BuildingLeftSmall ||
            type == Stage5BuildingType::BuildingRightSmall) {
            assert(primitiveCount >= 14 && primitiveCount <= 20);
        }
        assert(Stage5CityModelView::MountCount(type) == 3);
        for (std::size_t mount = 0; mount < Stage5CityModelView::MountCount(type); ++mount) {
            assert(Stage5CityModelView::SignMount(type, mount).transform.scale.x > 0.0f);
        }

        // 公開寸法と実際に列挙される外形を同じFBX基準値へ固定する
        const Vector3 declaredSize = Stage5CityModelView::ModelSize(type);
        const auto bounds = ModelBounds(type);
        const Vector3 visitedSize = bounds[1] - bounds[0];
        const Vector3 expectedSize = ExpectedModelSizes[building];
        assert(std::abs(declaredSize.x - expectedSize.x) < 0.001f);
        assert(std::abs(declaredSize.y - expectedSize.y) < 0.001f);
        assert(std::abs(declaredSize.z - expectedSize.z) < 0.001f);
        assert(std::abs(visitedSize.x - expectedSize.x) < 0.001f);
        assert(std::abs(visitedSize.y - expectedSize.y) < 0.001f);
        assert(std::abs(visitedSize.z - expectedSize.z) < 0.001f);
        assert(std::abs(bounds[0].y) < 0.001f);
    }

    // 主塔は円柱主体と12枚のBox外装で構成する
    std::array<int, 7> towerShapes {};
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::Tower, Stage5ModelTransform {},
        [&](PrimitiveShape shape, const Matrix4x4&, const ColorF&) {
            ++towerShapes[static_cast<std::size_t>(shape)];
        });
    assert(towerShapes[static_cast<std::size_t>(PrimitiveShape::Cylinder)] == 10);
    assert(towerShapes[static_cast<std::size_t>(PrimitiveShape::Box)] == 12);

    // TallとSmallは同じ4×4設計を保ち高さだけを短縮する
    const Vector3 tallSubSize = Stage5CityModelView::ModelSize(Stage5BuildingType::TowerSub);
    const Vector3 smallSubSize = Stage5CityModelView::ModelSize(Stage5BuildingType::TowerSubSmall);
    assert(tallSubSize.x == smallSubSize.x && tallSubSize.z == smallSubSize.z);
    assert(tallSubSize.y > smallSubSize.y);

    // 左右ビルの柱は同じ描画順のままX位置だけを反転する
    std::array<Vector3, 21> leftCenters {};
    std::array<Vector3, 21> rightCenters {};
    std::size_t leftCount = 0;
    std::size_t rightCount = 0;
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::BuildingLeft, Stage5ModelTransform {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
            leftCenters[leftCount++] = world.TransformPoint(Vector3::Zero);
        });
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::BuildingRight, Stage5ModelTransform {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
            rightCenters[rightCount++] = world.TransformPoint(Vector3::Zero);
        });
    assert(leftCount == 20 && rightCount == 21);
    assert(std::abs(leftCenters[1].x + 0.10f) < 0.0001f);
    assert(std::abs(rightCenters[1].x - 0.10f) < 0.0001f);
    assert(std::abs(leftCenters[13].x + rightCenters[13].x) < 0.0001f);
    assert(std::abs(leftCenters[14].x + rightCenters[14].x) < 0.0001f);
    assert(std::abs(leftCenters[13].y - 6.60f) < 0.0001f);
    assert(std::abs(rightCenters[20].x - 2.052941f) < 0.0001f);

    // Small版は共通帯寸法を0.8倍にし左だけ屋上箱、右だけ端柱を持つ
    const Vector3 largeLeftSize = Stage5CityModelView::ModelSize(Stage5BuildingType::BuildingLeft);
    const Vector3 smallLeftSize = Stage5CityModelView::ModelSize(Stage5BuildingType::BuildingLeftSmall);
    float largeBandWidth = 0.0f;
    float smallBandWidth = 0.0f;
    Vector3 smallRoofCenter {};
    std::size_t partIndex = 0;
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::BuildingLeft, Stage5ModelTransform {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
            if (partIndex++ == 2) largeBandWidth = world.TransformVector(Vector3::Right).Length();
        });
    partIndex = 0;
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::BuildingLeftSmall, Stage5ModelTransform {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
            if (partIndex == 1) smallBandWidth = world.TransformVector(Vector3::Right).Length();
            if (partIndex++ == 19) smallRoofCenter = world.TransformPoint(Vector3::Zero);
        });
    assert(std::abs(smallBandWidth / largeBandWidth - 0.8f) < 0.0001f);
    assert(std::abs(smallRoofCenter.y - 7.44f) < 0.0001f);
    assert(smallLeftSize.y < largeLeftSize.y);

    // Sub Towerの代表縦板はFBX外周位置と底面1.115からの中心を維持する
    Vector3 tallArmorCenter {};
    Vector3 smallArmorCenter {};
    partIndex = 0;
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::TowerSub, Stage5ModelTransform {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
            if (partIndex++ == 8) tallArmorCenter = world.TransformPoint(Vector3::Zero);
        });
    partIndex = 0;
    Stage5CityModelView::VisitBuilding(Stage5BuildingType::TowerSubSmall, Stage5ModelTransform {},
        [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
            if (partIndex++ == 7) smallArmorCenter = world.TransformPoint(Vector3::Zero);
        });
    assert(std::abs(tallArmorCenter.x + 1.632f) < 0.0001f);
    assert(std::abs(tallArmorCenter.y - 8.0f) < 0.0001f);
    assert(std::abs(smallArmorCenter.y - 5.25f) < 0.0001f);

    // TAYAMAfaceだけは3 Primitiveの独立した広告Attachmentとして扱う
    for (int ad = 0; ad < static_cast<int>(Stage5AdType::Count); ++ad) {
        int primitiveCount = 0;
        Vector3 panelSize {};
        const auto type = static_cast<Stage5AdType>(ad);
        Stage5CityModelView::VisitAd(type, Matrix4x4::Identity,
            [&](PrimitiveShape, const Matrix4x4& world, const ColorF&) {
                if (type == Stage5AdType::TayamaFace && primitiveCount == 0) {
                    panelSize = {
                        world.TransformVector(Vector3::Right).Length(),
                        world.TransformVector(Vector3::Up).Length(),
                        world.TransformVector(Vector3::Back).Length()
                    };
                }
                ++primitiveCount;
            });
        assert(type == Stage5AdType::TayamaFace ? primitiveCount == 3 : primitiveCount >= 5);
        if (type == Stage5AdType::TayamaFace) {
            assert(std::abs(panelSize.x - 0.924f) < 0.0001f);
            assert(std::abs(panelSize.y - 1.68f) < 0.0001f);
            assert(std::abs(panelSize.z - 0.576f) < 0.0001f);
        }
    }

    // 壁面警備ドローンは全部位を52 Primitiveで列挙し、内訳を維持する
    std::array<int, 6> dronePartCounts {};
    WallSecurityDroneModelView::DrawAll({}, {},
        [&](PrimitiveShape, const Matrix4x4&, const ColorF&,
            WallSecurityDronePartGroup group) {
            ++dronePartCounts[static_cast<std::size_t>(group)];
        });
    assert(dronePartCounts[0] == WallSecurityDroneModelView::StaticPrimitiveCount);
    assert(dronePartCounts[1] == WallSecurityDroneModelView::SensorPrimitiveCount);
    assert(dronePartCounts[2] == WallSecurityDroneModelView::SearchLightPrimitiveCount);
    assert(dronePartCounts[3] == WallSecurityDroneModelView::MachineGunPrimitiveCount);
    assert(dronePartCounts[4] == WallSecurityDroneModelView::WallContactPrimitiveCount);
    assert(dronePartCounts[5] == WallSecurityDroneModelView::WarningLightPrimitiveCount);

    // サーチライトと機関銃の照準APIが描画と同じYaw、Pitch、展開量へ追従する
    const Vector3 lightForward =
        WallSecurityDroneModelView::SearchLightForwardLocalDirection(0.0f, 0.0f);
    const Vector3 aimedLightForward =
        WallSecurityDroneModelView::SearchLightForwardLocalDirection(0.45f, 0.25f);
    assert((lightForward - Vector3::Back).LengthSquared() < 0.0001f);
    assert(std::abs(aimedLightForward.LengthSquared() - 1.0f) < 0.0001f);
    assert((aimedLightForward - lightForward).LengthSquared() > 0.01f);
    const Vector3 stowedMuzzle =
        WallSecurityDroneModelView::MachineGunMuzzleLocalPosition(0.0f, 0.0f, 0.0f);
    const Vector3 deployedMuzzle =
        WallSecurityDroneModelView::MachineGunMuzzleLocalPosition(0.0f, 0.0f, 1.0f);
    assert(deployedMuzzle.y < stowedMuzzle.y);
    assert(deployedMuzzle.z < stowedMuzzle.z);

    // 接触部の描画中心が0から1の伸縮で壁面側へ移動する
    auto ContactMaximumZ = [](float extension) {
        float maximumZ = -1000.0f;
        WallSecurityDroneModelView::DrawWallContactUnit({}, extension,
            [&](PrimitiveShape, const Matrix4x4& world, const ColorF&,
                WallSecurityDronePartGroup) {
                maximumZ = (std::max)(maximumZ,
                    world.TransformPoint(Vector3::Zero).z);
            });
        return maximumZ;
    };
    assert(ContactMaximumZ(1.0f) > ContactMaximumZ(0.0f) + 0.15f);
}
