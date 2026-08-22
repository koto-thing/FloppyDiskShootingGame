#include "Ray.h"
#include "Sphere.h"
#include "Box.h"
#include <algorithm>
#include <cmath>
Ray::Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.LengthSquared() <= Math::Epsilon ? Vector3::Forward : d.Normalized()) {}
bool Ray::Intersects(const Sphere& sphere, RayHit* hit) const {
    const Vector3 delta = sphere.center - origin; const float radius = std::abs(sphere.radius); const float t = Vector3::Dot(delta, direction); const float discriminant = t * t - (delta.LengthSquared() - radius * radius);
    if (discriminant < 0.0f) return false; const float root = std::sqrt(discriminant); float distance = t - root; if (distance < 0.0f) distance = t + root; if (distance < 0.0f) return false;
    if (hit) { hit->distance = distance; hit->point = PointAt(distance); hit->normal = (hit->point - sphere.center).Normalized(); } return true;
}
bool Ray::Intersects(const Box& box, RayHit* hit) const {
    const auto a = box.Min(), b = box.Max(); float enter = 0.0f, exit = INFINITY; int enterAxis = -1;
    const float o[3] = {origin.x, origin.y, origin.z}, d[3] = {direction.x, direction.y, direction.z}, mn[3] = {a.x, a.y, a.z}, mx[3] = {b.x, b.y, b.z};
    for (int axis = 0; axis < 3; ++axis) { if (std::abs(d[axis]) <= Math::Epsilon) { if (o[axis] < mn[axis] || o[axis] > mx[axis]) return false; continue; } float t1 = (mn[axis] - o[axis]) / d[axis], t2 = (mx[axis] - o[axis]) / d[axis]; if (t1 > t2) std::swap(t1, t2); if (t1 > enter) { enter = t1; enterAxis = axis; } exit = std::min(exit, t2); if (enter > exit) return false; }
    if (hit) { hit->distance = enter; hit->point = PointAt(enter); hit->normal = Vector3::Zero; if (enterAxis >= 0) { const float value = (&hit->point.x)[enterAxis]; const float low = mn[enterAxis]; const float high = mx[enterAxis]; (&hit->normal.x)[enterAxis] = std::abs(value-low) < std::abs(value-high) ? -1.0f : 1.0f; } } return true;
}
