#include <cassert>
#include <cmath>

#include "../Engine/Graphics/IRenderBackend.h"
#include "../Presentation/Gameplay/Stages/Stage3/Stage3BossModelView.h"

/**
 * @brief Stage3ボスのPrimitive数、Mount情報、可動入力を検証する
 * @return なし
 */
void RunStage3BossModelViewTests() {
    // Stage3モデルが使う旧形状番号で砲身2がCylinder、装甲4がPrismになることを確認する
    static_assert(PrimitiveShapeFromLegacyIndex(2) == PrimitiveShape::Cylinder);
    static_assert(PrimitiveShapeFromLegacyIndex(4) == PrimitiveShape::Prism);
    int topGunShapes[4] {};
    int topGunShapeCount = 0;
    Stage3BossModelView::DrawTopGun(0, {}, {}, false,
        [&](int shape, const Vector3&, const Vector3&, const float[4], float, float) {
            topGunShapes[topGunShapeCount++] = shape;
        });
    assert(topGunShapeCount == 4);
    assert(PrimitiveShapeFromLegacyIndex(topGunShapes[0]) == PrimitiveShape::Cylinder);
    assert(PrimitiveShapeFromLegacyIndex(topGunShapes[1]) == PrimitiveShape::Prism);
    assert(PrimitiveShapeFromLegacyIndex(topGunShapes[2]) == PrimitiveShape::Cylinder);
    assert(PrimitiveShapeFromLegacyIndex(topGunShapes[3]) == PrimitiveShape::Cylinder);

    // 全部位を個別APIから描画して宣言済みPrimitive数と一致することを確認する
    int primitiveCount = 0;
    auto countPart = [&](int, const Vector3&, const Vector3&, const float[4], float, float) {
        ++primitiveCount;
    };
    Stage3BossModelView::DrawStaticBody({}, countPart);
    Stage3BossModelView::DrawGondolaBody({}, countPart);
    for (int i = 0; i < Stage3BossModelView::TopGunCount; ++i) {
        Stage3BossModelView::DrawTopGun(i, {}, {}, false, countPart);
    }
    for (int i = 0; i < Stage3BossModelView::GondolaMachineGunCount; ++i) {
        Stage3BossModelView::DrawGondolaMachineGun(i, {}, {}, countPart);
    }
    for (int i = 0; i < Stage3BossModelView::HeavyCannonCount; ++i) {
        Stage3BossModelView::DrawHeavyCannon(i, {}, {}, countPart);
    }
    for (int i = 0; i < Stage3BossModelView::MissilePodCount; ++i) {
        Stage3BossModelView::DrawMissilePod(i, {}, 0.0f, countPart);
    }
    for (int i = 0; i < Stage3BossModelView::FunnelPodCount; ++i) {
        Stage3BossModelView::DrawFunnelPod(i, {}, 0.0f, countPart);
    }
    assert(primitiveCount == Stage3BossModelView::PrimitiveCount);

    // 一回目で上部船体に穴が開き、二回目で上部船体が全消失することを確認する
    int intactBodyCount = 0;
    int piercedBodyCount = 0;
    int swallowedBodyCount = 0;
    auto CountBody = [](int& count) {
        return [&count](int, const Vector3&, const Vector3&, const float[4], float, float) {
            ++count;
        };
    };
    Stage3BossModelView::DrawDamagedStaticBody({}, 0, CountBody(intactBodyCount));
    Stage3BossModelView::DrawDamagedStaticBody({}, 1, CountBody(piercedBodyCount));
    Stage3BossModelView::DrawDamagedStaticBody({}, 2, CountBody(swallowedBodyCount));
    assert(intactBodyCount == Stage3BossModelView::StaticBodyPrimitiveCount);
    assert(piercedBodyCount > 0 && piercedBodyCount < intactBodyCount);
    assert(swallowedBodyCount == 0);

    // 各Mountが部位種別と論理インデックスを保持することを確認する
    assert(Stage3BossModelView::TopGunMount(2).type == Stage3BossPartType::TopMachineGun);
    assert(Stage3BossModelView::TopGunMount(2).index == 2);
    assert(Stage3BossModelView::GondolaMachineGunMount(4).index == 4);
    assert(Stage3BossModelView::HeavyCannonMount(1).index == 1);
    assert(Stage3BossModelView::MissilePodMount(1).index == 1);
    assert(Stage3BossModelView::FunnelPodMount(2).index == 2);

    // 全上部砲台の基部下面が各取付位置の船体上面へ接していることを確認する
    constexpr float TopGunBaseHalfHeight = 0.32f * 0.5f;
    constexpr float deckTopY[Stage3BossModelView::TopGunCount] = {
        2.90f, 2.90f, 2.90f, 2.90f, 2.90f, 1.47f
    };
    for (int i = 0; i < Stage3BossModelView::TopGunCount; ++i) {
        const float baseBottomY =
            Stage3BossModelView::TopGunMount(i).localPosition.y - TopGunBaseHalfHeight;
        assert(std::fabs(baseBottomY - deckTopY[i]) < 0.0001f);
    }

    // Phase1は艦尾から2基ずつ艦首へ進むことを確認する
    static_assert(Stage3BossModelView::Phase1SectionCount == 3);
    static_assert(Stage3BossModelView::Phase1TopGunIndex(0, 0) == 5);
    static_assert(Stage3BossModelView::Phase1TopGunIndex(0, 1) == 4);
    static_assert(Stage3BossModelView::Phase1TopGunIndex(2, 0) == 1);
    static_assert(Stage3BossModelView::Phase1TopGunIndex(2, 1) == 0);
    static_assert(Stage3BossModelView::Phase1PartIndexForTopGun(5) == 0);
    static_assert(Stage3BossModelView::Phase1PartIndexForTopGun(0) == 5);
    constexpr int sectionHp[] = {0, 0, 10, 0, 0, 0};
    static_assert(Stage3BossModelView::IsPhase1SectionDestroyed(0, sectionHp));
    static_assert(!Stage3BossModelView::IsPhase1SectionDestroyed(1, sectionHp));

    // ファンネル射出口が対応ポッド前方にあることを確認する
    const Vector3 podPosition = Stage3BossModelView::FunnelPodMount(0).localPosition;
    const Vector3 launchPosition = Stage3BossModelView::FunnelLaunchLocalPosition(0);
    assert(launchPosition.x < podPosition.x);
    assert(launchPosition.y == podPosition.y);
    assert(launchPosition.z == podPosition.z);

    // ゴンドラ機銃のYawとPitchが描画値へ渡ることを確認する
    float yaw = 0.0f;
    float pitch = 0.0f;
    Stage3BossModelView::DrawGondolaMachineGun(0, {}, {0.25f, 0.50f, 0.0f},
        [&](int, const Vector3&, const Vector3&, const float[4], float partYaw, float partPitch) {
            yaw = partYaw;
            pitch = partPitch;
        });
    assert(std::fabs(yaw - 0.50f) < 0.0001f);
    assert(std::fabs(pitch - 0.25f) < 0.0001f);

    // アクティブ砲台だけ上部装甲色が変わることを確認する
    float inactiveArmorRed = 0.0f;
    float activeArmorRed = 0.0f;
    int inactivePart = 0;
    int activePart = 0;
    Stage3BossModelView::DrawTopGun(0, {}, {}, false,
        [&](int, const Vector3&, const Vector3&, const float color[4], float, float) {
            if (inactivePart++ == 1) inactiveArmorRed = color[0];
        });
    Stage3BossModelView::DrawTopGun(0, {}, {}, true,
        [&](int, const Vector3&, const Vector3&, const float color[4], float, float) {
            if (activePart++ == 1) activeArmorRed = color[0];
        });
    assert(activeArmorRed > inactiveArmorRed);

    // 黒い取付基部は照準回転へ追従せず船体へ固定されることを確認する
    float baseYaw = 0.0f;
    float basePitch = 0.0f;
    int topGunPart = 0;
    Stage3BossModelView::DrawTopGun(0, {}, {0.25f, 0.50f, 0.0f}, false,
        [&](int, const Vector3&, const Vector3&, const float[4], float partYaw, float partPitch) {
            if (topGunPart++ == 0) {
                baseYaw = partYaw;
                basePitch = partPitch;
            }
        });
    assert(std::fabs(baseYaw) < 0.0001f);
    assert(std::fabs(basePitch) < 0.0001f);

    // 横視点とレール視点の双方で砲身が目標方向へ旋回することを確認する
    const Vector3 sideAim = Stage3BossModelView::TopGunAimRotation(
        2, {{0.0f, 0.0f, 12.2f}, {}, 0.0f, 4.0f}, {-8.0f, 0.0f, 10.0f});
    const Vector3 railAim = Stage3BossModelView::TopGunAimRotation(
        2, {{0.0f, -11.6f, 31.0f}, {}, Math::HalfPi, 4.0f}, {0.0f, 0.0f, 8.0f});
    assert(std::isfinite(sideAim.x) && std::isfinite(sideAim.y));
    assert(std::isfinite(railAim.x) && std::isfinite(railAim.y));
    assert(std::fabs(sideAim.y) > 0.01f);
    assert(std::fabs(railAim.y) > 0.01f);

    // 気球損傷位置が親Transformに追従し、全点がゴンドラより上の表面へ並ぶことを確認する
    const BossModelTransform damageTransform {{2.0f, -3.0f, 5.0f}, {}, 0.0f, 2.0f};
    for (int i = 0; i < Stage3BossModelView::BalloonDamagePointCount; ++i) {
        const Vector3 damage = Stage3BossModelView::BalloonDamageWorldPosition(i, damageTransform);
        assert(damage.x > damageTransform.position.x - 14.0f);
        assert(damage.x < damageTransform.position.x + 14.0f);
        assert(damage.y > damageTransform.position.y + 4.0f);
    }
    const Vector3 damageCenter = Stage3BossModelView::BalloonDamageWorldPosition(
        Stage3BossModelView::BalloonDamagePointCount / 2, damageTransform);
    assert(std::fabs(damageCenter.x - damageTransform.position.x) < 0.0001f);

    // 大口径砲の砲口が支点ではなく回転後の砲身先端へ一致することを確認する
    const BossModelTransform cannonTransform {{4.0f, -3.0f, 12.0f}, {}, 0.0f, 1.8f};
    const Vector3 cannonMuzzle = Stage3BossModelView::HeavyCannonMuzzleWorldPosition(
        0, cannonTransform, {0.0f, 0.0f, 0.0f});
    const Vector3 cannonMount = Stage3BossModelView::HeavyCannonMount(0).localPosition;
    assert(std::fabs(cannonMuzzle.x -
        (cannonTransform.position.x + (cannonMount.x - 2.63f) * cannonTransform.scale)) < 0.0001f);
    assert(std::fabs(cannonMuzzle.y -
        (cannonTransform.position.y + (cannonMount.y - 0.48f) * cannonTransform.scale)) < 0.0001f);
}
