#include "ModelTestScene.h"
#include "../Gameplay/BossModelView.h"

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Time/Time.h"

/**
 * @brief テスト用カメラを円柱全体が見える位置へ設定する
 */
void ModelTestScene::Initialize() {
    /** @brief 円柱の上面と側面を確認できる視点を設定する */
    m_camera.SetPosition({12.0f, 8.0f, -18.0f});
    m_camera.LookAt({0.0f, 1.5f, 0.0f});
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
 * @brief 外形を全方向から確認できるよう円柱を自動回転させる
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
        Matrix4x4::Translation({0.0f, -2.0f, 0.0f}) *
        Matrix4x4::Scale({12.0f, 1.0f, 8.0f});

    renderer.Draw({
        PrimitiveShape::Plate,
        viewProj * floorWorld,
        Vector3::One,
        {0.12f, 0.16f, 0.22f, 1.0f}
    });

    // BossModelView.h 用描画アダプタ
    auto drawBossPart =
        [&](int shape,
            const Vector3& position,
            const Vector3& scale,
            const float color[4],
            float yaw) {

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
            Matrix4x4::Scale(scale);

        renderer.Draw({
            primitiveShape,
            viewProj * world,
            Vector3::One,
            partColor,
            yaw
        });
    };

    // -------------------------
    // 下部：砂中潜航艦
    // -------------------------

    BossModelTransform submarineTransform;

    submarineTransform.position = {
        0.0f,
        0.0f,
        0.0f
    };

    submarineTransform.yaw = m_rotationAngle;
    submarineTransform.scale = 1.0f;

    SandSubmarineView::Draw(
        submarineTransform,
        drawBossPart
    );

    // -------------------------
    // 上部：陸上戦艦
    // -------------------------

    BossModelTransform battleshipTransform;

    battleshipTransform.position = {
        0.0f,
        1.55f,
        0.0f
    };

    battleshipTransform.yaw = m_rotationAngle;
    battleshipTransform.scale = 1.0f;

    LandBattleshipView::Draw(
        battleshipTransform,
        drawBossPart
    );

    // UI描画へ戻す
    renderer.ResetCamera();

    renderer.DrawText(
        "BOSS MODEL TEST",
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
