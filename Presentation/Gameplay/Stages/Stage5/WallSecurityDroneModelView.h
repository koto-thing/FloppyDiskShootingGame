#pragma once

#include "Stage5ModelView.h"

/** @brief 壁面警備ドローンの可動部位姿勢 */
struct WallSecurityDronePose {
    float sensorYaw = 0.0f;
    float sensorPitch = 0.0f;
    float searchLightYaw = 0.0f;
    float searchLightPitch = -0.35f;
    float machineGunYaw = 0.0f;
    float machineGunPitch = 0.0f;
    float machineGunDeployment = 0.0f;
    float contactExtension = 1.0f;
    ColorF warningLightColor {0.10f, 0.65f, 0.85f, 1.0f};
};

/** @brief 壁面警備ドローンの描画部位 */
enum class WallSecurityDronePartGroup {
    StaticBody,
    SensorHead,
    SearchLight,
    MachineGun,
    WallContactUnit,
    WarningLight
};

/** @brief NEO AIZUのビル外壁を巡回する警備ドローンのプロシージャル描画 */
class WallSecurityDroneModelView final {
public:
    static constexpr int StaticPrimitiveCount = 21;
    static constexpr int SensorPrimitiveCount = 4;
    static constexpr int SearchLightPrimitiveCount = 6;
    static constexpr int MachineGunPrimitiveCount = 7;
    static constexpr int WallContactPrimitiveCount = 12;
    static constexpr int WarningLightPrimitiveCount = 2;
    static constexpr int PrimitiveCount = StaticPrimitiveCount + SensorPrimitiveCount +
        SearchLightPrimitiveCount + MachineGunPrimitiveCount + WallContactPrimitiveCount +
        WarningLightPrimitiveCount;

    inline static constexpr ColorF PatrolWarning {0.10f, 0.65f, 0.85f, 1.0f};
    inline static constexpr ColorF DetectedWarning {0.95f, 0.42f, 0.06f, 1.0f};
    inline static constexpr ColorF AttackWarning {0.85f, 0.08f, 0.035f, 1.0f};

    /**
     * @brief 全部位を指定姿勢で描画する
     * @param transform モデル全体のTransform
     * @param pose センサー、灯体、機関銃、接触機構の姿勢
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawAll(const Stage5ModelTransform& transform, const WallSecurityDronePose& pose,
        DrawPart&& drawPart) {
        // 固定部と独立可動部を同じモデルTransformから描画する
        DrawStaticBody(transform, drawPart);
        DrawSensor(transform, pose.sensorYaw, pose.sensorPitch, drawPart);
        DrawSearchLight(transform, pose.searchLightYaw, pose.searchLightPitch, drawPart);
        DrawMachineGun(transform, pose.machineGunYaw, pose.machineGunPitch,
            pose.machineGunDeployment, drawPart);
        DrawWallContactUnit(transform, pose.contactExtension, drawPart);
        DrawWarningLight(transform, pose.warningLightColor, drawPart);
    }

    /**
     * @brief 中央装甲、左右推進器、アンテナを描画する
     * @param transform モデル全体のTransform
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawStaticBody(const Stage5ModelTransform& transform, DrawPart&& drawPart) {
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);

        // 箱一個に見えない積層装甲ボディを構成する
        Part(root, drawPart, PrimitiveShape::Box, {}, {0.94f, 0.58f, 0.48f}, MainArmor,
            WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Prism, {0.0f, 0.03f, -0.30f},
            {0.80f, 0.47f, 0.18f}, Armor, WallSecurityDronePartGroup::StaticBody,
            {0.0f, Math::Pi, 0.0f});
        Part(root, drawPart, PrimitiveShape::Box, {-0.50f, 0.01f, -0.01f},
            {0.16f, 0.48f, 0.42f}, Armor, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {0.50f, 0.01f, -0.01f},
            {0.16f, 0.48f, 0.42f}, Armor, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {0.0f, 0.34f, 0.01f},
            {0.72f, 0.14f, 0.40f}, SecondaryArmor, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {0.0f, -0.34f, 0.04f},
            {0.70f, 0.13f, 0.34f}, Dark, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {0.0f, 0.00f, 0.29f},
            {0.70f, 0.40f, 0.16f}, Dark, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {0.0f, -0.17f, -0.34f},
            {0.26f, 0.11f, 0.06f}, Dark, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {-0.31f, 0.23f, -0.35f},
            {0.18f, 0.055f, 0.055f}, Accent, WallSecurityDronePartGroup::StaticBody);
        Part(root, drawPart, PrimitiveShape::Box, {0.31f, 0.23f, -0.35f},
            {0.18f, 0.055f, 0.055f}, Accent, WallSecurityDronePartGroup::StaticBody);

        // 左右へ太いダクテッドスラスターを張り出す
        for (float side : {-1.0f, 1.0f}) {
            Part(root, drawPart, PrimitiveShape::Box, {side * 0.53f, 0.04f, 0.05f},
                {0.24f, 0.16f, 0.22f}, SecondaryArmor,
                WallSecurityDronePartGroup::StaticBody);
            Part(root, drawPart, PrimitiveShape::Cylinder, {side * 0.67f, 0.04f, 0.02f},
                {0.38f, 0.24f, 0.38f}, Armor, WallSecurityDronePartGroup::StaticBody,
                {Math::HalfPi, 0.0f, 0.0f});
            Part(root, drawPart, PrimitiveShape::Cylinder, {side * 0.67f, 0.04f, -0.115f},
                {0.28f, 0.05f, 0.28f}, Dark, WallSecurityDronePartGroup::StaticBody,
                {Math::HalfPi, 0.0f, 0.0f});
            Part(root, drawPart, PrimitiveShape::Sphere, {side * 0.67f, 0.04f, -0.15f},
                {0.105f, 0.105f, 0.055f}, Accent,
                WallSecurityDronePartGroup::StaticBody);
        }

        // 低解像度でも残る太さで二本の通信アンテナを立てる
        Part(root, drawPart, PrimitiveShape::Cylinder, {0.28f, 0.54f, 0.06f},
            {0.045f, 0.30f, 0.045f}, Dark, WallSecurityDronePartGroup::StaticBody,
            {0.0f, 0.0f, -0.16f});
        Part(root, drawPart, PrimitiveShape::Cylinder, {-0.22f, 0.52f, 0.10f},
            {0.045f, 0.25f, 0.045f}, Dark, WallSecurityDronePartGroup::StaticBody,
            {0.0f, 0.0f, 0.12f});
        Part(root, drawPart, PrimitiveShape::Sphere, {0.315f, 0.69f, 0.06f},
            {0.065f, 0.065f, 0.065f}, Accent,
            WallSecurityDronePartGroup::StaticBody);
    }

    /**
     * @brief 前面監視センサーをYawとPitchで描画する
     * @param transform モデル全体のTransform
     * @param sensorYaw センサーのY軸回転角度
     * @param sensorPitch センサーのX軸回転角度
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawSensor(const Stage5ModelTransform& transform, float sensorYaw,
        float sensorPitch, DrawPart&& drawPart) {
        const Matrix4x4 aimed = Stage5ModelDetail::Matrix(transform) *
            AimPivot(SensorPivot, sensorYaw, sensorPitch);

        // 装甲筒、リング、レンズ、上部バイザーを同じ照準軸へ追従させる
        Part(aimed, drawPart, PrimitiveShape::Cylinder, {}, {0.31f, 0.16f, 0.31f}, Dark,
            WallSecurityDronePartGroup::SensorHead, {Math::HalfPi, 0.0f, 0.0f});
        Part(aimed, drawPart, PrimitiveShape::Cylinder, {0.0f, 0.0f, -0.095f},
            {0.25f, 0.055f, 0.25f}, SecondaryArmor,
            WallSecurityDronePartGroup::SensorHead, {Math::HalfPi, 0.0f, 0.0f});
        Part(aimed, drawPart, PrimitiveShape::Sphere, {0.0f, 0.0f, -0.14f},
            {0.18f, 0.18f, 0.08f}, Sensor, WallSecurityDronePartGroup::SensorHead);
        Part(aimed, drawPart, PrimitiveShape::Box, {0.0f, 0.14f, -0.10f},
            {0.37f, 0.07f, 0.15f}, Armor, WallSecurityDronePartGroup::SensorHead);
    }

    /**
     * @brief 下面サーチライトを独立YawとPitchで描画する
     * @param transform モデル全体のTransform
     * @param searchLightYaw サーチライトのY軸回転角度
     * @param searchLightPitch サーチライトのX軸回転角度
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawSearchLight(const Stage5ModelTransform& transform, float searchLightYaw,
        float searchLightPitch, DrawPart&& drawPart) {
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);
        const Matrix4x4 yawPivot = root * Matrix4x4::Translation(SearchLightPivot) *
            Matrix4x4::RotationY(searchLightYaw);
        const Matrix4x4 pitchPivot = yawPivot * Matrix4x4::RotationX(searchLightPitch);

        // Yaw基部と左右アームは機体下面へ固定する
        Part(root, drawPart, PrimitiveShape::Cylinder, SearchLightPivot,
            {0.22f, 0.10f, 0.22f}, Dark, WallSecurityDronePartGroup::SearchLight);
        Part(yawPivot, drawPart, PrimitiveShape::Box, {-0.15f, -0.055f, -0.03f},
            {0.07f, 0.18f, 0.18f}, SecondaryArmor,
            WallSecurityDronePartGroup::SearchLight);
        Part(yawPivot, drawPart, PrimitiveShape::Box, {0.15f, -0.055f, -0.03f},
            {0.07f, 0.18f, 0.18f}, SecondaryArmor,
            WallSecurityDronePartGroup::SearchLight);
        Part(yawPivot, drawPart, PrimitiveShape::Cylinder, {0.0f, -0.10f, -0.03f},
            {0.13f, 0.34f, 0.13f}, Accent, WallSecurityDronePartGroup::SearchLight,
            {0.0f, 0.0f, Math::HalfPi});

        // Pitch軸の灯体と大径レンズを前方へ向ける
        Part(pitchPivot, drawPart, PrimitiveShape::Cylinder, {0.0f, -0.10f, -0.16f},
            {0.30f, 0.28f, 0.30f}, Dark, WallSecurityDronePartGroup::SearchLight,
            {Math::HalfPi, 0.0f, 0.0f});
        Part(pitchPivot, drawPart, PrimitiveShape::Sphere, {0.0f, -0.10f, -0.32f},
            {0.23f, 0.23f, 0.075f}, SearchLight,
            WallSecurityDronePartGroup::SearchLight);
    }

    /**
     * @brief 二連装機関銃を独立YawとPitchで描画する
     * @param transform モデル全体のTransform
     * @param machineGunYaw 機関銃のY軸回転角度
     * @param machineGunPitch 機関銃のX軸回転角度
     * @param deployment 0を収納、1を完全展開とする展開量
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawMachineGun(const Stage5ModelTransform& transform, float machineGunYaw,
        float machineGunPitch, float deployment, DrawPart&& drawPart) {
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);
        const Matrix4x4 yawPivot = root * Matrix4x4::Translation(
            MachineGunPivot(deployment)) * Matrix4x4::RotationY(machineGunYaw);
        const Matrix4x4 pitchPivot = yawPivot * Matrix4x4::RotationX(machineGunPitch);

        // 展開するYaw基部と横向きPitch関節を構成する
        Part(yawPivot, drawPart, PrimitiveShape::Cylinder, {}, {0.18f, 0.10f, 0.18f}, Dark,
            WallSecurityDronePartGroup::MachineGun);
        Part(yawPivot, drawPart, PrimitiveShape::Cylinder, {0.0f, -0.08f, -0.04f},
            {0.12f, 0.30f, 0.12f}, Accent, WallSecurityDronePartGroup::MachineGun,
            {0.0f, 0.0f, Math::HalfPi});

        // 低解像度でも武装と読める太い二連装砲身をPitch軸へ追従させる
        Part(pitchPivot, drawPart, PrimitiveShape::Box, {0.0f, -0.08f, -0.13f},
            {0.30f, 0.20f, 0.28f}, Armor, WallSecurityDronePartGroup::MachineGun);
        for (float side : {-1.0f, 1.0f}) {
            Part(pitchPivot, drawPart, PrimitiveShape::Cylinder,
                {side * 0.085f, -0.08f, -0.34f}, {0.075f, 0.36f, 0.075f}, Dark,
                WallSecurityDronePartGroup::MachineGun, {Math::HalfPi, 0.0f, 0.0f});
            Part(pitchPivot, drawPart, PrimitiveShape::Cylinder,
                {side * 0.085f, -0.08f, -0.53f}, {0.095f, 0.055f, 0.095f}, Accent,
                WallSecurityDronePartGroup::MachineGun, {Math::HalfPi, 0.0f, 0.0f});
        }
    }

    /**
     * @brief 背面四点接触機構を伸縮量付きで描画する
     * @param transform モデル全体のTransform
     * @param contactExtension 0を収納、1を壁面へ完全展開とする伸縮量
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawWallContactUnit(const Stage5ModelTransform& transform,
        float contactExtension, DrawPart&& drawPart) {
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);
        const float extension = Math::Clamp01(contactExtension);
        const float armLength = 0.10f + extension * 0.18f;
        const float padZ = 0.34f + armLength;

        // 四隅へサスペンションアーム、関節、磁着パッドを配置する
        for (float x : {-0.31f, 0.31f}) {
            for (float y : {-0.20f, 0.20f}) {
                Part(root, drawPart, PrimitiveShape::Box,
                    {x, y, 0.34f + armLength * 0.5f}, {0.085f, 0.085f, armLength}, Dark,
                    WallSecurityDronePartGroup::WallContactUnit);
                Part(root, drawPart, PrimitiveShape::Sphere, {x, y, padZ - 0.025f},
                    {0.105f, 0.105f, 0.105f}, SecondaryArmor,
                    WallSecurityDronePartGroup::WallContactUnit);
                Part(root, drawPart, PrimitiveShape::Cylinder, {x, y, padZ + 0.035f},
                    {0.20f, 0.075f, 0.20f}, Accent,
                    WallSecurityDronePartGroup::WallContactUnit,
                    {Math::HalfPi, 0.0f, 0.0f});
            }
        }
    }

    /**
     * @brief 上部警告灯を外部指定色で描画する
     * @param transform モデル全体のTransform
     * @param color 状態を表す警告灯色
     * @param drawPart 形状、ワールド行列、色、部位を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawWarningLight(const Stage5ModelTransform& transform, const ColorF& color,
        DrawPart&& drawPart) {
        const Matrix4x4 root = Stage5ModelDetail::Matrix(transform);

        // 装甲基部と大きな発光ドームを重ねる
        Part(root, drawPart, PrimitiveShape::Cylinder, {-0.02f, 0.48f, -0.08f},
            {0.19f, 0.11f, 0.19f}, Dark, WallSecurityDronePartGroup::WarningLight);
        Part(root, drawPart, PrimitiveShape::Sphere, {-0.02f, 0.58f, -0.08f},
            {0.15f, 0.18f, 0.15f}, color, WallSecurityDronePartGroup::WarningLight);
    }

    /**
     * @brief センサー回転中心のモデルローカル位置を取得する
     * @return センサーYawとPitchの回転中心
     */
    static constexpr Vector3 SensorLocalPosition() {
        return SensorPivot;
    }

    /**
     * @brief サーチライト照射原点のモデルローカル位置を取得する
     * @param yaw サーチライトのY軸回転角度
     * @param pitch サーチライトのX軸回転角度
     * @return レンズ前面中央のモデルローカル位置
     */
    static Vector3 SearchLightOriginLocalPosition(float yaw, float pitch) {
        return AimPivot(SearchLightPivot, yaw, pitch).TransformPoint(
            {0.0f, -0.10f, -0.37f});
    }

    /**
     * @brief サーチライトのモデルローカル照射方向を取得する
     * @param yaw サーチライトのY軸回転角度
     * @param pitch サーチライトのX軸回転角度
     * @return 正規化済みの照射方向
     */
    static Vector3 SearchLightForwardLocalDirection(float yaw, float pitch) {
        return (Matrix4x4::RotationY(yaw) * Matrix4x4::RotationX(pitch))
            .TransformVector(Vector3::Back).Normalized();
    }

    /**
     * @brief 機関銃の弾生成位置をモデルローカル座標で取得する
     * @param yaw 機関銃のY軸回転角度
     * @param pitch 機関銃のX軸回転角度
     * @param deployment 0を収納、1を完全展開とする展開量
     * @return 二砲身間にある弾生成位置
     */
    static Vector3 MachineGunMuzzleLocalPosition(float yaw, float pitch,
        float deployment = 1.0f) {
        return AimPivot(MachineGunPivot(deployment), yaw, pitch).TransformPoint(
            {0.0f, -0.08f, -0.565f});
    }

private:
    inline static constexpr ColorF MainArmor {0.10f, 0.11f, 0.12f, 1.0f};
    inline static constexpr ColorF Armor {0.16f, 0.17f, 0.18f, 1.0f};
    inline static constexpr ColorF SecondaryArmor {0.24f, 0.25f, 0.27f, 1.0f};
    inline static constexpr ColorF Dark {0.035f, 0.04f, 0.045f, 1.0f};
    inline static constexpr ColorF Sensor {0.10f, 0.65f, 0.85f, 1.0f};
    inline static constexpr ColorF SearchLight {0.95f, 0.80f, 0.45f, 1.0f};
    inline static constexpr ColorF Accent {0.68f, 0.08f, 0.42f, 1.0f};
    inline static constexpr Vector3 SensorPivot {0.0f, 0.07f, -0.39f};
    inline static constexpr Vector3 SearchLightPivot {-0.20f, -0.39f, -0.14f};

    /**
     * @brief 機関銃の収納位置と展開位置を補間する
     * @param deployment 0を収納、1を完全展開とする展開量
     * @return 機関銃Yaw回転中心のモデルローカル位置
     */
    static constexpr Vector3 MachineGunPivot(float deployment) {
        return Vector3::Lerp({0.27f, -0.25f, -0.08f}, {0.27f, -0.42f, -0.23f},
            Math::Clamp01(deployment));
    }

    /**
     * @brief ローカル回転中心へYawとPitchを合成する
     * @param pivot モデルローカル回転中心
     * @param yaw Y軸回転角度
     * @param pitch X軸回転角度
     * @return モデルローカル照準行列
     */
    static Matrix4x4 AimPivot(const Vector3& pivot, float yaw, float pitch) {
        return Matrix4x4::Translation(pivot) * Matrix4x4::RotationY(yaw) *
            Matrix4x4::RotationX(pitch);
    }

    /**
     * @brief 親行列へ一つのPrimitiveを合成して描画する
     * @param parent 親行列
     * @param drawPart 描画関数
     * @param shape Primitive形状
     * @param position 親基準の位置
     * @param scale Primitive寸法
     * @param color Primitive色
     * @param group Primitiveの部位
     * @param rotation 親基準のXYZ回転
     * @return なし
     */
    template<class DrawPart>
    static void Part(const Matrix4x4& parent, DrawPart& drawPart, PrimitiveShape shape,
        const Vector3& position, const Vector3& scale, const ColorF& color,
        WallSecurityDronePartGroup group, const Vector3& rotation = {}) {
        drawPart(shape, parent * Stage5ModelDetail::Matrix({position, rotation, scale}),
            color, group);
    }
};

static_assert(WallSecurityDroneModelView::PrimitiveCount == 52);
