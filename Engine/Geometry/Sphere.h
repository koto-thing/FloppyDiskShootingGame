#pragma once
#include "../Math/Vector3.h"
struct Box;

/** @brief 3次元空間の球 */
struct Sphere {
    Vector3 center = Vector3::Zero;
    float radius = 0.0f;
    /** @brief 点が球の内部または境界上にあるか判定する */
    bool Contains(const Vector3& point) const;
    /** @brief 球同士の交差を判定する */
    bool Intersects(const Sphere& other) const;
    /** @brief 球と境界箱の交差を判定する */
    bool Intersects(const Box& other) const;
};
