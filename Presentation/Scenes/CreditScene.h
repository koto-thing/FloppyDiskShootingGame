#pragma once

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/Entities/GameObject.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"
#include "../CreditScene/CreditContentController.h"
#include "../CreditScene/CreditContentPresenter.h"

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
    std::unique_ptr<GameObject> m_creditObject;
    std::shared_ptr<CreditContentController> m_creditController;
    CreditContentPresenter m_creditPresenter;
    std::unique_ptr<Button> m_backButton;
};
