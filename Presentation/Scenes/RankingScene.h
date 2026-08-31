#pragma once

#include <array>
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Infrastructure/Repositories/ScoreRepository.h"

class Button;

/** @brief 難易度別の上位5件スコアを表示するシーン */
class RankingScene final : public IScene<SceneType, SceneSharedData> {
public:
    void Initialize() override;
    void ProcessInput() override;
    void Tick() override;
    void Dispose() override;
    void Render(Renderer& renderer) override;

private:
    ScoreRepository::Rankings m_rankings {};
    std::unique_ptr<Button> m_returnButton;
};
