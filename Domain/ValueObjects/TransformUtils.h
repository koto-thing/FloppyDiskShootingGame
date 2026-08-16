#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

// --- XMFLOAT3 / XMFLOAT4 の便利な算術演算子オーバーロード (Unity風の直感的な記述用) ---
inline DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
    return DirectX::XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
    return DirectX::XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& a, float scalar) {
    return DirectX::XMFLOAT3(a.x * scalar, a.y * scalar, a.z * scalar);
}

inline DirectX::XMFLOAT3 operator*(float scalar, const DirectX::XMFLOAT3& a) {
    return DirectX::XMFLOAT3(a.x * scalar, a.y * scalar, a.z * scalar);
}

inline DirectX::XMFLOAT3 operator/(const DirectX::XMFLOAT3& a, float scalar) {
    return DirectX::XMFLOAT3(a.x / scalar, a.y / scalar, a.z / scalar);
}

inline DirectX::XMFLOAT3& operator+=(DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
    a.x += b.x; a.y += b.y; a.z += b.z;
    return a;
}

inline DirectX::XMFLOAT3& operator-=(DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
    a.x -= b.x; a.y -= b.y; a.z -= b.z;
    return a;
}

inline DirectX::XMFLOAT3& operator*=(DirectX::XMFLOAT3& a, float scalar) {
    a.x *= scalar; a.y *= scalar; a.z *= scalar;
    return a;
}

inline DirectX::XMFLOAT3& operator/=(DirectX::XMFLOAT3& a, float scalar) {
    a.x /= scalar; a.y /= scalar; a.z /= scalar;
    return a;
}

/**
 * @brief 3Dグラフィックス・シューティングゲームで頻出する位置・回転・座標変換用ユーティリティ
 */
namespace TransformUtils {
    // 円周率
    const float PI = 3.1415926535f;

    // --- 角度変換 ---
    /**
     * 度をラジアンに変換する
     * @param degrees 度
     * @return ラジアン
     */
    inline float ToRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    /**
     * ラジアンを度に変換する
     * @param radians ラジアン
     * @return 度
     */
    inline float ToDegrees(float radians) {
        return radians * (180.0f / PI);
    }

    // --- 回転・オイラー角ユーティリティ ---
    /**
     * @brief オイラー角をクォータニオンに変換する
     * @param eulerDegrees オイラー角
     * @return クォータニオン
     */
    inline DirectX::XMVECTOR EulerToQuaternion(const DirectX::XMFLOAT3& eulerDegrees) {
        return DirectX::XMQuaternionRotationRollPitchYaw(
            ToRadians(eulerDegrees.x),
            ToRadians(eulerDegrees.y),
            ToRadians(eulerDegrees.z)
        );
    }

    /**
     * クォータニオンをオイラー角に変換する
     * @param q クォータニオン
     * @return オイラー角
     */
    inline DirectX::XMFLOAT3 QuaternionToEuler(const DirectX::XMVECTOR& q) {
        DirectX::XMFLOAT4 qf;
        DirectX::XMStoreFloat4(&qf, q);

        DirectX::XMFLOAT3 euler;

        // Roll (X軸回転)
        float sinr_cosp = 2.0f * (qf.w * qf.x + qf.y * qf.z);
        float cosr_cosp = 1.0f - 2.0f * (qf.x * qf.x + qf.y * qf.y);
        euler.x = ToDegrees(std::atan2(sinr_cosp, cosr_cosp));

        // Pitch (Y軸回転)
        float sinp = 2.0f * (qf.w * qf.y - qf.z * qf.x);
        if (std::abs(sinp) >= 1.0f) {
            euler.y = ToDegrees(std::copysign(PI / 2.0f, sinp));
        } else {
            euler.y = ToDegrees(std::asin(sinp));
        }

        // Yaw (Z軸回転)
        float siny_cosp = 2.0f * (qf.w * qf.z + qf.x * qf.y);
        float cosy_cosp = 1.0f - 2.0f * (qf.y * qf.y + qf.z * qf.z);
        euler.z = ToDegrees(std::atan2(siny_cosp, cosy_cosp));

        return euler;
    }

    /**
     * @brief オイラー角を回転行列に変換する
     * @param eulerDegrees オイラー角
     * @return 回転行列
     */
    inline DirectX::XMMATRIX EulerToRotationMatrix(const DirectX::XMFLOAT3& eulerDegrees) {
        return DirectX::XMMatrixRotationRollPitchYaw(
            ToRadians(eulerDegrees.x),
            ToRadians(eulerDegrees.y),
            ToRadians(eulerDegrees.z)
        );
    }

    /**
     * @brief 方向ベクトルと上方向ベクトルからオイラー角を計算する
     * @param forwardDir 方向ベクトル
     * @param upDir 上方向ベクトル
     * @return オイラー角
     */
    inline DirectX::XMFLOAT3 LookRotationToEuler(const DirectX::XMFLOAT3& forwardDir, const DirectX::XMFLOAT3& upDir = DirectX::XMFLOAT3(0, 1, 0)) {
        DirectX::XMVECTOR f = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&forwardDir));
        DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&upDir);
        
        DirectX::XMVECTOR r = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(u, f));
        u = DirectX::XMVector3Cross(f, r);

        DirectX::XMMATRIX rotationMatrix;
        rotationMatrix.r[0] = r;
        rotationMatrix.r[1] = u;
        rotationMatrix.r[2] = f;
        rotationMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        DirectX::XMVECTOR q = DirectX::XMQuaternionRotationMatrix(rotationMatrix);
        return QuaternionToEuler(q);
    }

    /**
     * @brief 角度の線形補間 (Lerp) を行う。角度は360度でラップする。
     * @param start 開始角度
     * @param end 終了角度
     * @param t 補間係数 (0.0f ~ 1.0f)
     * @return 補間後の角度
     */
    inline float LerpAngle(float start, float end, float t) {
        float difference = std::fmod(end - start, 360.0f);
        if (difference < -180.0f) difference += 360.0f;
        if (difference > 180.0f) difference -= 360.0f;
        return start + difference * t;
    }

    inline DirectX::XMFLOAT3 LerpEuler(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float t) {
        return DirectX::XMFLOAT3(
            LerpAngle(start.x, end.x, t),
            LerpAngle(start.y, end.y, t),
            LerpAngle(start.z, end.z, t)
        );
    }

    // --- 位置・ベクトル・座標変換ユーティリティ ---

    /**
     * @brief 2点間の距離を計算する
     * @param a 地点A
     * @param b 地点B
     * @return AとBの距離
     */
    inline float Distance(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float dz = b.z - a.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    /**
     * @brief ベクトルの長さを計算する
     * @param v ベクトル
     * @return ベクトルの長さ
     */
    inline float Length(const DirectX::XMFLOAT3& v) {
        return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    }

    /**
     * ベクトルを正規化する
     * @param v ベクトル
     * @return 正規化後のベクトル
     */
    inline DirectX::XMFLOAT3 Normalize(const DirectX::XMFLOAT3& v) {
        float len = Length(v);
        if (len > 0.0001f) {
            return DirectX::XMFLOAT3(v.x / len, v.y / len, v.z / len);
        }
        return DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    /**
     * @brief 2点間の線形補間 (Lerp) を行う
     * @param start 開始地点
     * @param end 終了地点
     * @param t 保管係数
     * @return 補間後の地点
     */
    inline DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float t) {
        return DirectX::XMFLOAT3(
            start.x + (end.x - start.x) * t,
            start.y + (end.y - start.y) * t,
            start.z + (end.z - start.z) * t
        );
    }

    /**
     * @brief 3Dベクトルの各成分を指定範囲内にクランプする
     * @param val 成分
     * @param minVal 最小値
     * @param maxVal 最大値
     * @return クランプ後の値
     */
    inline DirectX::XMFLOAT3 Clamp(const DirectX::XMFLOAT3& val, const DirectX::XMFLOAT3& minVal, const DirectX::XMFLOAT3& maxVal) {
        return DirectX::XMFLOAT3(
            std::clamp(val.x, minVal.x, maxVal.x),
            std::clamp(val.y, minVal.y, maxVal.y),
            std::clamp(val.z, minVal.z, maxVal.z)
        );
    }

    /**
     * @brief ローカル座標を、親のワールド行列を適用してワールド座標に変換する
     * @param localPoint 親の相対座標
     * @param worldMatrix 親のワールド行列
     */
    inline DirectX::XMFLOAT3 TransformPoint(const DirectX::XMFLOAT3& localPoint, const DirectX::XMMATRIX& worldMatrix) {
        DirectX::XMVECTOR lp = DirectX::XMLoadFloat3(&localPoint);
        DirectX::XMVECTOR wp = DirectX::XMVector3TransformCoord(lp, worldMatrix);
        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, wp);
        return result;
    }

    /**
     * @brief ワールド座標を、親のワールド行列の逆行列を適用してローカル座標に変換する
     * @param worldPoint ワールド座標
     * @param worldMatrix 親のワールド行列
     * @return ローカル座標
     */
    inline DirectX::XMFLOAT3 InverseTransformPoint(const DirectX::XMFLOAT3& worldPoint, const DirectX::XMMATRIX& worldMatrix) {
        DirectX::XMVECTOR wp = DirectX::XMLoadFloat3(&worldPoint);
        DirectX::XMVECTOR det;
        DirectX::XMMATRIX invWorld = DirectX::XMMatrixInverse(&det, worldMatrix);
        DirectX::XMVECTOR lp = DirectX::XMVector3TransformCoord(wp, invWorld);
        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, lp);
        return result;
    }
}
