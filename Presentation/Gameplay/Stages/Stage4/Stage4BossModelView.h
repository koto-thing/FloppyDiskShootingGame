#pragma once

#include <algorithm>
#include <cmath>

#include "../Common/BossModelTransform.h"

#include "../../../../Engine/Graphics/IRenderBackend.h"

/** @brief Stage4ボスの独立破壊対象を描画へ反映する */
struct Stage4BossModelState {
    bool mainTurret = true;
    bool mainCannon = true;
    bool secondaryGuns[6] = {true, true, true, true, true, true};
    bool commandTower = true;
    bool exhaustStacks[2] = {true, true};
    bool frontRam = true;
    bool mainCannonHit = false;
    bool secondaryGunsHit[6] = {};
};

/** @brief Stage4ボスへ装着または単体運搬する主砲の種類 */
enum class Stage4MainWeaponType {
    Phase1Cannon,
    SiegeMortar,
    RomanceCannon
};

/** @brief 主砲単体の取付補正と可動部位の姿勢 */
struct Stage4MainWeaponPose {
    Vector3 localPositionOffset {};
    float localYaw = 0.0f;
    float barrelPitch = 0.0f;
    float recoil = 0.0f;
};

/** @brief 黒塗り高級車と超重戦車を融合したStage4ボスのプロシージャル描画 */
class Stage4BossModelView {
public:
    static constexpr int Stage4LaneCount = 4;
    static constexpr float Stage4LaneWidth = 6.0f;
    static constexpr float ModelWidth = Stage4LaneCount * Stage4LaneWidth;
    static constexpr int ChassisPrimitiveCount = 10;
    static constexpr int TrackPrimitiveCount = 106;
    static constexpr int LuxuryBodyPrimitiveCount = 25;
    static constexpr int MainTurretPrimitiveCount = 9;
    static constexpr int MainCannonPrimitiveCount = 6;
    static constexpr int SiegeMortarPrimitiveCount = 24;
    static constexpr int RomanceCannonPrimitiveCount = 35;
    static constexpr int CommandTowerPrimitiveCount = 9;
    static constexpr int SecondaryGunPrimitiveCount = 18;
    static constexpr int ExhaustPrimitiveCount = 8;
    static constexpr int FrontPrimitiveCount = 19;
    static constexpr int DecorationPrimitiveCount = 10;
    static constexpr int PrimitiveCount = ChassisPrimitiveCount + TrackPrimitiveCount +
        LuxuryBodyPrimitiveCount + MainTurretPrimitiveCount + MainCannonPrimitiveCount +
        CommandTowerPrimitiveCount + SecondaryGunPrimitiveCount + ExhaustPrimitiveCount +
        FrontPrimitiveCount + DecorationPrimitiveCount;

    /** @brief Siege Mortarの標準仰角 */
    static constexpr float SiegeMortarDefaultPitch = Math::Pi * 52.5f / 180.0f;
    /** @brief Siege Mortarの最小仰角 */
    static constexpr float SiegeMortarMinPitch = 0.0f;
    /** @brief Siege Mortarの2D時の最大仰角 */
    static constexpr float SiegeMortar2DMaxPitch = Math::Pi * 85.0f / 180.0f;
    /** @brief Siege Mortarの3D時の最大仰角 */
    static constexpr float SiegeMortar3DMaxPitch = Math::Pi * 10.0f / 180.0f;
    /** @brief Phase1主砲の最小仰角 */
    static constexpr float Phase1CannonMinElevation = -Math::Pi * 15.0f / 180.0f;
    /** @brief Phase1主砲の最大仰角 */
    static constexpr float Phase1CannonMaxElevation = Math::Pi * 70.0f / 180.0f;

    /**
     * @brief 三種類の主砲が共有する車体ローカル取付位置を取得する
     * @return 車体ローカル取付位置
     */
    static constexpr Vector3 MainWeaponMountLocalPosition() {
        return {-3.25f, 3.55f, 0.0f};
    }

    /**
     * @brief 車体Transformから共通主砲マウントの独立Transformを作る
     * @param tankTransform 車体Transform
     * @return ワールド座標へ変換済みの主砲Transform
     */
    static BossModelTransform MainWeaponMount(const BossModelTransform& tankTransform) {
        BossModelTransform weaponTransform = tankTransform;
        weaponTransform.position = LocalToWorldPosition(tankTransform, MainWeaponMountLocalPosition());
        weaponTransform.trackRoll = 0.0f;
        return weaponTransform;
    }

    /**
     * @brief 主砲種別に対応する標準姿勢を取得する
     * @param weaponType 主砲種別
     * @return 標準姿勢
     */
    static Stage4MainWeaponPose DefaultMainWeaponPose(Stage4MainWeaponType weaponType) {
        Stage4MainWeaponPose pose;
        if (weaponType == Stage4MainWeaponType::SiegeMortar) {
            pose.barrelPitch = SiegeMortarDefaultPitch;
        }
        return pose;
    }

    /**
     * @brief 主砲ローカル点を姿勢適用後のワールド座標へ変換する
     * @param weaponTransform 主砲単体Transform
     * @param pose 主砲姿勢
     * @param localPosition 主砲ローカル座標
     * @return ワールド座標
     */
    static Vector3 MainWeaponPointWorldPosition(const BossModelTransform& weaponTransform,
        const Stage4MainWeaponPose& pose, const Vector3& localPosition) {
        return LocalToWorldPosition(ApplyWeaponPose(weaponTransform, pose), localPosition);
    }

    /**
     * @brief 指定主砲の砲口ローカル位置を取得する
     * @param weaponType 主砲種別
     * @param pose 主砲姿勢
     * @return 主砲Transform基準の砲口位置
     */
    static Vector3 MainWeaponMuzzleLocalPosition(Stage4MainWeaponType weaponType,
        const Stage4MainWeaponPose& pose) {
        const float recoil = Math::Clamp01(pose.recoil);
        switch (weaponType) {
        case Stage4MainWeaponType::SiegeMortar: {
            const float pitch = (std::clamp)(pose.barrelPitch,
                SiegeMortarMinPitch, SiegeMortar2DMaxPitch);
            return Vector3 {-0.25f, 1.28f, 0.0f} +
                AxisLocalPosition(4.10f - recoil * 0.55f, pitch);
        }
        case Stage4MainWeaponType::RomanceCannon: {
            const float pitch = (std::clamp)(pose.barrelPitch, -0.18f, 0.42f);
            return Vector3 {-0.35f, 1.48f, 0.0f} +
                AxisLocalPosition(10.80f - recoil * 2.40f, pitch);
        }
        default:
            return {-4.47f + recoil * 0.80f, 0.0f, 0.0f};
        }
    }

    /**
     * @brief Siege Mortarの砲口ローカル位置を取得する
     * @param barrelPitch 砲身仰角
     * @param recoil 0から1の後座量
     * @return 主砲Transform基準の砲口位置
     */
    static Vector3 SiegeMortarMuzzleLocalPosition(float barrelPitch, float recoil = 0.0f) {
        Stage4MainWeaponPose pose;
        pose.barrelPitch = barrelPitch;
        pose.recoil = recoil;
        return MainWeaponMuzzleLocalPosition(Stage4MainWeaponType::SiegeMortar, pose);
    }

    /**
     * @brief Romance Cannonの砲口ローカル位置を取得する
     * @param barrelPitch 砲身仰角
     * @param recoil 0から1の後座量
     * @return 主砲Transform基準の砲口位置
     */
    static Vector3 RomanceCannonMuzzleLocalPosition(float barrelPitch, float recoil = 0.0f) {
        Stage4MainWeaponPose pose;
        pose.barrelPitch = barrelPitch;
        pose.recoil = recoil;
        return MainWeaponMuzzleLocalPosition(Stage4MainWeaponType::RomanceCannon, pose);
    }

    /**
     * @brief 主砲を運搬するドローン用把持点数を取得する
     * @param weaponType 主砲種別
     * @return 把持点数
     */
    static constexpr int CarryPointCount(Stage4MainWeaponType weaponType) {
        return weaponType == Stage4MainWeaponType::Phase1Cannon ? 2 :
            weaponType == Stage4MainWeaponType::SiegeMortar ? 3 : 4;
    }

    /**
     * @brief 主砲を運搬するドローン用把持点を取得する
     * @param weaponType 主砲種別
     * @param index 0からCarryPointCount未満の把持点番号
     * @return 主砲Transform基準の把持点
     */
    static Vector3 CarryPointLocalPosition(Stage4MainWeaponType weaponType, int index) {
        constexpr Vector3 Phase1Points[] = {
            {-0.85f, 0.72f, 0.0f}, {-3.05f, 0.60f, 0.0f}
        };
        constexpr Vector3 MortarPoints[] = {
            {0.45f, 1.65f, -1.30f}, {0.45f, 1.65f, 1.30f}, {-1.50f, 4.03f, 0.0f}
        };
        constexpr Vector3 RomancePoints[] = {
            {-2.20f, 2.56f, -1.55f}, {-2.20f, 2.56f, 1.55f},
            {-3.45f, 2.56f, -1.55f}, {-3.45f, 2.56f, 1.55f}
        };
        if (index < 0 || index >= CarryPointCount(weaponType)) return {};
        if (weaponType == Stage4MainWeaponType::Phase1Cannon) return Phase1Points[index];
        if (weaponType == Stage4MainWeaponType::SiegeMortar) return MortarPoints[index];
        return RomancePoints[index];
    }

    /**
     * @brief パージ用補助エンジンの噴射口位置を取得する
     * @param weaponType 主砲種別
     * @return 主砲Transform基準の噴射口位置
     */
    static constexpr Vector3 PurgeThrusterLocalPosition(Stage4MainWeaponType weaponType) {
        return weaponType == Stage4MainWeaponType::Phase1Cannon ?
            Vector3 {0.0f, -1.18f, 0.0f} :
            weaponType == Stage4MainWeaponType::SiegeMortar ?
                Vector3 {0.35f, -0.48f, 0.0f} : Vector3 {0.35f, -0.56f, 0.0f};
    }

    /**
     * @brief Stage4ボスをローカル部品の組み合わせで描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param state 独立破壊対象の描画状態
     * @return なし
     */
    template<class DrawPart>
    static void Draw(const BossModelTransform& transform, DrawPart&& drawPart,
        const Stage4BossModelState& state = {}) {
        // 常設車体を下層から順に描画する
        DrawChassis(transform, drawPart);
        DrawTracks(transform, drawPart);
        DrawLuxuryBody(transform, drawPart);

        // 将来の部位破壊単位ごとに上部構造を描画する
        if (state.mainTurret) DrawMainTurret(transform, drawPart);
        if (state.mainCannon) {
            DrawMainWeapon(Stage4MainWeaponType::Phase1Cannon, MainWeaponMount(transform),
                DefaultMainWeaponPose(Stage4MainWeaponType::Phase1Cannon), drawPart,
                state.mainCannonHit);
        }
        if (state.commandTower) DrawCommandTower(transform, drawPart);
        DrawSecondaryGuns(transform, drawPart, state);
        DrawExhaustStacks(transform, drawPart, state);
        DrawFront(transform, drawPart, state.frontRam);
        DrawDecorations(transform, drawPart);
    }

    /**
     * @brief 交換式主砲を除くStage4ボス車体を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param state 独立破壊対象の描画状態
     * @return なし
     */
    template<class DrawPart>
    static void DrawBody(const BossModelTransform& transform, DrawPart&& drawPart,
        const Stage4BossModelState& state = {}) {
        DrawChassis(transform, drawPart);
        DrawTracks(transform, drawPart);
        DrawLuxuryBody(transform, drawPart);
        if (state.mainTurret) DrawMainTurret(transform, drawPart);
        if (state.commandTower) DrawCommandTower(transform, drawPart);
        DrawSecondaryGuns(transform, drawPart, state);
        DrawExhaustStacks(transform, drawPart, state);
        DrawFront(transform, drawPart, state.frontRam);
        DrawDecorations(transform, drawPart);
    }

    /**
     * @brief 任意Transformから交換式主砲を単体描画する
     * @param weaponType 主砲種別
     * @param weaponTransform 主砲単体Transform
     * @param pose 取付補正、砲身仰角、後座量
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawMainWeapon(Stage4MainWeaponType weaponType,
        const BossModelTransform& weaponTransform, const Stage4MainWeaponPose& pose,
        DrawPart&& drawPart, bool hit = false) {
        const BossModelTransform posedTransform = ApplyWeaponPose(weaponTransform, pose);
        switch (weaponType) {
        case Stage4MainWeaponType::SiegeMortar:
            DrawSiegeMortar(posedTransform, pose, drawPart, hit);
            break;
        case Stage4MainWeaponType::RomanceCannon:
            DrawRomanceCannon(posedTransform, pose, drawPart, hit);
            break;
        default:
            DrawPhase1Cannon(posedTransform, pose, drawPart, hit);
            break;
        }
    }

    /**
     * @brief 砲身を失い砲尾だけ残った半壊Romance Cannonを描画する
     * @param weaponTransform 主砲単体Transform
     * @param pose 取付補正と砲身姿勢
     * @param drawPart 形状、ワールド座標、寸法、色、向きを受け取る描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawDamagedRomanceCannon(const BossModelTransform& weaponTransform,
        const Stage4MainWeaponPose& pose, DrawPart&& drawPart, bool hit = false) {
        const BossModelTransform posedTransform = ApplyWeaponPose(weaponTransform, pose);
        DrawRomanceCannonBase(posedTransform, drawPart, hit);
        DrawRomanceCannonRearBreech(posedTransform, drawPart, hit);
    }

private:
    inline static constexpr float MainBlack[] = {0.025f, 0.025f, 0.030f, 1.0f};
    inline static constexpr float ArmorBlack[] = {0.060f, 0.065f, 0.075f, 1.0f};
    inline static constexpr float HighlightBlack[] = {0.10f, 0.10f, 0.12f, 1.0f};
    inline static constexpr float Window[] = {0.015f, 0.020f, 0.035f, 1.0f};
    inline static constexpr float Gold[] = {0.55f, 0.38f, 0.08f, 1.0f};
    inline static constexpr float Light[] = {0.90f, 0.80f, 0.50f, 1.0f};
    inline static constexpr float Track[] = {0.035f, 0.038f, 0.045f, 1.0f};
    inline static constexpr float Wheel[] = {0.13f, 0.14f, 0.16f, 1.0f};
    inline static constexpr float Hit[] = {1.0f, 0.03f, 0.03f, 1.0f};
    inline static constexpr float Ember[] = {0.85f, 0.12f, 0.025f, 1.0f};

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
     * @brief 主砲単体Transformへ取付補正を適用する
     * @param transform 主砲単体Transform
     * @param pose 取付補正
     * @return 補正済みTransform
     */
    static BossModelTransform ApplyWeaponPose(const BossModelTransform& transform,
        const Stage4MainWeaponPose& pose) {
        BossModelTransform result = transform;
        result.position = LocalToWorldPosition(transform, pose.localPositionOffset);
        result.yaw += pose.localYaw;
        return result;
    }

    /**
     * @brief 仰角を持つ砲軸上のローカル座標を取得する
     * @param distance 支点から砲口方向への距離
     * @param pitch 砲身仰角
     * @return 砲軸上のローカル座標
     */
    static Vector3 AxisLocalPosition(float distance, float pitch) {
        return {-std::cos(pitch) * distance, std::sin(pitch) * distance, 0.0f};
    }

    /**
     * @brief 履帯板の巡回位置を取得する
     * @param value 巡回前の位置
     * @param length 巡回する長さ
     * @return 0以上length未満へ丸めた位置
     */
    static float WrapTrackPosition(float value, float length) {
        const float wrapped = std::fmod(value, length);
        return wrapped < 0.0f ? wrapped + length : wrapped;
    }

    /**
     * @brief ローカル部品をStage2と共通の親Transform合成処理へ渡す
     * @param transform ボス全体の親Transform
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
    static void Part(const BossModelTransform& transform, DrawPart& drawPart, PrimitiveShape shape,
        const Vector3& localPosition, const Vector3& scale, const float color[4],
        float localYaw = 0.0f, float pitch = 0.0f) {
        const Vector3 position = LocalToWorldPosition(transform, localPosition);
        drawPart(static_cast<int>(shape), position, scale * transform.scale,
            color, transform.yaw + localYaw, pitch);
    }

    /**
     * @brief 主砲軸へ沿って配置した部品を描画する
     * @param transform 主砲単体Transform
     * @param drawPart 描画関数
     * @param localPivot 砲軸支点のローカル座標
     * @param shape 描画形状
     * @param distance 支点から砲口方向への距離
     * @param scale ローカル寸法
     * @param color 部品色
     * @param barrelPitch 砲身仰角
     * @return なし
     */
    template<class DrawPart>
    static void WeaponAxisPart(const BossModelTransform& transform, DrawPart& drawPart,
        const Vector3& localPivot, PrimitiveShape shape, float distance, const Vector3& scale,
        const float color[4], float barrelPitch) {
        const Vector3 pivot = LocalToWorldPosition(transform, localPivot);
        const float yawCosine = std::cos(transform.yaw);
        const float yawSine = std::sin(transform.yaw);
        const float pitchCosine = std::cos(barrelPitch);
        const Vector3 direction {
            -yawCosine * pitchCosine,
            std::sin(barrelPitch),
            yawSine * pitchCosine
        };
        const Vector3 position = pivot + direction * (distance * transform.scale);
        drawPart(static_cast<int>(shape), position, scale * transform.scale,
            color, transform.yaw, -barrelPitch);
    }

    /**
     * @brief 砲基部を支点に照準先へ向けた部品を描画する
     * @param transform ボス全体の親Transformと照準先
     * @param drawPart 描画関数
     * @param tracksTarget 照準先へ向ける場合true
     * @param aimTarget 照準先
     * @param localPivot 砲基部のローカル座標
     * @param shape 描画形状
     * @param distance 基部から砲口方向への距離
     * @param scale ローカル寸法
     * @param color 部品色
     * @param tracksYaw 照準先へYawを向ける場合true
     * @param clampElevation 仰角を制限する場合true
     * @param minElevation 最小仰角
     * @param maxElevation 最大仰角
     * @return なし
     */
    template<class DrawPart>
    static void GunPart(const BossModelTransform& transform, DrawPart& drawPart,
        bool tracksTarget, const Vector3& aimTarget, const Vector3& localPivot,
        PrimitiveShape shape, float distance, const Vector3& scale, const float color[4],
        bool tracksYaw = true, bool clampElevation = false,
        float minElevation = -Math::HalfPi, float maxElevation = Math::HalfPi) {
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        const Vector3 pivot {
            transform.position.x +
                (localPivot.x * cosine + localPivot.z * sine) * transform.scale,
            transform.position.y + localPivot.y * transform.scale,
            transform.position.z +
                (-localPivot.x * sine + localPivot.z * cosine) * transform.scale
        };
        const Vector3 delta = tracksTarget ? aimTarget - pivot :
            Vector3 {-cosine, 0.0f, sine};
        const float forwardX = -cosine;
        const float forwardZ = sine;
        const float horizontal = (std::max)(0.001f, tracksYaw ?
            std::sqrt(delta.x * delta.x + delta.z * delta.z) :
            std::abs(delta.x * forwardX + delta.z * forwardZ));
        const float length = (std::max)(0.001f,
            std::sqrt(horizontal * horizontal + delta.y * delta.y));
        const float gunYaw = tracksYaw ? std::atan2(delta.z, -delta.x) : transform.yaw;
        const float rawElevation = tracksTarget ? std::asin(delta.y / length) : 0.0f;
        const float gunElevation = clampElevation ?
            (std::clamp)(rawElevation, minElevation, maxElevation) : rawElevation;
        const float pitchCosine = std::cos(gunElevation);
        const Vector3 direction {
            -std::cos(gunYaw) * pitchCosine,
            std::sin(gunElevation),
            std::sin(gunYaw) * pitchCosine
        };
        const Vector3 position = pivot + direction * (distance * transform.scale);
        drawPart(static_cast<int>(shape), position, scale * transform.scale,
            color, gunYaw, -gunElevation);
    }

    /**
     * @brief 低重心の積層装甲車体を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawChassis(const BossModelTransform& transform, DrawPart& drawPart) {
        // 中央、前部傾斜、後部機関室で一枚箱に見えない基礎車体を作る
        Part(transform, drawPart, PrimitiveShape::Box, {0.0f, 0.35f, 0.0f}, {14.4f, 1.35f, ModelWidth}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-6.55f, 0.28f, 0.0f}, {2.0f, 1.10f, ModelWidth - 0.6f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {5.75f, 0.65f, 0.0f}, {2.45f, 1.65f, ModelWidth - 0.4f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {0.15f, -0.58f, 0.0f}, {12.9f, 0.50f, ModelWidth - 2.0f}, Track);

        // 左右の張り出し装甲で幅と重量感を補う
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {-0.15f, 0.48f, side * 11.82f}, {11.8f, 0.72f, 0.36f}, ArmorBlack);
            Part(transform, drawPart, PrimitiveShape::Prism, {-6.35f, 0.36f, side * 11.70f}, {1.55f, 0.86f, 0.54f}, HighlightBlack);
            Part(transform, drawPart, PrimitiveShape::Prism, {5.65f, 0.64f, side * 11.70f}, {1.55f, 1.02f, 0.54f}, HighlightBlack, Math::Pi);
        }
    }

    /**
     * @brief 丸い端部と転輪を持つ左右の大型履帯を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawTracks(const BossModelTransform& transform, DrawPart& drawPart) {
        constexpr float TrackLength = 13.2f;
        constexpr float TrackLeft = -TrackLength * 0.5f;
        constexpr float ShoeSpacing = 2.20f;
        constexpr int ShoeCount = 7;
        const float shoePhase = WrapTrackPosition(transform.trackRoll * 0.54f, ShoeSpacing);

        for (float side : {-1.0f, 1.0f}) {
            // 長い履帯板と円柱端部で角張りすぎない外周を作る
            Part(transform, drawPart, PrimitiveShape::Box, {0.0f, -0.08f, side * 11.10f}, {13.4f, 2.25f, 0.54f}, Track);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {-6.70f, -0.08f, side * 11.10f}, {2.28f, 0.58f, 2.28f}, Track, Math::HalfPi, Math::HalfPi);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {6.70f, -0.08f, side * 11.10f}, {2.28f, 0.58f, 2.28f}, Track, Math::HalfPi, Math::HalfPi);
            Part(transform, drawPart, PrimitiveShape::Box, {0.0f, 0.91f, side * 11.16f}, {11.8f, 0.48f, 0.66f}, MainBlack);
            Part(transform, drawPart, PrimitiveShape::Box, {0.0f, -1.08f, side * 11.16f}, {11.8f, 0.30f, 0.66f}, MainBlack);

            // 履帯板を移動距離と連動した位相で上下逆方向へ流す
            for (int shoe = 0; shoe < ShoeCount; ++shoe) {
                const float offset = static_cast<float>(shoe) * ShoeSpacing + shoePhase;
                const float topX = TrackLeft + WrapTrackPosition(offset, TrackLength);
                const float bottomX = TrackLeft + WrapTrackPosition(TrackLength - offset, TrackLength);
                Part(transform, drawPart, PrimitiveShape::Box, {topX, 1.04f, side * 11.62f},
                    {0.72f, 0.12f, 0.78f}, HighlightBlack);
                Part(transform, drawPart, PrimitiveShape::Box, {bottomX, -1.18f, side * 11.62f},
                    {0.72f, 0.12f, 0.78f}, HighlightBlack);
            }

            // 規則的な中心位置と大小差を持つ八輪を外装の間から見せる
            for (int wheel = 0; wheel < 8; ++wheel) {
                const float x = -5.25f + static_cast<float>(wheel) * 1.50f;
                const float edge = std::fabs(static_cast<float>(wheel) - 3.5f) / 3.5f;
                const float diameter = 1.48f - edge * 0.22f;
                Part(transform, drawPart, PrimitiveShape::Cylinder, {x, -0.18f, side * 11.40f},
                    {diameter, 0.22f, diameter}, Wheel, Math::HalfPi, Math::HalfPi);
                Part(transform, drawPart, PrimitiveShape::Cylinder, {x, -0.18f, side * 11.53f},
                    {diameter * 0.42f, 0.12f, diameter * 0.42f}, Gold, Math::HalfPi, Math::HalfPi);
                Part(transform, drawPart, PrimitiveShape::Box, {x, -0.18f, side * 11.64f},
                    {diameter * 0.70f, 0.06f, 0.08f}, Gold, 0.0f, -transform.trackRoll);
                Part(transform, drawPart, PrimitiveShape::Box, {x, -0.18f, side * 11.65f},
                    {diameter * 0.70f, 0.06f, 0.08f}, Gold, 0.0f,
                    -transform.trackRoll + Math::HalfPi);
            }

            // 前後の斜め装甲で履帯上部を車体へつなぐ
            Part(transform, drawPart, PrimitiveShape::Prism, {-6.20f, 0.82f, side * 11.16f}, {1.30f, 0.62f, 0.70f}, ArmorBlack);
            Part(transform, drawPart, PrimitiveShape::Prism, {6.20f, 0.82f, side * 11.16f}, {1.30f, 0.62f, 0.70f}, ArmorBlack, Math::Pi);
        }
    }

    /**
     * @brief 戦車上へ融合した低く長いリムジン車体を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawLuxuryBody(const BossModelTransform& transform, DrawPart& drawPart) {
        // 長いボディを滑らかな段差で積み上げる
        Part(transform, drawPart, PrimitiveShape::Box, {-0.35f, 1.36f, 0.0f}, {11.8f, 1.08f, 19.8f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-5.75f, 1.65f, 0.0f}, {2.40f, 0.82f, 19.2f}, HighlightBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {-0.55f, 2.02f, 0.0f}, {6.75f, 0.88f, 17.8f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {3.85f, 1.82f, 0.0f}, {2.05f, 0.78f, 18.8f}, ArmorBlack, Math::Pi);
        Part(transform, drawPart, PrimitiveShape::Prism, {-0.35f, 2.62f, 0.0f}, {6.15f, 0.40f, 16.4f}, HighlightBlack);

        // 両側の窓列と金色モールで高級車の読みやすさを作る
        for (float side : {-1.0f, 1.0f}) {
            for (int window = 0; window < 6; ++window) {
                const float x = -3.65f + static_cast<float>(window) * 1.30f;
                Part(transform, drawPart, PrimitiveShape::Box, {x, 2.18f, side * 9.02f},
                    {0.96f, 0.48f, 0.10f}, Window);
            }
            Part(transform, drawPart, PrimitiveShape::Box, {-0.15f, 1.58f, side * 9.96f}, {8.75f, 0.10f, 0.08f}, Gold);
            for (int handle = 0; handle < 3; ++handle) {
                Part(transform, drawPart, PrimitiveShape::Box,
                    {-2.90f + static_cast<float>(handle) * 2.45f, 1.83f, side * 9.97f},
                    {0.42f, 0.10f, 0.08f}, Gold);
            }
        }
    }

    /**
     * @brief 厚い前面と斜め側面を持つ主砲塔を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawMainTurret(const BossModelTransform& transform, DrawPart& drawPart) {
        Part(transform, drawPart, PrimitiveShape::Cylinder, {-1.45f, 2.92f, 0.0f}, {3.35f, 0.55f, 11.4f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {-1.30f, 3.40f, 0.0f}, {3.65f, 1.05f, 10.4f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {-3.10f, 3.38f, 0.0f}, {1.15f, 1.18f, 10.0f}, HighlightBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {0.52f, 3.34f, 0.0f}, {1.05f, 0.92f, 9.6f}, MainBlack, Math::Pi);
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Prism, {-1.28f, 3.48f, side * 5.15f}, {2.80f, 0.82f, 0.38f}, HighlightBlack, side * 0.08f);
            Part(transform, drawPart, PrimitiveShape::Box, {-1.55f, 3.92f, side * 4.82f}, {1.65f, 0.12f, 0.10f}, Gold);
        }
        Part(transform, drawPart, PrimitiveShape::Box, {-0.95f, 4.02f, 0.0f}, {2.30f, 0.26f, 7.8f}, MainBlack);
    }

    /**
     * @brief 既存外観を維持したPhase1主砲を単体描画する
     * @param transform 主砲単体Transform
     * @param pose 主砲姿勢
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawPhase1Cannon(const BossModelTransform& transform,
        const Stage4MainWeaponPose& pose, DrawPart& drawPart, bool hit) {
        const float* armor = hit ? Hit : ArmorBlack;
        const float* highlight = hit ? Hit : HighlightBlack;
        const float* main = hit ? Hit : MainBlack;
        const float* window = hit ? Hit : Window;
        const float* gold = hit ? Hit : Gold;
        BossModelTransform aimedTransform = transform;

        // 後座量だけ支点を砲尾方向へ移し既存六部品の寸法と並びを保つ
        const float recoil = Math::Clamp01(pose.recoil) * 0.80f;
        const float cosine = std::cos(transform.yaw);
        const float sine = std::sin(transform.yaw);
        aimedTransform.position += Vector3 {cosine * recoil, 0.0f, -sine * recoil} * transform.scale;
        GunPart(aimedTransform, drawPart, aimedTransform.mainGunTracksTarget,
            aimedTransform.aimTarget, {},
            PrimitiveShape::Cylinder, 0.00f, {1.18f, 1.18f, 1.18f}, armor,
            aimedTransform.mainGunTracksYaw, true,
            aimedTransform.mainGunMinElevation, aimedTransform.mainGunMaxElevation);
        GunPart(aimedTransform, drawPart, aimedTransform.mainGunTracksTarget,
            aimedTransform.aimTarget, {},
            PrimitiveShape::Cylinder, 1.30f, {2.35f, 0.92f, 0.92f}, highlight,
            aimedTransform.mainGunTracksYaw, true,
            aimedTransform.mainGunMinElevation, aimedTransform.mainGunMaxElevation);
        GunPart(aimedTransform, drawPart, aimedTransform.mainGunTracksTarget,
            aimedTransform.aimTarget, {},
            PrimitiveShape::Cylinder, 3.25f, {1.65f, 0.76f, 0.76f}, armor,
            aimedTransform.mainGunTracksYaw, true,
            aimedTransform.mainGunMinElevation, aimedTransform.mainGunMaxElevation);
        GunPart(aimedTransform, drawPart, aimedTransform.mainGunTracksTarget,
            aimedTransform.aimTarget, {},
            PrimitiveShape::Cylinder, 4.23f, {0.42f, 1.16f, 1.16f}, main,
            aimedTransform.mainGunTracksYaw, true,
            aimedTransform.mainGunMinElevation, aimedTransform.mainGunMaxElevation);
        GunPart(aimedTransform, drawPart, aimedTransform.mainGunTracksTarget,
            aimedTransform.aimTarget, {},
            PrimitiveShape::Cylinder, 4.47f, {0.18f, 0.68f, 0.68f}, window,
            aimedTransform.mainGunTracksYaw, true,
            aimedTransform.mainGunMinElevation, aimedTransform.mainGunMaxElevation);
        GunPart(aimedTransform, drawPart, aimedTransform.mainGunTracksTarget,
            aimedTransform.aimTarget, {},
            PrimitiveShape::Cylinder, 4.00f, {0.12f, 1.22f, 1.22f}, gold,
            aimedTransform.mainGunTracksYaw, true,
            aimedTransform.mainGunMinElevation, aimedTransform.mainGunMaxElevation);
    }

    /**
     * @brief Siege Mortarの固定基部を描画する
     * @param transform 主砲単体Transform
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawSiegeMortarBase(const BossModelTransform& transform,
        DrawPart& drawPart, bool hit) {
        const float* main = hit ? Hit : MainBlack;
        const float* armor = hit ? Hit : ArmorBlack;
        const float* highlight = hit ? Hit : HighlightBlack;
        const float* gold = hit ? Hit : Gold;

        // 幅広い旋回座と左右の支持架で臼砲の重量を受ける
        Part(transform, drawPart, PrimitiveShape::Cylinder, {0.35f, 0.05f, 0.0f},
            {3.10f, 0.48f, 5.20f}, main);
        Part(transform, drawPart, PrimitiveShape::Box, {0.10f, 0.48f, 0.0f},
            {3.65f, 0.72f, 4.55f}, armor);
        Part(transform, drawPart, PrimitiveShape::Prism, {-1.25f, 0.82f, 0.0f},
            {1.40f, 1.10f, 4.20f}, highlight);
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Cylinder, {-0.25f, 1.28f, side * 1.72f},
                {1.15f, 0.48f, 1.15f}, armor, Math::HalfPi, Math::HalfPi);
            Part(transform, drawPart, PrimitiveShape::Box, {0.55f, 1.18f, side * 1.42f},
                {1.40f, 1.65f, 0.34f}, highlight);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {0.45f, 1.65f, side * 1.30f},
                {0.34f, 0.16f, 0.34f}, gold);
        }
    }

    /**
     * @brief Siege Mortarの仰角可動砲身を描画する
     * @param transform 主砲単体Transform
     * @param pose 砲身仰角と後座量
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawSiegeMortarBarrel(const BossModelTransform& transform,
        const Stage4MainWeaponPose& pose, DrawPart& drawPart, bool hit) {
        const float* main = hit ? Hit : MainBlack;
        const float* armor = hit ? Hit : ArmorBlack;
        const float* highlight = hit ? Hit : HighlightBlack;
        const float* window = hit ? Hit : Window;
        const float* gold = hit ? Hit : Gold;
        const float pitch = (std::clamp)(pose.barrelPitch,
            SiegeMortarMinPitch, SiegeMortar2DMaxPitch);
        const float recoil = Math::Clamp01(pose.recoil) * 0.55f;
        auto BarrelPart = [&](PrimitiveShape shape, float distance, const Vector3& scale,
            const float color[4]) {
            WeaponAxisPart(transform, drawPart, {-0.25f, 1.28f, 0.0f}, shape,
                distance - recoil, scale, color, pitch);
        };

        // 短い極太外筒へ大きな段差と暗い砲口を重ねる
        BarrelPart(PrimitiveShape::Sphere, -0.25f, {2.65f, 2.65f, 2.65f}, armor);
        BarrelPart(PrimitiveShape::Cylinder, 0.35f, {1.55f, 2.45f, 2.45f}, main);
        BarrelPart(PrimitiveShape::Cylinder, 1.42f, {2.05f, 2.12f, 2.12f}, armor);
        BarrelPart(PrimitiveShape::Cylinder, 2.72f, {1.42f, 1.88f, 1.88f}, highlight);
        BarrelPart(PrimitiveShape::Cylinder, 0.05f, {0.22f, 2.72f, 2.72f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 1.20f, {0.20f, 2.40f, 2.40f}, armor);
        BarrelPart(PrimitiveShape::Cylinder, 2.35f, {0.20f, 2.18f, 2.18f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 3.58f, {0.72f, 2.68f, 2.68f}, main);
        BarrelPart(PrimitiveShape::Cylinder, 3.92f, {0.18f, 2.85f, 2.85f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 4.10f, {0.12f, 2.18f, 2.18f}, window);

        // 油圧補強と中央把持金具を砲身へ追従させる
        for (float side : {-1.0f, 1.0f}) {
            const Vector3 brace = Vector3 {-0.25f, 1.28f, 0.0f} +
                AxisLocalPosition(0.55f - recoil, pitch) +
                Vector3 {0.0f, -0.62f, side * 1.25f};
            Part(transform, drawPart, PrimitiveShape::Box, brace,
                {1.55f, 0.24f, 0.20f}, highlight, 0.0f, -pitch);
        }
        const Vector3 carryPoint = Vector3 {-0.25f, 1.28f, 0.0f} +
            AxisLocalPosition(2.05f - recoil, pitch) +
            Vector3 {0.0f, 1.12f, 0.0f};
        Part(transform, drawPart, PrimitiveShape::Cylinder, carryPoint,
            {0.34f, 0.16f, 0.34f}, gold);
        Part(transform, drawPart, PrimitiveShape::Box,
            Vector3 {-0.25f, 1.28f, 0.0f} + AxisLocalPosition(-0.55f - recoil, pitch) +
                Vector3 {0.0f, -0.78f, 0.0f},
            {1.10f, 0.28f, 1.18f}, armor, 0.0f, -pitch);
        Part(transform, drawPart, PrimitiveShape::Box,
            Vector3 {-0.25f, 1.28f, 0.0f} + AxisLocalPosition(0.35f - recoil, pitch) +
                Vector3 {0.0f, 0.72f, 0.0f},
            {0.82f, 0.18f, 1.45f}, gold, 0.0f, -pitch);
    }

    /**
     * @brief Siege Mortarを固定基部と可動砲身に分けて描画する
     * @param transform 主砲単体Transform
     * @param pose 主砲姿勢
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawSiegeMortar(const BossModelTransform& transform,
        const Stage4MainWeaponPose& pose, DrawPart& drawPart, bool hit) {
        DrawSiegeMortarBase(transform, drawPart, hit);
        DrawSiegeMortarBarrel(transform, pose, drawPart, hit);
    }

    /**
     * @brief Romance Cannonの固定基部を描画する
     * @param transform 主砲単体Transform
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawRomanceCannonBase(const BossModelTransform& transform,
        DrawPart& drawPart, bool hit) {
        const float* main = hit ? Hit : MainBlack;
        const float* armor = hit ? Hit : ArmorBlack;
        const float* highlight = hit ? Hit : HighlightBlack;
        const float* gold = hit ? Hit : Gold;

        // 超長砲身を受ける低い旋回台と左右ロックを作る
        Part(transform, drawPart, PrimitiveShape::Cylinder, {0.35f, 0.08f, 0.0f},
            {4.30f, 0.56f, 6.20f}, main);
        Part(transform, drawPart, PrimitiveShape::Box, {0.25f, 0.60f, 0.0f},
            {4.65f, 0.92f, 5.45f}, armor);
        Part(transform, drawPart, PrimitiveShape::Prism, {-1.65f, 0.92f, 0.0f},
            {1.65f, 1.18f, 5.05f}, highlight);
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Cylinder, {-0.35f, 1.48f, side * 2.18f},
                {1.48f, 0.62f, 1.48f}, armor, Math::HalfPi, Math::HalfPi);
            Part(transform, drawPart, PrimitiveShape::Box, {0.62f, 1.25f, side * 1.90f},
                {1.85f, 1.85f, 0.42f}, main);
            Part(transform, drawPart, PrimitiveShape::Box, {0.15f, 1.82f, side * 2.12f},
                {1.25f, 0.22f, 0.18f}, gold);
        }
    }

    /**
     * @brief Romance Cannon後部の圧力機関と砲尾を描画する
     * @param transform 主砲単体Transform
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawRomanceCannonRearBreech(const BossModelTransform& transform,
        DrawPart& drawPart, bool hit) {
        const float* main = hit ? Hit : MainBlack;
        const float* armor = hit ? Hit : ArmorBlack;
        const float* highlight = hit ? Hit : HighlightBlack;
        const float* gold = hit ? Hit : Gold;

        // 砲尾後方へ箱形圧力室と左右蓄圧器を大きく張り出す
        Part(transform, drawPart, PrimitiveShape::Box, {2.10f, 1.62f, 0.0f},
            {3.20f, 2.65f, 4.65f}, main);
        Part(transform, drawPart, PrimitiveShape::Sphere, {3.35f, 1.62f, 0.0f},
            {2.35f, 2.35f, 3.45f}, armor);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {2.20f, 1.62f, 0.0f},
            {2.70f, 2.05f, 2.05f}, highlight);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {0.95f, 1.62f, 0.0f},
            {0.20f, 2.55f, 2.55f}, gold);
        Part(transform, drawPart, PrimitiveShape::Box, {4.38f, 1.62f, 0.0f},
            {0.42f, 1.72f, 2.75f}, gold);
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Cylinder, {2.20f, 2.85f, side * 1.72f},
                {1.85f, 0.65f, 0.65f}, armor);
            Part(transform, drawPart, PrimitiveShape::Box, {2.25f, 1.55f, side * 2.38f},
                {1.80f, 1.35f, 0.18f}, gold);
        }
    }

    /**
     * @brief Romance Cannonの後座可能な超長砲身を描画する
     * @param transform 主砲単体Transform
     * @param pose 砲身仰角と後座量
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawRomanceCannonBarrel(const BossModelTransform& transform,
        const Stage4MainWeaponPose& pose, DrawPart& drawPart, bool hit) {
        const float* main = hit ? Hit : MainBlack;
        const float* armor = hit ? Hit : ArmorBlack;
        const float* highlight = hit ? Hit : HighlightBlack;
        const float* window = hit ? Hit : Window;
        const float* gold = hit ? Hit : Gold;
        const float* ember = hit ? Hit : Ember;
        const float pitch = (std::clamp)(pose.barrelPitch, -0.18f, 0.42f);
        const float recoil = Math::Clamp01(pose.recoil) * 2.40f;
        auto BarrelPart = [&](PrimitiveShape shape, float distance, const Vector3& scale,
            const float color[4]) {
            WeaponAxisPart(transform, drawPart, {-0.35f, 1.48f, 0.0f}, shape,
                distance - recoil, scale, color, pitch);
        };

        // 圧力室から砲口まで六段の大きな径差で異常な長さを読ませる
        BarrelPart(PrimitiveShape::Cylinder, -0.35f, {2.20f, 2.45f, 2.45f}, main);
        BarrelPart(PrimitiveShape::Cylinder, 1.25f, {2.15f, 2.18f, 2.18f}, armor);
        BarrelPart(PrimitiveShape::Cylinder, 2.85f, {1.35f, 1.92f, 1.92f}, highlight);
        BarrelPart(PrimitiveShape::Cylinder, 5.10f, {3.20f, 1.68f, 1.68f}, main);
        BarrelPart(PrimitiveShape::Cylinder, 7.95f, {2.55f, 1.48f, 1.48f}, armor);
        BarrelPart(PrimitiveShape::Cylinder, 0.20f, {0.22f, 2.72f, 2.72f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 2.10f, {0.22f, 2.35f, 2.35f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 4.05f, {0.22f, 2.05f, 2.05f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 6.65f, {0.22f, 1.88f, 1.88f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 9.72f, {0.90f, 2.72f, 2.72f}, main);
        BarrelPart(PrimitiveShape::Cylinder, 10.28f, {0.24f, 3.05f, 3.05f}, gold);
        BarrelPart(PrimitiveShape::Cylinder, 10.68f, {0.18f, 2.42f, 2.42f}, window);
        BarrelPart(PrimitiveShape::Cylinder, 10.80f, {0.08f, 1.48f, 1.48f}, ember);

        // 四機の運搬ドローンへ対応する左右一対二組の金色把持金具を付ける
        for (float distance : {1.85f, 3.10f}) {
            for (float side : {-1.0f, 1.0f}) {
                const Vector3 carryPoint = Vector3 {-0.35f, 1.48f, 0.0f} +
                    AxisLocalPosition(distance - recoil, pitch) +
                    Vector3 {0.0f, 1.08f, side * 1.55f};
                Part(transform, drawPart, PrimitiveShape::Cylinder, carryPoint,
                    {0.38f, 0.18f, 0.38f}, gold);
            }
        }
    }

    /**
     * @brief Romance Cannonを固定基部、砲尾機関、可動砲身に分けて描画する
     * @param transform 主砲単体Transform
     * @param pose 主砲姿勢
     * @param drawPart 描画関数
     * @param hit 被弾色で描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawRomanceCannon(const BossModelTransform& transform,
        const Stage4MainWeaponPose& pose, DrawPart& drawPart, bool hit) {
        DrawRomanceCannonBase(transform, drawPart, hit);
        DrawRomanceCannonRearBreech(transform, drawPart, hit);
        DrawRomanceCannonBarrel(transform, pose, drawPart, hit);
    }

    /**
     * @brief モデル最高点となる段積み指揮塔を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawCommandTower(const BossModelTransform& transform, DrawPart& drawPart) {
        Part(transform, drawPart, PrimitiveShape::Prism, {1.50f, 3.28f, 0.0f}, {2.25f, 0.62f, 2.05f}, ArmorBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {1.65f, 3.82f, 0.0f}, {1.72f, 0.72f, 1.62f}, MainBlack);
        Part(transform, drawPart, PrimitiveShape::Prism, {1.72f, 4.35f, 0.0f}, {1.35f, 0.48f, 1.28f}, HighlightBlack);
        Part(transform, drawPart, PrimitiveShape::Box, {1.72f, 4.72f, -0.67f}, {0.82f, 0.25f, 0.10f}, Window);
        Part(transform, drawPart, PrimitiveShape::Box, {1.72f, 4.72f, 0.67f}, {0.82f, 0.25f, 0.10f}, Window);
        Part(transform, drawPart, PrimitiveShape::Box, {1.03f, 4.72f, 0.0f}, {0.10f, 0.25f, 0.72f}, Window);
        Part(transform, drawPart, PrimitiveShape::Box, {2.41f, 4.72f, 0.0f}, {0.10f, 0.25f, 0.72f}, Window);
        Part(transform, drawPart, PrimitiveShape::Cylinder, {1.72f, 5.18f, 0.0f}, {0.16f, 0.72f, 0.16f}, Gold);
        Part(transform, drawPart, PrimitiveShape::Sphere, {1.72f, 5.62f, 0.0f}, {0.40f, 0.40f, 0.40f}, Light);
    }

    /**
     * @brief 主砲横二基を含む左右六基の副砲を独立状態で描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param state 副砲ごとの描画状態
     * @return なし
     */
    template<class DrawPart>
    static void DrawSecondaryGuns(const BossModelTransform& transform, DrawPart& drawPart,
        const Stage4BossModelState& state) {
        constexpr Vector3 Positions[] = {
            {-2.85f, 3.48f, -3.45f}, {-2.85f, 3.48f, 3.45f},
            {-3.85f, 2.80f, -7.65f}, {-3.85f, 2.80f, 7.65f},
            {3.65f, 2.72f, -7.15f}, {3.65f, 2.72f, 7.15f}
        };
        for (int gun = 0; gun < 6; ++gun) {
            if (!state.secondaryGuns[gun]) continue;
            const Vector3& position = Positions[gun];
            const float* color = state.secondaryGunsHit[gun] ? Hit : MainBlack;
            Part(transform, drawPart, PrimitiveShape::Cylinder, position, {0.78f, 0.38f, 0.78f}, color);
            const Vector3 pivot {position.x, position.y + 0.28f, position.z};
            GunPart(transform, drawPart, transform.secondaryGunsTrackTarget,
                transform.secondaryAimTarget, pivot,
                PrimitiveShape::Box, 0.18f, {0.88f, 0.42f, 0.68f},
                state.secondaryGunsHit[gun] ? Hit : ArmorBlack);
            GunPart(transform, drawPart, transform.secondaryGunsTrackTarget,
                transform.secondaryAimTarget, pivot,
                PrimitiveShape::Cylinder, 0.92f, {1.25f, 0.24f, 0.24f},
                state.secondaryGunsHit[gun] ? Hit : HighlightBlack);
        }
    }

    /**
     * @brief 後方斜め上へ伸びる二本の段付き排気筒を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param state 排気筒ごとの描画状態
     * @return なし
     */
    template<class DrawPart>
    static void DrawExhaustStacks(const BossModelTransform& transform, DrawPart& drawPart,
        const Stage4BossModelState& state) {
        for (int stack = 0; stack < 2; ++stack) {
            if (!state.exhaustStacks[stack]) continue;
            const float side = stack == 0 ? -1.0f : 1.0f;
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.10f, 2.34f, side * 5.85f}, {0.62f, 0.78f, 0.62f}, MainBlack);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.30f, 3.05f, side * 5.85f}, {0.48f, 0.82f, 0.48f}, ArmorBlack, 0.0f, -0.22f);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.52f, 3.76f, side * 5.85f}, {0.36f, 0.72f, 0.36f}, HighlightBlack, 0.0f, -0.22f);
            Part(transform, drawPart, PrimitiveShape::Cylinder, {5.61f, 4.07f, side * 5.85f}, {0.48f, 0.16f, 0.48f}, Gold, 0.0f, -0.22f);
        }
    }

    /**
     * @brief 高級車風グリル、灯火、前方ラムを描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @param frontRam ラムを描画する場合true
     * @return なし
     */
    template<class DrawPart>
    static void DrawFront(const BossModelTransform& transform, DrawPart& drawPart, bool frontRam) {
        // 二重グリルと七本の縦桟を最前面中央へ配置する
        Part(transform, drawPart, PrimitiveShape::Box, {-7.42f, 1.18f, 0.0f}, {0.18f, 1.62f, 1.82f}, Gold);
        Part(transform, drawPart, PrimitiveShape::Box, {-7.53f, 1.18f, 0.0f}, {0.10f, 1.34f, 1.52f}, Window);
        for (int bar = 0; bar < 7; ++bar) {
            const float z = -0.60f + static_cast<float>(bar) * 0.20f;
            Part(transform, drawPart, PrimitiveShape::Box, {-7.60f, 1.18f, z}, {0.08f, 1.22f, 0.07f}, Gold);
        }

        // 左右二灯ずつの丸型ライトを装甲ハウジングから覗かせる
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {-7.25f, 1.48f, side * 9.25f}, {0.32f, 0.72f, 0.76f}, MainBlack);
            for (float offset : {-0.18f, 0.18f}) {
                Part(transform, drawPart, PrimitiveShape::Cylinder, {-7.48f, 1.48f, side * 9.25f + offset},
                    {0.28f, 0.16f, 0.28f}, Light, 0.0f, Math::HalfPi);
            }
        }

        // ラム本体と三本の歯を一単位として将来の突進破壊へ分離する
        if (frontRam) {
            Part(transform, drawPart, PrimitiveShape::Prism, {-8.00f, -0.48f, 0.0f}, {1.65f, 0.76f, ModelWidth}, ArmorBlack);
            for (float side : {-1.0f, 0.0f, 1.0f}) {
                Part(transform, drawPart, PrimitiveShape::Cone, {-8.78f, -0.50f, side * 8.0f},
                    {0.42f, 1.15f, 0.42f}, HighlightBlack, 0.0f, Math::HalfPi);
            }
        }
    }

    /**
     * @brief 黒い大面積を分割する少量の金装飾を描画する
     * @param transform ボス全体の親Transform
     * @param drawPart 描画関数
     * @return なし
     */
    template<class DrawPart>
    static void DrawDecorations(const BossModelTransform& transform, DrawPart& drawPart) {
        for (float side : {-1.0f, 1.0f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {2.20f, 1.12f, side * 10.35f}, {4.30f, 0.11f, 0.09f}, Gold);
            Part(transform, drawPart, PrimitiveShape::Sphere, {-5.55f, 1.86f, side * 9.95f}, {0.38f, 0.38f, 0.12f}, Gold);
            Part(transform, drawPart, PrimitiveShape::Box, {4.80f, 1.52f, side * 9.98f}, {0.46f, 0.18f, 0.10f}, Light);
        }
        for (float x : {-4.70f, -1.80f, 1.10f, 4.00f}) {
            Part(transform, drawPart, PrimitiveShape::Box, {x, 0.98f, 0.0f}, {1.45f, 0.10f, 19.4f}, Gold);
        }
    }
};

static_assert(Stage4BossModelView::PrimitiveCount == 220);
