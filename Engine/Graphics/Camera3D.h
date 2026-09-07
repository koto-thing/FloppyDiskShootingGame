#pragma once

#include "CameraMatrices.h"
#include "Viewport.h"
#include "../Geometry/Ray.h"
#include "../Math/Quaternion.h"

enum class ProjectionMode { Perspective, Orthographic };

/** @brief 左手系、+Z前方の3Dカメラ */
class Camera3D {
public:
    /** @brief カメラ位置を取得する */
    const Vector3& Position() const { return m_position; }
    /** @brief カメラ位置を設定する */
    void SetPosition(const Vector3& value) { m_position = value; }
    /** @brief カメラ回転を取得する */
    const Quaternion& Rotation() const { return m_rotation; }
    /** @brief カメラ回転を設定する */
    void SetRotation(const Quaternion& value) { m_rotation = value.Normalized(); }
    /**
     * @brief 指定した位置へカメラを向ける
     * @param target 注視点
     * @param up 上方向の基準
     * @return 姿勢を設定できた場合true
     */
    bool LookAt(const Vector3& target, const Vector3& up = Vector3::Up);
    /** @brief カメラの前方向を取得する */
    Vector3 Forward() const { return m_rotation.Rotate(Vector3::Forward).Normalized(); }
    /** @brief カメラの右方向を取得する */
    Vector3 Right() const { return m_rotation.Rotate(Vector3::Right).Normalized(); }
    /** @brief カメラの上方向を取得する */
    Vector3 Up() const { return m_rotation.Rotate(Vector3::Up).Normalized(); }
    /** @brief 投影方式を取得する */
    ProjectionMode Mode() const { return m_mode; }
    /** @brief 投影方式を設定する */
    void SetProjectionMode(ProjectionMode value) { m_mode = value; }
    /** @brief 視野角を取得する */
    float FieldOfView() const { return m_fieldOfView; }
    /** @brief 視野角を設定する */
    void SetFieldOfView(float value) { m_fieldOfView = Math::Clamp(value, Math::Epsilon, Math::Pi - Math::Epsilon); }
    /** @brief 正射影の高さを取得する */
    float OrthographicHeight() const { return m_orthographicHeight; }
    /** @brief 正射影の高さを設定する */
    void SetOrthographicHeight(float value) { m_orthographicHeight = value <= Math::Epsilon ? Math::Epsilon : value; }
    /** @brief ニアクリップ距離を取得する */
    float NearClip() const { return m_nearClip; }
    /** @brief ニアクリップ距離を設定する */
    void SetNearClip(float value) { m_nearClip = value <= Math::Epsilon ? Math::Epsilon : value; if (m_farClip <= m_nearClip) m_farClip = m_nearClip + 1.0f; }
    /** @brief ファークリップ距離を取得する */
    float FarClip() const { return m_farClip; }
    /** @brief ファークリップ距離を設定する */
    void SetFarClip(float value) { m_farClip = value <= m_nearClip ? m_nearClip + 1.0f : value; }
    /** @brief 描画領域を取得する */
    const Viewport& GetViewport() const { return m_viewport; }
    /** @brief 描画領域を設定する */
    void SetViewport(const Viewport& value) { m_viewport = value; }
    /** @brief ビュー行列を生成する */
    Matrix4x4 ViewMatrix() const;
    /** @brief 投影行列を生成する */
    Matrix4x4 ProjectionMatrix() const;
    /** @brief ビュー行列と投影行列を取得する */
    CameraMatrices Matrices() const { return {ViewMatrix(), ProjectionMatrix()}; }
    /**
     * @brief ワールド座標をスクリーン座標へ変換する
     * @param world ワールド座標
     * @param screen 変換結果の格納先
     * @param depth 深度の格納先
     * @return 変換できた場合true
     */
    bool TryWorldToScreen(const Vector3& world, Vector2& screen, float* depth = nullptr) const;
    /**
     * @brief スクリーン座標からレイを生成する
     * @param screen スクリーン座標
     * @return スクリーン座標を通るレイ
     */
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
