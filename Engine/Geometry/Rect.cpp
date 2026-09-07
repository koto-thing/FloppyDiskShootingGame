#include "Rect.h"

#include "Circle.h"

#include <algorithm>

/** @brief 正規化後の最小座標を取得する */
Vector2 Rect::Min() const {
    return {std::min(position.x, position.x + size.x), std::min(position.y, position.y + size.y)};
}

/** @brief 正規化後の最大座標を取得する */
Vector2 Rect::Max() const {
    return {std::max(position.x, position.x + size.x), std::max(position.y, position.y + size.y)};
}

/** @brief 矩形の中心を取得する */
Vector2 Rect::Center() const {
    const Vector2 minimum = Min();
    const Vector2 maximum = Max();
    return (minimum + maximum) * 0.5f;
}

/** @brief 点が矩形の内部または境界上にあるか判定する */
bool Rect::Contains(const Vector2& point) const {
    const Vector2 minimum = Min();
    const Vector2 maximum = Max();
    return point.x >= minimum.x && point.x <= maximum.x &&
           point.y >= minimum.y && point.y <= maximum.y;
}

/** @brief 矩形同士の交差を判定する */
bool Rect::Intersects(const Rect& other) const {
    const Vector2 minimum = Min();
    const Vector2 maximum = Max();
    const Vector2 otherMinimum = other.Min();
    const Vector2 otherMaximum = other.Max();
    return minimum.x <= otherMaximum.x && maximum.x >= otherMinimum.x &&
           minimum.y <= otherMaximum.y && maximum.y >= otherMinimum.y;
}

/** @brief 矩形と円の交差を判定する */
bool Rect::Intersects(const Circle& other) const {
    return other.Intersects(*this);
}
