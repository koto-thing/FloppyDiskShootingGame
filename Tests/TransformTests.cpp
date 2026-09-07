#include "../Engine/Scene/Transform.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
/** @brief テスト条件を検証する */
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
/** @brief 浮動小数点値を検証する */
void RequireNear(float actual, float expected, const char* message) {
    if (std::abs(actual - expected) > 0.0001f) throw std::runtime_error(message);
}
}

/** @brief Transformの親子階層とキャッシュを検証する */
void RunTransformTests() {
    // 親子関係によるワールド座標を検証する
    try {
        Transform parent;
        Transform child;
        parent.SetPosition({10.0f, 0.0f, 0.0f});
        child.SetLocalPosition({2.0f, 0.0f, 0.0f});
        Require(child.SetParent(&parent, false), "Parent assignment must succeed");
        RequireNear(child.Position().x, 12.0f, "Child position must include parent");
        const Matrix4x4& first = child.WorldMatrix();
        const Matrix4x4& second = child.WorldMatrix();
        Require(&first == &second, "World matrix must be cached");
        parent.SetPosition({20.0f, 0.0f, 0.0f});
        RequireNear(child.Position().x, 22.0f, "Dirty state must propagate to children");

        // 親変更時のワールド姿勢維持を検証する
        Transform otherParent;
        otherParent.SetPosition({-5.0f, 0.0f, 0.0f});
        Require(child.SetParent(&otherParent), "Parent change must succeed");
        RequireNear(child.Position().x, 22.0f, "World position must be preserved");
        RequireNear(child.LocalPosition().x, 27.0f, "Local position must be recalculated");
        // 循環する親子関係を拒否することを検証する
        Transform cycleRoot;
        Transform cycleChild;
        Require(cycleChild.SetParent(&cycleRoot, false), "Cycle test setup must succeed");
        Require(!cycleRoot.SetParent(&cycleChild), "Circular parent assignment must fail");
    } catch (const std::exception& exception) {
        std::cerr << "TransformTests failed: " << exception.what() << '\n';
        throw;
    }
}
