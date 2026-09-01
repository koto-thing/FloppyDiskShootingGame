#pragma once

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/Graphics/Camera3D.h"
#include "../../Engine/Graphics/Renderer.h"

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

    //敵機とボスの描画用
    void drawEnemy0(Renderer& renderer);
    void drawEnemy1(Renderer& renderer);
    void drawEnemy2(Renderer& renderer);
    void drawEnemy3(Renderer& renderer);
    void drawEnemy4(Renderer& renderer);
    void drawBoss0(Renderer& renderer);
    void drawBoss1(Renderer& renderer);
    void drawBoss2(Renderer& renderer);
    void drawBoss3(Renderer& renderer);
    void drawBoss4(Renderer& renderer);

    //パーツごとの描画
    void DrawPart(
        Renderer& renderer,
        PrimitiveShape shape,
        const Vector3& position,//左右、高さ、前後
        const Vector3& scale,
        const Vector3& rotation,
        const ColorF& color
    );
};
