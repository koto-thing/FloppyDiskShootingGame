#include "../Engine/UI/Button.h"
#include "../Engine/UI/Slider.h"

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

void RunUITests() {
    Button alignedButton{{0.4f, 0.2f}, RectAlign::BottomCenter, "Start", {0.1f, 0.05f}};
    RequireNear(alignedButton.Bounds().position.x, -0.1f,
                "Button alignment must place its bounds at the bottom center with its offset");
    RequireNear(alignedButton.Bounds().position.y, -0.95f,
                "Button alignment must preserve its vertical offset");

    Slider alignedSlider;
    alignedSlider.SetBounds({0.4f, 0.2f}, RectAlign::Center);
    RequireNear(alignedSlider.Bounds().position.x, -0.2f,
                "Slider alignment must place its bounds at the screen center");
    RequireNear(alignedSlider.Bounds().position.y, -0.1f,
                "Slider alignment must center its bounds vertically");

    Button button{{{-0.5f, -0.25f}, {1.0f, 0.5f}}, "Start"};
    int clickCount = 0;
    button.SetOnClick([&clickCount] { ++clickCount; });
    button.Update({{0.0f, 0.0f}, true, true, false});
    Require(button.IsHovered() && button.IsPressed(), "Button must become pressed after a pointer press inside bounds");
    button.Update({{0.0f, 0.0f}, false, false, true});
    Require(clickCount == 1 && !button.IsPressed(), "Button must click only when released inside bounds");
    button.Update({{0.0f, 0.0f}, true, true, false});
    button.Update({{0.8f, 0.0f}, false, false, true});
    Require(clickCount == 1, "Button must not click when released outside bounds");

    Slider slider{{{-0.5f, -0.1f}, {1.0f, 0.2f}}, 10.0f, 20.0f, 10.0f};
    int changeCount = 0;
    slider.SetOnValueChanged([&changeCount](float) { ++changeCount; });
    slider.Update({{0.0f, 0.0f}, true, true, false});
    RequireNear(slider.Value(), 15.0f, "Slider click must map its center to the middle of the range");
    slider.Update({{1.0f, 0.0f}, true, false, false});
    RequireNear(slider.Value(), 20.0f, "Slider dragging past its end must clamp to maximum");
    slider.Update({{1.0f, 0.0f}, false, false, true});
    Require(!slider.IsDragging() && changeCount == 2, "Slider must finish dragging and notify only when values change");
    slider.SetRange(5.0f, -5.0f);
    RequireNear(slider.Minimum(), -5.0f, "Slider range must normalize its minimum");
    RequireNear(slider.Maximum(), 5.0f, "Slider range must normalize its maximum");
}
