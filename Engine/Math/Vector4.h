#pragma once

#include <cmath>

#include "Math.h"
#include "Vector3.h"

/**
 * @brief 4次元ベクトルまたは同次座標を表す
 */
struct Vector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    /** @brief ゼロベクトルを生成する */
    constexpr Vector4() = default;

    /** @brief 各成分を指定して生成する */
    constexpr Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /** @brief Vector3とW成分から生成する */
    constexpr Vector4(const Vector3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    /** @brief XYZ成分をVector3として取得する */
    constexpr Vector3 XYZ() const { return {x, y, z}; }

    /** @brief 2つのベクトルを加算する */
    constexpr Vector4 operator+(const Vector4& other) const { return {x + other.x, y + other.y, z + other.z, w + other.w}; }

    /** @brief 2つのベクトルを減算する */
    constexpr Vector4 operator-(const Vector4& other) const { return {x - other.x, y - other.y, z - other.z, w - other.w}; }

    /** @brief 各成分をスカラー倍する */
    constexpr Vector4 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar, w * scalar}; }

    /** @brief 各成分をスカラーで除算する */
    constexpr Vector4 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar, w / scalar}; }

    /** @brief ベクトルを加算して代入する */
    constexpr Vector4& operator+=(const Vector4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }

    /** @brief ベクトルを減算して代入する */
    constexpr Vector4& operator-=(const Vector4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }

    /** @brief スカラー倍して代入する */
    constexpr Vector4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }

    /** @brief スカラーで除算して代入する */
    constexpr Vector4& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    /** @brief 各成分の符号を反転する */
    constexpr Vector4 operator-() const { return {-x, -y, -z, -w}; }

    /** @brief 各成分が等しいかを判定する */
    constexpr bool operator==(const Vector4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }

    /** @brief 各成分が異なるかを判定する */
    constexpr bool operator!=(const Vector4& other) const { return !(*this == other); }

    /** @brief ベクトルの長さを取得する */
    float Length() const { return std::sqrt(LengthSquared()); }

    /** @brief ベクトルの長さの2乗を取得する */
    constexpr float LengthSquared() const { return x * x + y * y + z * z + w * w; }

    /** @brief 正規化したベクトルを取得する */
    Vector4 Normalized() const {
        const float length = Length();
        return length <= Math::Epsilon ? Zero : *this / length;
    }

    /** @brief 2つのベクトルの内積を取得する */
    static constexpr float Dot(const Vector4& a, const Vector4& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    /** @brief 2つのベクトルを線形補間する */
    static constexpr Vector4 Lerp(const Vector4& a, const Vector4& b, float amount) {
        return a + (b - a) * amount;
    }

    static const Vector4 Zero;
    static const Vector4 One;
};

/** @brief 左辺のスカラーでベクトルを乗算する */
constexpr Vector4 operator*(float scalar, const Vector4& vector) { return vector * scalar; }

inline constexpr Vector4 Vector4::Zero{0.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vector4 Vector4::One{1.0f, 1.0f, 1.0f, 1.0f};
