#pragma once

#include "../Math/Vector2.h"

struct Circle;

/**
 * @brief 2次元の軸平行矩形
 *
 * 負のサイズは各軸の向きを反転した矩形として正規化して扱う
 */
struct Rect {
    Vector2 position {};
    Vector2 size {};

    /** @brief 矩形の中心を取得する */
    Vector2 Center() const;
    /** @brief 正規化後の最小座標を取得する */
    Vector2 Min() const;
    /** @brief 正規化後の最大座標を取得する */
    Vector2 Max() const;
    /** @brief 点が矩形の内部または境界上にあるか判定する */
    bool Contains(const Vector2& point) const;
    /** @brief 矩形同士の交差を判定する */
    bool Intersects(const Rect& other) const;
    /** @brief 矩形と円の交差を判定する */
    bool Intersects(const Circle& other) const;
};
