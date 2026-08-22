#include "Camera3D.h"
#include <cmath>

bool Camera3D::LookAt(const Vector3& target, const Vector3& up) {
    const Vector3 forward = (target - m_position).Normalized();
    if (forward == Vector3::Zero) return false;
    Vector3 right = Vector3::Cross(up, forward).Normalized();
    if (right == Vector3::Zero) right = Vector3::Cross(Vector3::Right, forward).Normalized();
    if (right == Vector3::Zero) return false;
    const Vector3 correctedUp = Vector3::Cross(forward, right).Normalized();
    const float trace = 1.0f + right.x + correctedUp.y + forward.z;
    if (trace > Math::Epsilon) {
        const float s = 0.5f / std::sqrt(trace);
        m_rotation = Quaternion{(correctedUp.z - forward.y) * s, (forward.x - right.z) * s, (right.y - correctedUp.x) * s, 0.25f / s}.Normalized();
    } else return false;
    return true;
}

Matrix4x4 Camera3D::ViewMatrix() const {
    const Vector3 right = Right(), up = Up(), forward = Forward();
    return {right.x, right.y, right.z, -Vector3::Dot(right, m_position), up.x, up.y, up.z, -Vector3::Dot(up, m_position), forward.x, forward.y, forward.z, -Vector3::Dot(forward, m_position), 0, 0, 0, 1};
}

Matrix4x4 Camera3D::ProjectionMatrix() const {
    const float aspect = m_viewport.AspectRatio();
    if (m_mode == ProjectionMode::Orthographic) {
        const float height = m_orthographicHeight, width = height * aspect;
        return {2.0f / width, 0, 0, 0, 0, 2.0f / height, 0, 0, 0, 0, 1.0f / (m_farClip - m_nearClip), -m_nearClip / (m_farClip - m_nearClip), 0, 0, 0, 1};
    }
    const float f = 1.0f / std::tan(m_fieldOfView * 0.5f);
    return {f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, m_farClip / (m_farClip - m_nearClip), -m_nearClip * m_farClip / (m_farClip - m_nearClip), 0, 0, 1, 0};
}

bool Camera3D::TryWorldToScreen(const Vector3& world, Vector2& screen, float* depth) const {
    if (!m_viewport.IsValid()) return false;
    const Vector4 clip = ProjectionMatrix() * (ViewMatrix() * Vector4(world, 1.0f));
    if (clip.w <= Math::Epsilon || !std::isfinite(clip.w)) return false;
    const float nx = clip.x / clip.w, ny = clip.y / clip.w;
    screen = {m_viewport.x + (nx + 1.0f) * m_viewport.width * 0.5f, m_viewport.y + (1.0f - ny) * m_viewport.height * 0.5f};
    if (depth != nullptr) *depth = clip.z / clip.w;
    return std::isfinite(screen.x) && std::isfinite(screen.y);
}

Ray Camera3D::ScreenPointToRay(const Vector2& screen) const {
    if (!m_viewport.IsValid()) return Ray::Invalid();
    const float nx = (screen.x - m_viewport.x) / m_viewport.width * 2.0f - 1.0f;
    const float ny = 1.0f - (screen.y - m_viewport.y) / m_viewport.height * 2.0f;
    Matrix4x4 inverse;
    if (!Matrices().TryInverseViewProjection(inverse)) return Ray::Invalid();
    const Vector3 nearPoint = inverse.TransformPoint({nx, ny, 0.0f});
    const Vector3 farPoint = inverse.TransformPoint({nx, ny, 1.0f});
    return m_mode == ProjectionMode::Perspective ? Ray(m_position, farPoint - m_position) : Ray(nearPoint, farPoint - nearPoint);
}
