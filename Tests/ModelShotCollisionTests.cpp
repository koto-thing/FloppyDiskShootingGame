#include <cassert>
#include <cstdio>
#include "../Presentation/Gameplay/Models/ModelShotCollision.h"

/** @brief 描画頂点、端、隙間、掃引、回転の回帰チェック @return 成功時0 */
int main() {
    using namespace ModelShotCollision;
    const Matrix4x4 hull = Matrix4x4::Scale({12.0f, 2.0f, 3.0f});
    // 中央球では外れていた細長い船体の端と角へ命中する
    assert(Hit(PrimitiveShape::Box, hull, {5.9f, 0.9f, -4}, {5.9f, 0.9f, 4}, 0.02f, false));
    assert(Hit(PrimitiveShape::Box, hull, {5.9f, 0.9f, 0}, {5.9f, 0.9f, 0}, 0.02f, true));
    assert(!Hit(PrimitiveShape::Box, hull, {6.2f, 1.2f, 0}, {6.2f, 1.2f, 0}, 0.1f, true));
    assert(Hit(PrimitiveShape::Box, hull, {-8, 0, 0}, {8, 0, 0}, 0.0f, true));
    assert(Hit(PrimitiveShape::Box, hull, {}, {}, 0.0f, false));
    // 円柱・三角柱・円錐の外接箱内でも、描かれない角は命中しない
    for (auto shape : {PrimitiveShape::Cylinder, PrimitiveShape::Cone, PrimitiveShape::Sphere}) {
        assert(!Hit(shape, Matrix4x4::Identity, {0.49f, 0, 0.49f}, {0.49f, 0, 0.49f}, 0.001f, false));
        assert(Hit(shape, Matrix4x4::Identity, {0, 0, -2}, {0, 0, 2}, 0.001f, false));
    }
    assert(!Hit(PrimitiveShape::Prism, Matrix4x4::Identity,
        {0.45f, 0, 0.45f}, {0.45f, 0, 0.45f}, 0.01f, false));
    // 回転した薄い翼・ゼロ長線分・退化三角形でも安定する
    const auto wing = Matrix4x4::RotationZ(Math::HalfPi) * hull;
    assert(Hit(PrimitiveShape::Box, wing, {0, 5.8f, -3}, {0, 5.8f, 3}, 0.01f, false));
    assert(!Hit(PrimitiveShape::Box, wing, {5.8f, 0, -3}, {5.8f, 0, 3}, 0.01f, false));
    assert(HitTriangle({0, 0, 0}, {0, 0, 0}, 0, {}, {}, {}));
    assert(!HitTriangle({1, 0, 0}, {1, 0, 0}, 0, {}, {}, {}));
    assert(!Hit(PrimitiveShape::Prism, Matrix4x4::Scale({1, 0, 1}),
        {0.45f, 0, 0.45f}, {0.45f, 0, 0.45f}, 0.01f, false));
    std::puts("ModelShotCollisionTests passed");
}
