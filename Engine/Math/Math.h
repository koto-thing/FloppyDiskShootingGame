#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

/**
 * @brief 汎用的なスカラー数学関数を提供する
 */
namespace Math {
    inline constexpr float Pi = std::numbers::pi_v<float>;
    inline constexpr float TwoPi = Pi * 2.0f;
    inline constexpr float HalfPi = Pi * 0.5f;
    inline constexpr float DegreesToRadians = Pi / 180.0f;
    inline constexpr float RadiansToDegrees = 180.0f / Pi;
    inline constexpr float Epsilon = 1.0e-6f;

    /** @brief 値を指定範囲へ制限する */
    template <typename T>
    constexpr T Clamp(T value, T minimum, T maximum) {
        return std::clamp(value, minimum, maximum);
    }

    /** @brief 値を0から1の範囲へ制限する */
    template <typename T>
    constexpr T Clamp01(T value) {
        return Clamp(value, T{0}, T{1});
    }

    /** @brief 2つの値を線形補間する */
    template <typename T, typename U>
    constexpr T Lerp(const T& start, const T& end, U amount) {
        return start + (end - start) * amount;
    }

    /** @brief 2つの値を0から1の範囲で線形補間する */
    template <typename T, typename U>
    constexpr T LerpClamped(const T& start, const T& end, U amount) {
        return Lerp(start, end, Clamp01(amount));
    }

    /** @brief 度をラジアンへ変換する */
    constexpr float ToRadians(float degrees) {
        return degrees * DegreesToRadians;
    }

    /** @brief ラジアンを度へ変換する */
    constexpr float ToDegrees(float radians) {
        return radians * RadiansToDegrees;
    }

    /** @brief 2つの浮動小数点値が許容誤差内で等しいかを判定する */
    inline bool Approximately(float a, float b, float epsilon = Epsilon) {
        // 値の大きさに応じて許容誤差を拡張する
        const float scale = (std::max)(1.0f, (std::max)(std::abs(a), std::abs(b)));
        return std::abs(a - b) <= epsilon * scale;
    }

    /** @brief 値の符号を取得する */
    constexpr float Sign(float value) {
        return value < 0.0f ? -1.0f : 1.0f;
    }

    /** @brief 値を指定周期の範囲へ折り返す */
    inline float Repeat(float value, float length) {
        if (length <= 0.0f) {
            return 0.0f;
        }
        return value - std::floor(value / length) * length;
    }
}
