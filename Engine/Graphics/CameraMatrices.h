#pragma once

#include "../Math/Matrix4x4.h"

/** @brief カメラのビュー行列と投影行列を値で保持する */
struct CameraMatrices {
    Matrix4x4 view = Matrix4x4::Identity;
    Matrix4x4 projection = Matrix4x4::Identity;
    /** @brief projection * viewを返す */
    Matrix4x4 ViewProjection() const { return projection * view; }
    /** @brief ビュー投影行列の逆行列を取得する */
    bool TryInverseViewProjection(Matrix4x4& result) const { return ViewProjection().TryInverse(result); }
};
