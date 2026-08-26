#include "../Engine/Scene/Transform.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void RequireNear(float actual, float expected, const char* message) {
    if (std::abs(actual - expected) > 0.0001f) throw std::runtime_error(message);
}
}

void RunTransformTests() {
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

        Transform otherParent;
        otherParent.SetPosition({-5.0f, 0.0f, 0.0f});
        Require(child.SetParent(&otherParent), "Parent change must succeed");
        RequireNear(child.Position().x, 22.0f, "World position must be preserved");
        RequireNear(child.LocalPosition().x, 27.0f, "Local position must be recalculated");
        Transform cycleRoot;
        Transform cycleChild;
        Require(cycleChild.SetParent(&cycleRoot, false), "Cycle test setup must succeed");
        Require(!cycleRoot.SetParent(&cycleChild), "Circular parent assignment must fail");
    } catch (const std::exception& exception) {
        std::cerr << "TransformTests failed: " << exception.what() << '\n';
        throw;
    }
}
