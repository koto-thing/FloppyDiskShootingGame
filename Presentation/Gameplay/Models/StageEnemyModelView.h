#pragma once

#include "../../../Engine/Graphics/Renderer.h"
#include "../../../Engine/Math/Math.h"

/** @brief ステージ固有の通常敵プロシージャルモデル */
class StageEnemyModelView final {
public:
    /**
     * @brief 指定ステージのモデル部品数を取得する
     * @param stageNumber ステージ番号
     * @return モデル部品数、対象外の場合0
     */
    static constexpr int PartCount(int stageNumber) {
        constexpr int Counts[] = {0, 5, 5, 3, 7};
        return stageNumber >= 1 && stageNumber <= 4 ? Counts[stageNumber] : 0;
    }

    /**
     * @brief 指定ステージの通常敵モデルを描画する
     * @param stageNumber ステージ番号
     * @param position モデル中心のワールド座標
     * @param yaw モデル全体のY軸回転角度
     * @param scale モデル全体の倍率
     * @param drawPart 形状、ワールド行列、色を受け取る描画関数
     * @return なし
     */
    template<class DrawPart>
    static void Draw(int stageNumber, const Vector3& position, float yaw, float scale, DrawPart&& drawPart) {
        constexpr ColorF Red {0.9f, 0.1f, 0.1f, 1.0f};
        constexpr ColorF Blue {0.0f, 0.0f, 1.0f, 1.0f};
        constexpr ColorF Yellow {1.0f, 1.0f, 0.0f, 1.0f};
        constexpr ColorF Green {0.0f, 1.0f, 0.0f, 1.0f};
        const Matrix4x4 parent = Matrix4x4::Translation(position) * Matrix4x4::RotationY(yaw) *
            Matrix4x4::Scale({scale, scale, scale});

        // 参照モデルのステージ別構成を同じプリミティブで描画する
        switch (stageNumber) {
        case 1:
            Part(parent, drawPart, PrimitiveShape::Cylinder, {}, {0.4f, 1.0f, 0.4f}, {Math::HalfPi, 0.0f, 0.0f}, Red);
            Part(parent, drawPart, PrimitiveShape::Cone, {0.0f, 0.0f, 0.75f}, {0.4f, 0.5f, 0.4f}, {Math::HalfPi, 0.0f, 0.0f}, Red);
            Part(parent, drawPart, PrimitiveShape::Prism, {-0.5f, 0.0f, 0.0f}, {1.0f, 0.025f, 0.35f}, {}, Red);
            Part(parent, drawPart, PrimitiveShape::Prism, {0.5f, 0.0f, 0.0f}, {1.0f, 0.025f, 0.35f}, {}, Red);
            Part(parent, drawPart, PrimitiveShape::Box, {}, {1.0f, 0.025f, 0.35f}, {}, Red);
            break;
        case 2:
            Part(parent, drawPart, PrimitiveShape::Cylinder, {}, {0.6f, 1.0f, 0.6f}, {Math::HalfPi, 0.0f, 0.0f}, Blue);
            Part(parent, drawPart, PrimitiveShape::Cylinder, {0.0f, 0.15f, 0.7f}, {0.2f, 0.4f, 0.2f}, {Math::HalfPi, 0.0f, 0.0f}, Blue);
            Part(parent, drawPart, PrimitiveShape::Box, {0.0f, 0.0f, -0.5f}, {0.4f, 1.6f, 0.05f}, {}, Blue);
            Part(parent, drawPart, PrimitiveShape::Box, {0.0f, 0.0f, -0.5f}, {1.6f, 0.4f, 0.05f}, {}, Blue);
            Part(parent, drawPart, PrimitiveShape::Cylinder, {0.0f, 0.0f, -0.5f}, {0.3f, 0.2f, 0.3f}, {Math::HalfPi, 0.0f, 0.0f}, Blue);
            break;
        case 3:
            Part(parent, drawPart, PrimitiveShape::Cylinder, {}, {1.0f, 0.5f, 1.0f}, {}, Yellow);
            Part(parent, drawPart, PrimitiveShape::Cylinder, {0.0f, 0.4f, 0.0f}, {0.5f, 0.3f, 0.5f}, {}, Yellow);
            Part(parent, drawPart, PrimitiveShape::Cylinder, {0.0f, 0.4f, 0.45f}, {0.2f, 0.4f, 0.2f}, {Math::HalfPi, 0.0f, 0.0f}, Yellow);
            break;
        case 4:
            Part(parent, drawPart, PrimitiveShape::Box, {}, {0.5f, 0.5f, 0.5f}, {}, Green);
            Part(parent, drawPart, PrimitiveShape::Cone, {0.0f, 0.75f, 0.0f}, {0.5f, 1.0f, 0.5f}, {}, Green);
            Part(parent, drawPart, PrimitiveShape::Cone, {0.0f, -0.75f, 0.0f}, {0.5f, 1.0f, 0.5f}, {Math::Pi, 0.0f, 0.0f}, Green);
            Part(parent, drawPart, PrimitiveShape::Cone, {-0.75f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.5f}, {0.0f, 0.0f, Math::HalfPi}, Green);
            Part(parent, drawPart, PrimitiveShape::Cone, {0.75f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.5f}, {0.0f, 0.0f, -Math::HalfPi}, Green);
            Part(parent, drawPart, PrimitiveShape::Cone, {0.0f, 0.0f, -0.75f}, {0.5f, 1.0f, 0.5f}, {-Math::HalfPi, 0.0f, 0.0f}, Green);
            Part(parent, drawPart, PrimitiveShape::Cone, {0.0f, 0.0f, 0.75f}, {0.5f, 1.0f, 0.5f}, {Math::HalfPi, 0.0f, 0.0f}, Green);
            break;
        default: break;
        }
    }

private:
    /**
     * @brief 一つの部品を親行列へ合成する
     * @param parent モデル全体の親行列
     * @param drawPart 描画関数
     * @param shape プリミティブ形状
     * @param position ローカル座標
     * @param scale ローカル寸法
     * @param rotation X、Y、Z軸のローカル回転角度
     * @param color 部品色
     * @return なし
     */
    template<class DrawPart>
    static void Part(const Matrix4x4& parent, DrawPart& drawPart, PrimitiveShape shape,
        const Vector3& position, const Vector3& scale, const Vector3& rotation, const ColorF& color) {
        const Matrix4x4 local = Matrix4x4::Translation(position) * Matrix4x4::RotationY(rotation.y) *
            Matrix4x4::RotationX(rotation.x) * Matrix4x4::RotationZ(rotation.z) * Matrix4x4::Scale(scale);
        drawPart(shape, parent * local, color);
    }
};

static_assert(StageEnemyModelView::PartCount(1) == 5);
static_assert(StageEnemyModelView::PartCount(2) == 5);
static_assert(StageEnemyModelView::PartCount(3) == 3);
static_assert(StageEnemyModelView::PartCount(4) == 7);
