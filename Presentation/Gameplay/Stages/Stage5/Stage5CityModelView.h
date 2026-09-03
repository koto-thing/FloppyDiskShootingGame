#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "Stage5ModelView.h"

/** @brief 添付FBXのシルエットを近似するNEO AIZU建築形状 */
enum class Stage5BuildingType {
    Tower,
    TowerSub,
    TowerSubSmall,
    BuildingLeft,
    BuildingLeftSmall,
    BuildingRight,
    BuildingRightSmall,
    Count
};

/** @brief Stage5の独立広告形状 */
enum class Stage5AdType {
    VerticalNeon,
    Hanging,
    Rooftop,
    LightBox,
    Hologram,
    Lantern,
    Marquee,
    TayamaFace,
    Count
};
/** @brief 広告取付位置の用途 */
enum class Stage5SignMountKind { LeftWall, RightWall, FrontLarge, Rooftop, CornerBand };

/** @brief ビルのローカル広告取付位置 */
struct Stage5SignMount {
    Stage5SignMountKind kind = Stage5SignMountKind::FrontLarge;
    Stage5PartTransform transform {};
};

/** @brief NEO AIZU都市モデルをPrimitive3Dだけで列挙する */
class Stage5CityModelView final {
public:
    inline static constexpr ColorF ConcreteDark {0.055f, 0.10f, 0.20f, 1.0f};
    inline static constexpr ColorF MetalDark {0.025f, 0.05f, 0.11f, 1.0f};
    inline static constexpr ColorF WindowDark {0.04f, 0.08f, 0.16f, 1.0f};
    inline static constexpr ColorF FrameMetal {0.30f, 0.38f, 0.52f, 1.0f};
    inline static constexpr ColorF NeonAccent {0.12f, 0.82f, 0.98f, 1.0f};
    inline static constexpr ColorF Wall {0.055f, 0.10f, 0.20f, 1.0f};
    inline static constexpr ColorF DarkWall {0.025f, 0.05f, 0.11f, 1.0f};
    inline static constexpr ColorF Window {0.04f, 0.08f, 0.16f, 1.0f};
    inline static constexpr ColorF Cyan {0.12f, 0.82f, 0.98f, 1.0f};
    inline static constexpr ColorF Pink {0.90f, 0.18f, 0.76f, 1.0f};
    inline static constexpr ColorF Orange {0.92f, 0.58f, 0.18f, 1.0f};
    inline static constexpr ColorF Red {0.80f, 0.20f, 0.18f, 1.0f};
    inline static constexpr ColorF SignWhite {0.92f, 0.92f, 0.95f, 1.0f};

    /**
     * @brief 指定ビルの広告取付位置を取得する
     * @param type ビル形状
     * @param index 0=正面、1=側面、2=屋上
     * @return 指定位置、範囲外なら正面位置
     */
    static constexpr Stage5SignMount SignMount(Stage5BuildingType type, std::size_t index) {
        if (index == 1) return SideSignMount(type);
        if (index == 2) return RoofSignMount(type);
        return FrontSignMount(type);
    }

    /**
     * @brief 指定ビルの広告取付位置数を取得する
     * @param type ビル形状
     * @return 有効なビルなら3、範囲外なら0
     */
    static constexpr std::size_t MountCount(Stage5BuildingType type) {
        return static_cast<std::size_t>(type) <
            static_cast<std::size_t>(Stage5BuildingType::Count) ? 3 : 0;
    }

    /**
     * @brief 指定ビルの正面広告取付位置を取得する
     * @param type ビル形状
     * @return 正面広告のローカルTransform
     */
    static constexpr Stage5SignMount FrontSignMount(Stage5BuildingType type) {
        switch (type) {
        case Stage5BuildingType::Tower:
            return {Stage5SignMountKind::FrontLarge, {{0.0f, 3.7f, -2.42f}}};
        case Stage5BuildingType::TowerSub:
            return {Stage5SignMountKind::FrontLarge, {{0.0f, 8.0f, -2.30f}}};
        case Stage5BuildingType::TowerSubSmall:
            return {Stage5SignMountKind::FrontLarge, {{0.0f, 5.0f, -2.30f}}};
        case Stage5BuildingType::BuildingLeft:
        case Stage5BuildingType::BuildingRight:
            return {Stage5SignMountKind::FrontLarge, {{0.0f, 6.7f, -1.90f}}};
        case Stage5BuildingType::BuildingLeftSmall:
        case Stage5BuildingType::BuildingRightSmall:
            return {Stage5SignMountKind::FrontLarge, {{0.0f, 3.7f, -1.47f}}};
        default:
            return {};
        }
    }

    /**
     * @brief 指定ビルの側面広告取付位置を取得する
     * @param type ビル形状
     * @return 側面広告のローカルTransform
     */
    static constexpr Stage5SignMount SideSignMount(Stage5BuildingType type) {
        switch (type) {
        case Stage5BuildingType::Tower:
            return {Stage5SignMountKind::RightWall,
                {{2.42f, 3.7f, 0.0f}, {0.0f, -Math::HalfPi, 0.0f}}};
        case Stage5BuildingType::TowerSub:
            return {Stage5SignMountKind::RightWall,
                {{2.30f, 8.0f, 0.0f}, {0.0f, -Math::HalfPi, 0.0f}}};
        case Stage5BuildingType::TowerSubSmall:
            return {Stage5SignMountKind::RightWall,
                {{2.30f, 5.0f, 0.0f}, {0.0f, -Math::HalfPi, 0.0f}}};
        case Stage5BuildingType::BuildingLeft:
            return {Stage5SignMountKind::LeftWall,
                {{-2.61f, 6.7f, 0.0f}, {0.0f, Math::HalfPi, 0.0f}}};
        case Stage5BuildingType::BuildingLeftSmall:
            return {Stage5SignMountKind::LeftWall,
                {{-1.98f, 3.7f, 0.0f}, {0.0f, Math::HalfPi, 0.0f}}};
        case Stage5BuildingType::BuildingRight:
            return {Stage5SignMountKind::RightWall,
                {{2.61f, 6.7f, 0.0f}, {0.0f, -Math::HalfPi, 0.0f}}};
        case Stage5BuildingType::BuildingRightSmall:
            return {Stage5SignMountKind::RightWall,
                {{1.98f, 3.7f, 0.0f}, {0.0f, -Math::HalfPi, 0.0f}}};
        default:
            return {};
        }
    }

    /**
     * @brief 指定ビルの屋上広告取付位置を取得する
     * @param type ビル形状
     * @return 屋上広告のローカルTransform
     */
    static constexpr Stage5SignMount RoofSignMount(Stage5BuildingType type) {
        switch (type) {
        case Stage5BuildingType::Tower:
            return {Stage5SignMountKind::Rooftop, {{0.0f, 7.743f, 0.0f}}};
        case Stage5BuildingType::TowerSub:
            return {Stage5SignMountKind::Rooftop, {{0.0f, 15.33f, 0.0f}}};
        case Stage5BuildingType::TowerSubSmall:
            return {Stage5SignMountKind::Rooftop, {{0.0f, 9.83f, 0.0f}}};
        case Stage5BuildingType::BuildingLeft:
            return {Stage5SignMountKind::Rooftop, {{-0.60f, 11.40f, 0.0f}}};
        case Stage5BuildingType::BuildingLeftSmall:
            return {Stage5SignMountKind::Rooftop, {{0.48f, 7.92f, 0.0f}}};
        case Stage5BuildingType::BuildingRight:
            return {Stage5SignMountKind::Rooftop, {{0.60f, 11.40f, 0.0f}}};
        case Stage5BuildingType::BuildingRightSmall:
            return {Stage5SignMountKind::Rooftop, {{-0.48f, 7.44f, 0.0f}}};
        default:
            return {};
        }
    }

    /**
     * @brief ビルの基準外形寸法を取得する
     * @param type ビル形状
     * @return 幅、高さ、奥行き
     */
    static constexpr Vector3 ModelSize(Stage5BuildingType type) {
        constexpr std::array<Vector3, 7> Sizes {{
            {4.40f, 7.743f, 4.40f},
            {4.40f, 15.33f, 4.40f},
            {4.40f, 9.83f, 4.40f},
            {4.62f, 11.40f, 3.168f},
            {3.36f, 7.92f, 2.304f},
            {4.62f, 11.40f, 3.168f},
            {3.37f, 7.44f, 2.304f}
        }};
        const std::size_t index = static_cast<std::size_t>(type);
        return index < Sizes.size() ? Sizes[index] : Vector3::One;
    }

    /**
     * @brief 指定ビルのPrimitive数を取得する
     * @param type ビル形状
     * @return 描画するPrimitive数
     */
    static constexpr std::size_t PrimitiveCount(Stage5BuildingType type) {
        constexpr std::array<std::size_t, 7> Counts {22, 12, 11, 20, 20, 21, 20};
        const std::size_t index = static_cast<std::size_t>(type);
        return index < Counts.size() ? Counts[index] : 0;
    }

    /**
     * @brief ビルを構成するPrimitiveを列挙する
     * @param type ビル形状
     * @param transform モデル全体のTransform
     * @param drawPart 形状、ワールド行列、色を受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitBuilding(Stage5BuildingType type, const Stage5ModelTransform& transform,
        DrawPart&& drawPart) {
        VisitBuilding(type, Stage5ModelDetail::Matrix(transform),
            static_cast<DrawPart&&>(drawPart));
    }

    /**
     * @brief 非均一拡縮可能な行列からビルのPrimitiveを列挙する
     * @param type ビル形状
     * @param root モデル全体のワールド行列
     * @param drawPart 形状、ワールド行列、色を受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitBuilding(Stage5BuildingType type, const Matrix4x4& root,
        DrawPart&& drawPart) {
        auto part = [&](PrimitiveShape shape, const Vector3& position, const Vector3& scale,
            const ColorF& color, const Vector3& rotation = {}) {
            drawPart(shape, root * Stage5ModelDetail::Matrix({position, rotation, scale}), color);
        };

        // FBXのZ-up寸法をゲームのY-upへ移して共通Primitiveで組み立てる
        switch (type) {
        case Stage5BuildingType::Tower:
            VisitTower(part);
            break;
        case Stage5BuildingType::TowerSub:
            VisitSubTower(15.0f, 13.77f, 14.05f, 5, part);
            break;
        case Stage5BuildingType::TowerSubSmall:
            VisitSubTower(9.5f, 8.27f, 8.275f, 4, part);
            break;
        case Stage5BuildingType::BuildingLeft:
            VisitModularBuilding(false, false, part);
            break;
        case Stage5BuildingType::BuildingLeftSmall:
            VisitModularBuilding(true, false, part);
            break;
        case Stage5BuildingType::BuildingRight:
            VisitModularBuilding(false, true, part);
            break;
        case Stage5BuildingType::BuildingRightSmall:
            VisitModularBuilding(true, true, part);
            break;
        default:
            break;
        }
    }

    /**
     * @brief TAYAMAfaceの広告パネルを独立して列挙する
     * @param world 広告全体のワールド行列
     * @param drawPart 形状、ワールド行列、色を受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitTayamaFace(const Matrix4x4& world, DrawPart&& drawPart) {
        // 広告用途に合わせてFBX外形を幅0.924、高さ1.68、奥行き0.576へ立てる
        drawPart(PrimitiveShape::Box,
            world * Stage5ModelDetail::Matrix({{}, {}, {0.924f, 1.68f, 0.576f}}), SignWhite);
        drawPart(PrimitiveShape::Box,
            world * Stage5ModelDetail::Matrix({{-0.30f, 0.0f, 0.36f}, {}, {0.12f, 0.46f, 0.18f}}),
            MetalDark);
        drawPart(PrimitiveShape::Box,
            world * Stage5ModelDetail::Matrix({{0.30f, 0.0f, 0.36f}, {}, {0.12f, 0.46f, 0.18f}}),
            MetalDark);
    }

    /**
     * @brief 独立した広告モデルを列挙する
     * @param type 広告形状
     * @param world 広告のワールド行列
     * @param drawPart 形状、ワールド行列、色を受け取る関数
     * @return なし
     */
    template<class DrawPart>
    static void VisitAd(Stage5AdType type, const Matrix4x4& world, DrawPart&& drawPart) {
        auto part = [&](PrimitiveShape shape, const Vector3& position, const Vector3& scale,
            const ColorF& color, const Vector3& rotation = {}) {
            drawPart(shape, world * Stage5ModelDetail::Matrix({position, rotation, scale}), color);
        };

        // 表示面と支持部を分けて広告単位で交換可能にする
        switch (type) {
        case Stage5AdType::VerticalNeon:
            part(PrimitiveShape::Box, {}, {1.1f, 4.2f, 0.18f}, MetalDark);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {0.78f, 3.75f, 0.08f}, Pink);
            for (int mark = -1; mark <= 1; ++mark) {
                part(PrimitiveShape::Box, {0.0f, static_cast<float>(mark) * 1.05f, -0.18f},
                    {0.5f, 0.16f, 0.07f}, SignWhite);
            }
            break;
        case Stage5AdType::Hanging:
            part(PrimitiveShape::Box, {0.0f, 0.8f, 0.0f}, {1.8f, 0.12f, 0.12f}, MetalDark);
            part(PrimitiveShape::Box, {-0.8f, 0.2f, 0.0f}, {0.12f, 1.3f, 0.12f}, MetalDark);
            part(PrimitiveShape::Box, {-0.8f, -0.7f, 0.0f}, {1.4f, 0.8f, 0.16f}, Orange);
            part(PrimitiveShape::Box, {-0.8f, -0.7f, -0.12f}, {0.8f, 0.12f, 0.06f}, SignWhite);
            part(PrimitiveShape::Sphere, {-0.8f, 0.78f, 0.0f}, {0.18f, 0.18f, 0.18f}, NeonAccent);
            break;
        case Stage5AdType::Rooftop:
            part(PrimitiveShape::Box, {}, {5.0f, 2.4f, 0.16f}, MetalDark);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {4.6f, 2.0f, 0.08f}, NeonAccent);
            for (int side : {-1, 1}) {
                part(PrimitiveShape::Box, {static_cast<float>(side) * 1.8f, -1.8f, 0.0f},
                    {0.16f, 1.4f, 0.16f}, MetalDark);
            }
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.18f}, {2.8f, 0.16f, 0.06f}, SignWhite);
            break;
        case Stage5AdType::LightBox:
            part(PrimitiveShape::Box, {}, {2.8f, 1.3f, 0.18f}, MetalDark);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {2.45f, 0.95f, 0.08f}, SignWhite);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.18f}, {1.5f, 0.16f, 0.06f}, Red);
            part(PrimitiveShape::Box, {-1.28f, 0.0f, 0.0f}, {0.12f, 1.1f, 0.12f}, NeonAccent);
            part(PrimitiveShape::Box, {1.28f, 0.0f, 0.0f}, {0.12f, 1.1f, 0.12f}, NeonAccent);
            break;
        case Stage5AdType::Hologram:
            part(PrimitiveShape::Cylinder, {0.0f, -0.8f, 0.0f}, {0.8f, 0.45f, 0.8f}, MetalDark);
            part(PrimitiveShape::Cone, {0.0f, -0.35f, 0.0f}, {0.5f, 0.7f, 0.5f}, NeonAccent);
            part(PrimitiveShape::Box, {0.0f, 0.8f, 0.0f}, {2.4f, 1.8f, 0.06f},
                {NeonAccent.r, NeonAccent.g, NeonAccent.b, 0.42f});
            part(PrimitiveShape::Box, {0.0f, 0.8f, -0.06f}, {1.5f, 0.10f, 0.04f}, SignWhite);
            part(PrimitiveShape::Sphere, {0.0f, 0.8f, -0.10f}, {0.26f, 0.26f, 0.10f}, Pink);
            break;
        case Stage5AdType::Lantern:
            part(PrimitiveShape::Cylinder, {}, {1.0f, 2.2f, 1.0f}, Red);
            part(PrimitiveShape::Cylinder, {0.0f, 1.15f, 0.0f}, {1.1f, 0.18f, 1.1f}, MetalDark);
            part(PrimitiveShape::Cylinder, {0.0f, -1.15f, 0.0f}, {1.1f, 0.18f, 1.1f}, MetalDark);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.52f}, {0.14f, 1.45f, 0.06f}, SignWhite);
            part(PrimitiveShape::Sphere, {0.0f, 0.0f, -0.58f}, {0.22f, 0.22f, 0.08f}, Orange);
            break;
        case Stage5AdType::Marquee:
            part(PrimitiveShape::Box, {}, {6.0f, 0.9f, 0.18f}, MetalDark);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {5.65f, 0.55f, 0.08f}, Orange);
            for (int light = -2; light <= 2; ++light) {
                part(PrimitiveShape::Sphere, {static_cast<float>(light), 0.0f, -0.22f},
                    {0.16f, 0.16f, 0.16f}, SignWhite);
            }
            break;
        case Stage5AdType::TayamaFace:
            VisitTayamaFace(world, static_cast<DrawPart&&>(drawPart));
            break;
        default:
            break;
        }
    }

private:
    /**
     * @brief tower.fbxを円柱と12枚の外周装甲で近似する
     * @param part Primitiveを受け取る関数
     * @return なし
     */
    template<class Part>
    static void VisitTower(Part& part) {
        // 中央塔とFBX由来の太い上下カラーを配置する
        part(PrimitiveShape::Cylinder, {0.0f, 3.0f, 0.0f}, {4.0f, 6.0f, 4.0f}, ConcreteDark);
        part(PrimitiveShape::Cylinder, {0.0f, 1.0f, 0.0f}, {4.4f, 1.98f, 4.4f}, MetalDark);
        part(PrimitiveShape::Cylinder, {0.0f, 6.0f, 0.0f}, {4.4f, 0.99f, 4.4f}, MetalDark);

        // 薄いリングを高さ方向へ分散して低解像度でも段差を残す
        constexpr std::array<float, 6> RingHeights {2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f};
        for (float y : RingHeights) {
            part(PrimitiveShape::Cylinder, {0.0f, y, 0.0f}, {4.4f, 0.099f, 4.4f}, FrameMetal);
        }

        // FBXの12枚の板を15度ずつ回し円筒を覆う縦分割へ近似する
        for (int panel = 0; panel < 12; ++panel) {
            const float angle = static_cast<float>(panel) * Math::Pi / 12.0f;
            part(PrimitiveShape::Box, {0.0f, 3.7f, 0.0f}, {0.20f, 4.0f, 4.20f},
                panel % 2 == 0 ? FrameMetal : ConcreteDark, {0.0f, angle, 0.0f});
        }

        // FBX上端の小径シリンダーを同じ比率で載せる
        part(PrimitiveShape::Cylinder, {0.0f, 7.0f, 0.0f},
            {3.08f, 1.485f, 3.08f}, ConcreteDark);
    }

    /**
     * @brief tower_sub系を高さとリブ数だけ変えて近似する
     * @param bodyHeight 本体の高さ
     * @param armorHeight 縦リブの高さ
     * @param lastRibY 最上段横リブの高さ
     * @param ribCount 描画する代表横リブ数
     * @param part Primitiveを受け取る関数
     * @return なし
     */
    template<class Part>
    static void VisitSubTower(float bodyHeight, float armorHeight, float lastRibY,
        int ribCount, Part& part) {
        // 4×4角塔へFBXと同寸法の下部帯と上部キャップを追加する
        part(PrimitiveShape::Box, {0.0f, bodyHeight * 0.5f, 0.0f},
            {4.0f, bodyHeight, 4.0f}, ConcreteDark);
        part(PrimitiveShape::Box, {0.0f, 1.5f, 0.0f}, {4.4f, 0.825f, 4.4f}, MetalDark);
        part(PrimitiveShape::Box, {0.0f, bodyHeight, 0.0f}, {4.4f, 0.66f, 4.4f}, FrameMetal);

        // 元の多数リブから等間隔の代表段だけを残してPrimitive予算を守る
        for (int rib = 0; rib < ribCount; ++rib) {
            const float amount = ribCount == 1 ? 0.0f :
                static_cast<float>(rib) / static_cast<float>(ribCount - 1);
            part(PrimitiveShape::Box, {0.0f, Math::Lerp(2.5f, lastRibY, amount), 0.0f},
                {4.18f, 0.165f, 4.18f}, FrameMetal);
        }

        // 20枚の縦板は正面と側面の代表4枚へ間引く
        const float armorCenter = 1.115f + armorHeight * 0.5f;
        for (int side : {-1, 1}) {
            const float offset = static_cast<float>(side) * 1.632f;
            part(PrimitiveShape::Box, {offset, armorCenter, 0.0f},
                {0.082f, armorHeight, 4.08f}, NeonAccent);
            part(PrimitiveShape::Box, {0.0f, armorCenter, offset},
                {4.08f, armorHeight, 0.082f}, MetalDark);
        }
    }

    /**
     * @brief building_leftとbuilding_right系を同じ積層処理で近似する
     * @param isSmall 小型FBXならtrue
     * @param mirror 右型ならtrue
     * @param part Primitiveを受け取る関数
     * @return なし
     */
    template<class Part>
    static void VisitModularBuilding(bool isSmall, bool mirror, Part& part) {
        const float size = isSmall ? 0.8f : 1.0f;
        const float floorFirstY = isSmall ? 1.20f : 3.60f;
        const float windowFirstY = isSmall ? 0.672f : 2.94f;
        const float spacing = isSmall ? 1.20f : 1.50f;
        const float windowX = mirror ? (isSmall ? 0.08f : 0.10f) :
            (isSmall ? -0.08f : -0.10f);

        // 大型版だけはFBXの2.4m基礎を先に置く
        if (!isSmall) {
            part(PrimitiveShape::Box, {0.0f, 1.20f, 0.0f},
                {4.20f, 2.40f, 2.88f}, ConcreteDark);
        }

        // 六層の奥まった窓帯と床帯を同じループで積み上げる
        for (int floor = 0; floor < 6; ++floor) {
            const float offset = static_cast<float>(floor) * spacing;
            const ColorF& windowColor = floor % 3 == 0 ? WindowDark :
                (floor % 3 == 1 ? ColorF {0.035f, 0.12f, 0.16f, 1.0f} :
                    ColorF {0.10f, 0.045f, 0.16f, 1.0f});
            part(PrimitiveShape::Box, {windowX, windowFirstY + offset, 0.0f},
                {3.938f * size, 1.134f * size, 2.722f * size}, windowColor);
            part(PrimitiveShape::Box, {0.0f, floorFirstY + offset, 0.0f},
                {4.20f * size, 0.60f * size, 2.88f * size}, FrameMetal);
        }

        // 二本の柱位置を原点まわりで反転して左右型を共有する
        const float direction = mirror ? -1.0f : 1.0f;
        const float columnCenterY = isSmall ? 3.60f : 6.60f;
        const float columnHeight = 9.0f * size;
        part(PrimitiveShape::Box, {direction * -0.60f * size, columnCenterY, 0.0f},
            {0.12f * size, columnHeight, 2.76f * size}, MetalDark);
        part(PrimitiveShape::Box, {direction * 0.90f * size, columnCenterY, 0.0f},
            {0.12f * size, columnHeight, 2.76f * size}, NeonAccent);

        // 六本ある補助トリムのうち輪郭に効く五本だけを残す
        constexpr std::array<float, 5> LargeTrimHeights {1.90f, 4.40f, 5.90f, 8.90f, 10.40f};
        constexpr std::array<float, 5> SmallTrimHeights {0.60f, 1.80f, 3.0f, 5.40f, 6.60f};
        for (int trim = 0; trim < 5; ++trim) {
            const float trimY = isSmall ? SmallTrimHeights[trim] : LargeTrimHeights[trim];
            const Vector3 trimScale = isSmall ?
                Vector3 {3.293f, 0.24f, 2.258f} :
                (trim == 0 ? Vector3 {4.62f, 0.33f, 3.168f} :
                    Vector3 {4.20f, 0.30f, 2.88f});
            part(PrimitiveShape::Box, {0.0f, trimY, 0.0f}, trimScale, MetalDark);
        }

        // 小型左右に固有の屋上箱または端柱だけを追加する
        if (isSmall && !mirror) {
            part(PrimitiveShape::Box, {0.48f, 7.44f, 0.0f},
                {0.96f, 0.96f, 0.96f}, ConcreteDark);
        } else if (mirror) {
            part(PrimitiveShape::Box, {(isSmall ? 1.642353f : 2.052941f), columnCenterY, 0.0f},
                {0.12f * size, columnHeight, 2.76f * size}, MetalDark);
        }
    }
};
