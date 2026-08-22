#include "Box.h"
#include "Sphere.h"
#include <algorithm>
#include <cmath>
Vector3 Box::HalfSize() const { return {std::abs(size.x) * 0.5f, std::abs(size.y) * 0.5f, std::abs(size.z) * 0.5f}; }
Vector3 Box::Min() const { return center - HalfSize(); }
Vector3 Box::Max() const { return center + HalfSize(); }
bool Box::Contains(const Vector3& point) const { const auto a = Min(), b = Max(); return point.x >= a.x && point.x <= b.x && point.y >= a.y && point.y <= b.y && point.z >= a.z && point.z <= b.z; }
bool Box::Intersects(const Box& other) const { const auto a = Min(), b = Max(), c = other.Min(), d = other.Max(); return a.x <= d.x && b.x >= c.x && a.y <= d.y && b.y >= c.y && a.z <= d.z && b.z >= c.z; }
bool Box::Intersects(const Sphere& sphere) const { const auto a = Min(), b = Max(); const Vector3 p{std::clamp(sphere.center.x, a.x, b.x), std::clamp(sphere.center.y, a.y, b.y), std::clamp(sphere.center.z, a.z, b.z)}; const float r = std::abs(sphere.radius); return (sphere.center - p).LengthSquared() <= r * r; }
