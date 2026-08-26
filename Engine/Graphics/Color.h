#pragma once

/**
 * @brief 線形色空間のRGBA色
 */
struct ColorF {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    /** @brief 白色を返す */
    static constexpr ColorF White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    /** @brief 黒色を返す */
    static constexpr ColorF Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    /** @brief 赤色を返す */
    static constexpr ColorF Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    /** @brief 完全透明色を返す */
    static constexpr ColorF Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
};
