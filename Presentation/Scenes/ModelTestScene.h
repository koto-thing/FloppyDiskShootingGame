#pragma once

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/Graphics/Camera3D.h"

/**
 * @brief 3Dモデルの外形を確認するためのテストシーン
 */
class ModelTestScene : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief テストシーンを初期化する */
    void Initialize() override;
    /** @brief テストシーンの入力を処理する */
    void ProcessInput() override;
    /** @brief 円柱の回転状態を更新する */
    void Tick() override;
    /** @brief テストモデルと操作説明を描画する */
    void Render(Renderer& renderer) override;

private:
    Camera3D m_camera;
    float m_rotationAngle = 0.0f;
};
