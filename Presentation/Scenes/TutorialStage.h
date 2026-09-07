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
    /** @brief チュートリアルステージを生成する */
    TutorialStage();
    /** @brief チュートリアルステージを破棄する */
    ~TutorialStage() override;
    /** @brief チュートリアルステージを初期化する */
    void Initialize() override;
    /** @brief チュートリアルステージの入力を処理する */
    void ProcessInput() override;
    /** @brief チュートリアルステージを更新する */
    void Tick() override;
    /** @brief チュートリアルステージのリソースを解放する */
    void Dispose() override;
    /**
     * @brief チュートリアルステージを描画する
     * @param renderer 描画コマンドを記録するRenderer
     */
    void Render(Renderer& renderer) override;

private:
    /** @brief 初回チュートリアル完了を保存してStage1へ進む */
    void FinishTutorial();

    std::unique_ptr<SideScrollingShooter> m_game;
    std::unique_ptr<Button> m_skipButton;
    std::unique_ptr<Button> m_nextButton;
};
