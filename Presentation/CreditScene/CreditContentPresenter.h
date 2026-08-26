#pragma once

class CreditSceneContent;
class GameObject;
class Renderer;

/**
 * @brief クレジット内容をRendererへ描画するプレゼンター
 */
class CreditContentPresenter {
public:
    /**
     * @brief クレジットをGameObjectの現在位置で描画する
     * @param renderer 描画コマンドを記録するRenderer
     * @param creditObject スクロール位置を保持するGameObject
     * @param content 描画するクレジット内容
     */
    void Render(Renderer& renderer, const GameObject& creditObject, const CreditSceneContent& content) const;
};
