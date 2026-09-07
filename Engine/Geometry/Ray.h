#pragma once
#include "../Math/Vector3.h"
#include "RayHit.h"
struct Sphere;
struct Box;

/** @brief 3次元空間の始点と方向を持つレイ */
struct Ray {
    Vector3 origin = Vector3::Zero;
    Vector3 direction = Vector3::Forward;
    /** @brief 原点と前方向でレイを生成する */
    Ray() = default;
    /**
     * @brief 始点と方向からレイを生成する
     * @param origin レイの始点
     * @param direction レイの方向
     */
    Ray(const Vector3& origin, const Vector3& direction);
    /** @brief 無効なレイを生成する */
    static Ray Invalid() { return {{}, {}}; }
    /**
     * @brief 始点から指定距離の位置を取得する
     * @param distance 始点からの距離
     * @return レイ上の座標
     */
    Vector3 PointAt(float distance) const { return origin + direction * distance; }
    /**
     * @brief レイと球の交差を判定する
     * @param sphere 判定対象の球
     * @param hit 交差結果の格納先
     * @return 交差した場合true
     */
    bool Intersects(const Sphere& sphere, RayHit* hit = nullptr) const;
    /**
     * @brief レイと境界箱の交差を判定する
     * @param box 判定対象の境界箱
     * @param hit 交差結果の格納先
     * @return 交差した場合true
     */
    bool Intersects(const Box& box, RayHit* hit = nullptr) const;
};
