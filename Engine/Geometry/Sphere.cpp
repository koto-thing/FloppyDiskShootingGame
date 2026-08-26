#include "Sphere.h"
#include "Box.h"
#include <cmath>
bool Sphere::Contains(const Vector3& point) const { const float r = std::abs(radius); return (point - center).LengthSquared() <= r * r; }
bool Sphere::Intersects(const Sphere& other) const { const float r = std::abs(radius) + std::abs(other.radius); return (other.center - center).LengthSquared() <= r * r; }
bool Sphere::Intersects(const Box& other) const { return other.Intersects(*this); }
