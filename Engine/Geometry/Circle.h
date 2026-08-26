#pragma once

#include "../Math/Vector2.h"

struct Rect;

/**
 * @brief 2次元の円
 *
 * 半径が負の場合は絶対値を有効な半径として扱う
 */
struct Circle {
    Vector2 center {};
    float radius = 0.0f;

    /** @brief 点が円の内部または境界上にあるか判定する */
    bool Contains(const Vector2& point) const;
    /** @brief 円同士の交差を判定する */
    bool Intersects(const Circle& other) const;
    /** @brief 円と矩形の交差を判定する */
    bool Intersects(const Rect& other) const;
};
