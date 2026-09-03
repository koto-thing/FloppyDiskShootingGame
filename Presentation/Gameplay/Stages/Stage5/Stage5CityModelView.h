#pragma once

#include <array>
#include <cstddef>

#include "Stage5ModelView.h"

/** @brief Stage5のNEO AIZUビル形状 */
enum class Stage5BuildingType { Slender, Vision, Narrow, Rounded, Arcade, Utility, Count };

/** @brief Stage5の独立広告形状 */
enum class Stage5AdType { VerticalNeon, Hanging, Rooftop, LightBox, Hologram, Lantern, Marquee, Count };

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
     * @param index 0から始まる取付位置番号
     * @return 指定位置、範囲外なら先頭位置
     */
    static constexpr Stage5SignMount SignMount(Stage5BuildingType type, std::size_t index) {
        const std::size_t building = static_cast<std::size_t>(type);
        const std::size_t safeBuilding = building < Mounts.size() ? building : 0;
        return Mounts[safeBuilding][index < MountCount(type) ? index : 0];
    }

    /**
     * @brief 指定ビルの広告取付位置数を取得する
     * @param type ビル形状
     * @return 取付位置数
     */
    static constexpr std::size_t MountCount(Stage5BuildingType type) {
        constexpr std::array<std::size_t, 6> Counts {3, 3, 2, 2, 3, 2};
        const std::size_t index = static_cast<std::size_t>(type);
        return index < Counts.size() ? Counts[index] : 0;
    }

    /**
     * @brief ビルの基準外形寸法を取得する
     * @param type ビル形状
     * @return 屋上設備を含む幅、高さ、奥行き
     */
    static constexpr Vector3 ModelSize(Stage5BuildingType type) {
        constexpr std::array<Vector3, 6> Sizes {{
            {3.35f, 15.5f, 3.2f}, {7.66f, 12.0f, 5.72f},
            {3.12f, 8.6f, 3.7f}, {5.12f, 10.65f, 5.96f},
            {9.8f, 7.65f, 4.6f}, {6.0f, 10.4f, 5.6f}
        }};
        const std::size_t index = static_cast<std::size_t>(type);
        return index < Sizes.size() ? Sizes[index] : Vector3::One;
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
        auto windows = [&](int rows, float width, float frontZ, float firstY, float spacing) {
            // 窓はテクスチャを使わず、遠景でも読める連続した暗色帯にする
            for (int row = 0; row < rows; ++row) {
                part(PrimitiveShape::Box, {0.0f, firstY + row * spacing, frontZ},
                    {width, 0.38f, 0.12f}, Window);
            }
        };

        // 各形状は本体、窓帯、屋上設備、側面ディテールだけに絞る
        switch (type) {
        case Stage5BuildingType::Slender:
            part(PrimitiveShape::Box, {0.0f, 6.0f, 0.0f}, {3.0f, 12.0f, 3.2f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 1.0f, -1.64f}, {3.35f, 1.5f, 0.18f}, Wall);
            windows(7, 2.45f, -1.64f, 2.2f, 1.25f);
            part(PrimitiveShape::Box, {1.56f, 6.6f, 0.35f}, {0.14f, 7.8f, 1.5f}, Cyan);
            part(PrimitiveShape::Box, {0.0f, 12.35f, 0.0f}, {2.2f, 0.7f, 2.2f}, Wall);
            part(PrimitiveShape::Cylinder, {0.0f, 14.0f, 0.0f}, {0.22f, 2.6f, 0.22f}, DarkWall);
            part(PrimitiveShape::Sphere, {0.0f, 15.25f, 0.0f}, {0.42f, 0.42f, 0.42f}, Red);
            break;
        case Stage5BuildingType::Vision:
            part(PrimitiveShape::Box, {0.0f, 5.0f, 0.0f}, {7.5f, 10.0f, 5.0f}, Wall);
            part(PrimitiveShape::Prism, {0.0f, 10.8f, 0.0f}, {6.8f, 1.6f, 4.4f}, DarkWall);
            windows(4, 6.7f, -2.56f, 1.4f, 1.15f);
            part(PrimitiveShape::Box, {0.0f, 7.2f, -2.72f}, {5.6f, 3.0f, 0.22f}, Window);
            part(PrimitiveShape::Box, {0.0f, 7.2f, -2.86f}, {5.25f, 2.65f, 0.08f}, Pink);
            part(PrimitiveShape::Box, {-3.83f, 4.2f, 0.0f}, {0.16f, 7.0f, 3.8f}, Cyan);
            part(PrimitiveShape::Box, {1.8f, 10.75f, 0.6f}, {1.8f, 0.8f, 1.8f}, DarkWall);
            part(PrimitiveShape::Cylinder, {1.8f, 11.55f, 0.6f}, {0.7f, 0.8f, 0.7f}, DarkWall);
            break;
        case Stage5BuildingType::Narrow:
            part(PrimitiveShape::Box, {0.0f, 4.0f, 0.0f}, {2.6f, 8.0f, 3.0f}, Wall);
            windows(5, 2.1f, -1.56f, 1.2f, 1.18f);
            for (int floor = 0; floor < 3; ++floor) {
                const float y = 2.0f + floor * 2.0f;
                part(PrimitiveShape::Box, {0.0f, y, -1.85f}, {2.9f, 0.14f, 0.7f}, DarkWall);
                part(PrimitiveShape::Box, {1.5f, y - 0.8f, -1.7f}, {0.12f, 1.7f, 0.12f}, DarkWall);
            }
            part(PrimitiveShape::Box, {-1.38f, 4.3f, 0.2f}, {0.16f, 5.8f, 1.8f}, Orange);
            part(PrimitiveShape::Box, {0.55f, 8.3f, 0.0f}, {0.9f, 0.6f, 1.2f}, DarkWall);
            break;
        case Stage5BuildingType::Rounded:
            part(PrimitiveShape::Box, {0.0f, 4.5f, 0.4f}, {5.0f, 9.0f, 3.4f}, DarkWall);
            part(PrimitiveShape::Cylinder, {0.0f, 4.5f, -1.3f}, {5.0f, 9.0f, 5.0f}, Wall);
            for (int row = 0; row < 5; ++row) {
                part(PrimitiveShape::Cylinder, {0.0f, 1.2f + row * 1.45f, -1.34f},
                    {5.12f, 0.42f, 5.12f}, row == 3 ? Cyan : Window);
            }
            part(PrimitiveShape::Cylinder, {0.0f, 9.35f, -1.3f}, {4.2f, 0.7f, 4.2f}, DarkWall);
            part(PrimitiveShape::Box, {1.4f, 10.1f, 0.0f}, {1.1f, 1.1f, 1.1f}, DarkWall);
            break;
        case Stage5BuildingType::Arcade:
            part(PrimitiveShape::Box, {0.0f, 3.3f, 0.0f}, {9.0f, 6.6f, 4.0f}, Wall);
            part(PrimitiveShape::Box, {0.0f, 1.2f, -2.1f}, {9.4f, 1.8f, 0.22f}, DarkWall);
            windows(3, 8.2f, -2.06f, 3.0f, 1.15f);
            for (int shop = -3; shop <= 3; shop += 2) {
                part(PrimitiveShape::Box, {static_cast<float>(shop), 0.9f, -2.3f},
                    {1.45f, 1.1f, 0.18f}, shop == 1 ? Pink : Orange);
            }
            part(PrimitiveShape::Box, {0.0f, 6.85f, 0.0f}, {7.6f, 0.5f, 3.0f}, DarkWall);
            part(PrimitiveShape::Box, {4.7f, 4.2f, 0.0f}, {0.4f, 3.2f, 2.5f}, DarkWall);
            break;
        case Stage5BuildingType::Utility:
            part(PrimitiveShape::Box, {0.0f, 3.5f, 0.0f}, {6.0f, 7.0f, 5.0f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 1.4f, -2.56f}, {2.0f, 2.4f, 0.15f}, Wall);
            windows(2, 5.1f, -2.56f, 4.1f, 1.15f);
            for (int side : {-1, 1}) {
                const float x = static_cast<float>(side) * 2.2f;
                part(PrimitiveShape::Cylinder, {x, 5.7f, -2.7f}, {0.45f, 4.2f, 0.45f}, Wall);
                part(PrimitiveShape::Box, {x, 7.4f, 0.6f}, {1.2f, 0.8f, 1.5f}, Wall);
            }
            part(PrimitiveShape::Cylinder, {-1.2f, 8.7f, 0.6f}, {1.3f, 3.4f, 1.3f}, DarkWall);
            part(PrimitiveShape::Cylinder, {1.2f, 8.0f, 0.6f}, {0.9f, 2.0f, 0.9f}, Wall);
            part(PrimitiveShape::Box, {0.0f, 3.1f, -2.72f}, {5.4f, 0.16f, 0.16f}, Red);
            break;
        default:
            break;
        }

        if (type != Stage5BuildingType::Count) {
            // 共通の点検扉、換気口、足元灯で近距離でも壁面の密度を保つ
            for (int side : {-1, 1}) {
                const float x = static_cast<float>(side) * 0.78f;
                part(PrimitiveShape::Box, {x, 0.52f, -1.72f}, {0.54f, 0.82f, 0.10f}, DarkWall);
                part(PrimitiveShape::Box, {x, 0.52f, -1.79f}, {0.30f, 0.12f, 0.05f}, Window);
                part(PrimitiveShape::Box, {x, 1.18f, -1.78f}, {0.42f, 0.22f, 0.08f}, Wall);
                part(PrimitiveShape::Box, {x, 0.12f, -1.86f}, {0.18f, 0.12f, 0.08f}, Orange);
                part(PrimitiveShape::Cylinder, {x, 1.72f, -1.70f}, {0.12f, 0.52f, 0.12f}, DarkWall);
                part(PrimitiveShape::Sphere, {x, 1.98f, -1.70f}, {0.16f, 0.16f, 0.16f}, Cyan);
            }
        }
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

        // 表示面と支持部を分離し、呼び出し側で広告単位の点灯や交換を可能にする
        switch (type) {
        case Stage5AdType::VerticalNeon:
            part(PrimitiveShape::Box, {}, {1.1f, 4.2f, 0.18f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {0.78f, 3.75f, 0.08f}, Pink);
            for (int mark = -1; mark <= 1; ++mark) part(PrimitiveShape::Box,
                {0.0f, static_cast<float>(mark) * 1.05f, -0.18f}, {0.5f, 0.16f, 0.07f}, SignWhite);
            break;
        case Stage5AdType::Hanging:
            part(PrimitiveShape::Box, {0.0f, 0.8f, 0.0f}, {1.8f, 0.12f, 0.12f}, DarkWall);
            part(PrimitiveShape::Box, {-0.8f, 0.2f, 0.0f}, {0.12f, 1.3f, 0.12f}, DarkWall);
            part(PrimitiveShape::Box, {-0.8f, -0.7f, 0.0f}, {1.4f, 0.8f, 0.16f}, Orange);
            part(PrimitiveShape::Box, {-0.8f, -0.7f, -0.12f}, {0.8f, 0.12f, 0.06f}, SignWhite);
            part(PrimitiveShape::Sphere, {-0.8f, 0.78f, 0.0f}, {0.18f, 0.18f, 0.18f}, Cyan);
            break;
        case Stage5AdType::Rooftop:
            part(PrimitiveShape::Box, {}, {5.0f, 2.4f, 0.16f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {4.6f, 2.0f, 0.08f}, Cyan);
            for (int side : {-1, 1}) part(PrimitiveShape::Box,
                {static_cast<float>(side) * 1.8f, -1.8f, 0.0f}, {0.16f, 1.4f, 0.16f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.18f}, {2.8f, 0.16f, 0.06f}, SignWhite);
            break;
        case Stage5AdType::LightBox:
            part(PrimitiveShape::Box, {}, {2.8f, 1.3f, 0.18f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {2.45f, 0.95f, 0.08f}, SignWhite);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.18f}, {1.5f, 0.16f, 0.06f}, Red);
            part(PrimitiveShape::Box, {-1.28f, 0.0f, 0.0f}, {0.12f, 1.1f, 0.12f}, Cyan);
            part(PrimitiveShape::Box, {1.28f, 0.0f, 0.0f}, {0.12f, 1.1f, 0.12f}, Cyan);
            break;
        case Stage5AdType::Hologram:
            part(PrimitiveShape::Cylinder, {0.0f, -0.8f, 0.0f}, {0.8f, 0.45f, 0.8f}, DarkWall);
            part(PrimitiveShape::Cone, {0.0f, -0.35f, 0.0f}, {0.5f, 0.7f, 0.5f}, Cyan);
            part(PrimitiveShape::Box, {0.0f, 0.8f, 0.0f}, {2.4f, 1.8f, 0.06f}, {Cyan.r, Cyan.g, Cyan.b, 0.42f});
            part(PrimitiveShape::Box, {0.0f, 0.8f, -0.06f}, {1.5f, 0.10f, 0.04f}, SignWhite);
            part(PrimitiveShape::Sphere, {0.0f, 0.8f, -0.10f}, {0.26f, 0.26f, 0.10f}, Pink);
            break;
        case Stage5AdType::Lantern:
            part(PrimitiveShape::Cylinder, {}, {1.0f, 2.2f, 1.0f}, Red);
            part(PrimitiveShape::Cylinder, {0.0f, 1.15f, 0.0f}, {1.1f, 0.18f, 1.1f}, DarkWall);
            part(PrimitiveShape::Cylinder, {0.0f, -1.15f, 0.0f}, {1.1f, 0.18f, 1.1f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.52f}, {0.14f, 1.45f, 0.06f}, SignWhite);
            part(PrimitiveShape::Sphere, {0.0f, 0.0f, -0.58f}, {0.22f, 0.22f, 0.08f}, Orange);
            break;
        case Stage5AdType::Marquee:
            part(PrimitiveShape::Box, {}, {6.0f, 0.9f, 0.18f}, DarkWall);
            part(PrimitiveShape::Box, {0.0f, 0.0f, -0.12f}, {5.65f, 0.55f, 0.08f}, Orange);
            for (int light = -2; light <= 2; ++light) part(PrimitiveShape::Sphere,
                {static_cast<float>(light), 0.0f, -0.22f}, {0.16f, 0.16f, 0.16f}, SignWhite);
            break;
        default:
            break;
        }
    }

private:
    inline static constexpr std::array<std::array<Stage5SignMount, 3>, 6> Mounts {{
        {{{Stage5SignMountKind::RightWall, {{1.7f, 6.2f, 0.0f}, {0.0f, Math::HalfPi, 0.0f}, Vector3::One}}, {Stage5SignMountKind::FrontLarge, {{0.0f, 6.0f, -1.78f}, {}, Vector3::One}}, {Stage5SignMountKind::Rooftop, {{0.0f, 13.7f, 0.0f}, {}, Vector3::One}}}},
        {{{Stage5SignMountKind::FrontLarge, {{0.0f, 7.2f, -2.95f}, {}, Vector3::One}}, {Stage5SignMountKind::Rooftop, {{0.0f, 12.0f, 0.0f}, {}, Vector3::One}}, {Stage5SignMountKind::LeftWall, {{-3.95f, 5.0f, 0.0f}, {0.0f, Math::HalfPi, 0.0f}, Vector3::One}}}},
        {{{Stage5SignMountKind::LeftWall, {{-1.5f, 4.4f, 0.0f}, {0.0f, Math::HalfPi, 0.0f}, Vector3::One}}, {Stage5SignMountKind::FrontLarge, {{0.0f, 6.0f, -1.72f}, {}, Vector3::One}}, {}}},
        {{{Stage5SignMountKind::CornerBand, {{0.0f, 5.55f, -3.92f}, {}, Vector3::One}}, {Stage5SignMountKind::Rooftop, {{0.0f, 10.25f, -1.3f}, {}, Vector3::One}}, {}}},
        {{{Stage5SignMountKind::FrontLarge, {{0.0f, 1.2f, -2.45f}, {}, Vector3::One}}, {Stage5SignMountKind::Rooftop, {{0.0f, 7.4f, 0.0f}, {}, Vector3::One}}, {Stage5SignMountKind::RightWall, {{4.95f, 3.4f, 0.0f}, {0.0f, Math::HalfPi, 0.0f}, Vector3::One}}}},
        {{{Stage5SignMountKind::FrontLarge, {{0.0f, 3.1f, -2.88f}, {}, Vector3::One}}, {Stage5SignMountKind::Rooftop, {{0.0f, 9.5f, 0.0f}, {}, Vector3::One}}, {}}}
    }};
};
