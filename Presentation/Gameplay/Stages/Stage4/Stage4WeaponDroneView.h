#pragma once

#include <cmath>

#include "../Common/BossModelTransform.h"

#include "../../../../Engine/Graphics/IRenderBackend.h"

/** @brief Stage4主砲交換ドローンの可動部位姿勢 */
struct Stage4WeaponDronePose {
    float leftShoulderPitch = 0.0f;
    float leftElbowPitch = 0.0f;
    float leftClampOpen = 0.0f;
    float rightShoulderPitch = 0.0f;
    float rightElbowPitch = 0.0f;
    float rightClampOpen = 0.0f;
    float thrusterTilt = 0.0f;
};

/** @brief Stage4交換主砲を空輸する重作業ドローンのプロシージャル描画 */
class Stage4WeaponDroneView {
public:
    static constexpr int CorePrimitiveCount = 9;
    static constexpr int ThrusterCount = 4;
    static constexpr int ThrusterPrimitiveCount = 16;
    static constexpr int ArmPrimitiveCount = 14;
    static constexpr int PrimitiveCount = CorePrimitiveCount + ThrusterPrimitiveCount +
        ArmPrimitiveCount;

    /**
     * @brief 任意Transformからドローンを単体描画する
     * @param transform ドローン全体の親Transform
     * @param pose 左右アーム、クランプ、推進器の姿勢
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, const Stage4WeaponDronePose& pose,
        DrawPart&& drawPart) {
        // 装甲コアとセンサーを描画する
        DrawCore(transform, drawPart);

        // 独立して傾けられる四基の重量物用推進器を描画する
        DrawThrusters(transform, pose.thrusterTilt, drawPart);

        // 左右で独立した二関節アームとクランプを描画する
        DrawArm(transform, true, pose.leftShoulderPitch, pose.leftElbowPitch,
            pose.leftClampOpen, drawPart);
        DrawArm(transform, false, pose.rightShoulderPitch, pose.rightElbowPitch,
            pose.rightClampOpen, drawPart);
    }

    /**
     * @brief 左作業アームのクランプ中心を取得する
     * @param pose ドローン姿勢
     * @return ドローンTransform基準のクランプ中心
     */
    static Vector3 LeftClampLocalPosition(const Stage4WeaponDronePose& pose = {}) {
        return ClampLocalPosition(true, pose.leftShoulderPitch, pose.leftElbowPitch);
    }

    /**
     * @brief 右作業アームのクランプ中心を取得する
     * @param pose ドローン姿勢
     * @return ドローンTransform基準のクランプ中心
     */
    static Vector3 RightClampLocalPosition(const Stage4WeaponDronePose& pose = {}) {
        return ClampLocalPosition(false, pose.rightShoulderPitch, pose.rightElbowPitch);
    }

    /**
     * @brief 左右クランプ間の主砲把持中心を取得する
     * @param pose ドローン姿勢
     * @return ドローンTransform基準の把持中心
     */
    static Vector3 LiftPointLocalPosition(const Stage4WeaponDronePose& pose = {}) {
        return (LeftClampLocalPosition(pose) + RightClampLocalPosition(pose)) * 0.5f;
    }

    /**
     * @brief 左右クランプ間の主砲把持中心をワールド座標で取得する
     * @param transform ドローンTransform
     * @param pose ドローン姿勢
     * @return ワールド座標の把持中心
     */
    static Vector3 LiftPointWorldPosition(const BossModelTransform& transform,
        const Stage4WeaponDronePose& pose = {}) {
        return LocalToWorldPosition(transform, LiftPointLocalPosition(pose));
    }

    /**
     * @brief 把持中心が指定ワールド座標へ一致するTransformを作る
     * @param transform 向きと拡縮を設定済みのドローンTransform
     * @param pose ドローン姿勢
     * @param targetWorldPosition 合わせる主砲CarryPoint
     * @return 位置補正済みのドローンTransform
     */
    static BossModelTransform PlaceLiftPointAt(const BossModelTransform& transform,
        const Stage4WeaponDronePose& pose, const Vector3& targetWorldPosition) {
        BossModelTransform result = transform;
        result.position += targetWorldPosition - LiftPointWorldPosition(transform, pose);
        return result;
    }

private:
    inline static constexpr float MainBlack[] = {0.03f, 0.03f, 0.035f, 1.0f};
    inline static constexpr float ArmorBlack[] = {0.08f, 0.08f, 0.09f, 1.0f};
    inline static constexpr float Metal[] = {0.25f, 0.24f, 0.22f, 1.0f};
    inline static constexpr float Gold[] = {0.55f, 0.38f, 0.08f, 1.0f};
    inline static constexpr float Sensor[] = {0.75f, 0.18f, 0.04f, 1.0f};
    static constexpr float ShoulderY = -0.30f;
    static constexpr float ShoulderZ = 0.62f;
    static constexpr float UpperArmLength = 0.48f;
    static constexpr float ForeArmLength = 0.42f;
    static constexpr float ClampDrop = 0.17f;

    /**
     * @brief ローカル座標を親Transformのワールド座標へ変換する
     * @param transform 親Transform
     * @param localPosition ローカル座標
     * @return ワールド座標
     */
    static Vector3 LocalToWorldPosition(const BossModelTransform& transform,
        const Vector3& localPosition) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        return {
            transform.position.x +
                (localPosition.x * cosine + localPosition.z * sine) * transform.scale,
            transform.position.y + localPosition.y * transform.scale,
            transform.position.z +
                (-localPosition.x * sine + localPosition.z * cosine) * transform.scale
        };
    }

    /**
     * @brief 下向きアーム軸に沿うローカル方向を取得する
     * @param pitch 下向きを0とするアーム角度
     * @return 正規化済みローカル方向
     */
    static Vector3 ArmDirection(float pitch) {
        return {std::sin(pitch), -std::cos(pitch), 0.0f};
    }

    /**
     * @brief 片側アームのクランプ中心を取得する
     * @param left 左アームの場合true
     * @param shoulderPitch 肩関節角度
     * @param elbowPitch 肘関節の相対角度
     * @return ドローンTransform基準のクランプ中心
     */
    static Vector3 ClampLocalPosition(bool left, float shoulderPitch, float elbowPitch) {
        const float side = left ? -1.0f : 1.0f;
        const Vector3 shoulder {0.0f, ShoulderY, side * ShoulderZ};
        const Vector3 elbow = shoulder + ArmDirection(shoulderPitch) * UpperArmLength;
        const float forePitch = shoulderPitch + elbowPitch;
        return elbow + ArmDirection(forePitch) * (ForeArmLength + ClampDrop);
    }

    /**
     * @brief ローカル部品へ親Transformを合成して描画する
     * @param transform 親Transform
     * @param drawPart 描画関数
     * @param shape 描画形状
     * @param localPosition ローカル座標
     * @param scale ローカル寸法
     * @param color 部品色
     * @param localYaw 部品固有のY軸回転
     * @param pitch 部品固有のZ軸回転
     * @return なし
     */
    template<class DrawPart>
    static void Part(const BossModelTransform& transform, DrawPart& drawPart,
        PrimitiveShape shape, const Vector3& localPosition, const Vector3& scale,
        const float color[4], float localYaw = 0.0f, float pitch = 0.0f) {
        drawPart(static_cast<int>(shape), LocalToWorldPosition(transform, localPosition),
            scale * transform.scale, color, transform.yaw + localYaw, pitch);
    }

    /**
     * @brief 装甲コア、整備センサー、状態灯を描画する
     * @param transform ドローンTransform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawCore(const BossModelTransform& transform, DrawPart& drawPart) {
        // 箱一個に見えない積層装甲コアを構成する
        Part(transform, drawPart, PrimitiveShape::Box, {}, {1.18f, 0.34f, 1.18f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-0.12f, 0.23f, 0.0f},
            {0.82f, 0.18f, 0.92f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-0.67f, -0.02f, 0.0f},
            {0.28f, 0.28f, 0.86f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {0.64f, 0.02f, 0.0f},
            {0.24f, 0.29f, 0.92f}, Metal);
        Part(transform, drawPart, PrimitiveShape::Box, {0.0f, -0.25f, 0.0f},
            {0.66f, 0.16f, 0.82f}, Metal);

        // 前下面の装着認識センサーと少数の状態灯を配置する
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-0.48f, -0.27f, 0.0f},
            {0.24f, 0.18f, 0.24f}, Gold, 0.0f, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Sphere, {-0.61f, -0.28f, 0.0f},
            {0.13f, 0.13f, 0.13f}, Sensor);
        Part(transform, drawPart, PrimitiveShape::Sphere, {-0.39f, 0.22f, -0.45f},
            {0.055f, 0.055f, 0.055f}, Gold);
        Part(transform, drawPart, PrimitiveShape::Sphere, {-0.39f, 0.22f, 0.45f},
            {0.055f, 0.055f, 0.055f}, Gold);
    }

    /**
     * @brief 四基のダクテッド推進器を描画する
     * @param transform ドローンTransform
     * @param thrusterTilt 推進器の傾斜角度
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawThrusters(const BossModelTransform& transform, float thrusterTilt,
        DrawPart& drawPart) {
        const float tilt = Math::Clamp(thrusterTilt, -0.65f, 0.65f);
        const Vector3 down = ArmDirection(tilt);
        for (float x : {-0.72f, 0.72f}) {
            for (float z : {-0.82f, 0.82f}) {
                const Vector3 anchor {x, 0.02f, z};

                // 太い支持桁とGoldリング、暗い吸気面、噴射ノズルを重ねる
                Part(transform, drawPart, PrimitiveShape::Box,
                    {x * 0.72f, 0.02f, z * 0.72f}, {0.48f, 0.12f, 0.18f}, Metal,
                    std::atan2(z, x));
                Part(transform, drawPart, PrimitiveShape::Cylinder, anchor + down * 0.08f,
                    {0.43f, 0.20f, 0.43f}, Gold, 0.0f, tilt);
                Part(transform, drawPart, PrimitiveShape::Cylinder, anchor + down * 0.11f,
                    {0.34f, 0.22f, 0.34f}, MainBlack, 0.0f, tilt);
                Part(transform, drawPart, PrimitiveShape::Cone, anchor + down * 0.31f,
                    {0.29f, 0.38f, 0.29f}, Metal, 0.0f, tilt);
            }
        }
    }

    /**
     * @brief 二関節作業アームと二枚爪クランプを描画する
     * @param transform ドローンTransform
     * @param left 左アームの場合true
     * @param shoulderPitch 肩関節角度
     * @param elbowPitch 肘関節の相対角度
     * @param clampOpen 0で閉、1で全開となる開閉量
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawArm(const BossModelTransform& transform, bool left, float shoulderPitch,
        float elbowPitch, float clampOpen, DrawPart& drawPart) {
        const float side = left ? -1.0f : 1.0f;
        const Vector3 shoulder {0.0f, ShoulderY, side * ShoulderZ};
        const Vector3 upperDirection = ArmDirection(shoulderPitch);
        const Vector3 elbow = shoulder + upperDirection * UpperArmLength;
        const float forePitch = shoulderPitch + elbowPitch;
        const Vector3 foreDirection = ArmDirection(forePitch);
        const Vector3 wrist = elbow + foreDirection * ForeArmLength;
        const Vector3 clamp = wrist + foreDirection * ClampDrop;

        // Gold関節で太い上腕と前腕を接続する
        Part(transform, drawPart, PrimitiveShape::Cylinder, shoulder,
            {0.20f, 0.25f, 0.20f}, Gold, Math::HalfPi, Math::HalfPi);
        Part(transform, drawPart, PrimitiveShape::Box,
            shoulder + upperDirection * (UpperArmLength * 0.5f),
            {0.20f, UpperArmLength, 0.22f}, ArmorBlack, 0.0f, -shoulderPitch);
        Part(transform, drawPart, PrimitiveShape::Sphere, elbow,
            {0.18f, 0.18f, 0.18f}, Gold);
        Part(transform, drawPart, PrimitiveShape::Box,
            elbow + foreDirection * (ForeArmLength * 0.5f),
            {0.17f, ForeArmLength, 0.19f}, Metal, 0.0f, -forePitch);
        Part(transform, drawPart, PrimitiveShape::Cylinder, wrist,
            {0.16f, 0.20f, 0.16f}, Gold, Math::HalfPi, Math::HalfPi);

        // 開閉量を二枚爪の角度と間隔へ反映する
        const float open = Math::Clamp01(clampOpen);
        const float jawPitch = 0.10f + open * 0.58f;
        const float jawOffset = 0.07f + open * 0.08f;
        Part(transform, drawPart, PrimitiveShape::Box,
            clamp + Vector3 {-jawOffset, 0.0f, 0.0f}, {0.12f, 0.34f, 0.14f},
            MainBlack, 0.0f, jawPitch);
        Part(transform, drawPart, PrimitiveShape::Box,
            clamp + Vector3 {jawOffset, 0.0f, 0.0f}, {0.12f, 0.34f, 0.14f},
            MainBlack, 0.0f, -jawPitch);
    }
};

static_assert(Stage4WeaponDroneView::PrimitiveCount == 39);
