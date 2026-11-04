#pragma once

#include <algorithm>
#include <array>
#include <span>
#include "../../../Engine/Graphics/IRenderBackend.h"

/** @brief 描画と同じ頂点による掃引球とモデルの衝突判定 */
namespace ModelShotCollision {
namespace Vertices {
using float3 = Vector3;
// HLSLの頂点配列を直接共有し、見た目と判定の形状定義を二重管理しない
#include "../../../Shaders/ObjectShader.hlsl"
}

/** @brief 描画形状の三角形リストを取得する @param shape 描画形状 @return 単位モデルの頂点列 */
inline std::span<const Vector3> VerticesFor(PrimitiveShape shape) {
    // 球もシェーダーと同じ8緯度帯・12経度帯で一度だけ生成する
    static const auto sphere = [] {
        std::array<Vector3, 576> result {};
        constexpr int indices[] = {0, 2, 1, 0, 3, 2};
        for (int quad = 0; quad < 96; ++quad) {
            const float theta0 = Math::Pi * (quad / 12) / 8.0f;
            const float theta1 = Math::Pi * (quad / 12 + 1) / 8.0f;
            const float phi0 = Math::Pi * 2.0f * (quad % 12) / 12.0f;
            const float phi1 = Math::Pi * 2.0f * (quad % 12 + 1) / 12.0f;
            const Vector3 points[] = {
                {std::sin(theta0) * std::cos(phi0), std::cos(theta0), std::sin(theta0) * std::sin(phi0)},
                {std::sin(theta1) * std::cos(phi0), std::cos(theta1), std::sin(theta1) * std::sin(phi0)},
                {std::sin(theta1) * std::cos(phi1), std::cos(theta1), std::sin(theta1) * std::sin(phi1)},
                {std::sin(theta0) * std::cos(phi1), std::cos(theta0), std::sin(theta0) * std::sin(phi1)}
            };
            for (int i = 0; i < 6; ++i) result[quad * 6 + i] = points[indices[i]] * 0.5f;
        }
        return result;
    }();
    switch (shape) {
    case PrimitiveShape::Box: return Vertices::cubeVertices;
    case PrimitiveShape::Prism: return Vertices::prismVertices;
    case PrimitiveShape::Cylinder: return Vertices::cylinderVertices;
    case PrimitiveShape::Cone: return Vertices::coneVertices;
    case PrimitiveShape::Sphere: return sphere;
    default: return {};
    }
}

/** @brief 点と線分の距離の二乗 @param p 点 @param a 始点 @param b 終点 @return 距離の二乗 */
inline float PointSegment(const Vector3& p, const Vector3& a, const Vector3& b) {
    const Vector3 axis = b - a;
    const float length = axis.LengthSquared();
    const float t = length > 0.0f ? std::clamp(Vector3::Dot(p - a, axis) / length, 0.0f, 1.0f) : 0.0f;
    return (p - a - axis * t).LengthSquared();
}

/** @brief 二線分の距離の二乗 @param a 第一始点 @param b 第一終点 @param c 第二始点 @param d 第二終点 @return 距離の二乗 */
inline float SegmentSegment(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
    float distance = (std::min)({PointSegment(a, c, d), PointSegment(b, c, d),
        PointSegment(c, a, b), PointSegment(d, a, b)});
    const Vector3 u = b - a, v = d - c, w = a - c;
    const float uu = Vector3::Dot(u, u), vv = Vector3::Dot(v, v), uv = Vector3::Dot(u, v);
    const float uw = Vector3::Dot(u, w), vw = Vector3::Dot(v, w);
    const float denominator = uu * vv - uv * uv;
    // 平行線分と長さゼロの線分は端点の距離だけで求まる
    if (denominator > 1.0e-12f) {
        const float s = (uv * vw - vv * uw) / denominator;
        const float t = (uu * vw - uv * uw) / denominator;
        if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f)
            distance = (std::min)(distance, (w + u * s - v * t).LengthSquared());
    }
    return distance;
}

/** @brief 三角形内部への投影を判定する @param p 点 @param a 頂点A @param b 頂点B @param c 頂点C @param n 面法線 @return 投影が内部ならtrue */
inline bool InsideTriangle(const Vector3& p, const Vector3& a, const Vector3& b,
    const Vector3& c, const Vector3& n) {
    return Vector3::Dot(Vector3::Cross(b - a, p - a), n) >= -1.0e-7f &&
        Vector3::Dot(Vector3::Cross(c - b, p - b), n) >= -1.0e-7f &&
        Vector3::Dot(Vector3::Cross(a - c, p - c), n) >= -1.0e-7f;
}

/** @brief 掃引球と三角形を判定する @param start 掃引始点 @param end 掃引終点 @param radius 球半径 @param a 頂点A @param b 頂点B @param c 頂点C @return 接触時true */
inline bool HitTriangle(const Vector3& start, const Vector3& end, float radius,
    const Vector3& a, const Vector3& b, const Vector3& c) {
    const float radiusSquared = radius * radius;
    // 面内通過、辺、頂点を含め、退化した三角形でもゼロ除算しない
    if (SegmentSegment(start, end, a, b) <= radiusSquared ||
        SegmentSegment(start, end, b, c) <= radiusSquared ||
        SegmentSegment(start, end, c, a) <= radiusSquared) return true;
    const Vector3 n = Vector3::Cross(b - a, c - a);
    const float nn = n.LengthSquared();
    if (nn < 1.0e-12f) return false;
    const float first = Vector3::Dot(start - a, n), last = Vector3::Dot(end - a, n);
    if (first * first <= radiusSquared * nn && InsideTriangle(start, a, b, c, n)) return true;
    if (last * last <= radiusSquared * nn && InsideTriangle(end, a, b, c, n)) return true;
    if ((first < 0.0f) != (last < 0.0f)) {
        const Vector3 intersection = start + (end - start) * (first / (first - last));
        return InsideTriangle(intersection, a, b, c, n);
    }
    return false;
}

/** @brief 描画部品へ掃引球を当てる @param shape 描画形状 @param world モデルから判定空間への行列 @param start 掃引始点 @param end 掃引終点 @param radius 弾半径 @param flat XY輪郭で判定する場合true @return 接触時true */
inline bool Hit(PrimitiveShape shape, const Matrix4x4& world, Vector3 start,
    Vector3 end, float radius, bool flat) {
    if (flat) start.z = end.z = 0.0f;
    const auto vertices = VerticesFor(shape);
    if (vertices.empty()) return false;
    // 部品の外接箱で先に除外し、遠い弾では三角形を走査しない
    Vector3 low {1.0e30f, 1.0e30f, 1.0e30f}, high {-1.0e30f, -1.0e30f, -1.0e30f};
    for (int corner = 0; corner < 8; ++corner) {
        Vector3 p = world.TransformPoint({corner & 1 ? 0.5f : -0.5f,
            corner & 2 ? 0.5f : -0.5f, corner & 4 ? 0.5f : -0.5f});
        if (flat) p.z = 0.0f;
        low = {(std::min)(low.x, p.x), (std::min)(low.y, p.y), (std::min)(low.z, p.z)};
        high = {(std::max)(high.x, p.x), (std::max)(high.y, p.y), (std::max)(high.z, p.z)};
    }
    if ((std::max)(start.x, end.x) + radius < low.x || (std::min)(start.x, end.x) - radius > high.x ||
        (std::max)(start.y, end.y) + radius < low.y || (std::min)(start.y, end.y) - radius > high.y ||
        (std::max)(start.z, end.z) + radius < low.z || (std::min)(start.z, end.z) - radius > high.z) return false;
    const Vector3 center = world.TransformPoint({});
    bool inside = !flat;
    bool hasInterior = false;
    for (std::size_t i = 0; i < vertices.size(); i += 3) {
        Vector3 a = world.TransformPoint(vertices[i]);
        Vector3 b = world.TransformPoint(vertices[i + 1]);
        Vector3 c = world.TransformPoint(vertices[i + 2]);
        if (flat) a.z = b.z = c.z = 0.0f;
        if (HitTriangle(start, end, radius, a, b, c)) return true;
        // 閉じた凸形状の内部で生まれた弾も命中とする
        const Vector3 normal = Vector3::Cross(b - a, c - a);
        hasInterior |= std::fabs(Vector3::Dot(center - a, normal)) > 1.0e-7f;
        if (normal.LengthSquared() > 1.0e-12f &&
            Vector3::Dot(start - a, normal) * Vector3::Dot(center - a, normal) < -1.0e-7f) inside = false;
    }
    return inside && hasInterior;
}
}
