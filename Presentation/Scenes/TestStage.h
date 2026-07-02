#pragma once
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Application/Interfaces/IScene.h"
#include <memory>

class ShootingGame;

/**
 * @brief テスト用のゲームプレイシーン (東方風シューティングゲーム)
 */
class TestStage : public IScene<SceneType, SceneSharedData> {
public:
    TestStage();
    ~TestStage() override;

    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Render(D3D12Renderer& renderer) override;

private:
    //std::unique_ptr<ShootingGame> m_game;
};
