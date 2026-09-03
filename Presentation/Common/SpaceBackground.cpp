#include "SpaceBackground.h"

#include <cmath>

#include "../../Engine/Graphics/Renderer.h"

namespace {
constexpr float FieldHalfSize = 1.04f;
constexpr float FieldSize = FieldHalfSize * 2.0f;
constexpr ColorF BackgroundColor {0.004f, 0.008f, 0.035f, 1.0f};
constexpr ColorF StarColor {0.72f, 0.80f, 1.0f, 1.0f};
}

/**
 * @brief UIシーン共通の星空背景を描画する
 * @param renderer 描画コマンドを記録するRenderer
 * @param elapsedTime 背景アニメーションの経過秒数
 * @return なし
 */
void SpaceBackground::Render(Renderer& renderer, float elapsedTime) {
    // 画面全体を深い宇宙色で塗りつぶす
    renderer.Draw(Rect {{0.0f, 0.0f}, {2.0f, 2.0f}}, BackgroundColor);

    // 決定的に分散した星をゆっくり流しながら個別の周期で明滅させる
    for (int index = 0; index < StarCount; ++index) {
        const unsigned int seed = static_cast<unsigned int>(index) * 22695477u + 1u;
        const float x = -FieldHalfSize +
            static_cast<float>(seed % 997u) / 997.0f * FieldSize;
        const float initialY =
            static_cast<float>((seed / 997u) % 991u) / 991.0f * FieldSize;
        const float speed = 0.006f + static_cast<float>(index % 4) * 0.003f;
        float wrappedY = std::fmod(initialY + elapsedTime * speed, FieldSize);
        if (wrappedY < 0.0f) wrappedY += FieldSize;

        const float twinkle = 0.58f + 0.32f *
            std::sin(elapsedTime * (0.45f + static_cast<float>(index % 5) * 0.08f) +
                static_cast<float>(seed % 31u));
        const float size = index % 17 == 0 ? 0.008f : (index % 7 == 0 ? 0.005f : 0.003f);
        renderer.Draw(
            Rect {{x, wrappedY - FieldHalfSize}, {size, size}},
            {StarColor.r * twinkle, StarColor.g * twinkle,
             StarColor.b * twinkle, 0.55f + twinkle * 0.40f});
    }
}
