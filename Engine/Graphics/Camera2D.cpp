#include "Camera2D.h"
#include <cmath>

Matrix4x4 Camera2D::ViewMatrix() const {
    return Matrix4x4::RotationZ(-m_rotation) * Matrix4x4::Translation(Vector3{-m_position.x, -m_position.y, 0.0f});
}

Matrix4x4 Camera2D::ProjectionMatrix() const {
    if (!m_viewport.IsValid()) return Matrix4x4::Identity;
    const float halfWidth = static_cast<float>(m_viewport.width) / (2.0f * m_zoom);
    const float halfHeight = static_cast<float>(m_viewport.height) / (2.0f * m_zoom);
    return {1.0f / halfWidth, 0, 0, 0, 0, 1.0f / halfHeight, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

Vector2 Camera2D::WorldToScreen(const Vector2& world) const { Vector2 result{}; TryWorldToScreen(world, result); return result; }
Vector2 Camera2D::ScreenToWorld(const Vector2& screen) const { Vector2 result{}; TryScreenToWorld(screen, result); return result; }

bool Camera2D::TryWorldToScreen(const Vector2& world, Vector2& screen) const {
    if (!m_viewport.IsValid()) return false;
    const Vector2 delta = world - m_position;
    const float c = std::cos(m_rotation), s = std::sin(m_rotation);
    const float cameraX = (delta.x * c + delta.y * s) * m_zoom;
    const float cameraY = (-delta.x * s + delta.y * c) * m_zoom;
    screen = {m_viewport.x + m_viewport.width * 0.5f + cameraX, m_viewport.y + m_viewport.height * 0.5f - cameraY};
    return std::isfinite(screen.x) && std::isfinite(screen.y);
}

bool Camera2D::TryScreenToWorld(const Vector2& screen, Vector2& world) const {
    if (!m_viewport.IsValid()) return false;
    const float x = (screen.x - m_viewport.x - m_viewport.width * 0.5f) / m_zoom;
    const float y = -(screen.y - m_viewport.y - m_viewport.height * 0.5f) / m_zoom;
    const float c = std::cos(m_rotation), s = std::sin(m_rotation);
    world = m_position + Vector2{x * c - y * s, x * s + y * c};
    return std::isfinite(world.x) && std::isfinite(world.y);
}
