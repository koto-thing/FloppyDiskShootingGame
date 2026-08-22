#pragma once

#include "CameraMatrices.h"
#include "Viewport.h"

/** @brief 左手系の2Dワールドカメラ */
class Camera2D {
public:
    const Vector2& Position() const { return m_position; }
    void SetPosition(const Vector2& value) { m_position = value; }
    float Rotation() const { return m_rotation; }
    void SetRotation(float value) { m_rotation = value; }
    float Zoom() const { return m_zoom; }
    void SetZoom(float value) { m_zoom = value <= Math::Epsilon ? Math::Epsilon : value; }
    const Viewport& GetViewport() const { return m_viewport; }
    void SetViewport(const Viewport& value) { m_viewport = value; }
    Matrix4x4 ViewMatrix() const;
    Matrix4x4 ProjectionMatrix() const;
    CameraMatrices Matrices() const { return {ViewMatrix(), ProjectionMatrix()}; }
    Vector2 WorldToScreen(const Vector2& world) const;
    Vector2 ScreenToWorld(const Vector2& screen) const;
    bool TryWorldToScreen(const Vector2& world, Vector2& screen) const;
    bool TryScreenToWorld(const Vector2& screen, Vector2& world) const;
private:
    Vector2 m_position = Vector2::Zero;
    float m_rotation = 0.0f;
    float m_zoom = 1.0f;
    Viewport m_viewport {};
};
