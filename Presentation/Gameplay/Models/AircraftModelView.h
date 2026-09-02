#pragma once

#include <cmath>

#include "../../../Engine/Math/Vector3.h"

/** @brief プレイヤー機と通常敵機で共有するプロシージャルモデル */
class AircraftModelView final {
public:
    /**
     * @brief プレイヤー機を描画する
     * @param position モデル中心のワールド座標
     * @param yaw Y軸回転角度
     * @param scale モデル倍率
     * @param drawPart 形状、位置、寸法、色、Yaw、Pitchを受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawPlayer(const Vector3& position, float yaw, float scale, DrawPart&& drawPart) {
        constexpr float Body[] = {0.80f, 0.80f, 0.85f, 1.0f};
        constexpr float Accent[] = {0.10f, 0.90f, 0.90f, 1.0f};
        Draw(position, yaw, scale, Body, Accent, drawPart);
    }

    /**
     * @brief 通常敵機を描画する
     * @param position モデル中心のワールド座標
     * @param yaw Y軸回転角度
     * @param scale 敵種別のモデル倍率
     * @param drawPart 形状、位置、寸法、色、Yaw、Pitchを受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawEnemy(const Vector3& position, float yaw, float scale, DrawPart&& drawPart) {
        constexpr float Body[] = {0.90f, 0.12f, 0.12f, 1.0f};
        constexpr float Accent[] = {1.00f, 0.55f, 0.08f, 1.0f};
        Draw(position, yaw, scale, Body, Accent, drawPart);
    }

private:
    /**
     * @brief 共通機体形状を指定配色で描画する
     * @param position モデル中心のワールド座標
     * @param yaw Y軸回転角度
     * @param scale モデル倍率
     * @param bodyColor 胴体色
     * @param accentColor 機首と翼の色
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const Vector3& position, float yaw, float scale,
        const float bodyColor[4], const float accentColor[4], DrawPart& drawPart) {
        Part(position, yaw, scale, 1, {}, {0.58f, 0.32f, 1.35f}, bodyColor, drawPart);
        Part(position, yaw, scale, 3, {0.0f, 0.0f, 0.82f}, {0.42f, 0.42f, 0.78f}, accentColor, drawPart);
        Part(position, yaw, scale, 4, {-0.75f, 0.0f, -0.08f}, {1.15f, 0.12f, 0.62f}, accentColor, drawPart);
        Part(position, yaw, scale, 4, {0.75f, 0.0f, -0.08f}, {1.15f, 0.12f, 0.62f}, accentColor, drawPart);
    }

    /**
     * @brief 一つの機体部品を親座標とYawへ合成する
     * @param position モデル中心のワールド座標
     * @param yaw Y軸回転角度
     * @param scale モデル倍率
     * @param shape 既存PrimitiveShapeに対応する番号
     * @param localPosition 部品のローカル座標
     * @param localScale 部品のローカル寸法
     * @param color 部品色
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void Part(const Vector3& position, float yaw, float scale, int shape,
        const Vector3& localPosition, const Vector3& localScale, const float color[4], DrawPart& drawPart) {
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        const Vector3 offset {
            (localPosition.x * cosine + localPosition.z * sine) * scale,
            localPosition.y * scale,
            (-localPosition.x * sine + localPosition.z * cosine) * scale
        };
        drawPart(shape, position + offset, localScale * scale, color, yaw, 0.0f);
    }
};
