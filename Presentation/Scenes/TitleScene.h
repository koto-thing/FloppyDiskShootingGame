#pragma once

#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Application/Interfaces/IScene.h"
#include "../../Engine/UI/Button.h"

#include <memory>

/**
 * @brief タイトル画面のシーンクラス
 * @details IScene インターフェースを実装し、タイトル画面における入力処理、更新処理、描画処理を制御します。
 */
class TitleScene : public IScene<SceneType, SceneSharedData> {
public:
    /**
     * @brief タイトルシーンの初期化処理を行います。
     */
    void Initialize() override;

    /**
     * @brief タイトルシーンでのユーザー入力処理を行います。
     */
    void ProcessInput() override;

    /**
     * @brief タイトルシーンの状態更新処理を行います。
     */
    void Tick() override;
    void Dispose() override;

    /**
     * @brief タイトルシーンの描画処理を行います。
     * @param renderer Rendererの参照
     */
    void Render(Renderer& renderer) override;

private:
    std::unique_ptr<Button> m_startButton;
    std::unique_ptr<Button> m_modelTestButton;
    std::unique_ptr<Button> m_optionButton;
    std::unique_ptr<Button> m_exitButton;
    std::unique_ptr<Button> m_creditButton;
};
