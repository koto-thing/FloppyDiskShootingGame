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
    int soundCount = 0;
    Button::ClickSound playedSound = Button::ClickSound::Cancel;
    Button::SetClickSoundHandler([&](Button::ClickSound sound) {
        ++soundCount;
        playedSound = sound;
    });
    button.SetOnClick([&clickCount] { ++clickCount; });
    button.Update({{0.0f, 0.0f}, true, true, false});
    Require(button.IsHovered() && button.IsPressed(), "Button must become pressed after a pointer press inside bounds");
    button.Update({{0.0f, 0.0f}, false, false, true});
    Require(clickCount == 1 && !button.IsPressed(), "Button must click only when released inside bounds");
    Require(soundCount == 1 && playedSound == Button::ClickSound::Confirm,
            "Button must play the confirm sound when clicked by default");
    button.Update({{0.0f, 0.0f}, true, true, false});
    button.Update({{0.8f, 0.0f}, false, false, true});
    Require(clickCount == 1, "Button must not click when released outside bounds");
    Require(soundCount == 1, "Button must not play a sound when released outside bounds");
    button.SetClickSound(Button::ClickSound::Cancel);
    button.Update({{0.0f, 0.0f}, true, true, false});
    button.Update({{0.0f, 0.0f}, false, false, true});
    Require(soundCount == 2 && playedSound == Button::ClickSound::Cancel,
            "Button must play its configured cancel sound");
    Button::SetClickSoundHandler({});

    // 解放イベントなしで入力が消えた押下は後続の解放でもクリックしない
    button.Update({{0.0f, 0.0f}, true, true, false});
    button.Update({{0.0f, 0.0f}, false, false, false});
    Require(!button.IsPressed(), "Button must cancel a press when held input disappears");
    button.Update({{0.0f, 0.0f}, false, false, true});
    Require(clickCount == 2, "Cancelled button press must not click on a later release");

    Renderer renderer;
    renderer.BeginFrame();
    button.Render(renderer);
    RequireNear(renderer.Command(1).characterSpacing, 0.002f,
                "Button text must use slightly expanded character spacing by default");
    RequireNear(renderer.Command(1).position.x, -0.04f,
                "Button text must remain centered when character spacing is applied");

    Slider slider{{{-0.5f, -0.1f}, {1.0f, 0.2f}}, 10.0f, 20.0f, 10.0f};
    int changeCount = 0;
    slider.SetOnValueChanged([&changeCount](float) { ++changeCount; });
    slider.Update({{0.0f, 0.0f}, true, true, false});
    RequireNear(slider.Value(), 15.0f, "Slider click must map its center to the middle of the range");
    slider.Update({{1.0f, 0.0f}, true, false, false});
    RequireNear(slider.Value(), 20.0f, "Slider dragging past its end must clamp to maximum");
    slider.Update({{1.0f, 0.0f}, false, false, true});
    Require(!slider.IsDragging() && changeCount == 2, "Slider must finish dragging and notify only when values change");

    // 解放イベントなしで入力が消えたドラッグもその場で終了する
    slider.Update({{0.0f, 0.0f}, true, true, false});
    slider.Update({{0.0f, 0.0f}, false, false, false});
    Require(!slider.IsDragging(), "Slider must cancel dragging when held input disappears");
    slider.SetRange(5.0f, -5.0f);
    RequireNear(slider.Minimum(), -5.0f, "Slider range must normalize its minimum");
    RequireNear(slider.Maximum(), 5.0f, "Slider range must normalize its maximum");
}
