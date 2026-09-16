#pragma once
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Application/Interfaces/IScene.h"
#include "../../Engine/UI/Button.h"
#include "../../Engine/UI/Slider.h"
#include <memory>
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
#include "../../Domain/ValueObjects/CooperativeInput.h"
#endif

class SideScrollingShooter;

/**
 * @brief 横スクロールシューティングのゲームプレイシーン
 */
class TestStage : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief テストステージを生成する */
    TestStage();
    /** @brief テストステージを破棄する */
    ~TestStage() override;

    /** @brief テストステージを初期化する */
    void Initialize() override;
    /** @brief テストステージの入力を処理する */
    void ProcessInput() override;
    /** @brief テストステージを更新する */
    void Tick() override;
    /** @brief テストステージのリソースを解放する */
    void Dispose() override;
    /**
     * @brief テストステージを描画する
     * @param renderer 描画コマンドを記録するRenderer
     */
    void Render(Renderer& renderer) override;

private:
    /** @brief 2人用の両コントローラーが接続中か取得する @return 開始時の台数を満たすならtrue */
    bool ControllersConnected() const;

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
#if defined(SPACEYAKUZA_EDITION_Steam) || defined(SPACEYAKUZA_EDITION_Online)
    CooperativeInput m_networkInput {};
    bool m_waitingForPeer = false;
    bool m_peerPaused = false;
#endif
};

