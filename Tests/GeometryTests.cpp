#include "../Engine/Geometry/Circle.h"
#include "../Engine/Geometry/Rect.h"
#include "../Engine/Graphics/Color.h"

#include <cmath>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void RequireNear(float actual, float expected, const char* message) {
    if (std::abs(actual - expected) > 0.0001f) throw std::runtime_error(message);
}
}

void RunGeometryTests() {
    const ColorF white = ColorF::White();
    Require(white.r == 1.0f && white.g == 1.0f && white.b == 1.0f && white.a == 1.0f,
            "White color must be opaque white");
    Require(ColorF::Transparent().a == 0.0f, "Transparent color must have zero alpha");

    const Circle circle{{0.0f, 0.0f}, 5.0f};
    Require(circle.Contains({3.0f, 4.0f}), "Circle must contain its boundary");
    Require(!circle.Contains({5.1f, 0.0f}), "Circle must exclude points outside");
    Require(circle.Intersects(Circle{{10.0f, 0.0f}, 5.0f}), "Touching circles must intersect");
    Require(!circle.Intersects(Circle{{10.1f, 0.0f}, 5.0f}), "Separated circles must not intersect");

    const Rect rect{{-2.0f, -1.0f}, {4.0f, 2.0f}};
    RequireNear(rect.Center().x, 0.0f, "Rect center X must be correct");
    RequireNear(rect.Center().y, 0.0f, "Rect center Y must be correct");
    Require(rect.Contains({2.0f, 1.0f}), "Rect must contain its boundary");
    Require(rect.Intersects(Rect{{2.0f, 1.0f}, {3.0f, 2.0f}}), "Touching rectangles must intersect");
    Require(!rect.Intersects(Rect{{2.1f, 1.1f}, {3.0f, 2.0f}}), "Separated rectangles must not intersect");
    Require(circle.Intersects(rect) == rect.Intersects(circle), "Circle-rect result must be symmetric");
    Require(circle.Intersects(rect), "Circle overlapping rectangle must intersect");
    Require(Rect{{2.0f, 2.0f}, {-4.0f, -2.0f}}.Contains({0.0f, 0.0f}),
            "Negative rectangle sizes must be normalized");
    Require(Circle{{0.0f, 0.0f}, -5.0f}.Contains({3.0f, 4.0f}),
            "Negative circle radius must use its absolute value");
}
