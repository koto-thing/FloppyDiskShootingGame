#pragma once
#include "../Math/Vector3.h"
struct Box;
struct Sphere {
    Vector3 center = Vector3::Zero;
    float radius = 0.0f;
    bool Contains(const Vector3& point) const;
    bool Intersects(const Sphere& other) const;
    bool Intersects(const Box& other) const;
};
