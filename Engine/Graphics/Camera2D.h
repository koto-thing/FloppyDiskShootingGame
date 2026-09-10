#pragma once

#include "CameraMatrices.h"
#include "Viewport.h"

/** @brief 左手系の2Dワールドカメラ */
class Camera2D {
public:
    /** @brief カメラ位置を取得する */
    const Vector2& Position() const { return m_position; }
    /** @brief カメラ位置を設定する */
    void SetPosition(const Vector2& value) { m_position = value; }
    /** @brief カメラ回転を取得する */
    float Rotation() const { return m_rotation; }
    /** @brief カメラ回転を設定する */
    void SetRotation(float value) { m_rotation = value; }
    /** @brief カメラ倍率を取得する */
    float Zoom() const { return m_zoom; }
    /** @brief カメラ倍率を設定する */
    void SetZoom(float value) { m_zoom = value <= Math::Epsilon ? Math::Epsilon : value; }
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
     * @return スクリーン座標
     */
    Vector2 WorldToScreen(const Vector2& world) const;
    /**
     * @brief スクリーン座標をワールド座標へ変換する
     * @param screen スクリーン座標
     * @return ワールド座標
     */
    Vector2 ScreenToWorld(const Vector2& screen) const;
    /**
     * @brief ワールド座標をスクリーン座標へ安全に変換する
     * @param world ワールド座標
     * @param screen 変換結果の格納先
     * @return 変換できた場合true
     */
    bool TryWorldToScreen(const Vector2& world, Vector2& screen) const;
    /**
     * @brief スクリーン座標をワールド座標へ安全に変換する
     * @param screen スクリーン座標
     * @param world 変換結果の格納先
     * @return 変換できた場合true
     */
    bool TryScreenToWorld(const Vector2& screen, Vector2& world) const;
private:
    Vector2 m_position = Vector2::Zero;
    float m_rotation = 0.0f;
    float m_zoom = 1.0f;
    Viewport m_viewport {};
};
