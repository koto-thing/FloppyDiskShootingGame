#pragma once

namespace GameplayRandom {
inline unsigned State = 0x9e3779b9U;

/**
 * @brief 指定範囲内の軽量な疑似乱数を取得する
 * @param minValue 乱数の最小値
 * @param maxValue 乱数の最大値
 * @return minValue以上maxValue以下の疑似乱数
 */
inline float Range(float minValue, float maxValue) {
    State ^= State << 13;
    State ^= State >> 17;
    State ^= State << 5;
    return minValue + (maxValue - minValue) * static_cast<float>(State & 0xffffU) / 65535.0f;
}
}
