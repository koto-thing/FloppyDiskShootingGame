#include "Stage4Module.h"

#include <algorithm>
#include <cmath>

#include "Stage4BossModelView.h"
#include "Stage4EnemySheet.h"

using ShooterStages::Stage4::ShotKind;

namespace {

constexpr float Stage4BossScale = 1.00f;
constexpr float Stage4ModelYawOffset = -Math::HalfPi;
constexpr float Stage4TrackWheelRadius = 0.74f;
constexpr float Stage4CannonballSpeed = 0.56f;
constexpr float Stage4CannonballGravity = 0.00135f;
constexpr float Stage4CannonMuzzleDistance = 4.47f;
constexpr float Stage4CannonballRailRadius = 0.52f;
constexpr float Stage4CannonballSideRadius = 0.075f;
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

static_assert(sizeof(Stage4SecondaryGunLocal) / sizeof(Stage4SecondaryGunLocal[0]) == 6);
static_assert(sizeof(Stage4BodyHitLocal) / sizeof(Stage4BodyHitLocal[0]) == 12);

}

const SideScrollingShooter::Stage& SideScrollingShooter::Stage4Module::Definition() {
    static const Stage4EnemySheet definition;
    return definition;
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
    aimedTransform.mainGunTracksTarget = state.mainCannon && enemy.phase <= 0.0f;
    aimedTransform.trackRoll = (shooter.IsRailGameplayActive() ?
        enemy.z - enemy.baseZ : ToWorldX(enemy.x - enemy.baseX)) /
        Stage4TrackWheelRadius;

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

        if (part == BossNose) {
            SpawnMainCannonball(shooter, boss);
            continue;
        }

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

bool SideScrollingShooter::Stage4Module::HitsHazard(
    const SideScrollingShooter& shooter, float x, float y, float z, float radius) {
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
    if (shot.stage4.gravity) shot.vy -= Stage4CannonballGravity;
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
        (shot.z <= 0.0f || shot.z >= 72.0f ||
            std::abs(shot.x) >= 1.2f || std::abs(shot.y) >= 1.24f) :
        (shot.x <= Side2DPlayerMinX || shot.x >= Side2DPlayerMaxX ||
            shot.y >= Side2DPlayerMaxY);
    const bool hitPlayerZ = shooter.IsRailGameplayActive() &&
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
        shot.x, hitGround && !impactAtPlayerZ ? groundY : shot.y, impactZ);
    shot.active = false;
}

bool SideScrollingShooter::Stage4Module::IsShotCullProtected(const Shot& shot) {
    return shot.stage4.kind == ShotKind::Cannonball;
}

float SideScrollingShooter::Stage4Module::EnemyShotHitRadius(
    const Shot& shot, bool railMode) {
    if (shot.stage4.kind != ShotKind::Cannonball) {
        return railMode ? 0.28f : 0.022f;
    }
    return railMode ? Stage4CannonballRailRadius : Stage4CannonballSideRadius;
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

bool SideScrollingShooter::Stage4Module::DrawSpecialShot(
    const SideScrollingShooter& shooter, Renderer& renderer,
    const Camera3D& camera, const Shot& shot, float yaw) {
    if (!shot.enemy || shot.stage4.kind != ShotKind::Cannonball) return false;

    // 主砲弾は通常敵弾より大きな鉄球として描画する
    constexpr float ShellColor[] = {0.16f, 0.16f, 0.17f, 1.0f};
    constexpr float HotCoreColor[] = {0.85f, 0.18f, 0.035f, 1.0f};
    const float radius = shooter.IsRailGameplayActive() ?
        Stage4CannonballRailRadius : Stage4CannonballSideRadius * WorldXScale;
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z,
        radius, radius, radius, ShellColor, yaw + shot.age * 0.12f);
    DrawModelPrimitive(renderer, camera, 2, ToWorldX(shot.x), ToWorldY(shot.y), shot.z - 0.02f,
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

Vector3 SideScrollingShooter::Stage4Module::BossPartLocalPosition(BossPart part) {
    const int secondaryGun = SecondaryGunIndex(part);
    if (secondaryGun >= 0) return Stage4SecondaryGunLocal[secondaryGun];
    return Stage4MainCannonLocal;
}

void SideScrollingShooter::Stage4Module::SpawnMainCannonball(
    SideScrollingShooter& shooter, const Enemy& boss) {
    Shot* available = nullptr;
    for (auto& shot : shooter.m_shots) {
        if (!shot.active) {
            available = &shot;
            break;
        }
    }
    if (available == nullptr) return;

    // 見た目の主砲と同じピボット、照準先から砲口位置と射出方向を決める
    const Vector3 pivot = LocalToWorld(shooter, boss, Stage4MainCannonPivotLocal);
    const Vector3 aimTarget {
        ToWorldX(boss.turretAimX),
        ToWorldY(boss.turretAimY),
        shooter.IsRailGameplayActive() ? boss.turretAimZ : pivot.z
    };
    const Vector3 delta = aimTarget - pivot;
    const float length = (std::max)(0.001f,
        std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z));
    const Vector3 direction = delta / length;
    const Vector3 muzzle = pivot + direction * Stage4CannonMuzzleDistance;

    Shot& shot = *available;
    shot = {};
    shot.x = FromWorldX(muzzle.x);
    shot.y = FromWorldY(muzzle.y);
    shot.z = muzzle.z;
    shot.transitionSideX = shot.x;
    shot.transitionSideY = shot.y;
    shot.vx = FromWorldX(direction.x * Stage4CannonballSpeed);
    shot.vy = FromWorldY(direction.y * Stage4CannonballSpeed);
    shot.vz = shooter.IsRailGameplayActive() ? direction.z * Stage4CannonballSpeed : 0.0f;
    shot.hitRadius = Stage4CannonballSideRadius;
    shot.damage = 2;
    shot.enemy = true;
    shot.stage4.kind = ShotKind::Cannonball;
    shot.stage4.gravity = false;
    shot.active = true;
}
