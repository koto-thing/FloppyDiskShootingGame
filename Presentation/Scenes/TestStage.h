#pragma once
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Application/Interfaces/IScene.h"
#include "../../Engine/UI/Button.h"
#include "../../Engine/UI/Slider.h"
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
    /** @brief ポーズメニュー用のUIを初期化する */
    void InitializePauseMenu();
    /** @brief ポーズメニュー用のUI入力を処理する */
    void ProcessPauseMenuInput();
    /** @brief ポーズメニューまたはオプション画面を描画する */
    void RenderPauseMenu(Renderer& renderer) const;

    std::unique_ptr<SideScrollingShooter> m_game;
    std::unique_ptr<Button> m_returnToTitleButton;
    std::unique_ptr<Button> m_openOptionsButton;
    std::unique_ptr<Button> m_closeMenuButton;
    std::unique_ptr<Button> m_backToMenuButton;
    std::unique_ptr<Button> m_retroEffectButton;
    Slider m_masterVolumeSlider;
    Slider m_bgmVolumeSlider;
    Slider m_seVolumeSlider;
    bool m_retroEffectEnabled = true;
    bool m_pauseMenuOpen = false;
    bool m_optionsOpen = false;
    int m_allClearTimer = 0;
};

