#pragma once
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Engine/UI/Button.h"
#include "../../Engine/UI/Slider.h"

class OptionScene : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief オプション画面を初期化する */
    void Initialize() override;
    /** @brief オプション画面への入力を処理する */
    void ProcessInput() override;
    /** @brief オプション画面の状態を更新する */
    void Tick() override;
    /** @brief オプション画面のリソースを解放する */
    void Dispose() override;
    /** @brief オプション画面を描画する */
    void Render(Renderer& renderer) override;
    
private:
    std::unique_ptr<Button> m_backToTitleButton;
    std::unique_ptr<Button> m_retroEffectButton;
    Slider m_masterVolumeSlider;
    Slider m_bgmVolumeSlider;
    Slider m_seVolumeSlider;
    bool m_retroEffectEnabled = true;
};
