#pragma once

#include <array>
#include <cstddef>

#include "Stage5ModelTypes.h"
#include "Stage5CityModelView.h"

/** @brief EASTSOURCEの描画と破壊判定に使うパーツグループ */
enum class EastsourcePartGroup {
    Body,
    Nose,
    LeftWing,
    RightWing,
    LeftEngine,
    RightEngine,
    Count
};

inline constexpr std::size_t EastsourcePartGroupCount =
    static_cast<std::size_t>(EastsourcePartGroup::Count);

/** @brief EASTSOURCEのグループ表示、破壊、被弾表示状態 */
struct EastsourceModelState {
    std::array<bool, EastsourcePartGroupCount> visible {true, true, true, true, true, true};
    std::array<bool, EastsourcePartGroupCount> destroyed {};
    std::array<bool, EastsourcePartGroupCount> hitFlash {};

    /**
     * @brief 指定グループを描画および当たり判定へ含めるか判定する
     * @param group 判定するパーツグループ
     * @return 表示中かつ未破壊ならtrue、非表示または破壊済みならfalse
     */
    constexpr bool IsVisible(EastsourcePartGroup group) const {
        const std::size_t index = static_cast<std::size_t>(group);
        return visible[index] && !destroyed[index];
    }
};

/** @brief EASTSOURCEを構成する一つのプリミティブ */
struct EastsourcePart {
    PrimitiveShape shape = PrimitiveShape::Box;
    Stage5PartTransform local {};
    EastsourcePartGroup group = EastsourcePartGroup::Body;
    ColorF color {};
};

/** @brief 参照ブランチのdrawBoss1をそのまま構造化したEASTSOURCEモデル */
class EastsourceModelView {
public:
    static constexpr std::size_t PrimitiveCount = 26;
    inline static constexpr ColorF Gray {0.5f, 0.5f, 0.5f, 1.0f};
    inline static constexpr ColorF White {0.6f, 0.6f, 0.6f, 1.0f};
    inline static constexpr ColorF Black {0.2f, 0.2f, 0.2f, 1.0f};
    inline static constexpr ColorF Hit {1.0f, 0.04f, 0.03f, 1.0f};

    // drawBoss1の形状、位置、寸法、XYZ回転、色を変更せず保持する
    inline static constexpr std::array<EastsourcePart, PrimitiveCount> Parts {{
        {PrimitiveShape::Cylinder, {{0.0f, 0.75f, -3.5f}, {Math::HalfPi, 0.0f, 0.0f}, {1.5f, 1.0f, 1.5f}}, EastsourcePartGroup::Nose, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, 0.5f, -4.375f}, {Math::HalfPi, 0.0f, 0.0f}, {0.5f, 0.75f, 0.5f}}, EastsourcePartGroup::Nose, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, 1.125f, -5.0f}, {Math::HalfPi, 0.0f, 0.0f}, {0.25f, 2.0f, 0.25f}}, EastsourcePartGroup::Nose, Black},

        {PrimitiveShape::Cylinder, {{0.0f, 0.5f, 0.0f}, {Math::HalfPi, 0.0f, 0.0f}, {4.5f, 4.0f, 4.5f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, 0.5f, -2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {3.5f, 1.0f, 3.5f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, 0.5f, 2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {3.5f, 1.0f, 3.5f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Box, {{0.0f, 2.75f, 0.0f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, 3.0f, 0.5f}, {Math::HalfPi, 0.0f, 0.0f}, {0.25f, 1.0f, 0.25f}}, EastsourcePartGroup::Body, Black},

        {PrimitiveShape::Cylinder, {{0.0f, -3.0f, 0.0f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 2.5f, 1.0f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, -3.75f, 0.25f}, {Math::HalfPi, 0.0f, 0.0f}, {0.5f, 2.0f, 0.5f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Cylinder, {{0.0f, -3.0f, -1.75f}, {Math::HalfPi, 0.0f, 0.0f}, {0.25f, 1.5f, 0.25f}}, EastsourcePartGroup::Body, Black},
        {PrimitiveShape::Box, {{0.5f, -2.0f, 0.0f}, {Math::HalfPi, 0.0f, 0.0f}, {1.25f, 0.25f, 0.25f}}, EastsourcePartGroup::Body, Black},
        {PrimitiveShape::Box, {{-0.5f, -2.0f, 0.0f}, {Math::HalfPi, 0.0f, 0.0f}, {1.25f, 0.25f, 0.25f}}, EastsourcePartGroup::Body, Black},

        {PrimitiveShape::Box, {{3.25f, 0.5f, 0.0f}, {}, {3.0f, 1.0f, 3.0f}}, EastsourcePartGroup::LeftWing, White},
        {PrimitiveShape::Box, {{5.25f, 0.5f, 0.0f}, {}, {2.5f, 0.5f, 2.0f}}, EastsourcePartGroup::LeftWing, White},
        {PrimitiveShape::Box, {{-3.25f, 0.5f, 0.0f}, {}, {3.0f, 1.0f, 3.0f}}, EastsourcePartGroup::RightWing, White},
        {PrimitiveShape::Box, {{-5.25f, 0.5f, 0.0f}, {}, {2.5f, 0.5f, 2.0f}}, EastsourcePartGroup::RightWing, White},

        {PrimitiveShape::Cylinder, {{0.0f, 0.75f, 3.75f}, {Math::HalfPi, 0.0f, 0.0f}, {2.5f, 1.5f, 2.5f}}, EastsourcePartGroup::Body, Gray},
        {PrimitiveShape::Cylinder, {{1.75f, 0.75f, 4.5f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.5f, 1.0f}}, EastsourcePartGroup::Body, Black},
        {PrimitiveShape::Cylinder, {{-1.75f, 0.75f, 4.5f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.5f, 1.0f}}, EastsourcePartGroup::Body, Black},
        {PrimitiveShape::Box, {{0.0f, -1.0f, 4.125f}, {}, {0.75f, 2.0f, 0.2f}}, EastsourcePartGroup::Body, White},
        {PrimitiveShape::Box, {{0.0f, 2.5f, 4.125f}, {}, {0.75f, 2.0f, 0.2f}}, EastsourcePartGroup::Body, White},

        {PrimitiveShape::Cylinder, {{1.5f, -1.5f, 2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 2.5f, 1.0f}}, EastsourcePartGroup::LeftEngine, Black},
        {PrimitiveShape::Cylinder, {{1.5f, -1.5f, 4.0f}, {Math::HalfPi, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}}, EastsourcePartGroup::LeftEngine, Black},
        {PrimitiveShape::Cylinder, {{-1.5f, -1.5f, 2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 2.5f, 1.0f}}, EastsourcePartGroup::RightEngine, Black},
        {PrimitiveShape::Cylinder, {{-1.5f, -1.5f, 4.0f}, {Math::HalfPi, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}}, EastsourcePartGroup::RightEngine, Black}
    }};

    /**
     * @brief 表示中の全パーツを共通ワールド行列へ解決して列挙する
     * @param transform モデル全体のTransform
     * @param state グループの表示と破壊状態
     * @param drawPart 形状、ワールド行列、色、グループを受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitParts(const Stage5ModelTransform& transform, const EastsourceModelState& state,
        DrawPart&& drawPart) {
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);

        // 描画と当たり判定で同じパーツ行列を共有する
        for (const EastsourcePart& part : Parts) {
            if (!state.IsVisible(part.group)) continue;
            const std::size_t group = static_cast<std::size_t>(part.group);
            const ColorF color = state.hitFlash[group] ? Hit : part.color;
            drawPart(part.shape, root * Stage5ModelDetail::Matrix(part.local), color, part.group);
        }
    }

    /**
     * @brief 描画パーツと同じワールド行列からグループ境界を求める
     * @param transform モデル全体のTransform
     * @param state グループの表示と破壊状態
     * @param group 境界を求めるグループ
     * @return 表示パーツがない場合はvalidがfalseの球形境界
     */
    static Stage5GroupBounds GroupBounds(const Stage5ModelTransform& transform,
        const EastsourceModelState& state, EastsourcePartGroup group) {
        Stage5GroupBounds bounds;
        VisitParts(transform, state,
            [&](PrimitiveShape, const Matrix4x4& world, const ColorF&, EastsourcePartGroup partGroup) {
                if (partGroup != group) return;
                Stage5ModelDetail::Include(bounds, world.TransformPoint(Vector3::Zero),
                    Stage5ModelDetail::WorldPartRadius(world));
            });
        return bounds;
    }
};

/** @brief TAYAMAの変形、弱点、崩壊へ使用するパーツグループ */
enum class TayamaPartGroup {
    CentralHull,
    LeftFlightDeck,
    RightFlightDeck,
    Bridge,
    LeftSearchlight,
    RightSearchlight,
    FireControlRadar,
    LeftLiftEngine,
    RightLiftEngine,
    CommandCore,
    ArmorPanel,
    Hangar,
    MainThruster,
    RunwayLight,
    Count
};

inline constexpr std::size_t TayamaPartGroupCount =
    static_cast<std::size_t>(TayamaPartGroup::Count);

/** @brief TAYAMAの表示、破壊、被弾表示、グループ崩壊Transform */
struct TayamaModelState {
    std::array<bool, TayamaPartGroupCount> visible {
        true, true, true, true, true, true, true,
        true, true, true, true, true, true, true
    };
    std::array<bool, TayamaPartGroupCount> destroyed {};
    std::array<bool, TayamaPartGroupCount> hitFlash {};
    std::array<Stage5PartTransform, TayamaPartGroupCount> collapseOffsets {};

    /**
     * @brief 指定グループを描画および当たり判定へ含めるか判定する
     * @param group 判定するパーツグループ
     * @return 表示中かつ未破壊ならtrue、非表示または破壊済みならfalse
     */
    constexpr bool IsVisible(TayamaPartGroup group) const {
        const std::size_t index = static_cast<std::size_t>(group);
        return visible[index] && !destroyed[index];
    }
};

/** @brief ビル形態と巨大メカ形態を共有するTAYAMAの一つのプリミティブ */
struct TayamaPart {
    PrimitiveShape shape = PrimitiveShape::Box;
    Stage5PartTransform tower {};
    Stage5PartTransform mecha {};
    TayamaPartGroup group = TayamaPartGroup::CentralHull;
    ColorF color {};
};

/** @brief 添付FBX近似ビルをビル形態と巨大メカ形態で共有するTAYAMAの一部品 */
struct TayamaBuildingPart {
    Stage5BuildingType type = Stage5BuildingType::Tower;
    Stage5PartTransform tower {};
    Stage5PartTransform mecha {};
    TayamaPartGroup group = TayamaPartGroup::CentralHull;
};

/** @brief 同じパーツ群を補間してビルから巨大メカへ変形するTAYAMAモデル */
class TayamaModelView {
public:
    inline static constexpr Vector3 TowerBoundsMin {-7.2f, -11.0f, -3.7f};
    inline static constexpr Vector3 TowerBoundsMax {7.2f, 8.0768f, 5.2f};
    inline static constexpr Vector3 TowerSize {14.4f, 19.0768f, 8.9f};
    inline static constexpr ColorF Hull {0.15f, 0.17f, 0.21f, 1.0f};
    inline static constexpr ColorF Armor {0.24f, 0.27f, 0.32f, 1.0f};
    inline static constexpr ColorF LightArmor {0.36f, 0.40f, 0.46f, 1.0f};
    inline static constexpr ColorF Dark {0.055f, 0.065f, 0.085f, 1.0f};
    inline static constexpr ColorF Window {0.06f, 0.30f, 0.42f, 1.0f};
    inline static constexpr ColorF Warning {1.0f, 0.30f, 0.035f, 1.0f};
    inline static constexpr ColorF Runway {0.08f, 0.72f, 1.0f, 1.0f};
    inline static constexpr ColorF Core {1.0f, 0.08f, 0.025f, 1.0f};
    inline static constexpr ColorF Hit {1.0f, 0.04f, 0.03f, 1.0f};

    /**
     * @brief ビル形態の下端を指定面へ接地する親Y座標を取得する
     * @param surfaceY 接地面のY座標
     * @param scale モデル全体の均一倍率
     * @return 接地済み親Y座標
     */
    static constexpr float GroundedRootY(float surfaceY, float scale) {
        return surfaceY - TowerBoundsMin.y * scale;
    }

    // FBX近似ビルは画像内と同じ役割で再配置し、格子と積層外装をそのまま共有する
    inline static constexpr std::array<TayamaBuildingPart, 8> BuildingParts {{
        {Stage5BuildingType::BuildingLeft,
            {{2.1040f, -10.9687f, 0.525f}}, {{-3.0f, -11.0f, 0.0f}},
            TayamaPartGroup::Hangar},
        {Stage5BuildingType::BuildingRight,
            {{-2.0960f, -10.9687f, 0.525f}}, {{3.0f, -11.0f, 0.0f}},
            TayamaPartGroup::Hangar},
        {Stage5BuildingType::TowerSub,
            {{-3.5f, -11.0f, 3.0f}}, {{0.0f, -0.51525f, -0.5f}, {}, {1.0f, 0.85f, 1.0f}},
            TayamaPartGroup::CentralHull},
        {Stage5BuildingType::TowerSubSmall,
            {{3.5f, -11.0f, 3.0f}}, {{-4.915f, 11.0f, 0.2f}, {0.0f, 0.0f, -Math::HalfPi}},
            TayamaPartGroup::CentralHull},
        {Stage5BuildingType::BuildingLeftSmall,
            {{1.7053f, 0.1568f, 0.525f}}, {{-9.964f, 11.0f, 0.0f}, {0.0f, 0.0f, -Math::HalfPi}, {0.9f, 0.9f, 1.0f}},
            TayamaPartGroup::RightFlightDeck},
        {Stage5BuildingType::BuildingRightSmall,
            {{-1.6119f, 0.1568f, 0.525f}}, {{9.748f, 11.0f, 0.0f}, {0.0f, 0.0f, Math::HalfPi}, {0.9f, 0.9f, 1.0f}},
            TayamaPartGroup::LeftFlightDeck},
        {Stage5BuildingType::Tower,
            {{-5.0f, -11.0f, -1.5f}}, {{-9.532584f, 2.469002f, 0.0f}, {0.0f, 0.0f, -0.27f}},
            TayamaPartGroup::RightFlightDeck},
        {Stage5BuildingType::Tower,
            {{5.0f, -11.0f, -1.5f}}, {{9.532584f, 2.469002f, 0.0f}, {0.0f, 0.0f, 0.27f}},
            TayamaPartGroup::LeftFlightDeck}
    }};

    // 同梱Blenderデータの34箱を外形中心で正規化したTAYAMA頭部
    inline static constexpr std::array<Stage5PartTransform, 34> HeadParts {{
        {{0.0f, -0.0305f, -0.2041f}, {}, {4.0f, 4.4f, 2.8f}},
        {{-2.0f, -0.2305f, -0.2041f}, {}, {1.2f, 1.2f, 0.36f}},
        {{2.0f, -0.2305f, -0.2041f}, {}, {1.2f, 1.2f, 0.36f}},
        {{1.5f, -0.8305f, -1.2041f}, {}, {1.6f, 1.6f, 1.12f}},
        {{-1.5f, -0.8305f, -1.2041f}, {}, {1.6f, 1.6f, 1.12f}},
        {{0.0f, -2.3305f, -1.2041f}, {}, {1.6f, 1.04f, 1.12f}},
        {{0.0f, -0.8305f, -1.2041f}, {}, {0.56f, 1.28f, 1.12f}},
        {{0.0f, -1.0305f, -1.2041f}, {}, {0.924f, 0.576f, 1.68f}},
        {{0.0f, -1.6305f, -1.2041f}, {}, {1.6f, 0.16f, 1.12f}},
        {{0.9f, 0.8695f, -1.6041f}, {}, {1.152f, 0.64f, 0.448f}},
        {{-0.9f, 0.8695f, -1.6041f}, {}, {1.152f, 0.64f, 0.448f}},
        {{0.9f, 0.1695f, -1.5041f}, {}, {0.9216f, 0.256f, 0.3584f}},
        {{-0.9f, 0.1695f, -1.5041f}, {}, {0.9216f, 0.256f, 0.3584f}},
        {{0.0f, -2.5305f, -0.9041f}, {}, {2.64f, 0.88f, 1.232f}},
        {{0.0f, 2.4273f, 0.0959f}, {}, {4.18f, 0.7755f, 3.6652f}},
        {{-0.7f, 2.1273f, 0.0459f}, {}, {3.08f, 0.7755f, 3.9964f}},
        {{1.7f, 2.1273f, 0.0459f}, {}, {1.078f, 0.7755f, 3.9964f}},
        {{0.0f, 0.2495f, 0.7959f}, {}, {3.2f, 3.76f, 2.24f}},
        {{0.0f, 2.6215f, 0.0959f}, {}, {3.762f, 0.6979f, 3.2987f}},
        {{0.0f, 0.2159f, 0.7959f}, {}, {3.872f, 3.3088f, 2.464f}},
        {{0.0f, 0.4095f, -1.0441f}, {}, {1.6f, 0.16f, 1.6f}},
        {{1.0f, -0.2905f, -1.0441f}, {}, {1.28f, 0.16f, 1.6f}},
        {{1.72f, 0.1095f, -1.0441f}, {}, {0.16f, 0.64f, 1.6f}},
        {{0.28f, 0.1095f, -1.0441f}, {}, {0.16f, 0.64f, 1.6f}},
        {{-1.0f, 0.4095f, -1.0441f}, {}, {1.6f, 0.16f, 1.6f}},
        {{-1.0f, -0.2905f, -1.0441f}, {}, {1.28f, 0.16f, 1.6f}},
        {{-0.28f, 0.1095f, -1.0441f}, {}, {0.16f, 0.64f, 1.6f}},
        {{-1.72f, 0.1095f, -1.0441f}, {}, {0.16f, 0.64f, 1.6f}},
        {{2.1f, 1.5695f, -0.0041f}, {}, {0.3f, 1.4f, 3.8f}},
        {{-2.1f, 1.5695f, -0.0041f}, {}, {0.3f, 1.4f, 3.8f}},
        {{2.2f, 1.5695f, -0.0041f}, {}, {0.405f, 1.26f, 3.38f}},
        {{-2.2f, 1.5695f, -0.0041f}, {}, {0.405f, 1.26f, 3.38f}},
        {{-2.0f, 0.5695f, 1.0259f}, {}, {0.3f, 1.4f, 1.33f}},
        {{2.0f, 0.5695f, 1.0259f}, {}, {0.3f, 1.4f, 1.33f}}
    }};

    inline static constexpr Stage5PartTransform TowerHead {
        {0.0f, 2.1055f, 0.2291f}};
    inline static constexpr Stage5PartTransform MechaHead {
        {0.0f, 16.0f, 0.0f}};

    // FBXにないドリル、攻略用弱点、開閉装甲、発光部だけを追加Primitiveで補う
    inline static constexpr std::array<TayamaPart, 35> Parts {{
        {PrimitiveShape::Box, {{0.0f, -1.0f, 0.5f}, {}, {5.8f, 1.2f, 3.2f}}, {{0.0f, 0.4f, 0.0f}, {}, {6.4f, 1.5f, 3.6f}}, TayamaPartGroup::CentralHull, Hull},
        {PrimitiveShape::Box, {{0.0f, 1.5f, 0.2f}, {}, {1.2f, 1.0f, 1.2f}}, {{0.0f, 12.85f, 0.0f}, {}, {1.5f, 1.4f, 1.6f}}, TayamaPartGroup::Bridge, Dark},

        {PrimitiveShape::Cylinder, {{-5.0f, -2.2f, -1.5f}, {}, {4.5f, 0.55f, 4.5f}}, {{-8.1f, 10.1f, 0.0f}, {0.0f, 0.0f, Math::HalfPi}, {1.4f, 1.8f, 1.4f}}, TayamaPartGroup::RightFlightDeck, Dark},
        {PrimitiveShape::Cylinder, {{5.0f, -2.2f, -1.5f}, {}, {4.5f, 0.55f, 4.5f}}, {{8.1f, 10.1f, 0.0f}, {0.0f, 0.0f, Math::HalfPi}, {1.4f, 1.8f, 1.4f}}, TayamaPartGroup::LeftFlightDeck, Dark},
        {PrimitiveShape::Cone, {{-0.8f, 7.2f, 0.0f}, {}, {0.08f, 0.08f, 0.08f}}, {{-10.593842f, -1.365602f, 0.0f}, {0.0f, 0.0f, Math::Pi - 0.27f}, {2.7f, 8.0f, 2.7f}}, TayamaPartGroup::RightFlightDeck, LightArmor},
        {PrimitiveShape::Cone, {{0.8f, 7.2f, 0.0f}, {}, {0.08f, 0.08f, 0.08f}}, {{10.593842f, -1.365602f, 0.0f}, {0.0f, 0.0f, -Math::Pi + 0.27f}, {2.7f, 8.0f, 2.7f}}, TayamaPartGroup::LeftFlightDeck, LightArmor},
        {PrimitiveShape::Cylinder, {{-5.0f, -10.4f, -1.5f}, {}, {4.6f, 0.55f, 4.6f}}, {{-9.532584f, 2.469002f, 0.0f}, {0.0f, 0.0f, -0.27f}, {4.7f, 0.65f, 4.7f}}, TayamaPartGroup::RightFlightDeck, Armor},
        {PrimitiveShape::Cylinder, {{5.0f, -10.4f, -1.5f}, {}, {4.6f, 0.55f, 4.6f}}, {{9.532584f, 2.469002f, 0.0f}, {0.0f, 0.0f, 0.27f}, {4.7f, 0.65f, 4.7f}}, TayamaPartGroup::LeftFlightDeck, Armor},

        {PrimitiveShape::Cylinder, {{1.6f, 4.8f, -3.0f}, {Math::HalfPi, 0.0f, 0.0f}, {0.8f, 0.7f, 0.8f}}, {{4.8f, 10.7f, -2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}}, TayamaPartGroup::LeftSearchlight, Dark},
        {PrimitiveShape::Cone, {{1.6f, 4.8f, -3.1f}, {-Math::HalfPi, 0.0f, 0.0f}, {0.7f, 1.0f, 0.7f}}, {{4.8f, 10.7f, -3.2f}, {-Math::HalfPi, 0.0f, 0.0f}, {0.8f, 1.2f, 0.8f}}, TayamaPartGroup::LeftSearchlight, Warning},
        {PrimitiveShape::Cylinder, {{-1.6f, 4.8f, -3.0f}, {Math::HalfPi, 0.0f, 0.0f}, {0.8f, 0.7f, 0.8f}}, {{-4.8f, 10.7f, -2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}}, TayamaPartGroup::RightSearchlight, Dark},
        {PrimitiveShape::Cone, {{-1.6f, 4.8f, -3.1f}, {-Math::HalfPi, 0.0f, 0.0f}, {0.7f, 1.0f, 0.7f}}, {{-4.8f, 10.7f, -3.2f}, {-Math::HalfPi, 0.0f, 0.0f}, {0.8f, 1.2f, 0.8f}}, TayamaPartGroup::RightSearchlight, Warning},

        {PrimitiveShape::Cylinder, {{0.0f, 6.4f, 3.7f}, {}, {0.45f, 2.2f, 0.45f}}, {{0.0f, 12.2f, -2.5f}, {Math::HalfPi, 0.0f, 0.0f}, {0.5f, 1.4f, 0.5f}}, TayamaPartGroup::FireControlRadar, Dark},
        {PrimitiveShape::Prism, {{0.0f, 7.6f, 3.7f}, {Math::HalfPi, 0.0f, 0.0f}, {2.2f, 0.3f, 1.4f}}, {{0.0f, 12.8f, -2.7f}, {Math::HalfPi, 0.0f, 0.0f}, {2.5f, 0.3f, 1.5f}}, TayamaPartGroup::FireControlRadar, LightArmor},
        {PrimitiveShape::Sphere, {{0.0f, 7.7f, 3.2f}, {}, {0.55f, 0.55f, 0.55f}}, {{0.0f, 12.2f, -3.25f}, {}, {0.7f, 0.7f, 0.7f}}, TayamaPartGroup::FireControlRadar, Warning},

        {PrimitiveShape::Cylinder, {{2.1f, -8.0f, 0.5f}, {}, {1.3f, 1.8f, 1.3f}}, {{3.0f, -6.8f, 1.8f}, {Math::HalfPi, 0.0f, 0.0f}, {1.25f, 1.6f, 1.25f}}, TayamaPartGroup::LeftLiftEngine, Dark},
        {PrimitiveShape::Cone, {{2.1f, -9.2f, 0.5f}, {0.0f, 0.0f, Math::Pi}, {0.9f, 1.2f, 0.9f}}, {{3.0f, -6.8f, 2.9f}, {Math::HalfPi, 0.0f, 0.0f}, {0.9f, 1.1f, 0.9f}}, TayamaPartGroup::LeftLiftEngine, Warning},
        {PrimitiveShape::Cylinder, {{-2.1f, -8.0f, 0.5f}, {}, {1.3f, 1.8f, 1.3f}}, {{-3.0f, -6.8f, 1.8f}, {Math::HalfPi, 0.0f, 0.0f}, {1.25f, 1.6f, 1.25f}}, TayamaPartGroup::RightLiftEngine, Dark},
        {PrimitiveShape::Cone, {{-2.1f, -9.2f, 0.5f}, {0.0f, 0.0f, Math::Pi}, {0.9f, 1.2f, 0.9f}}, {{-3.0f, -6.8f, 2.9f}, {Math::HalfPi, 0.0f, 0.0f}, {0.9f, 1.1f, 0.9f}}, TayamaPartGroup::RightLiftEngine, Warning},

        {PrimitiveShape::Sphere, {{0.0f, -1.0f, -3.25f}, {}, {1.4f, 1.4f, 0.8f}}, {{0.0f, 5.4f, -2.75f}, {}, {1.8f, 1.8f, 0.9f}}, TayamaPartGroup::CommandCore, Core},
        {PrimitiveShape::Box, {{0.0f, -1.0f, -3.0f}, {}, {2.6f, 2.6f, 0.35f}}, {{0.0f, 5.4f, -2.45f}, {}, {3.0f, 3.0f, 0.35f}}, TayamaPartGroup::CommandCore, Dark},

        {PrimitiveShape::Box, {{-1.2f, -1.0f, -3.4f}, {}, {1.1f, 3.0f, 0.3f}}, {{-1.2f, 5.4f, -3.0f}, {}, {1.15f, 3.2f, 0.3f}}, TayamaPartGroup::ArmorPanel, Armor},
        {PrimitiveShape::Box, {{1.2f, -1.0f, -3.4f}, {}, {1.1f, 3.0f, 0.3f}}, {{1.2f, 5.4f, -3.0f}, {}, {1.15f, 3.2f, 0.3f}}, TayamaPartGroup::ArmorPanel, Armor},
        {PrimitiveShape::Box, {{0.0f, 0.7f, -3.4f}, {}, {3.4f, 0.45f, 0.3f}}, {{0.0f, 7.05f, -3.0f}, {}, {3.5f, 0.5f, 0.3f}}, TayamaPartGroup::ArmorPanel, LightArmor},
        {PrimitiveShape::Box, {{0.0f, -2.7f, -3.4f}, {}, {3.4f, 0.45f, 0.3f}}, {{0.0f, 3.75f, -3.0f}, {}, {3.5f, 0.5f, 0.3f}}, TayamaPartGroup::ArmorPanel, LightArmor},

        {PrimitiveShape::Box, {{0.0f, -3.0f, -2.8f}, {}, {4.8f, 0.65f, 0.35f}}, {{0.0f, -0.4f, -2.0f}, {}, {5.6f, 0.7f, 0.35f}}, TayamaPartGroup::Hangar, Window},

        {PrimitiveShape::Cylinder, {{0.0f, -8.5f, 3.8f}, {Math::HalfPi, 0.0f, 0.0f}, {2.2f, 2.8f, 2.2f}}, {{0.0f, 4.0f, 2.8f}, {Math::HalfPi, 0.0f, 0.0f}, {2.2f, 3.0f, 2.2f}}, TayamaPartGroup::MainThruster, Dark},
        {PrimitiveShape::Cylinder, {{2.2f, -8.0f, 4.2f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 2.0f, 1.0f}}, {{2.6f, 2.5f, 2.8f}, {Math::HalfPi, 0.0f, 0.0f}, {1.2f, 2.4f, 1.2f}}, TayamaPartGroup::MainThruster, Warning},
        {PrimitiveShape::Cylinder, {{-2.2f, -8.0f, 4.2f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 2.0f, 1.0f}}, {{-2.6f, 2.5f, 2.8f}, {Math::HalfPi, 0.0f, 0.0f}, {1.2f, 2.4f, 1.2f}}, TayamaPartGroup::MainThruster, Warning},

        {PrimitiveShape::Box, {{-2.8f, -0.2f, -3.45f}, {}, {0.14f, 4.4f, 0.12f}}, {{-2.0f, 6.0f, -2.85f}, {}, {0.14f, 8.0f, 0.12f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{2.8f, -0.2f, -3.45f}, {}, {0.14f, 4.4f, 0.12f}}, {{2.0f, 6.0f, -2.85f}, {}, {0.14f, 8.0f, 0.12f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{0.0f, 1.0f, -3.45f}, {}, {5.8f, 0.14f, 0.12f}}, {{0.0f, 2.5f, -2.87f}, {}, {4.6f, 0.14f, 0.12f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{0.0f, 2.4f, -3.45f}, {}, {5.8f, 0.14f, 0.12f}}, {{0.0f, 5.0f, -2.87f}, {}, {4.6f, 0.14f, 0.12f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{0.0f, 3.8f, -3.45f}, {}, {5.8f, 0.14f, 0.12f}}, {{0.0f, 7.5f, -2.87f}, {}, {4.6f, 0.14f, 0.12f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{0.0f, 5.2f, -3.45f}, {}, {5.8f, 0.14f, 0.12f}}, {{0.0f, 10.0f, -2.87f}, {}, {8.4f, 0.14f, 0.12f}}, TayamaPartGroup::RunwayLight, Runway}
    }};

    static constexpr std::size_t BuildingPrimitiveCount =
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::BuildingLeft) +
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::BuildingRight) +
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::TowerSub) +
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::TowerSubSmall) +
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::BuildingLeftSmall) +
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::BuildingRightSmall) +
        Stage5CityModelView::PrimitiveCount(Stage5BuildingType::Tower) * 2;
    static constexpr std::size_t PrimitiveCount =
        Parts.size() + HeadParts.size() + BuildingPrimitiveCount;

    /**
     * @brief 一つのTAYAMAパーツをビル形態から巨大メカ形態へ補間する
     * @param part 補間する同一パーツ
     * @param progress 0がビル、1が巨大メカとなる変形率
     * @return 補間済みローカルTransform
     */
    static constexpr Stage5PartTransform InterpolatePart(const TayamaPart& part, float progress) {
        return Stage5ModelDetail::Lerp(part.tower, part.mecha, progress);
    }

    /**
     * @brief Blender頭部の部位番号から外装色を取得する
     * @param index HeadParts内の部位番号
     * @return 顔、髪、眼鏡、目を区別する色
     */
    static constexpr ColorF HeadPartColor(std::size_t index) {
        if (index == 11 || index == 12) return Runway;
        if (index == 8 || (index >= 20 && index <= 27)) return Dark;
        if ((index >= 14 && index <= 19) || index >= 28) return Hull;
        if (index == 7 || index == 13) return LightArmor;
        return Armor;
    }

    /**
     * @brief ビル形態を指定した外形と接地位置へ合わせるルート行列を生成する
     * @param centerX 配置する外形中央のX座標
     * @param bottomY 配置する外形底面のY座標
     * @param centerZ 配置する外形中央のZ座標
     * @param size 配置する外形の幅、高さ、奥行き
     * @return 非均一拡縮と位置補正を含むルート行列
     */
    static Matrix4x4 BuildingRoot(float centerX, float bottomY, float centerZ,
        const Vector3& size) {
        const Vector3 scale {
            size.x / TowerSize.x,
            size.y / TowerSize.y,
            size.z / TowerSize.z
        };
        const Vector3 localCenter = (TowerBoundsMin + TowerBoundsMax) * 0.5f;
        return Matrix4x4::Translation({
            centerX - localCenter.x * scale.x,
            bottomY - TowerBoundsMin.y * scale.y,
            centerZ - localCenter.z * scale.z
        }) * Matrix4x4::Scale(scale);
    }

    /**
     * @brief 表示中の全パーツを任意のルート行列からワールド行列へ解決する
     * @param root モデル全体のルート行列
     * @param progress 0がビル、1が巨大メカとなる変形率
     * @param state グループの表示、破壊、崩壊状態
     * @param drawPart 形状、ワールド行列、色、グループを受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitParts(const Matrix4x4& root, float progress,
        const TayamaModelState& state, DrawPart&& drawPart) {
        // 添付FBXと同じビル部品を形態間で移動回転し、格子と積層外装ごと列挙する
        for (const TayamaBuildingPart& building : BuildingParts) {
            if (!state.IsVisible(building.group)) continue;
            const std::size_t group = static_cast<std::size_t>(building.group);
            const Stage5PartTransform local =
                Stage5ModelDetail::Lerp(building.tower, building.mecha, progress);
            const Matrix4x4 buildingRoot = root *
                Stage5ModelDetail::Matrix(state.collapseOffsets[group]) *
                Stage5ModelDetail::Matrix(local);
            const bool hit = state.hitFlash[group];
            Stage5CityModelView::VisitBuilding(building.type, buildingRoot,
                [&](PrimitiveShape shape, const Matrix4x4& world, const ColorF& color) {
                    drawPart(shape, world, hit ? Hit : color, building.group);
                });
        }

        // 同梱Blenderデータの頭部を一体のBridgeグループとして列挙する
        if (state.IsVisible(TayamaPartGroup::Bridge)) {
            constexpr TayamaPartGroup HeadGroup = TayamaPartGroup::Bridge;
            const std::size_t group = static_cast<std::size_t>(HeadGroup);
            const Stage5PartTransform local =
                Stage5ModelDetail::Lerp(TowerHead, MechaHead, progress);
            const Matrix4x4 headRoot = root *
                Stage5ModelDetail::Matrix(state.collapseOffsets[group]) *
                Stage5ModelDetail::Matrix(local);
            for (std::size_t index = 0; index < HeadParts.size(); ++index) {
                const ColorF color = state.hitFlash[group] ? Hit : HeadPartColor(index);
                drawPart(PrimitiveShape::Box,
                    headRoot * Stage5ModelDetail::Matrix(HeadParts[index]), color, HeadGroup);
            }
        }

        // FBXにない攻略部品へグループ別の崩壊Transformを追加する
        for (const TayamaPart& part : Parts) {
            if (!state.IsVisible(part.group)) continue;
            const std::size_t group = static_cast<std::size_t>(part.group);
            const Stage5PartTransform local = InterpolatePart(part, progress);
            const Matrix4x4 world = root * Stage5ModelDetail::Matrix(state.collapseOffsets[group]) *
                Stage5ModelDetail::Matrix(local);
            const ColorF color = state.hitFlash[group] ? Hit : part.color;
            drawPart(part.shape, world, color, part.group);
        }
    }

    /**
     * @brief 表示中の全パーツをモデルTransformからワールド行列へ解決する
     * @param transform モデル全体のTransform
     * @param progress 0がビル、1が巨大メカとなる変形率
     * @param state グループの表示、破壊、崩壊状態
     * @param drawPart 形状、ワールド行列、色、グループを受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitParts(const Stage5ModelTransform& transform, float progress,
        const TayamaModelState& state, DrawPart&& drawPart) {
        VisitParts(Stage5ModelDetail::Matrix(transform), progress, state,
            drawPart);
    }

    /**
     * @brief 描画パーツと同じ補間済みワールド行列からグループ境界を求める
     * @param transform モデル全体のTransform
     * @param progress 0がビル、1が巨大メカとなる変形率
     * @param state グループの表示、破壊、崩壊状態
     * @param group 境界を求めるグループ
     * @return 表示パーツがない場合はvalidがfalseの球形境界
     */
    static Stage5GroupBounds GroupBounds(const Stage5ModelTransform& transform, float progress,
        const TayamaModelState& state, TayamaPartGroup group) {
        Stage5GroupBounds bounds;
        VisitParts(transform, progress, state,
            [&](PrimitiveShape, const Matrix4x4& world, const ColorF&, TayamaPartGroup partGroup) {
                if (partGroup != group) return;
                Stage5ModelDetail::Include(bounds, world.TransformPoint(Vector3::Zero),
                    Stage5ModelDetail::WorldPartRadius(world));
            });
        return bounds;
    }
};

namespace Stage5ModelChecks {
/**
 * @brief 配列内にある指定EASTSOURCEグループのパーツ数を数える
 * @param group 数えるグループ
 * @return グループに属するパーツ数
 */
constexpr std::size_t EastsourceGroupSize(EastsourcePartGroup group) {
    std::size_t count = 0;
    for (const EastsourcePart& part : EastsourceModelView::Parts) {
        if (part.group == group) ++count;
    }
    return count;
}

/**
 * @brief 全TAYAMAグループに最低一つの共有変形パーツがあるか確認する
 * @return 全グループが空でなければtrue、空のグループがあればfalse
 */
constexpr bool TayamaHasEveryGroup() {
    std::array<std::size_t, TayamaPartGroupCount> counts {};
    for (const TayamaPart& part : TayamaModelView::Parts) {
        ++counts[static_cast<std::size_t>(part.group)];
    }
    for (std::size_t count : counts) {
        if (count == 0) return false;
    }
    return true;
}
}

static_assert(EastsourceModelView::Parts.size() == EastsourceModelView::PrimitiveCount);
static_assert(Stage5ModelChecks::EastsourceGroupSize(EastsourcePartGroup::Body) == 15);
static_assert(Stage5ModelChecks::EastsourceGroupSize(EastsourcePartGroup::Nose) == 3);
static_assert(Stage5ModelChecks::EastsourceGroupSize(EastsourcePartGroup::LeftWing) == 2);
static_assert(Stage5ModelChecks::EastsourceGroupSize(EastsourcePartGroup::RightWing) == 2);
static_assert(Stage5ModelChecks::EastsourceGroupSize(EastsourcePartGroup::LeftEngine) == 2);
static_assert(Stage5ModelChecks::EastsourceGroupSize(EastsourcePartGroup::RightEngine) == 2);
static_assert(EastsourceModelView::Parts.front().local.position == Vector3 {0.0f, 0.75f, -3.5f});
static_assert(EastsourceModelView::Parts.back().local.scale == Vector3 {0.5f, 0.5f, 0.5f});
static_assert(TayamaModelView::Parts.size() + TayamaModelView::HeadParts.size() +
    TayamaModelView::BuildingPrimitiveCount == TayamaModelView::PrimitiveCount);
static_assert(TayamaModelView::PrimitiveCount == 217);
static_assert(Stage5ModelChecks::TayamaHasEveryGroup());
static_assert(TayamaModelView::InterpolatePart(TayamaModelView::Parts.front(), 0.0f).position ==
    TayamaModelView::Parts.front().tower.position);
static_assert(TayamaModelView::InterpolatePart(TayamaModelView::Parts.front(), 1.0f).position ==
    TayamaModelView::Parts.front().mecha.position);
static_assert(EastsourceModelView::PrimitiveCount + TayamaModelView::PrimitiveCount < 256);
