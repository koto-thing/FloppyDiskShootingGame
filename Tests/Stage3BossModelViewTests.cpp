#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage3/Stage3BossModelView.h"

/**
 * @brief Stage3ボスのPrimitive数、Mount情報、可動入力を検証する
 * @return なし
 */
void RunStage3BossModelViewTests() {
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
}
