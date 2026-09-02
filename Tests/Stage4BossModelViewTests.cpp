#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage4/Stage4BossModelView.h"

/**
 * @brief Stage4ボスの構成数、破壊単位、親Transform合成を検証する
 * @return なし
 */
void RunStage4BossModelViewTests() {
    // 完全状態のPrimitive数と使用形状を数える
    int primitiveCount = 0;
    int shapeCounts[6] {};
    Stage4BossModelView::Draw({}, [&](int shape, const Vector3&, const Vector3&,
        const float[4], float, float) {
        ++primitiveCount;
        ++shapeCounts[shape];
    });
    assert(primitiveCount == Stage4BossModelView::PrimitiveCount);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Box)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Sphere)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Cylinder)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Cone)] > 0);
    assert(shapeCounts[static_cast<int>(PrimitiveShape::Prism)] > 0);

    // Stage4の幅6.0の四車線全体を車体幅24.0で覆うことを確認する
    static_assert(Stage4BossModelView::Stage4LaneCount == 4);
    static_assert(Stage4BossModelView::ModelWidth == 24.0f);

    // 将来破壊対象となる全単位を非表示にして常設車体だけが残ることを確認する
    Stage4BossModelState destroyed;
    destroyed.mainTurret = false;
    destroyed.mainCannon = false;
    destroyed.commandTower = false;
    destroyed.frontRam = false;
    for (bool& secondaryGun : destroyed.secondaryGuns) secondaryGun = false;
    for (bool& exhaustStack : destroyed.exhaustStacks) exhaustStack = false;
    int remainingCount = 0;
    Stage4BossModelView::Draw({}, [&](int, const Vector3&, const Vector3&,
        const float[4], float, float) {
        ++remainingCount;
    }, destroyed);
    assert(remainingCount == Stage4BossModelView::PrimitiveCount -
        Stage4BossModelView::MainTurretPrimitiveCount - Stage4BossModelView::MainCannonPrimitiveCount -
        Stage4BossModelView::CommandTowerPrimitiveCount - Stage4BossModelView::SecondaryGunPrimitiveCount -
        Stage4BossModelView::ExhaustPrimitiveCount - 4);

    // 先頭車体部品へ親移動、Y回転、拡縮が一度だけ反映されることを確認する
    const BossModelTransform transform {{1.0f, 2.0f, 3.0f}, {}, Math::HalfPi, 2.0f};
    Vector3 firstPosition;
    Vector3 firstScale;
    float firstYaw = 0.0f;
    int visited = 0;
    Stage4BossModelView::Draw(transform, [&](int, const Vector3& position, const Vector3& scale,
        const float[4], float yaw, float) {
        if (visited++ != 0) return;
        firstPosition = position;
        firstScale = scale;
        firstYaw = yaw;
    });
    assert(std::fabs(firstPosition.x - 1.0f) < 0.0001f);
    assert(std::fabs(firstPosition.y - 2.7f) < 0.0001f);
    assert(std::fabs(firstPosition.z - 3.0f) < 0.0001f);
    assert(std::fabs(firstScale.x - 28.8f) < 0.0001f);
    assert(std::fabs(firstScale.y - 2.7f) < 0.0001f);
    assert(std::fabs(firstScale.z - 48.0f) < 0.0001f);
    assert(std::fabs(firstYaw - Math::HalfPi) < 0.0001f);
}
