#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage4/Stage4BossModelView.h"
#include "../Presentation/Gameplay/Stages/Stage4/Stage4State.h"
#include "../Presentation/Gameplay/Stages/Stage4/Stage4WeaponDroneView.h"

namespace {
/**
 * @brief 二つの座標が許容誤差内で一致するか判定する
 * @param actual 実測座標
 * @param expected 期待座標
 * @return 全成分が許容誤差内の場合true
 */
bool Near(const Vector3& actual, const Vector3& expected) {
    return std::fabs(actual.x - expected.x) < 0.0001f &&
        std::fabs(actual.y - expected.y) < 0.0001f &&
        std::fabs(actual.z - expected.z) < 0.0001f;
}
}

/**
 * @brief Stage4主砲交換ドローンの構成、可動部、CarryPoint整合を検証する
 * @return なし
 */
void RunStage4WeaponDroneViewTests() {
    // Primitive数、形状、任意Transform合成を検証する
    const BossModelTransform transform {{1.0f, 2.0f, 3.0f}, {}, Math::HalfPi, 2.0f};
    int primitiveCount = 0;
    int shapeCounts[6] {};
    Vector3 firstPosition;
    Vector3 firstScale;
    float firstYaw = 0.0f;
    Stage4WeaponDroneView::Draw(transform, {}, [&](int shape, const Vector3& position,
        const Vector3& scale, const float[4], float yaw, float) {
        if (primitiveCount == 0) {
            firstPosition = position;
            firstScale = scale;
            firstYaw = yaw;
        }
        ++primitiveCount;
        ++shapeCounts[shape];
    });
    assert(primitiveCount == Stage4WeaponDroneView::PrimitiveCount);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Box)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Sphere)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Cylinder)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Cone)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Prism)] > 0);
    assert(Near(firstPosition, transform.position));
    assert(Near(firstScale, {2.36f, 0.68f, 2.36f}));
    assert(std::fabs(firstYaw - Math::HalfPi) < 0.0001f);

    // 左アームだけを動かした場合に右クランプが変化しないことを検証する
    const Vector3 closedLeft = Stage4WeaponDroneView::LeftClampLocalPosition();
    const Vector3 closedRight = Stage4WeaponDroneView::RightClampLocalPosition();
    Stage4WeaponDronePose articulated;
    articulated.leftShoulderPitch = 0.35f;
    articulated.leftElbowPitch = -0.55f;
    assert(!Near(Stage4WeaponDroneView::LeftClampLocalPosition(articulated), closedLeft));
    assert(Near(Stage4WeaponDroneView::RightClampLocalPosition(articulated), closedRight));

    // Clamp openAmountとThruster Tiltが描画姿勢へ反映されることを検証する
    articulated.leftClampOpen = 1.0f;
    articulated.thrusterTilt = 0.42f;
    int visited = 0;
    float thrusterPitch = 0.0f;
    float leftJawPitch = 0.0f;
    Stage4WeaponDroneView::Draw({}, articulated, [&](int, const Vector3&, const Vector3&,
        const float[4], float, float pitch) {
        if (visited == Stage4WeaponDroneView::CorePrimitiveCount + 1) thrusterPitch = pitch;
        if (visited == Stage4WeaponDroneView::CorePrimitiveCount +
            Stage4WeaponDroneView::ThrusterPrimitiveCount + 5) leftJawPitch = pitch;
        ++visited;
    });
    assert(std::fabs(thrusterPitch - articulated.thrusterTilt) < 0.0001f);
    assert(std::fabs(leftJawPitch - 0.68f) < 0.0001f);

    // 交換主砲の全CarryPointへドローン把持中心を一致させられることを検証する
    const BossModelTransform weaponTransform {{2.0f, -1.0f, 4.0f}, {}, 0.42f, 0.7f};
    for (Stage4MainWeaponType weaponType : {
        Stage4MainWeaponType::SiegeMortar, Stage4MainWeaponType::RomanceCannon}) {
        const Stage4MainWeaponPose weaponPose =
            Stage4BossModelView::DefaultMainWeaponPose(weaponType);
        const int expectedCount = weaponType == Stage4MainWeaponType::SiegeMortar ? 3 : 4;
        assert(Stage4BossModelView::CarryPointCount(weaponType) == expectedCount);
        for (int index = 0; index < expectedCount; ++index) {
            const Vector3 carryPoint = Stage4BossModelView::MainWeaponPointWorldPosition(
                weaponTransform, weaponPose,
                Stage4BossModelView::CarryPointLocalPosition(weaponType, index));
            BossModelTransform droneTransform {{}, {}, -0.31f, weaponTransform.scale};
            droneTransform = Stage4WeaponDroneView::PlaceLiftPointAt(
                droneTransform, {}, carryPoint);
            assert(Near(Stage4WeaponDroneView::LiftPointWorldPosition(droneTransform),
                carryPoint));
        }
    }

    // 二種類の設定値と所有権遷移が同じ交換工程を最後まで通ることを検証する
    using namespace ShooterStages::Stage4;
    assert(SwapConfig(MainWeaponType::SiegeMortar).droneCount == 3);
    assert(SwapConfig(MainWeaponType::SiegeMortar).TotalFrames() == 377);
    assert(SwapConfig(MainWeaponType::RomanceCannon).droneCount == 4);
    assert(SwapConfig(MainWeaponType::RomanceCannon).TotalFrames() == 486);
    State swap;
    swap.phase = BossPhase::TransitionToPhase2;
    swap.swapState = WeaponSwapState::Prepare;
    swap.incomingWeapon = MainWeaponType::SiegeMortar;
    for (int transition = 0; transition < 12; ++transition) AdvanceWeaponSwap(swap);
    assert(swap.phase == BossPhase::Phase2);
    assert(swap.swapState == WeaponSwapState::None);
    assert(swap.currentWeapon == MainWeaponType::SiegeMortar);
    assert(swap.outgoingVisual == WeaponVisualState::Hidden);
    assert(swap.incomingVisual == WeaponVisualState::Attached);
}
