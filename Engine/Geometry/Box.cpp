#include "Box.h"
#include "Sphere.h"
#include <algorithm>
#include <cmath>
/** @brief 各軸の半サイズを取得する */
Vector3 Box::HalfSize() const { return {std::abs(size.x) * 0.5f, std::abs(size.y) * 0.5f, std::abs(size.z) * 0.5f}; }
/** @brief 境界箱の最小座標を取得する */
Vector3 Box::Min() const { return center - HalfSize(); }
/** @brief 境界箱の最大座標を取得する */
Vector3 Box::Max() const { return center + HalfSize(); }
/**
 * @brief 点が境界箱の内部または境界上にあるか判定する
 * @param point 判定対象の点
 * @return 内部または境界上ならtrue
 */
bool Box::Contains(const Vector3& point) const { const auto a = Min(), b = Max(); return point.x >= a.x && point.x <= b.x && point.y >= a.y && point.y <= b.y && point.z >= a.z && point.z <= b.z; }
/** @brief 境界箱同士の交差を判定する */
bool Box::Intersects(const Box& other) const { const auto a = Min(), b = Max(), c = other.Min(), d = other.Max(); return a.x <= d.x && b.x >= c.x && a.y <= d.y && b.y >= c.y && a.z <= d.z && b.z >= c.z; }
/** @brief 境界箱と球の交差を判定する */
bool Box::Intersects(const Sphere& sphere) const { const auto a = Min(), b = Max(); const Vector3 p{std::clamp(sphere.center.x, a.x, b.x), std::clamp(sphere.center.y, a.y, b.y), std::clamp(sphere.center.z, a.z, b.z)}; const float r = std::abs(sphere.radius); return (sphere.center - p).LengthSquared() <= r * r; }
