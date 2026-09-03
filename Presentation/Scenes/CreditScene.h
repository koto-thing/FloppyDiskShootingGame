#pragma once

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/Entities/GameObject.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"
#include "../CreditScene/CreditContentController.h"
#include "../CreditScene/CreditContentPresenter.h"

#include <array>
#include <cstddef>
#include <memory>

/**
 * @brief クレジットを下から上へスクロール表示するシーン
 */
class CreditScene : public IScene<SceneType, SceneSharedData> {
public:
    CreditScene() = default;
    ~CreditScene() override;

    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;

private:
    /** @brief クレジット画面を落下するクリック可能な隕石 */
    struct Meteor {
        Vector2 position {};
        Vector2 velocity {};
        float radius = 0.1f;
        unsigned int shrinkLevel = 0;
    };

    /**
     * @brief 指定した隕石を画面上部へ再配置する
     * @param index 再配置する隕石の添字
     * @param initial trueの場合は初期表示用に落下開始位置をずらす
     */
    void ResetMeteor(std::size_t index, bool initial);

    /** @brief 全隕石の落下と跳ね返りを更新する */
    void UpdateMeteors();

    /**
     * @brief 指定位置にある隕石を小さくして上へ跳ね返す
     * @param position クリックしたNDC座標
     */
    void HitMeteor(const Vector2& position);

    /**
     * @brief 全隕石を描画する
     * @param renderer 描画コマンドを記録するRenderer
     */
    void RenderMeteors(Renderer& renderer) const;

    std::unique_ptr<GameObject> m_creditObject;
    std::shared_ptr<CreditContentController> m_creditController;
    CreditContentPresenter m_creditPresenter;
    std::unique_ptr<Button> m_backButton;
    std::array<Meteor, 3> m_meteors {};
    unsigned int m_meteorRandomState = 0x4D455445u;
};
