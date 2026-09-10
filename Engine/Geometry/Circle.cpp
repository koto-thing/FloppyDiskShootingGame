#include "Circle.h"

#include "Rect.h"

#include <algorithm>
#include <cmath>

/**
 * @brief 点が円の内部または境界上にあるか判定する
 * @param point 判定対象の点
 * @return 内部または境界上ならtrue
 */
bool Circle::Contains(const Vector2& point) const {
    const float effectiveRadius = std::abs(radius);
    return (point - center).LengthSquared() <= effectiveRadius * effectiveRadius;
}

/** @brief 円同士の交差を判定する */
bool Circle::Intersects(const Circle& other) const {
    const float radiusSum = std::abs(radius) + std::abs(other.radius);
    return (other.center - center).LengthSquared() <= radiusSum * radiusSum;
}

/** @brief 円と矩形の交差を判定する */
bool Circle::Intersects(const Rect& other) const {
    const Vector2 minimum = other.Min();
    const Vector2 maximum = other.Max();
    const Vector2 closest{
        std::clamp(center.x, minimum.x, maximum.x),
        std::clamp(center.y, minimum.y, maximum.y)};
    const float effectiveRadius = std::abs(radius);
    return (center - closest).LengthSquared() <= effectiveRadius * effectiveRadius;
}
