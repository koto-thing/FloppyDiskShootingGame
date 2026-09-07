#include "Sphere.h"
#include "Box.h"
#include <cmath>
/** @brief 点が球の内部または境界上にあるか判定する */
bool Sphere::Contains(const Vector3& point) const { const float r = std::abs(radius); return (point - center).LengthSquared() <= r * r; }
/** @brief 球同士の交差を判定する */
bool Sphere::Intersects(const Sphere& other) const { const float r = std::abs(radius) + std::abs(other.radius); return (other.center - center).LengthSquared() <= r * r; }
/** @brief 球と境界箱の交差を判定する */
bool Sphere::Intersects(const Box& other) const { return other.Intersects(*this); }
