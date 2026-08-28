#pragma once
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Engine/UI/Button.h"
#include "../../Engine/UI/Slider.h"

class OptionScene : public IScene<SceneType, SceneSharedData> {
public:
    void Initialize() override;
    
    void ProcessInput() override;
    
    void Tick() override;
    
    void Dispose() override;
    
    void Render(Renderer& renderer) override;
    
private:
    std::unique_ptr<Button> m_backToTitleButton;
    Slider m_masterVolumeSlider;
    Slider m_bgmVolumeSlider;
    Slider m_seVolumeSlider;
};
