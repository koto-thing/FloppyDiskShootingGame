#include <cassert>
#include <cmath>

#include "../Presentation/Gameplay/Stages/Stage4/Stage4BossModelView.h"
#include "../Presentation/Gameplay/Stages/Stage4/Stage4State.h"

/**
 * @brief Stage4ボスの構成数、破壊単位、親Transform合成を検証する
 * @return なし
 */
void RunStage4BossModelViewTests() {
    // 範囲外から内側へ飛ぶ砲弾を発射フレームで着弾扱いにしないことを確認する
    static_assert(!ShooterStages::Stage4::CrossedRangeEdge(
        108.0f, 107.0f, 0.0f, 72.0f));
    static_assert(ShooterStages::Stage4::CrossedRangeEdge(
        71.0f, 72.0f, 0.0f, 72.0f));

    // 登場時は接触車が順に飛散し、非接触車が演出前半で消えることを確認する
    static_assert(ShooterStages::Stage4::TrafficKickRate(10, 0) == 0.0f);
    static_assert(ShooterStages::Stage4::TrafficKickRate(58, 0) == 1.0f);
    static_assert(ShooterStages::Stage4::TrafficKickRate(15, 1) == 0.0f);
    static_assert(ShooterStages::Stage4::TrafficFadeAlpha(0) == 1.0f);
    static_assert(ShooterStages::Stage4::TrafficFadeAlpha(
        ShooterStages::Stage4::TrafficFadeFrames) == 0.0f);

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

    // 主砲身は親Yawではなく照準先へのYaw/Pitchで描画されることを確認する
    BossModelTransform aimingTransform {{0.0f, 0.0f, 0.0f}, {-8.25f, 5.55f, 0.0f},
        0.0f, 1.0f, true};
    aimingTransform.mainGunMinElevation = Stage4BossModelView::Phase1CannonMinElevation;
    aimingTransform.mainGunMaxElevation = Stage4BossModelView::Phase1CannonMaxElevation;
    float cannonYaw = 0.0f;
    float cannonPitch = 0.0f;
    int aimingVisited = 0;
    Stage4BossModelView::Draw(aimingTransform, [&](int, const Vector3&, const Vector3&,
        const float[4], float yaw, float pitch) {
        if (aimingVisited++ != Stage4BossModelView::ChassisPrimitiveCount +
            Stage4BossModelView::TrackPrimitiveCount +
            Stage4BossModelView::LuxuryBodyPrimitiveCount +
            Stage4BossModelView::MainTurretPrimitiveCount) return;
        cannonYaw = yaw;
        cannonPitch = pitch;
    });
    assert(std::fabs(cannonYaw) < 0.0001f);
    assert(cannonPitch < -0.37f && cannonPitch > -0.39f);
    aimingTransform.mainGunTracksYaw = false;
    aimingTransform.aimTarget = {-8.25f, 5.55f, 20.0f};
    aimingVisited = 0;
    Stage4BossModelView::Draw(aimingTransform, [&](int, const Vector3&, const Vector3&,
        const float[4], float yaw, float pitch) {
        if (aimingVisited++ != Stage4BossModelView::ChassisPrimitiveCount +
            Stage4BossModelView::TrackPrimitiveCount +
            Stage4BossModelView::LuxuryBodyPrimitiveCount +
            Stage4BossModelView::MainTurretPrimitiveCount) return;
        cannonYaw = yaw;
        cannonPitch = pitch;
    });
    assert(std::fabs(cannonYaw) < 0.0001f);
    assert(cannonPitch < -0.37f && cannonPitch > -0.39f);
    aimingTransform.mainGunTracksYaw = true;
    aimingTransform.aimTarget = {-8.25f, -8.0f, 0.0f};
    aimingVisited = 0;
    Stage4BossModelView::Draw(aimingTransform, [&](int, const Vector3&, const Vector3&,
        const float[4], float, float pitch) {
        if (aimingVisited++ != Stage4BossModelView::ChassisPrimitiveCount +
            Stage4BossModelView::TrackPrimitiveCount +
            Stage4BossModelView::LuxuryBodyPrimitiveCount +
            Stage4BossModelView::MainTurretPrimitiveCount) return;
        cannonPitch = pitch;
    });
    assert(std::fabs(cannonPitch + Stage4BossModelView::Phase1CannonMinElevation) < 0.0001f);
    aimingTransform.aimTarget = {-0.01f, 100.0f, 0.0f};
    aimingVisited = 0;
    Stage4BossModelView::Draw(aimingTransform, [&](int, const Vector3&, const Vector3&,
        const float[4], float, float pitch) {
        if (aimingVisited++ != Stage4BossModelView::ChassisPrimitiveCount +
            Stage4BossModelView::TrackPrimitiveCount +
            Stage4BossModelView::LuxuryBodyPrimitiveCount +
            Stage4BossModelView::MainTurretPrimitiveCount) return;
        cannonPitch = pitch;
    });
    assert(std::fabs(cannonPitch + Stage4BossModelView::Phase1CannonMaxElevation) < 0.0001f);

    // 車体と三種類の交換式主砲が独立したPrimitive数で描画されることを確認する
    int bodyCount = 0;
    Stage4BossModelView::DrawBody({}, [&](int, const Vector3&, const Vector3&,
        const float[4], float, float) {
        ++bodyCount;
    });
    assert(bodyCount == Stage4BossModelView::PrimitiveCount -
        Stage4BossModelView::MainCannonPrimitiveCount);
    constexpr Stage4MainWeaponType WeaponTypes[] = {
        Stage4MainWeaponType::Phase1Cannon,
        Stage4MainWeaponType::SiegeMortar,
        Stage4MainWeaponType::RomanceCannon
    };
    constexpr int WeaponPrimitiveCounts[] = {
        Stage4BossModelView::MainCannonPrimitiveCount,
        Stage4BossModelView::SiegeMortarPrimitiveCount,
        Stage4BossModelView::RomanceCannonPrimitiveCount
    };
    for (int weapon = 0; weapon < 3; ++weapon) {
        int weaponCount = 0;
        Stage4BossModelView::DrawMainWeapon(WeaponTypes[weapon], {},
            Stage4BossModelView::DefaultMainWeaponPose(WeaponTypes[weapon]),
            [&](int, const Vector3&, const Vector3&, const float[4], float, float) {
                ++weaponCount;
            });
        assert(weaponCount == WeaponPrimitiveCounts[weapon]);
    }

    // 半壊主砲は固定基部と砲尾だけを残し、長砲身を描画しないことを確認する
    int damagedCannonCount = 0;
    Stage4BossModelView::DrawDamagedRomanceCannon({}, {},
        [&](int, const Vector3&, const Vector3&, const float[4], float, float) {
            ++damagedCannonCount;
        });
    assert(damagedCannonCount > 0);
    assert(damagedCannonCount < Stage4BossModelView::RomanceCannonPrimitiveCount);

    // 共通マウントが親移動、Yaw、拡縮を反映して独立Transformになることを確認する
    const BossModelTransform mounted = Stage4BossModelView::MainWeaponMount(transform);
    assert(std::fabs(mounted.position.x - 1.0f) < 0.0001f);
    assert(std::fabs(mounted.position.y - 9.1f) < 0.0001f);
    assert(std::fabs(mounted.position.z - 9.5f) < 0.0001f);
    assert(std::fabs(mounted.yaw - transform.yaw) < 0.0001f);
    assert(std::fabs(mounted.scale - transform.scale) < 0.0001f);

    // 迫撃砲の仰角とロマン砲の後座量が砲口位置へ反映されることを確認する
    const Vector3 mortarLow = Stage4BossModelView::SiegeMortarMuzzleLocalPosition(
        Math::Pi * 40.0f / 180.0f);
    const Vector3 mortarHigh = Stage4BossModelView::SiegeMortarMuzzleLocalPosition(
        Math::Pi * 65.0f / 180.0f);
    const Vector3 mortarHorizontal = Stage4BossModelView::SiegeMortarMuzzleLocalPosition(
        Stage4BossModelView::SiegeMortarMinPitch);
    const Vector3 mortarMax = Stage4BossModelView::SiegeMortarMuzzleLocalPosition(
        Stage4BossModelView::SiegeMortar2DMaxPitch);
    const Vector3 mortarOverMax = Stage4BossModelView::SiegeMortarMuzzleLocalPosition(
        Math::Pi);
    assert(mortarHigh.y > mortarLow.y);
    assert(mortarHigh.x > mortarLow.x);
    assert(mortarHorizontal.y < mortarLow.y);
    assert(mortarMax.y > mortarHigh.y);
    assert(std::fabs(mortarMax.x - mortarOverMax.x) < 0.0001f);
    assert(std::fabs(mortarMax.y - mortarOverMax.y) < 0.0001f);
    const Vector3 romanceReady = Stage4BossModelView::RomanceCannonMuzzleLocalPosition(0.0f, 0.0f);
    const Vector3 romanceRecoiled = Stage4BossModelView::RomanceCannonMuzzleLocalPosition(0.0f, 1.0f);
    assert(romanceReady.x < romanceRecoiled.x);
    assert(std::fabs(romanceRecoiled.x - romanceReady.x - 2.40f) < 0.0001f);

    // 各主砲が仕様数の運搬把持点を公開し範囲外入力を安全に扱うことを確認する
    static_assert(Stage4BossModelView::CarryPointCount(
        Stage4MainWeaponType::Phase1Cannon) == 2);
    static_assert(Stage4BossModelView::CarryPointCount(
        Stage4MainWeaponType::SiegeMortar) == 3);
    static_assert(Stage4BossModelView::CarryPointCount(
        Stage4MainWeaponType::RomanceCannon) == 4);
    assert(Stage4BossModelView::CarryPointLocalPosition(
        Stage4MainWeaponType::RomanceCannon, -1) == Vector3::Zero);
    assert(Stage4BossModelView::CarryPointLocalPosition(
        Stage4MainWeaponType::RomanceCannon, 4) == Vector3::Zero);

    // パージ用補助エンジン噴射口が各主砲の下面に配置されることを確認する
    assert(Stage4BossModelView::PurgeThrusterLocalPosition(
        Stage4MainWeaponType::Phase1Cannon).y < 0.0f);
    assert(Stage4BossModelView::PurgeThrusterLocalPosition(
        Stage4MainWeaponType::SiegeMortar).y < 0.0f);
    assert(Stage4BossModelView::PurgeThrusterLocalPosition(
        Stage4MainWeaponType::RomanceCannon).y < 0.0f);
}
