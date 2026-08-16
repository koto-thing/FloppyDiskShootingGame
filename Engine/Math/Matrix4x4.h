#pragma once

#include <algorithm>
#include <cmath>

#include "Math.h"
#include "Vector4.h"

/**
 * @brief 列ベクトル規約の4行4列行列を表す
 */
struct Matrix4x4 {
    float m[4][4]{};

    /** @brief 全要素が0の行列を生成する */
    constexpr Matrix4x4() = default;

    /** @brief 16個の要素を行優先で指定して生成する */
    constexpr Matrix4x4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13},
            {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}

    /** @brief 指定位置の要素を取得する */
    constexpr float& operator()(std::size_t row, std::size_t column) { return m[row][column]; }

    /** @brief 指定位置の要素を取得する */
    constexpr const float& operator()(std::size_t row, std::size_t column) const { return m[row][column]; }

    /** @brief 2つの行列を加算する */
    constexpr Matrix4x4 operator+(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                result.m[row][column] = m[row][column] + other.m[row][column];
            }
        }
        return result;
    }

    /** @brief 2つの行列を減算する */
    constexpr Matrix4x4 operator-(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                result.m[row][column] = m[row][column] - other.m[row][column];
            }
        }
        return result;
    }

    /** @brief 2つの行列を乗算する */
    constexpr Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                for (std::size_t index = 0; index < 4; ++index) {
                    result.m[row][column] += m[row][index] * other.m[index][column];
                }
            }
        }
        return result;
    }

    /** @brief 行列と4次元列ベクトルを乗算する */
    constexpr Vector4 operator*(const Vector4& vector) const {
        return {
            m[0][0] * vector.x + m[0][1] * vector.y + m[0][2] * vector.z + m[0][3] * vector.w,
            m[1][0] * vector.x + m[1][1] * vector.y + m[1][2] * vector.z + m[1][3] * vector.w,
            m[2][0] * vector.x + m[2][1] * vector.y + m[2][2] * vector.z + m[2][3] * vector.w,
            m[3][0] * vector.x + m[3][1] * vector.y + m[3][2] * vector.z + m[3][3] * vector.w
        };
    }

    /** @brief 行列を加算して代入する */
    constexpr Matrix4x4& operator+=(const Matrix4x4& other) { return *this = *this + other; }

    /** @brief 行列を減算して代入する */
    constexpr Matrix4x4& operator-=(const Matrix4x4& other) { return *this = *this - other; }

    /** @brief 行列を乗算して代入する */
    constexpr Matrix4x4& operator*=(const Matrix4x4& other) { return *this = *this * other; }

    /** @brief 転置行列を取得する */
    constexpr Matrix4x4 Transposed() const {
        Matrix4x4 result;
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                result.m[row][column] = m[column][row];
            }
        }
        return result;
    }

    /** @brief 逆行列の計算を試みる */
    bool TryInverse(Matrix4x4& result) const {
        // 元行列と単位行列を並べて掃き出し法を適用する
        float augmented[4][8]{};
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                augmented[row][column] = m[row][column];
                augmented[row][column + 4] = row == column ? 1.0f : 0.0f;
            }
        }

        for (std::size_t pivot = 0; pivot < 4; ++pivot) {
            // 数値誤差を抑えるため絶対値が最大の行を選択する
            std::size_t pivotRow = pivot;
            for (std::size_t row = pivot + 1; row < 4; ++row) {
                if (std::abs(augmented[row][pivot]) > std::abs(augmented[pivotRow][pivot])) {
                    pivotRow = row;
                }
            }
            if (std::abs(augmented[pivotRow][pivot]) <= Math::Epsilon) {
                return false;
            }
            if (pivotRow != pivot) {
                for (std::size_t column = 0; column < 8; ++column) {
                    std::swap(augmented[pivot][column], augmented[pivotRow][column]);
                }
            }

            // ピボットを1にして同じ列のほかの要素を0にする
            const float divisor = augmented[pivot][pivot];
            for (float& value : augmented[pivot]) {
                value /= divisor;
            }
            for (std::size_t row = 0; row < 4; ++row) {
                if (row == pivot) continue;
                const float factor = augmented[row][pivot];
                for (std::size_t column = 0; column < 8; ++column) {
                    augmented[row][column] -= factor * augmented[pivot][column];
                }
            }
        }

        // 拡大行列の右半分を逆行列として取り出す
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                result.m[row][column] = augmented[row][column + 4];
            }
        }
        return true;
    }

    /** @brief 座標として行列変換する */
    constexpr Vector3 TransformPoint(const Vector3& point) const {
        const Vector4 transformed = *this * Vector4(point, 1.0f);
        if (transformed.w != 0.0f && transformed.w != 1.0f) {
            return transformed.XYZ() / transformed.w;
        }
        return transformed.XYZ();
    }

    /** @brief 方向として行列変換する */
    constexpr Vector3 TransformVector(const Vector3& vector) const {
        return (*this * Vector4(vector, 0.0f)).XYZ();
    }

    /** @brief 平行移動行列を生成する */
    static constexpr Matrix4x4 Translation(const Vector3& translation) {
        Matrix4x4 result = Identity;
        result.m[0][3] = translation.x;
        result.m[1][3] = translation.y;
        result.m[2][3] = translation.z;
        return result;
    }

    /** @brief 拡大縮小行列を生成する */
    static constexpr Matrix4x4 Scale(const Vector3& scale) {
        Matrix4x4 result = Identity;
        result.m[0][0] = scale.x;
        result.m[1][1] = scale.y;
        result.m[2][2] = scale.z;
        return result;
    }

    /** @brief X軸回転行列を生成する */
    static Matrix4x4 RotationX(float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return {1, 0, 0, 0, 0, cosine, -sine, 0, 0, sine, cosine, 0, 0, 0, 0, 1};
    }

    /** @brief Y軸回転行列を生成する */
    static Matrix4x4 RotationY(float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return {cosine, 0, sine, 0, 0, 1, 0, 0, -sine, 0, cosine, 0, 0, 0, 0, 1};
    }

    /** @brief Z軸回転行列を生成する */
    static Matrix4x4 RotationZ(float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return {cosine, -sine, 0, 0, sine, cosine, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }

    static const Matrix4x4 Zero;
    static const Matrix4x4 Identity;
};

inline constexpr Matrix4x4 Matrix4x4::Zero{};
inline constexpr Matrix4x4 Matrix4x4::Identity{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};
