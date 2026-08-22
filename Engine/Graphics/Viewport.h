#pragma once

#include "../Math/Vector2.h"

/** @brief ピクセル単位の描画領域 */
struct Viewport {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    /** @brief 描画領域が有効か判定する */
    bool IsValid() const { return width > 0 && height > 0; }
    /** @brief アスペクト比を取得する */
    float AspectRatio() const { return height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height); }
    /** @brief スクリーン座標が領域内か判定する */
    bool Contains(const Vector2& point) const { return point.x >= x && point.y >= y && point.x <= x + width && point.y <= y + height; }
};
