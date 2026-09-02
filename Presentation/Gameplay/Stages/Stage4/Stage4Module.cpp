#include "Stage4Module.h"

#include "Stage4BossModelView.h"
#include "Stage4EnemySheet.h"

const SideScrollingShooter::Stage& SideScrollingShooter::Stage4Module::Definition() {
    static const Stage4EnemySheet definition;
    return definition;
}

bool SideScrollingShooter::Stage4Module::DrawBossModel(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Enemy& enemy, float yaw) {
    (void)shooter;
    if (enemy.type != 2) return false;

    // Stage2と同じ親Transform経由で専用モデルを描画する
    constexpr float BossScale = 1.00f;
    const BossModelTransform transform {
        {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z},
        {}, yaw - Math::HalfPi, BossScale
    };
    auto DrawBossPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float partYaw, float partPitch) {
        DrawModelPrimitive(renderer, camera, shape,
            position.x, position.y, position.z,
            scale.x, scale.y, scale.z, color, partYaw, partPitch);
    };
    Stage4BossModelView::Draw(transform, DrawBossPart);
    return true;
}
