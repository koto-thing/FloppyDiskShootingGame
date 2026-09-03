#pragma once

#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"

class SideScrollingShooter;

/** @brief 初回プレイ時のインタラクティブチュートリアルシーン */
class TutorialStage final : public IScene<SceneType, SceneSharedData> {
public:
    TutorialStage();
    ~TutorialStage() override;
    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;

private:
    /** @brief 初回チュートリアル完了を保存してStage1へ進む */
    void FinishTutorial();

    std::unique_ptr<SideScrollingShooter> m_game;
    std::unique_ptr<Button> m_skipButton;
    std::unique_ptr<Button> m_nextButton;
};
