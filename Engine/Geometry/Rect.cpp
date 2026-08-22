#include "Rect.h"

#include "Circle.h"

#include <algorithm>

Vector2 Rect::Min() const {
    return {std::min(position.x, position.x + size.x), std::min(position.y, position.y + size.y)};
}

Vector2 Rect::Max() const {
    return {std::max(position.x, position.x + size.x), std::max(position.y, position.y + size.y)};
}

Vector2 Rect::Center() const {
    const Vector2 minimum = Min();
    const Vector2 maximum = Max();
    return (minimum + maximum) * 0.5f;
}

bool Rect::Contains(const Vector2& point) const {
    const Vector2 minimum = Min();
    const Vector2 maximum = Max();
    return point.x >= minimum.x && point.x <= maximum.x &&
           point.y >= minimum.y && point.y <= maximum.y;
}

bool Rect::Intersects(const Rect& other) const {
    const Vector2 minimum = Min();
    const Vector2 maximum = Max();
    const Vector2 otherMinimum = other.Min();
    const Vector2 otherMaximum = other.Max();
    return minimum.x <= otherMaximum.x && maximum.x >= otherMinimum.x &&
           minimum.y <= otherMaximum.y && maximum.y >= otherMinimum.y;
}

bool Rect::Intersects(const Circle& other) const {
    return other.Intersects(*this);
}
