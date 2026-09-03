#pragma once

#include <algorithm>
#include <cmath>

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
    if (progress <= 0.0f) return from;
    if (progress >= 1.0f) return to;
    return {
        Vector3::Lerp(from.position, to.position, progress),
        Vector3::Lerp(from.rotation, to.rotation, progress),
        Vector3::Lerp(from.scale, to.scale, progress)
    };
}
}
