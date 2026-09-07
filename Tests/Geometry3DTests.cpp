#include "../Engine/Geometry/Sphere.h"
#include "../Engine/Geometry/Box.h"
#include "../Engine/Geometry/Ray.h"
#include <cmath>
#include <stdexcept>
namespace {
/** @brief テスト条件を検証する */
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
/** @brief 3D形状とレイの交差判定を検証する */
void RunGeometry3DTests() {
    // 球の内部判定と交差判定を検証する
    Sphere a{{0,0,0}, 1}, b{{2,0,0}, 1}; Require(a.Intersects(b), "Sphere touching failed"); Require(a.Contains({1,0,0}), "Sphere boundary failed");
    // 境界箱の判定を検証する
    Box box{{0,0,2}, {2,2,2}}; Require(a.Intersects(box), "Sphere box failed"); Require(box.Contains({1,1,3}), "Box boundary failed"); Require(box.Intersects(Box{{2,0,2},{2,2,2}}), "Box touching failed");
    // レイの交差位置を検証する
    Ray ray{{0,0,0}, {0,0,1}}; RayHit hit; Require(ray.Intersects(Sphere{{0,0,5},1}, &hit), "Ray sphere failed"); Require(std::abs(hit.distance - 4.0f) < 0.001f, "Ray sphere distance failed"); Require(ray.Intersects(box, &hit), "Ray box failed"); Require(hit.distance >= 1.0f, "Ray box distance failed");
}
