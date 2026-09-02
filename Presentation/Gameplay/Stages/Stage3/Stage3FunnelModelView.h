#pragma once

#include <cmath>

#include "../Common/BossModelTransform.h"
#include "../../../../Engine/Math/Math.h"

/** @brief Stage3ファンネルの可動姿勢 */
struct Stage3FunnelPose {
    float emitterOpenAmount = 0.0f;
    float gunYaw = 0.0f;
    float gunPitch = 0.0f;
    float barrelRecoil = 0.0f;
};

/** @brief Stage3ボス用ファンネルのプロシージャル描画 */
class Stage3FunnelModelView final {
public:
    static constexpr int CommonPrimitiveCount = 14;
    static constexpr int BarrierEmitterPrimitiveCount = 4;
    static constexpr int BarrierPrimitiveCount = CommonPrimitiveCount + BarrierEmitterPrimitiveCount * 2;
    static constexpr int ReflectTurretPrimitiveCount = 2;
    static constexpr int ReflectBarrelPrimitiveCount = 4;
    static constexpr int ReflectShotPrimitiveCount =
        CommonPrimitiveCount + ReflectTurretPrimitiveCount + ReflectBarrelPrimitiveCount;

    /**
     * @brief Barrier Funnelを描画する
     * @param transform 親Transform
     * @param emitterOpenAmount 0を収納、1を完全展開とする開度
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawBarrier(const BossModelTransform& transform, float emitterOpenAmount, DrawPart&& drawPart) {
        DrawCommonBody(transform, drawPart, BarrierBody, BarrierArmor, BarrierDark, BarrierEnergy);
        DrawBarrierEmitter(0, transform, emitterOpenAmount, drawPart);
        DrawBarrierEmitter(1, transform, emitterOpenAmount, drawPart);
    }

    /**
     * @brief Barrier FunnelのEmitterを個別に描画する
     * @param index 0を左、1を右とするEmitter番号
     * @param transform 親Transform
     * @param emitterOpenAmount 0を収納、1を完全展開とする開度
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawBarrierEmitter(int index, const BossModelTransform& transform,
        float emitterOpenAmount, DrawPart&& drawPart) {
        const float side = index == 0 ? -1.0f : 1.0f;
        const float angle = side * Math::ToRadians(42.0f) * Math::Clamp01(emitterOpenAmount);
        const Vector3 pivot {0.02f, 0.0f, side * 0.43f};

        // Emitter支柱、装甲、発生器、先端を同じ支点で開閉する
        PivotPart(transform, drawPart, pivot, {0.0f, 0.0f, side * 0.16f}, angle,
            1, {0.42f, 0.10f, 0.34f}, BarrierArmor);
        PivotPart(transform, drawPart, pivot, {0.04f, 0.0f, side * 0.35f}, angle,
            2, {0.20f, 0.16f, 0.20f}, BarrierDark);
        PivotPart(transform, drawPart, pivot, {-0.02f, 0.0f, side * 0.48f}, angle,
            5, {0.20f, 0.20f, 0.20f}, BarrierEnergy);
        PivotPart(transform, drawPart, pivot, {-0.12f, 0.0f, side * 0.60f}, angle,
            3, {0.14f, 0.20f, 0.14f}, BarrierEnergy, side * Math::HalfPi);
    }

    /**
     * @brief Barrier発生点のモデルローカル座標を取得する
     * @param index 0を左、1を右とするEmitter番号
     * @param emitterOpenAmount 0を収納、1を完全展開とする開度
     * @return Emitter先端のモデルローカル座標
     */
    static Vector3 BarrierAnchorLocalPosition(int index, float emitterOpenAmount) {
        const float side = index == 0 ? -1.0f : 1.0f;
        const float angle = side * Math::ToRadians(42.0f) * Math::Clamp01(emitterOpenAmount);
        return RotateAroundY({0.02f, 0.0f, side * 0.43f}, {-0.14f, 0.0f, side * 0.68f}, angle);
    }

    /**
     * @brief Reflect Shot Funnelを描画する
     * @param transform 親Transform
     * @param gunYaw 砲塔Yaw
     * @param gunPitch 砲身Pitch
     * @param barrelRecoil 0を通常位置とする砲身後退量
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawReflectShot(const BossModelTransform& transform, float gunYaw,
        float gunPitch, float barrelRecoil, DrawPart&& drawPart) {
        DrawCommonBody(transform, drawPart, ReflectBody, ReflectArmor, ReflectDark, ReflectCore);
        DrawReflectGunTurret(transform, gunYaw, drawPart);
        DrawReflectGunBarrel(transform, gunYaw, gunPitch, barrelRecoil, drawPart);
    }

    /**
     * @brief Reflect Shot砲塔を独立描画する
     * @param transform 親Transform
     * @param gunYaw 砲塔Yaw
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawReflectGunTurret(const BossModelTransform& transform, float gunYaw, DrawPart&& drawPart) {
        const Vector3 pivot {-0.18f, 0.0f, 0.0f};
        AimedPart(transform, drawPart, pivot, {0.0f, 0.0f, 0.0f}, gunYaw, 0.0f,
            2, {0.34f, 0.16f, 0.34f}, ReflectDark);
        AimedPart(transform, drawPart, pivot, {-0.12f, 0.0f, 0.0f}, gunYaw, 0.0f,
            4, {0.38f, 0.24f, 0.30f}, ReflectWeapon);
    }

    /**
     * @brief Reflect Shot砲身を独立描画する
     * @param transform 親Transform
     * @param gunYaw 砲塔Yaw
     * @param gunPitch 砲身Pitch
     * @param barrelRecoil 0を通常位置とする砲身後退量
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawReflectGunBarrel(const BossModelTransform& transform, float gunYaw,
        float gunPitch, float barrelRecoil, DrawPart&& drawPart) {
        const Vector3 pivot {-0.18f, 0.0f, 0.0f};
        const float recoil = Math::Clamp01(barrelRecoil) * 0.16f;
        AimedPart(transform, drawPart, pivot, {-0.30f + recoil, 0.0f, 0.0f}, gunYaw, gunPitch,
            2, {0.16f, 0.46f, 0.16f}, ReflectWeapon, Math::HalfPi);
        AimedPart(transform, drawPart, pivot, {-0.57f + recoil, 0.0f, 0.0f}, gunYaw, gunPitch,
            2, {0.10f, 0.42f, 0.10f}, ReflectMetal, Math::HalfPi);
        AimedPart(transform, drawPart, pivot, {-0.80f + recoil, 0.0f, 0.0f}, gunYaw, gunPitch,
            2, {0.18f, 0.16f, 0.18f}, ReflectDark, Math::HalfPi);
        AimedPart(transform, drawPart, pivot, {-0.89f + recoil, 0.0f, 0.0f}, gunYaw, gunPitch,
            2, {0.11f, 0.05f, 0.11f}, ReflectCore, Math::HalfPi);
    }

    /**
     * @brief Reflect Shot砲口のモデルローカル座標を取得する
     * @param gunYaw 砲塔Yaw
     * @param gunPitch 砲身Pitch
     * @param barrelRecoil 0を通常位置とする砲身後退量
     * @return 砲口先端のモデルローカル座標
     */
    static Vector3 ReflectShotMuzzleLocalPosition(float gunYaw, float gunPitch, float barrelRecoil) {
        const float distance = 1.00f - Math::Clamp01(barrelRecoil) * 0.16f;
        return Vector3 {-0.18f, 0.0f, 0.0f} +
            RotateYawPitch({-distance, 0.0f, 0.0f}, gunYaw, gunPitch);
    }

private:
    inline static constexpr float BarrierBody[4] = {0.15f, 0.16f, 0.16f, 1.0f};
    inline static constexpr float BarrierArmor[4] = {0.08f, 0.09f, 0.10f, 1.0f};
    inline static constexpr float BarrierDark[4] = {0.035f, 0.05f, 0.06f, 1.0f};
    inline static constexpr float BarrierEnergy[4] = {0.12f, 0.55f, 0.85f, 1.0f};
    inline static constexpr float ReflectBody[4] = {0.17f, 0.15f, 0.13f, 1.0f};
    inline static constexpr float ReflectArmor[4] = {0.08f, 0.07f, 0.06f, 1.0f};
    inline static constexpr float ReflectDark[4] = {0.04f, 0.035f, 0.03f, 1.0f};
    inline static constexpr float ReflectWeapon[4] = {0.30f, 0.27f, 0.22f, 1.0f};
    inline static constexpr float ReflectMetal[4] = {0.42f, 0.38f, 0.30f, 1.0f};
    inline static constexpr float ReflectCore[4] = {0.85f, 0.20f, 0.05f, 1.0f};

    /**
     * @brief 両タイプ共通の固定船体を描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param bodyColor 胴体色
     * @param armorColor 装甲色
     * @param darkColor 暗部色
     * @param coreColor 発光部色
     * @return なし
     */
    template<class DrawPart>
    static void DrawCommonBody(const BossModelTransform& transform, DrawPart& drawPart,
        const float bodyColor[4], const float armorColor[4], const float darkColor[4],
        const float coreColor[4]) {
        // 扁平な中央胴体と尖った機首を重ねて小型無人戦闘機の輪郭を作る
        Part(transform, drawPart, 5, {0.0f, 0.0f, 0.0f}, {0.92f, 0.42f, 0.58f}, bodyColor);
        Part(transform, drawPart, 4, {-0.57f, 0.0f, 0.0f}, {0.58f, 0.32f, 0.44f}, armorColor);
        Part(transform, drawPart, 3, {-0.91f, 0.0f, 0.0f}, {0.34f, 0.22f, 0.22f}, armorColor, 0.0f, Math::HalfPi);
        Part(transform, drawPart, 1, {0.35f, 0.20f, 0.0f}, {0.60f, 0.10f, 0.42f}, armorColor);
        Part(transform, drawPart, 5, {-0.28f, 0.20f, 0.0f}, {0.24f, 0.16f, 0.24f}, coreColor);

        // 左右翼と垂直フィンを固定部として共通化する
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, 4, {0.08f, -0.02f, side * 0.43f}, {0.64f, 0.10f, 0.46f}, armorColor);
            Part(transform, drawPart, 1, {0.40f, 0.12f, side * 0.32f}, {0.32f, 0.30f, 0.08f}, armorColor);
        }

        // 後部の双発推進器と発光排気を独立Primitiveで示す
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, 2, {0.55f, -0.06f, side * 0.19f}, {0.18f, 0.28f, 0.18f}, darkColor,
                0.0f, Math::HalfPi);
            Part(transform, drawPart, 3, {0.74f, -0.06f, side * 0.19f}, {0.16f, 0.24f, 0.16f}, coreColor,
                0.0f, -Math::HalfPi);
        }
        Part(transform, drawPart, 5, {-0.58f, 0.08f, -0.22f}, {0.12f, 0.12f, 0.12f}, coreColor);
    }

    static Vector3 RotateAroundY(const Vector3& pivot, const Vector3& position, float angle) {
        const Vector3 offset = position - pivot;
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);
        return pivot + Vector3 {offset.x * cosine + offset.z * sine, offset.y,
            -offset.x * sine + offset.z * cosine};
    }

    static Vector3 RotateYawPitch(const Vector3& position, float yaw, float pitch) {
        const float pitchCosine = std::cos(pitch);
        const float pitchSine = std::sin(pitch);
        const Vector3 pitched {position.x * pitchCosine - position.y * pitchSine,
            position.x * pitchSine + position.y * pitchCosine, position.z};
        const float yawCosine = std::cos(yaw);
        const float yawSine = std::sin(yaw);
        return {pitched.x * yawCosine + pitched.z * yawSine, pitched.y,
            -pitched.x * yawSine + pitched.z * yawCosine};
    }

    template<class DrawPart>
    static void PivotPart(const BossModelTransform& transform, DrawPart& drawPart,
        const Vector3& pivot, const Vector3& position, float angle, int shape,
        const Vector3& scale, const float color[4], float localPitch = 0.0f) {
        Part(transform, drawPart, shape, RotateAroundY(pivot, pivot + position, angle), scale, color,
            angle, localPitch);
    }

    template<class DrawPart>
    static void AimedPart(const BossModelTransform& transform, DrawPart& drawPart,
        const Vector3& pivot, const Vector3& position, float yaw, float pitch, int shape,
        const Vector3& scale, const float color[4], float shapePitch = 0.0f) {
        Part(transform, drawPart, shape, pivot + RotateYawPitch(position, yaw, pitch), scale, color,
            yaw, pitch + shapePitch);
    }

    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart, int shape,
        const Vector3& localPosition, const Vector3& scale, const float color[4],
        float localYaw = 0.0f, float localPitch = 0.0f) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 position {
            transform.position.x + (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z + (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
        drawPart(shape, position, scale * transform.scale, color,
            transform.yaw + localYaw, localPitch);
    }
};

static_assert(Stage3FunnelModelView::BarrierPrimitiveCount == 22);
static_assert(Stage3FunnelModelView::ReflectShotPrimitiveCount == 20);
