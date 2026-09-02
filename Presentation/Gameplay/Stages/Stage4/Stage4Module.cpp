#include "Stage4Module.h"

#include <algorithm>
#include <cmath>

#include "Stage4BossModelView.h"
#include "Stage4EnemySheet.h"

namespace {

constexpr float Stage4BossScale = 1.00f;
constexpr float Stage4ModelYawOffset = -Math::HalfPi;
constexpr Vector3 Stage4MainCannonLocal {-5.75f, 3.55f, 0.0f};
constexpr Vector3 Stage4SecondaryGunLocal[] = {
    {-3.40f, 3.76f, -3.45f}, {-3.40f, 3.76f, 3.45f},
    {-4.40f, 3.08f, -7.65f}, {-4.40f, 3.08f, 7.65f},
    {3.10f, 3.00f, -7.15f}, {3.10f, 3.00f, 7.15f}
};
constexpr float Stage4MainCannonHitRadius = 1.45f;
constexpr float Stage4SecondaryGunHitRadius = 0.82f;

static_assert(sizeof(Stage4SecondaryGunLocal) / sizeof(Stage4SecondaryGunLocal[0]) == 6);

}

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
    const float railWeight = Math::Clamp01(1.0f - yaw / Math::HalfPi);
    const Vector3 aimTarget {
        ToWorldX(enemy.turretAimX),
        ToWorldY(enemy.turretAimY),
        Math::Lerp(SidePlaneZ, enemy.turretAimZ, railWeight)
    };
    const BossModelTransform transform {
        {ToWorldX(enemy.x), ToWorldY(enemy.y), enemy.z},
        aimTarget, yaw + Stage4ModelYawOffset, Stage4BossScale,
        enemy.bossPartHp[BossNose] > 0, true
    };
    Stage4BossModelState state;
    state.mainCannon = enemy.bossPartHp[BossNose] > 0;
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
    aimedTransform.mainGunTracksTarget = state.mainCannon;

    // Stage4BossModelViewの出力を既存Primitive描画へ接続する
    auto DrawBossPart = [&](int shape, const Vector3& position, const Vector3& scale,
        const float color[4], float partYaw, float partPitch) {
        DrawModelPrimitive(renderer, camera, shape,
            position.x, position.y, position.z,
            scale.x, scale.y, scale.z, color, partYaw, partPitch);
    };
    Stage4BossModelView::Draw(aimedTransform, DrawBossPart, state);
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
    SideScrollingShooter& shooter, const Enemy& boss) {
    if (boss.type != 2) return;

    // 生存中の主砲身と副砲6基から、現在の自機位置へ直接撃つ
    for (int partIndex = 0; partIndex < BossPartCount; ++partIndex) {
        const BossPart part = static_cast<BossPart>(partIndex);
        if (boss.bossPartHp[part] <= 0) continue;
        if (part != BossNose && SecondaryGunIndex(part) < 0) continue;

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
