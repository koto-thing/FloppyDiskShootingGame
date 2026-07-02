#pragma once
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Application/Interfaces/IScene.h"

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

    /**
     * @brief タイトルシーンの描画処理を行います。
     * @param renderer DirectX 12 レンダラーの参照
     */
    void Render(D3D12RenderingService& renderer) override;
};
