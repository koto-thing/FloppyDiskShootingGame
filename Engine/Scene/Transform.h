#pragma once

#include <cstddef>
#include <vector>

#include "../Math/Matrix4x4.h"
#include "../Math/Quaternion.h"
#include "../Math/Vector3.h"

/**
 * @brief 軽量なローカル姿勢と親子階層を管理する値型
 */
class Transform {
public:
    Transform();
    ~Transform();

    /** @brief ローカル位置を取得する */
    const Vector3& LocalPosition() const;
    /** @brief ローカル位置を設定する */
    void SetLocalPosition(const Vector3& position);
    /** @brief ワールド位置を取得する */
    Vector3 Position() const;
    /** @brief ワールド位置を設定する */
    void SetPosition(const Vector3& position);
    /** @brief ローカル回転を取得する */
    const Quaternion& LocalRotation() const;
    /** @brief ローカル回転を設定する */
    void SetLocalRotation(const Quaternion& rotation);
    /** @brief ローカル拡縮を取得する */
    const Vector3& LocalScale() const;
    /** @brief ローカル拡縮を設定する */
    void SetLocalScale(const Vector3& scale);
    /** @brief ローカル位置へ移動量を加算する */
    void Translate(const Vector3& distance);
    /** @brief 親を設定し、必要ならワールド姿勢を維持する */
    bool SetParent(Transform* parent, bool keepWorldTransform = true);
    /** @brief 親を取得する */
    Transform* Parent() const;
    /** @brief 子の数を取得する */
    std::size_t ChildCount() const;
    /** @brief 子を取得する。範囲外ならnullptrを返す */
    Transform* Child(std::size_t index) const;
    /** @brief 遅延計算されたワールド行列を取得する */
    const Matrix4x4& WorldMatrix() const;

private:
    bool IsDescendantOf(const Transform* candidate) const;
    void RemoveChild(Transform* child);
    void SetLocalMatrix(const Matrix4x4& matrix);
    void MarkDirty();
    void RebuildWorldMatrix() const;

    Vector3 m_localPosition;
    Quaternion m_localRotation;
    Vector3 m_localScale;
    Transform* m_parent;
    std::vector<Transform*> m_children;
    mutable Matrix4x4 m_worldMatrix;
    mutable bool m_dirty;
};
