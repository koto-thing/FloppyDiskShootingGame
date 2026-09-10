#pragma once

#include <array>
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"
#include "../ModeSelectionScene/ModeSelectionStateController.h"

/** @brief 難易度とプレイヤー機体を順番に選択するシーン */
class ModeSelectionScene : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief モード選択シーンを生成する */
    ModeSelectionScene() = default;
    /** @brief モード選択シーンを破棄する */
    ~ModeSelectionScene() override = default;

    /** @brief モード選択シーンを初期化する */
    void Initialize() override;
    /** @brief モード選択シーンの入力を処理する */
    void ProcessInput() override;
    /** @brief モード選択シーンを更新する */
    void Tick() override;
    /** @brief モード選択シーンのリソースを解放する */
    void Dispose() override;
    /**
     * @brief モード選択シーンを描画する
     * @param renderer 描画コマンドを記録するRenderer
     */
    void Render(Renderer& renderer) override;

private:
    /** @brief 現在表示中のボタンへ入力を渡す */
    void UpdateActiveButtons(const UIInputState& inputState);

    /**
     * @brief マウスオーバー中の機体をデモ表示対象へ反映する
     * @return なし
     */
    void UpdatePlayerPreview();

    std::unique_ptr<ModeSelectionStateController> m_stateController;
    std::array<std::unique_ptr<Button>, 3> m_difficultyButtons;
    std::array<std::unique_ptr<Button>, 3> m_playerTypeButtons;
    std::unique_ptr<Button> m_backButton;
    PlayerType m_previewPlayerType = Homing;
};
