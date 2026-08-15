#pragma once

#include <cmath>

#include "Math.h"

/**
 * @brief 2次元の座標または方向を表す
 */
struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    /** @brief ゼロベクトルを生成する */
    constexpr Vector2() = default;

    /** @brief 各成分を指定して生成する */
    constexpr Vector2(float x, float y) : x(x), y(y) {}

    /** @brief 2つのベクトルを加算する */
    constexpr Vector2 operator+(const Vector2& other) const { return {x + other.x, y + other.y}; }

    /** @brief 2つのベクトルを減算する */
    constexpr Vector2 operator-(const Vector2& other) const { return {x - other.x, y - other.y}; }

    /** @brief 各成分をスカラー倍する */
    constexpr Vector2 operator*(float scalar) const { return {x * scalar, y * scalar}; }

    /** @brief 各成分をスカラーで除算する */
    constexpr Vector2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

    /** @brief ベクトルを加算して代入する */
    constexpr Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }

    /** @brief ベクトルを減算して代入する */
    constexpr Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }

    /** @brief スカラー倍して代入する */
    constexpr Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }

    /** @brief スカラーで除算して代入する */
    constexpr Vector2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

    /** @brief 各成分の符号を反転する */
    constexpr Vector2 operator-() const { return {-x, -y}; }

    /** @brief 各成分が等しいかを判定する */
    constexpr bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }

    /** @brief 各成分が異なるかを判定する */
    constexpr bool operator!=(const Vector2& other) const { return !(*this == other); }

    /** @brief ベクトルの長さを取得する */
    float Length() const { return std::sqrt(LengthSquared()); }

    /** @brief ベクトルの長さの2乗を取得する */
    constexpr float LengthSquared() const { return x * x + y * y; }

    /** @brief 正規化したベクトルを取得する */
    Vector2 Normalized() const {
        const float length = Length();
        return length <= Math::Epsilon ? Zero : *this / length;
    }

    /** @brief 2つのベクトルの内積を取得する */
    static constexpr float Dot(const Vector2& a, const Vector2& b) { return a.x * b.x + a.y * b.y; }

    /** @brief 2点間の距離を取得する */
    static float Distance(const Vector2& a, const Vector2& b) { return (b - a).Length(); }

    /** @brief 2つのベクトルを線形補間する */
    static constexpr Vector2 Lerp(const Vector2& a, const Vector2& b, float amount) {
        return a + (b - a) * amount;
    }

    static const Vector2 Zero;
    static const Vector2 One;
    static const Vector2 Up;
    static const Vector2 Down;
    static const Vector2 Left;
    static const Vector2 Right;
};

/** @brief 左辺のスカラーでベクトルを乗算する */
constexpr Vector2 operator*(float scalar, const Vector2& vector) { return vector * scalar; }

inline constexpr Vector2 Vector2::Zero{0.0f, 0.0f};
inline constexpr Vector2 Vector2::One{1.0f, 1.0f};
inline constexpr Vector2 Vector2::Up{0.0f, 1.0f};
inline constexpr Vector2 Vector2::Down{0.0f, -1.0f};
inline constexpr Vector2 Vector2::Left{-1.0f, 0.0f};
inline constexpr Vector2 Vector2::Right{1.0f, 0.0f};
