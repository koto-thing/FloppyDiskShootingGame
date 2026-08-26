#pragma once
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Application/Interfaces/IScene.h"
#include <memory>

class SideScrollingShooter;

/**
 * @brief 横スクロールシューティングのゲームプレイシーン
 */
class TestStage : public IScene<SceneType, SceneSharedData> {
public:
    TestStage();
    ~TestStage() override;

    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;

private:
    std::unique_ptr<SideScrollingShooter> m_game;
};

