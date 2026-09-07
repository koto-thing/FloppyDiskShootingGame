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
    /** @brief ランキングシーンを初期化する */
    void Initialize() override;
    /** @brief ランキングシーンの入力を処理する */
    void ProcessInput() override;
    /** @brief ランキングシーンを更新する */
    void Tick() override;
    /** @brief ランキングシーンのリソースを解放する */
    void Dispose() override;
    /**
     * @brief ランキングシーンを描画する
     * @param renderer 描画コマンドを記録するRenderer
     */
    void Render(Renderer& renderer) override;

private:
    ScoreRepository::Rankings m_rankings {};
    std::unique_ptr<Button> m_returnButton;
};
