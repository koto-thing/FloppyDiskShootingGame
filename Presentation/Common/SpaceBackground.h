#pragma once

class Renderer;

namespace SpaceBackground {
inline constexpr int StarCount = 96;

/**
 * @brief UIシーン共通の星空背景を描画する
 * @param renderer 描画コマンドを記録するRenderer
 * @param elapsedTime 背景アニメーションの経過秒数
 * @return なし
 */
void Render(Renderer& renderer, float elapsedTime);
}
