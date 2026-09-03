#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage3/Stage3FunnelModelView.h"
#include "../Presentation/Gameplay/Stages/Stage3/Stage3State.h"

/**
 * @brief Stage3ファンネルのPrimitive数と可動Mount座標を検証する
 * @return なし
 */
void RunStage3FunnelModelViewTests() {
    // Phase3状態が最大5基で初期化されることを確認する
    ShooterStages::Stage3::State state;
    assert(state.reflectFunnels.size() == 5);
    for (const auto& funnel : state.reflectFunnels) assert(!funnel.active && funnel.hp == 0);
    for (const int cooldown : state.funnelPortCooldowns) assert(cooldown == 0);

    // 空中機雷が5秒間残り、300フレーム目に自然爆発することを確認する
    assert(!ShooterStages::Stage3::IsFunnelMineExpired(299));
    assert(ShooterStages::Stage3::IsFunnelMineExpired(300));

    // 両モデルの描画数が宣言値と一致することを確認する
    auto countDraws = [](auto drawModel) {
        int count = 0;
        drawModel([&](int, const Vector3&, const Vector3&, const float[4], float, float) { ++count; });
        return count;
    };
    assert(countDraws([](auto&& drawPart) {
        Stage3FunnelModelView::DrawBarrier({}, 0.0f, drawPart);
    }) == Stage3FunnelModelView::BarrierPrimitiveCount);
    assert(countDraws([](auto&& drawPart) {
        Stage3FunnelModelView::DrawReflectShot({}, 0.0f, 0.0f, 0.0f, drawPart);
    }) == Stage3FunnelModelView::ReflectShotPrimitiveCount);

    // Emitter開閉で左右Anchorが外側かつ後方へ移動することを確認する
    const Vector3 closedLeft = Stage3FunnelModelView::BarrierAnchorLocalPosition(0, 0.0f);
    const Vector3 openLeft = Stage3FunnelModelView::BarrierAnchorLocalPosition(0, 1.0f);
    const Vector3 openRight = Stage3FunnelModelView::BarrierAnchorLocalPosition(1, 1.0f);
    assert(openLeft.z < 0.0f && openRight.z > 0.0f);
    assert(openLeft.x > closedLeft.x);
    assert(std::fabs(openLeft.x - openRight.x) < 0.0001f);
    assert(std::fabs(openLeft.z + openRight.z) < 0.0001f);

    // 砲口座標がYaw、Pitch、Recoilへ追従することを確認する
    const Vector3 muzzle = Stage3FunnelModelView::ReflectShotMuzzleLocalPosition(0.0f, 0.0f, 0.0f);
    const Vector3 aimed = Stage3FunnelModelView::ReflectShotMuzzleLocalPosition(0.4f, 0.3f, 0.0f);
    const Vector3 recoiled = Stage3FunnelModelView::ReflectShotMuzzleLocalPosition(0.0f, 0.0f, 1.0f);
    assert(muzzle.x < 0.0f);
    assert(std::fabs(aimed.y) > 0.0001f && std::fabs(aimed.z) > 0.0001f);
    assert(recoiled.x > muzzle.x);
}
