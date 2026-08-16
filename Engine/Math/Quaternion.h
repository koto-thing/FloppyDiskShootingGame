#pragma once

#include <cmath>

#include "Matrix4x4.h"

/**
 * @brief 3次元空間の回転を表すクォータニオン
 */
struct Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    /** @brief 単位クォータニオンを生成する */
    constexpr Quaternion() = default;

    /** @brief 各成分を指定して生成する */
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /** @brief 2つのクォータニオンを加算する */
    constexpr Quaternion operator+(const Quaternion& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    /** @brief 回転を合成する */
    constexpr Quaternion operator*(const Quaternion& other) const {
        return {
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        };
    }

    /** @brief 各成分をスカラー倍する */
    constexpr Quaternion operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar, w * scalar}; }

    /** @brief 各成分をスカラーで除算する */
    constexpr Quaternion operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar, w / scalar}; }

    /** @brief 各成分の符号を反転する */
    constexpr Quaternion operator-() const { return {-x, -y, -z, -w}; }

    /** @brief 回転を合成して代入する */
    constexpr Quaternion& operator*=(const Quaternion& other) { return *this = *this * other; }

    /** @brief 各成分が等しいかを判定する */
    constexpr bool operator==(const Quaternion& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    /** @brief 各成分が異なるかを判定する */
    constexpr bool operator!=(const Quaternion& other) const { return !(*this == other); }

    /** @brief クォータニオンの長さを取得する */
    float Length() const { return std::sqrt(LengthSquared()); }

    /** @brief クォータニオンの長さの2乗を取得する */
    constexpr float LengthSquared() const { return x * x + y * y + z * z + w * w; }

    /** @brief 正規化したクォータニオンを取得する */
    Quaternion Normalized() const {
        const float length = Length();
        return length <= Math::Epsilon ? Identity : *this / length;
    }

    /** @brief 共役クォータニオンを取得する */
    constexpr Quaternion Conjugated() const { return {-x, -y, -z, w}; }

    /** @brief 逆クォータニオンを取得する */
    Quaternion Inversed() const {
        const float lengthSquared = LengthSquared();
        return lengthSquared <= Math::Epsilon ? Identity : Conjugated() / lengthSquared;
    }

    /** @brief 3次元ベクトルを回転する */
    Vector3 Rotate(const Vector3& vector) const {
        // 正規化したクォータニオンによる回転を外積で展開する
        const Quaternion rotation = Normalized();
        const Vector3 imaginary{rotation.x, rotation.y, rotation.z};
        const Vector3 twiceCross = 2.0f * Vector3::Cross(imaginary, vector);
        return vector + rotation.w * twiceCross + Vector3::Cross(imaginary, twiceCross);
    }

    /** @brief 回転行列へ変換する */
    Matrix4x4 ToMatrix() const {
        const Quaternion q = Normalized();
        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;
        const float xy = q.x * q.y;
        const float xz = q.x * q.z;
        const float yz = q.y * q.z;
        const float wx = q.w * q.x;
        const float wy = q.w * q.y;
        const float wz = q.w * q.z;

        return {
            1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy), 0,
            2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx), 0,
            2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy), 0,
            0, 0, 0, 1
        };
    }

    /** @brief 軸と角度から回転を生成する */
    static Quaternion FromAxisAngle(const Vector3& axis, float radians) {
        const Vector3 normalizedAxis = axis.Normalized();
        if (normalizedAxis == Vector3::Zero) {
            return Identity;
        }

        const float halfAngle = radians * 0.5f;
        const float sine = std::sin(halfAngle);
        return {normalizedAxis.x * sine, normalizedAxis.y * sine, normalizedAxis.z * sine, std::cos(halfAngle)};
    }

    /** @brief X、Y、Z軸の順に適用するオイラー角から回転を生成する */
    static Quaternion FromEuler(const Vector3& radians) {
        // 列ベクトル規約に合わせてZ、Y、Xの順で合成する
        const Quaternion xRotation = FromAxisAngle(Vector3::Right, radians.x);
        const Quaternion yRotation = FromAxisAngle(Vector3::Up, radians.y);
        const Quaternion zRotation = FromAxisAngle(Vector3::Forward, radians.z);
        return (zRotation * yRotation * xRotation).Normalized();
    }

    /** @brief 2つのクォータニオンの内積を取得する */
    static constexpr float Dot(const Quaternion& a, const Quaternion& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    /** @brief 2つの回転を球面線形補間する */
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float amount) {
        Quaternion start = a.Normalized();
        Quaternion end = b.Normalized();
        float dot = Dot(start, end);

        // 同じ回転の逆符号表現を反転して最短経路を選択する
        if (dot < 0.0f) {
            end = -end;
            dot = -dot;
        }
        dot = Math::Clamp(dot, -1.0f, 1.0f);

        // 非常に近い場合は不安定な除算を避けて線形補間する
        if (dot > 0.9995f) {
            return (start * (1.0f - amount) + end * amount).Normalized();
        }

        const float angle = std::acos(dot);
        const float sine = std::sin(angle);
        const float startWeight = std::sin((1.0f - amount) * angle) / sine;
        const float endWeight = std::sin(amount * angle) / sine;
        return (start * startWeight + end * endWeight).Normalized();
    }

    static const Quaternion Identity;
};

/** @brief 左辺のスカラーでクォータニオンを乗算する */
constexpr Quaternion operator*(float scalar, const Quaternion& quaternion) { return quaternion * scalar; }

inline constexpr Quaternion Quaternion::Identity{0.0f, 0.0f, 0.0f, 1.0f};
