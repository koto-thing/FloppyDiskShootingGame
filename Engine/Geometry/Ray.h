#pragma once
#include "../Math/Vector3.h"
#include "RayHit.h"
struct Sphere;
struct Box;
struct Ray {
    Vector3 origin = Vector3::Zero;
    Vector3 direction = Vector3::Forward;
    Ray() = default;
    Ray(const Vector3& origin, const Vector3& direction);
    static Ray Invalid() { return {{}, {}}; }
    Vector3 PointAt(float distance) const { return origin + direction * distance; }
    bool Intersects(const Sphere& sphere, RayHit* hit = nullptr) const;
    bool Intersects(const Box& box, RayHit* hit = nullptr) const;
};
