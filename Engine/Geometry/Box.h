#pragma once
#include "../Math/Vector3.h"
struct Sphere;
struct Box {
    Vector3 center = Vector3::Zero;
    Vector3 size = Vector3::Zero;
    Vector3 HalfSize() const;
    Vector3 Min() const;
    Vector3 Max() const;
    bool Contains(const Vector3& point) const;
    bool Intersects(const Box& other) const;
    bool Intersects(const Sphere& sphere) const;
};
