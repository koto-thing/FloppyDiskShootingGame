#pragma once
#include "../Math/Vector3.h"
struct Sphere;

/** @brief 3次元空間の軸平行境界箱 */
struct Box {
    Vector3 center = Vector3::Zero;
    Vector3 size = Vector3::Zero;
    /** @brief 各軸の半サイズを取得する */
    Vector3 HalfSize() const;
    /** @brief 境界箱の最小座標を取得する */
    Vector3 Min() const;
    /** @brief 境界箱の最大座標を取得する */
    Vector3 Max() const;
    /** @brief 点が境界箱の内部または境界上にあるか判定する */
    bool Contains(const Vector3& point) const;
    /** @brief 境界箱同士の交差を判定する */
    bool Intersects(const Box& other) const;
    /** @brief 境界箱と球の交差を判定する */
    bool Intersects(const Sphere& sphere) const;
};
