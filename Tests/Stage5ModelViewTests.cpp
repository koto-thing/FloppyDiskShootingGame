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

    // 正規遷移を許可し、TAYAMA攻略順の飛び越しを拒否する
    assert(SideScrollingShooter::IsValidStage5Transition(
        Phase::EastsourceBattle, Phase::EastsourceFall));
    assert(SideScrollingShooter::IsValidStage5Transition(
        Phase::TayamaCommandCore, Phase::TayamaCollapse));
    assert(!SideScrollingShooter::IsValidStage5Transition(
        Phase::TayamaFireControl, Phase::TayamaCommandCore));
    assert(SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
        Weakpoint::FireControlRadar, Phase::TayamaFireControl));
    assert(!SideScrollingShooter::IsTayamaWeakpointActiveForPhase(
        Weakpoint::CommandCore, Phase::TayamaLiftEngines));

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
    assert(ShooterStages::Stage5::TayamaBossScale >
        ShooterStages::Stage5::PandDBuildingCapScale);
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
    assert(ShooterStages::Stage5::Part2RouteElapsedFrames(
        ShooterStages::Stage5::Phase::WallClimbMiddle, 0) ==
        ShooterStages::Stage5::WallClimbLowerFrames);
    assert(ShooterStages::Stage5::Part2RouteElapsedFrames(
        ShooterStages::Stage5::Phase::WallClimbUpper, 0) ==
        ShooterStages::Stage5::WallClimbLowerFrames +
        ShooterStages::Stage5::WallClimbMiddleFrames);
    assert(ShooterStages::Stage5::Part2RailEnemyEntryY > 10.0f);
    assert(ShooterStages::Stage5::Part2RailEnemyFallSpeed == 0.32f);
    assert(ShooterStages::Stage5::Part2RailEnemyPlaneZ > 0.0f);
    assert(ShooterStages::Stage5::Part2RailEnemyExitY < 0.0f);
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
