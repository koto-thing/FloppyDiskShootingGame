#include "ModelTestScene.h"
#include "../Gameplay/Stages/Stage4/Stage4BossModelView.h"

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Time/Time.h"

/**
 * @brief テスト用カメラをStage4ボス全体が見える位置へ設定する
 */
void ModelTestScene::Initialize() {
    // 長い車体の上面と側面を同時に確認できる斜め視点を設定する
    m_camera.SetPosition({22.0f, 13.0f, -36.0f});
    m_camera.LookAt({0.0f, 1.6f, 0.0f});
    m_camera.SetNearClip(0.1f);
    m_camera.SetFarClip(100.0f);
}

/**
 * @brief Escapeキーでタイトルシーンへ戻る
 */
void ModelTestScene::ProcessInput() {
    /** @brief テスト終了用の遷移入力を受け付ける */
    if (Input::GetKeyDown(KeyCode::Escape)) {
        changeScene(SceneType::Title);
    }
}

/**
 * @brief 正面、横、斜めを確認できるようStage4ボスを自動回転させる
 */
void ModelTestScene::Tick() {
    /** @brief 固定タイムステップに合わせてY軸の回転角度を進める */
    m_rotationAngle += Time::fixedDeltaTime * 0.8f;
    if (m_rotationAngle >= Math::Pi * 2.0f) {
        m_rotationAngle -= Math::Pi * 2.0f;
    }
}

/**
 * @brief モデルと床面およびUIを描画する
 */
void ModelTestScene::Render(Renderer& renderer) {
    // 3D描画設定
    m_camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});

    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(m_camera);

    const Matrix4x4 viewProj =
        m_camera.ProjectionMatrix() *
        m_camera.ViewMatrix();

    // 床
    const Matrix4x4 floorWorld =
        Matrix4x4::Translation({0.0f, -1.32f, 0.0f}) *
        Matrix4x4::Scale({18.0f, 1.0f, 16.0f});

    renderer.Draw({
        PrimitiveShape::Plate,
        viewProj * floorWorld,
        Vector3::One,
        {0.12f, 0.16f, 0.22f, 1.0f}
    });

    // Stage4BossModelView用描画アダプタ
    auto drawBossPart =
        [&](int shape,
            const Vector3& position,
            const Vector3& scale,
            const float color[4],
            float yaw,
            float pitch) {

        const PrimitiveShape primitiveShape =
            static_cast<PrimitiveShape>(shape);

        const ColorF partColor {
            color[0],
            color[1],
            color[2],
            color[3]
        };

        const Matrix4x4 world =
            Matrix4x4::Translation(position) *
            Matrix4x4::RotationY(yaw) *
            Matrix4x4::RotationZ(pitch) *
            Matrix4x4::Scale(scale);

        renderer.Draw({
            primitiveShape,
            viewProj * world,
            Vector3::One,
            partColor,
            yaw
        });
    };

    // 親Transformだけを回転させて全160パーツを360度確認する
    BossModelTransform bossTransform;
    bossTransform.position = {0.0f, 0.0f, 0.0f};
    bossTransform.yaw = m_rotationAngle;
    bossTransform.scale = 1.0f;
    Stage4BossModelView::Draw(bossTransform, drawBossPart);

    // UI描画へ戻す
    renderer.ResetCamera();

    renderer.DrawText(
        "STAGE 4 BOSS MODEL TEST",
        TextAlign::TopCenter,
        0.035f,
        ColorF::White(),
        {0.0f, -0.08f}
    );

    renderer.DrawText(
        "AUTO ROTATE",
        TextAlign::BottomCenter,
        0.018f,
        {0.70f, 0.82f, 0.95f, 1.0f},
        {0.0f, 0.12f}
    );

    renderer.DrawText(
        "ESC: BACK TO TITLE",
        TextAlign::BottomCenter,
        0.014f,
        {0.70f, 0.70f, 0.70f, 1.0f},
        {0.0f, 0.05f}
    );
}
