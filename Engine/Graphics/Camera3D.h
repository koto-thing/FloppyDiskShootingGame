#pragma once

#include "CameraMatrices.h"
#include "Viewport.h"
#include "../Geometry/Ray.h"
#include "../Math/Quaternion.h"

enum class ProjectionMode { Perspective, Orthographic };

/** @brief 左手系、+Z前方の3Dカメラ */
class Camera3D {
public:
    const Vector3& Position() const { return m_position; }
    void SetPosition(const Vector3& value) { m_position = value; }
    const Quaternion& Rotation() const { return m_rotation; }
    void SetRotation(const Quaternion& value) { m_rotation = value.Normalized(); }
    bool LookAt(const Vector3& target, const Vector3& up = Vector3::Up);
    Vector3 Forward() const { return m_rotation.Rotate(Vector3::Forward).Normalized(); }
    Vector3 Right() const { return m_rotation.Rotate(Vector3::Right).Normalized(); }
    Vector3 Up() const { return m_rotation.Rotate(Vector3::Up).Normalized(); }
    ProjectionMode Mode() const { return m_mode; }
    void SetProjectionMode(ProjectionMode value) { m_mode = value; }
    float FieldOfView() const { return m_fieldOfView; }
    void SetFieldOfView(float value) { m_fieldOfView = Math::Clamp(value, Math::Epsilon, Math::Pi - Math::Epsilon); }
    float OrthographicHeight() const { return m_orthographicHeight; }
    void SetOrthographicHeight(float value) { m_orthographicHeight = value <= Math::Epsilon ? Math::Epsilon : value; }
    float NearClip() const { return m_nearClip; }
    void SetNearClip(float value) { m_nearClip = value <= Math::Epsilon ? Math::Epsilon : value; if (m_farClip <= m_nearClip) m_farClip = m_nearClip + 1.0f; }
    float FarClip() const { return m_farClip; }
    void SetFarClip(float value) { m_farClip = value <= m_nearClip ? m_nearClip + 1.0f : value; }
    const Viewport& GetViewport() const { return m_viewport; }
    void SetViewport(const Viewport& value) { m_viewport = value; }
    Matrix4x4 ViewMatrix() const;
    Matrix4x4 ProjectionMatrix() const;
    CameraMatrices Matrices() const { return {ViewMatrix(), ProjectionMatrix()}; }
    bool TryWorldToScreen(const Vector3& world, Vector2& screen, float* depth = nullptr) const;
    Ray ScreenPointToRay(const Vector2& screen) const;
private:
    Vector3 m_position = Vector3::Zero;
    Quaternion m_rotation = Quaternion::Identity;
    ProjectionMode m_mode = ProjectionMode::Perspective;
    float m_fieldOfView = Math::Pi / 3.0f;
    float m_orthographicHeight = 10.0f;
    float m_nearClip = 0.1f;
    float m_farClip = 1000.0f;
    Viewport m_viewport {};
};
