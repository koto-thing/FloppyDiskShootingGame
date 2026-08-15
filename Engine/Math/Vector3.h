#pragma once

#include <cmath>

#include "Math.h"
#include "Vector2.h"

/**
 * @brief 3次元の座標または方向を表す
 */
struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    /** @brief ゼロベクトルを生成する */
    constexpr Vector3() = default;

    /** @brief 各成分を指定して生成する */
    constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    /** @brief Vector2とZ成分から生成する */
    constexpr Vector3(const Vector2& xy, float z) : x(xy.x), y(xy.y), z(z) {}

    /** @brief XY成分をVector2として取得する */
    constexpr Vector2 XY() const { return {x, y}; }

    /** @brief 2つのベクトルを加算する */
    constexpr Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }

    /** @brief 2つのベクトルを減算する */
    constexpr Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }

    /** @brief 各成分をスカラー倍する */
    constexpr Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }

    /** @brief 各成分をスカラーで除算する */
    constexpr Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    /** @brief ベクトルを加算して代入する */
    constexpr Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }

    /** @brief ベクトルを減算して代入する */
    constexpr Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }

    /** @brief スカラー倍して代入する */
    constexpr Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }

    /** @brief スカラーで除算して代入する */
    constexpr Vector3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

    /** @brief 各成分の符号を反転する */
    constexpr Vector3 operator-() const { return {-x, -y, -z}; }

    /** @brief 各成分が等しいかを判定する */
    constexpr bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }

    /** @brief 各成分が異なるかを判定する */
    constexpr bool operator!=(const Vector3& other) const { return !(*this == other); }

    /** @brief ベクトルの長さを取得する */
    float Length() const { return std::sqrt(LengthSquared()); }

    /** @brief ベクトルの長さの2乗を取得する */
    constexpr float LengthSquared() const { return x * x + y * y + z * z; }

    /** @brief 正規化したベクトルを取得する */
    Vector3 Normalized() const {
        const float length = Length();
        return length <= Math::Epsilon ? Zero : *this / length;
    }

    /** @brief 2つのベクトルの内積を取得する */
    static constexpr float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    /** @brief 2つのベクトルの外積を取得する */
    static constexpr Vector3 Cross(const Vector3& a, const Vector3& b) {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    /** @brief 2点間の距離を取得する */
    static float Distance(const Vector3& a, const Vector3& b) { return (b - a).Length(); }

    /** @brief 2つのベクトルを線形補間する */
    static constexpr Vector3 Lerp(const Vector3& a, const Vector3& b, float amount) {
        return a + (b - a) * amount;
    }

    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 Up;
    static const Vector3 Down;
    static const Vector3 Left;
    static const Vector3 Right;
    static const Vector3 Forward;
    static const Vector3 Back;
};

/** @brief 左辺のスカラーでベクトルを乗算する */
constexpr Vector3 operator*(float scalar, const Vector3& vector) { return vector * scalar; }

inline constexpr Vector3 Vector3::Zero{0.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::One{1.0f, 1.0f, 1.0f};
inline constexpr Vector3 Vector3::Up{0.0f, 1.0f, 0.0f};
inline constexpr Vector3 Vector3::Down{0.0f, -1.0f, 0.0f};
inline constexpr Vector3 Vector3::Left{-1.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::Right{1.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::Forward{0.0f, 0.0f, 1.0f};
inline constexpr Vector3 Vector3::Back{0.0f, 0.0f, -1.0f};
