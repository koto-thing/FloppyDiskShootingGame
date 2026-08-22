#include "Transform.h"

#include <algorithm>
#include <cmath>

namespace {

Quaternion RotationFromMatrix(const Matrix4x4& matrix, const Vector3& scale) {
    Matrix4x4 rotation = matrix;
    for (int column = 0; column < 3; ++column) {
        const float divisor = column == 0 ? scale.x : (column == 1 ? scale.y : scale.z);
        if (std::abs(divisor) > Math::Epsilon) {
            for (int row = 0; row < 3; ++row) rotation(row, column) /= divisor;
        }
    }

    const float trace = rotation(0, 0) + rotation(1, 1) + rotation(2, 2);
    Quaternion result;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        result.w = 0.25f * s;
        result.x = (rotation(2, 1) - rotation(1, 2)) / s;
        result.y = (rotation(0, 2) - rotation(2, 0)) / s;
        result.z = (rotation(1, 0) - rotation(0, 1)) / s;
    } else if (rotation(0, 0) > rotation(1, 1) && rotation(0, 0) > rotation(2, 2)) {
        const float s = std::sqrt(1.0f + rotation(0, 0) - rotation(1, 1) - rotation(2, 2)) * 2.0f;
        result.w = (rotation(2, 1) - rotation(1, 2)) / s;
        result.x = 0.25f * s;
        result.y = (rotation(0, 1) + rotation(1, 0)) / s;
        result.z = (rotation(0, 2) + rotation(2, 0)) / s;
    } else if (rotation(1, 1) > rotation(2, 2)) {
        const float s = std::sqrt(1.0f + rotation(1, 1) - rotation(0, 0) - rotation(2, 2)) * 2.0f;
        result.w = (rotation(0, 2) - rotation(2, 0)) / s;
        result.x = (rotation(0, 1) + rotation(1, 0)) / s;
        result.y = 0.25f * s;
        result.z = (rotation(1, 2) + rotation(2, 1)) / s;
    } else {
        const float s = std::sqrt(1.0f + rotation(2, 2) - rotation(0, 0) - rotation(1, 1)) * 2.0f;
        result.w = (rotation(1, 0) - rotation(0, 1)) / s;
        result.x = (rotation(0, 2) + rotation(2, 0)) / s;
        result.y = (rotation(1, 2) + rotation(2, 1)) / s;
        result.z = 0.25f * s;
    }
    return result.Normalized();
}

}

Transform::Transform()
    : m_localPosition(Vector3::Zero), m_localRotation(Quaternion::Identity),
      m_localScale(Vector3::One), m_parent(nullptr), m_worldMatrix(Matrix4x4::Identity), m_dirty(true) {}

Transform::~Transform() {
    if (m_parent != nullptr) {
        m_parent->RemoveChild(this);
        m_parent = nullptr;
    }
    for (Transform* child : m_children) {
        if (child == nullptr) continue;
        const Matrix4x4 childWorld = child->WorldMatrix();
        child->m_parent = nullptr;
        child->SetLocalMatrix(childWorld);
    }
    m_children.clear();
}

const Vector3& Transform::LocalPosition() const { return m_localPosition; }
void Transform::SetLocalPosition(const Vector3& position) { m_localPosition = position; MarkDirty(); }
Vector3 Transform::Position() const { return WorldMatrix().TransformPoint(Vector3::Zero); }
void Transform::SetPosition(const Vector3& position) {
    if (m_parent == nullptr) SetLocalPosition(position);
    else {
        Matrix4x4 inverse;
        if (m_parent->WorldMatrix().TryInverse(inverse)) SetLocalPosition(inverse.TransformPoint(position));
    }
}
const Quaternion& Transform::LocalRotation() const { return m_localRotation; }
void Transform::SetLocalRotation(const Quaternion& rotation) { m_localRotation = rotation.Normalized(); MarkDirty(); }
const Vector3& Transform::LocalScale() const { return m_localScale; }
void Transform::SetLocalScale(const Vector3& scale) { m_localScale = scale; MarkDirty(); }
void Transform::Translate(const Vector3& distance) { SetLocalPosition(m_localPosition + distance); }

bool Transform::SetParent(Transform* parent, bool keepWorldTransform) {
    if (parent == this || parent == m_parent || (parent != nullptr && parent->IsDescendantOf(this))) return false;
    const Matrix4x4 oldWorld = WorldMatrix();
    Matrix4x4 local = oldWorld;
    if (keepWorldTransform && parent != nullptr) {
        Matrix4x4 inverse;
        if (!parent->WorldMatrix().TryInverse(inverse)) return false;
        local = inverse * oldWorld;
    }
    if (m_parent != nullptr) m_parent->RemoveChild(this);
    m_parent = parent;
    if (m_parent != nullptr) m_parent->m_children.push_back(this);
    if (keepWorldTransform) {
        SetLocalMatrix(local);
    } else {
        MarkDirty();
    }
    return true;
}

Transform* Transform::Parent() const { return m_parent; }
std::size_t Transform::ChildCount() const { return m_children.size(); }
Transform* Transform::Child(std::size_t index) const { return index < m_children.size() ? m_children[index] : nullptr; }
const Matrix4x4& Transform::WorldMatrix() const { if (m_dirty) RebuildWorldMatrix(); return m_worldMatrix; }

void Transform::MarkDirty() {
    m_dirty = true;
    for (Transform* child : m_children) if (child != nullptr) child->MarkDirty();
}
bool Transform::IsDescendantOf(const Transform* candidate) const {
    for (const Transform* current = this; current != nullptr; current = current->m_parent) if (current == candidate) return true;
    return false;
}
void Transform::RemoveChild(Transform* child) {
    const auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) m_children.erase(it);
}
void Transform::SetLocalMatrix(const Matrix4x4& matrix) {
    m_localPosition = {matrix(0, 3), matrix(1, 3), matrix(2, 3)};
    const Vector3 scale{
        Vector3{matrix(0, 0), matrix(1, 0), matrix(2, 0)}.Length(),
        Vector3{matrix(0, 1), matrix(1, 1), matrix(2, 1)}.Length(),
        Vector3{matrix(0, 2), matrix(1, 2), matrix(2, 2)}.Length()};
    m_localScale = scale;
    m_localRotation = RotationFromMatrix(matrix, scale);
    MarkDirty();
}
void Transform::RebuildWorldMatrix() const {
    const Matrix4x4 local = Matrix4x4::Scale(m_localScale) * m_localRotation.ToMatrix() * Matrix4x4::Translation(m_localPosition);
    m_worldMatrix = m_parent == nullptr ? local : m_parent->WorldMatrix() * local;
    m_dirty = false;
}
