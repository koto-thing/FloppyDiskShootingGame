#pragma once

#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"

class ModeSelectionScene : public IScene<SceneType, SceneSharedData> {
public:
    ModeSelectionScene() = default;
    ~ModeSelectionScene() override;
    
    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;
    
private:
    std::shared_ptr<ModeSelectionStateController> m_stateController;
};
