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
    ModeSelectionScene() = default;
    ~ModeSelectionScene() override = default;
    
    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;
    
private:
    /** @brief 現在表示中のボタンへ入力を渡す */
    void UpdateActiveButtons(const UIInputState& inputState);

    std::unique_ptr<ModeSelectionStateController> m_stateController;
    std::array<std::unique_ptr<Button>, 3> m_difficultyButtons;
    std::array<std::unique_ptr<Button>, 3> m_playerTypeButtons;
    std::unique_ptr<Button> m_backButton;
};
