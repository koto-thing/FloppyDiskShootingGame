#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "../../../../Engine/Graphics/IRenderBackend.h"
#include "../../../../Engine/Math/Math.h"

/** @brief Stage5モデルのローカル位置、XYZ回転、寸法を表す */
struct Stage5PartTransform {
    Vector3 position {};
    Vector3 rotation {};
    Vector3 scale {Vector3::One};
};

/** @brief Stage5モデル全体の位置、XYZ回転、均一倍率を表す */
struct Stage5ModelTransform {
    Vector3 position {};
    Vector3 rotation {};
    float scale = 1.0f;
};

/** @brief 同一グループの描画パーツから求めた球形境界を表す */
struct Stage5GroupBounds {
    Vector3 center {};
    float radius = 0.0f;
    bool valid = false;
};

namespace Stage5ModelDetail {
/**
 * @brief XYZ回転を参照モデルと同じ順序で合成する
 * @param rotation XYZ軸の回転角度
 * @return Y、X、Z軸の順に乗算した回転行列
 */
inline Matrix4x4 Rotation(const Vector3& rotation) {
    return Matrix4x4::RotationY(rotation.y) *
        Matrix4x4::RotationX(rotation.x) *
        Matrix4x4::RotationZ(rotation.z);
}

/**
 * @brief ローカルTransformをT、Ry、Rx、Rz、Sの順序で行列化する
 * @param transform 行列化するローカルTransform
 * @return ローカル変換行列
 */
inline Matrix4x4 Matrix(const Stage5PartTransform& transform) {
    return Matrix4x4::Translation(transform.position) *
        Rotation(transform.rotation) * Matrix4x4::Scale(transform.scale);
}

/**
 * @brief モデル全体のTransformをT、Ry、Rx、Rz、Sの順序で行列化する
 * @param transform 行列化するモデルTransform
 * @return モデル全体の変換行列
 */
inline Matrix4x4 Matrix(const Stage5ModelTransform& transform) {
    return Matrix4x4::Translation(transform.position) *
        Rotation(transform.rotation) *
        Matrix4x4::Scale({transform.scale, transform.scale, transform.scale});
}

/**
 * @brief 変換済み単位プリミティブを包む球の半径を求める
 * @param world 描画にも使用するワールド行列
 * @return 単位立方体の8頂点を包む半径
 */
inline float WorldPartRadius(const Matrix4x4& world) {
    float radiusSquared = 0.0f;

    // 回転や非均一拡縮を含む8頂点から保守的な半径を求める
    for (float x : {-0.5f, 0.5f}) {
        for (float y : {-0.5f, 0.5f}) {
            for (float z : {-0.5f, 0.5f}) {
                radiusSquared = (std::max)(radiusSquared,
                    world.TransformVector({x, y, z}).LengthSquared());
            }
        }
    }
    return std::sqrt(radiusSquared);
}

/**
 * @brief 既存の球形境界へ別の球を統合する
 * @param bounds 更新する球形境界
 * @param center 追加する球の中心
 * @param radius 追加する球の半径
 * @return なし
 */
inline void Include(Stage5GroupBounds& bounds, const Vector3& center, float radius) {
    if (!bounds.valid) {
        bounds = {center, radius, true};
        return;
    }

    // 一方が他方を包含する場合は大きい球をそのまま使う
    const Vector3 delta = center - bounds.center;
    const float distance = delta.Length();
    if (bounds.radius >= distance + radius) return;
    if (radius >= distance + bounds.radius) {
        bounds = {center, radius, true};
        return;
    }

    // 二つの球を結ぶ直線上へ最小の包含球を広げる
    const float newRadius = (distance + bounds.radius + radius) * 0.5f;
    if (distance > Math::Epsilon) {
        bounds.center += delta * ((newRadius - bounds.radius) / distance);
    }
    bounds.radius = newRadius;
}

/**
 * @brief 二つのパーツTransformを線形補間する
 * @param from 補間開始Transform
 * @param to 補間終了Transform
 * @param progress 0から1の補間率
 * @return 補間済みTransform
 */
constexpr Stage5PartTransform Lerp(const Stage5PartTransform& from,
    const Stage5PartTransform& to, float progress) {
    const float amount = Math::Clamp01(progress);
    return {
        Vector3::Lerp(from.position, to.position, amount),
        Vector3::Lerp(from.rotation, to.rotation, amount),
        Vector3::Lerp(from.scale, to.scale, amount)
    };
}
}

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

/** @brief 同じパーツ群を補間してビルから巨大メカへ変形するTAYAMAモデル */
class TayamaModelView {
public:
    static constexpr std::size_t PrimitiveCount = 46;
    inline static constexpr Vector3 TowerBoundsMin {-4.8f, -11.0f, -3.7f};
    inline static constexpr Vector3 TowerBoundsMax {4.8f, 23.2f, 3.5f};
    inline static constexpr Vector3 TowerSize {9.6f, 34.2f, 7.2f};
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

    // 各要素はビルと巨大メカで同じパーツを表し、単純なモデル差し替えを行わない
    inline static constexpr std::array<TayamaPart, PrimitiveCount> Parts {{
        // 中央ビルを胸部、腰部、背部フレームへ畳み直す
        {PrimitiveShape::Box, {{0.0f, 2.0f, 0.0f}, {}, {6.0f, 8.0f, 6.0f}}, {{0.0f, 2.4f, 0.0f}, {}, {7.0f, 6.8f, 4.0f}}, TayamaPartGroup::CentralHull, Hull},
        {PrimitiveShape::Box, {{0.0f, -3.0f, 0.0f}, {}, {7.0f, 3.0f, 7.0f}}, {{0.0f, -1.7f, 0.2f}, {}, {5.2f, 2.4f, 3.4f}}, TayamaPartGroup::CentralHull, Armor},
        {PrimitiveShape::Prism, {{0.0f, 7.0f, 0.0f}, {}, {5.0f, 3.0f, 5.0f}}, {{0.0f, 3.6f, -2.15f}, {}, {5.6f, 3.0f, 1.3f}}, TayamaPartGroup::CentralHull, LightArmor},
        {PrimitiveShape::Prism, {{0.0f, -5.0f, 0.0f}, {0.0f, Math::Pi, 0.0f}, {6.0f, 2.0f, 6.0f}}, {{0.0f, -0.2f, -1.75f}, {0.0f, Math::Pi, 0.0f}, {4.4f, 2.5f, 1.2f}}, TayamaPartGroup::CentralHull, Armor},
        {PrimitiveShape::Cylinder, {{0.0f, -7.0f, 0.0f}, {}, {4.0f, 3.0f, 4.0f}}, {{0.0f, 6.0f, 0.0f}, {}, {2.2f, 1.3f, 2.2f}}, TayamaPartGroup::CentralHull, Dark},
        {PrimitiveShape::Box, {{0.0f, 11.0f, 0.0f}, {}, {2.0f, 5.0f, 2.0f}}, {{0.0f, 2.0f, 2.2f}, {}, {3.0f, 7.0f, 1.2f}}, TayamaPartGroup::CentralHull, LightArmor},

        // 左右外壁を肩、上腕、前腕へ分割する
        {PrimitiveShape::Box, {{3.6f, 2.0f, 0.0f}, {}, {0.8f, 10.0f, 6.0f}}, {{5.0f, 3.8f, 0.0f}, {}, {3.2f, 2.8f, 3.2f}}, TayamaPartGroup::LeftFlightDeck, Armor},
        {PrimitiveShape::Box, {{4.4f, 7.0f, 0.0f}, {}, {0.8f, 4.0f, 5.0f}}, {{6.4f, 0.8f, 0.0f}, {0.0f, 0.0f, -0.08f}, {2.2f, 4.2f, 2.2f}}, TayamaPartGroup::LeftFlightDeck, LightArmor},
        {PrimitiveShape::Prism, {{3.8f, -3.0f, 0.0f}, {}, {1.2f, 3.0f, 6.0f}}, {{6.8f, -2.4f, -0.5f}, {0.0f, 0.0f, -0.10f}, {2.8f, 3.2f, 3.0f}}, TayamaPartGroup::LeftFlightDeck, Hull},
        {PrimitiveShape::Box, {{-3.6f, 2.0f, 0.0f}, {}, {0.8f, 10.0f, 6.0f}}, {{-5.0f, 3.8f, 0.0f}, {}, {3.2f, 2.8f, 3.2f}}, TayamaPartGroup::RightFlightDeck, Armor},
        {PrimitiveShape::Box, {{-4.4f, 7.0f, 0.0f}, {}, {0.8f, 4.0f, 5.0f}}, {{-6.4f, 0.8f, 0.0f}, {0.0f, 0.0f, 0.08f}, {2.2f, 4.2f, 2.2f}}, TayamaPartGroup::RightFlightDeck, LightArmor},
        {PrimitiveShape::Prism, {{-3.8f, -3.0f, 0.0f}, {}, {1.2f, 3.0f, 6.0f}}, {{-6.8f, -2.4f, -0.5f}, {0.0f, 0.0f, 0.10f}, {2.8f, 3.2f, 3.0f}}, TayamaPartGroup::RightFlightDeck, Hull},

        // 上層塔を頭部と発光バイザーへ圧縮する
        {PrimitiveShape::Box, {{0.0f, 14.0f, 0.0f}, {}, {4.0f, 3.0f, 4.0f}}, {{0.0f, 7.4f, 0.0f}, {}, {4.0f, 2.4f, 3.4f}}, TayamaPartGroup::Bridge, Armor},
        {PrimitiveShape::Box, {{0.0f, 16.2f, 0.0f}, {}, {3.0f, 1.5f, 3.0f}}, {{0.0f, 8.7f, 0.0f}, {}, {3.2f, 1.2f, 2.8f}}, TayamaPartGroup::Bridge, LightArmor},
        {PrimitiveShape::Cylinder, {{0.0f, 18.0f, 0.0f}, {}, {1.0f, 2.0f, 1.0f}}, {{0.0f, 9.8f, 0.0f}, {}, {0.8f, 1.6f, 0.8f}}, TayamaPartGroup::Bridge, Dark},
        {PrimitiveShape::Box, {{0.0f, 19.3f, 0.0f}, {}, {2.5f, 0.4f, 2.5f}}, {{0.0f, 7.8f, -1.8f}, {}, {3.0f, 0.45f, 0.25f}}, TayamaPartGroup::Bridge, Window},

        // 左右サーチライトは基部と前方へ向く発光部を一緒に移動する
        {PrimitiveShape::Cylinder, {{2.3f, 18.0f, 0.8f}, {}, {1.0f, 0.6f, 1.0f}}, {{4.8f, 3.8f, -1.8f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.2f, 1.0f}}, TayamaPartGroup::LeftSearchlight, Dark},
        {PrimitiveShape::Cone, {{2.3f, 18.6f, -0.2f}, {-Math::HalfPi, 0.0f, 0.0f}, {0.9f, 1.4f, 0.9f}}, {{4.8f, 3.8f, -3.0f}, {-Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.8f, 1.0f}}, TayamaPartGroup::LeftSearchlight, Warning},
        {PrimitiveShape::Cylinder, {{-2.3f, 18.0f, 0.8f}, {}, {1.0f, 0.6f, 1.0f}}, {{-4.8f, 3.8f, -1.8f}, {Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.2f, 1.0f}}, TayamaPartGroup::RightSearchlight, Dark},
        {PrimitiveShape::Cone, {{-2.3f, 18.6f, -0.2f}, {-Math::HalfPi, 0.0f, 0.0f}, {0.9f, 1.4f, 0.9f}}, {{-4.8f, 3.8f, -3.0f}, {-Math::HalfPi, 0.0f, 0.0f}, {1.0f, 1.8f, 1.0f}}, TayamaPartGroup::RightSearchlight, Warning},

        // 火器管制レーダーを胸部中央へ露出させる
        {PrimitiveShape::Cylinder, {{0.0f, 20.5f, 0.0f}, {}, {0.5f, 3.0f, 0.5f}}, {{0.0f, 4.2f, -2.35f}, {Math::HalfPi, 0.0f, 0.0f}, {0.55f, 1.4f, 0.55f}}, TayamaPartGroup::FireControlRadar, Dark},
        {PrimitiveShape::Prism, {{0.0f, 22.2f, 0.0f}, {Math::HalfPi, 0.0f, 0.0f}, {3.0f, 0.4f, 2.0f}}, {{0.0f, 5.0f, -2.45f}, {Math::HalfPi, 0.0f, 0.0f}, {2.8f, 0.35f, 1.8f}}, TayamaPartGroup::FireControlRadar, LightArmor},
        {PrimitiveShape::Sphere, {{0.0f, 22.5f, 0.0f}, {}, {0.65f, 0.65f, 0.65f}}, {{0.0f, 4.2f, -3.1f}, {}, {0.75f, 0.75f, 0.75f}}, TayamaPartGroup::FireControlRadar, Warning},

        // 下部機関を左右の脚部と足部へ展開する
        {PrimitiveShape::Cylinder, {{2.8f, -6.0f, 0.0f}, {}, {2.0f, 3.0f, 2.0f}}, {{2.5f, -5.0f, 0.2f}, {}, {2.8f, 6.0f, 2.8f}}, TayamaPartGroup::LeftLiftEngine, Dark},
        {PrimitiveShape::Cone, {{2.8f, -8.0f, 0.0f}, {0.0f, 0.0f, Math::Pi}, {1.4f, 2.0f, 1.4f}}, {{2.5f, -8.8f, -1.0f}, {0.0f, 0.0f, Math::Pi}, {3.4f, 2.0f, 4.8f}}, TayamaPartGroup::LeftLiftEngine, Warning},
        {PrimitiveShape::Cylinder, {{-2.8f, -6.0f, 0.0f}, {}, {2.0f, 3.0f, 2.0f}}, {{-2.5f, -5.0f, 0.2f}, {}, {2.8f, 6.0f, 2.8f}}, TayamaPartGroup::RightLiftEngine, Dark},
        {PrimitiveShape::Cone, {{-2.8f, -8.0f, 0.0f}, {0.0f, 0.0f, Math::Pi}, {1.4f, 2.0f, 1.4f}}, {{-2.5f, -8.8f, -1.0f}, {0.0f, 0.0f, Math::Pi}, {3.4f, 2.0f, 4.8f}}, TayamaPartGroup::RightLiftEngine, Warning},

        // コアと外枠を胸部正面へ移動する
        {PrimitiveShape::Sphere, {{0.0f, 12.0f, -3.2f}, {}, {2.0f, 2.0f, 1.0f}}, {{0.0f, 1.8f, -2.45f}, {}, {2.0f, 2.0f, 1.0f}}, TayamaPartGroup::CommandCore, Core},
        {PrimitiveShape::Box, {{0.0f, 12.0f, -3.0f}, {}, {3.0f, 3.0f, 0.35f}}, {{0.0f, 1.8f, -2.10f}, {}, {3.2f, 3.2f, 0.35f}}, TayamaPartGroup::CommandCore, Dark},

        // 外壁パネルを胸部シャッターと肩装甲へ再配置する
        {PrimitiveShape::Box, {{1.2f, 12.0f, -3.15f}, {}, {1.1f, 3.2f, 0.3f}}, {{1.35f, 1.8f, -2.65f}, {}, {1.2f, 3.2f, 0.3f}}, TayamaPartGroup::ArmorPanel, Armor},
        {PrimitiveShape::Box, {{-1.2f, 12.0f, -3.15f}, {}, {1.1f, 3.2f, 0.3f}}, {{-1.35f, 1.8f, -2.65f}, {}, {1.2f, 3.2f, 0.3f}}, TayamaPartGroup::ArmorPanel, Armor},
        {PrimitiveShape::Prism, {{0.0f, 4.0f, -3.2f}, {}, {5.0f, 4.0f, 0.5f}}, {{0.0f, -2.7f, -1.2f}, {}, {5.0f, 1.5f, 2.5f}}, TayamaPartGroup::ArmorPanel, LightArmor},
        {PrimitiveShape::Box, {{3.15f, 0.0f, 0.0f}, {}, {0.4f, 4.0f, 5.0f}}, {{4.0f, 4.1f, 0.2f}, {0.0f, 0.0f, -0.25f}, {0.5f, 2.2f, 4.5f}}, TayamaPartGroup::ArmorPanel, Armor},
        {PrimitiveShape::Box, {{-3.15f, 0.0f, 0.0f}, {}, {0.4f, 4.0f, 5.0f}}, {{-4.0f, 4.1f, 0.2f}, {0.0f, 0.0f, 0.25f}, {0.5f, 2.2f, 4.5f}}, TayamaPartGroup::ArmorPanel, Armor},

        // 窓列を腰部の発光ベルトと側面装甲へ集約する
        {PrimitiveShape::Box, {{0.0f, 0.0f, -3.15f}, {}, {3.2f, 2.0f, 0.25f}}, {{0.0f, -1.5f, -1.7f}, {}, {3.2f, 0.6f, 0.3f}}, TayamaPartGroup::Hangar, Window},
        {PrimitiveShape::Box, {{2.0f, -1.5f, -3.15f}, {}, {1.5f, 1.2f, 0.25f}}, {{3.2f, -1.7f, 0.0f}, {}, {1.6f, 1.4f, 3.0f}}, TayamaPartGroup::Hangar, Dark},
        {PrimitiveShape::Box, {{-2.0f, -1.5f, -3.15f}, {}, {1.5f, 1.2f, 0.25f}}, {{-3.2f, -1.7f, 0.0f}, {}, {1.6f, 1.4f, 3.0f}}, TayamaPartGroup::Hangar, Dark},

        // 地下推進設備を背部の三連スラスターへ回転する
        {PrimitiveShape::Cylinder, {{0.0f, -9.0f, 0.0f}, {}, {3.0f, 4.0f, 3.0f}}, {{0.0f, 1.0f, 3.0f}, {Math::HalfPi, 0.0f, 0.0f}, {2.6f, 3.6f, 2.6f}}, TayamaPartGroup::MainThruster, Dark},
        {PrimitiveShape::Cylinder, {{2.4f, -7.8f, 0.0f}, {}, {1.4f, 3.0f, 1.4f}}, {{2.8f, 0.2f, 2.7f}, {Math::HalfPi, 0.0f, 0.0f}, {1.6f, 3.0f, 1.6f}}, TayamaPartGroup::MainThruster, Warning},
        {PrimitiveShape::Cylinder, {{-2.4f, -7.8f, 0.0f}, {}, {1.4f, 3.0f, 1.4f}}, {{-2.8f, 0.2f, 2.7f}, {Math::HalfPi, 0.0f, 0.0f}, {1.6f, 3.0f, 1.6f}}, TayamaPartGroup::MainThruster, Warning},

        // ビルのネオン列を腕部と脚部の発光ラインへ変形する
        {PrimitiveShape::Box, {{3.2f, 2.0f, -3.15f}, {}, {0.16f, 4.0f, 0.16f}}, {{5.2f, 3.8f, -1.75f}, {}, {0.18f, 2.2f, 0.18f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{3.2f, 7.0f, -3.15f}, {}, {0.16f, 4.0f, 0.16f}}, {{6.6f, 0.6f, -1.25f}, {}, {0.18f, 3.2f, 0.18f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{3.2f, 12.0f, -3.15f}, {}, {0.16f, 4.0f, 0.16f}}, {{2.5f, -5.0f, -1.25f}, {}, {0.18f, 4.2f, 0.18f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{-3.2f, 2.0f, -3.15f}, {}, {0.16f, 4.0f, 0.16f}}, {{-5.2f, 3.8f, -1.75f}, {}, {0.18f, 2.2f, 0.18f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{-3.2f, 7.0f, -3.15f}, {}, {0.16f, 4.0f, 0.16f}}, {{-6.6f, 0.6f, -1.25f}, {}, {0.18f, 3.2f, 0.18f}}, TayamaPartGroup::RunwayLight, Runway},
        {PrimitiveShape::Box, {{-3.2f, 12.0f, -3.15f}, {}, {0.16f, 4.0f, 0.16f}}, {{-2.5f, -5.0f, -1.25f}, {}, {0.18f, 4.2f, 0.18f}}, TayamaPartGroup::RunwayLight, Runway}
    }};

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
        // 各グループの崩壊Transformを補間後の同一パーツへ追加する
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
static_assert(TayamaModelView::Parts.size() == TayamaModelView::PrimitiveCount);
static_assert(Stage5ModelChecks::TayamaHasEveryGroup());
static_assert(TayamaModelView::InterpolatePart(TayamaModelView::Parts.front(), 0.0f).position ==
    TayamaModelView::Parts.front().tower.position);
static_assert(TayamaModelView::InterpolatePart(TayamaModelView::Parts.front(), 1.0f).position ==
    TayamaModelView::Parts.front().mecha.position);
static_assert(EastsourceModelView::PrimitiveCount + TayamaModelView::PrimitiveCount < 100);
