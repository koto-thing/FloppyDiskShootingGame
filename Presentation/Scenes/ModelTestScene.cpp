#include "ModelTestScene.h"

#include "../../Engine/Graphics/Renderer.h"
#include "../../Engine/Input/Input.h"
#include "../../Engine/Time/Time.h"

/**
 * @brief テスト用カメラを円柱全体が見える位置へ設定する
 */
void ModelTestScene::Initialize() {
    /** @brief 円柱の上面と側面を確認できる視点を設定する */
    m_camera.SetPosition({3.6f, 2.8f, -5.0f});
    m_camera.LookAt({0.0f, 0.0f, 0.0f});
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
    /** @brief 現在の出力解像度に追従する3Dビューポートを設定する */
    m_camera.SetViewport({ 0, 0, renderer.Width(), renderer.Height() });
    /** @brief 奥側の面を除外する3Dモデル用パイプラインを選択する */
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(m_camera);

    /** @brief 床面を描画する */
    const Matrix4x4 floorWorld = Matrix4x4::Translation({ 0.0f, -1.5f, 0.0f }) *
        Matrix4x4::Scale({ 4.0f, 1.0f, 4.0f });
    renderer.Draw({
        PrimitiveShape::Plate,
        m_camera.ProjectionMatrix() * m_camera.ViewMatrix() * floorWorld,
        Vector3::One,
        {0.12f, 0.16f, 0.22f, 1.0f}
        });

    drawEnemy0(renderer);

    /** @brief 3D描画後にUI座標系へ戻して操作説明を重ねる */
    renderer.ResetCamera();
    renderer.DrawText("3D MODEL TEST", TextAlign::TopCenter, 0.035f, ColorF::White(), { 0.0f, -0.08f });
    renderer.DrawText("CYLINDER - AUTO ROTATE", TextAlign::BottomCenter, 0.018f,
        { 0.70f, 0.82f, 0.95f, 1.0f }, { 0.0f, 0.12f });
    renderer.DrawText("ESC: BACK TO TITLE", TextAlign::BottomCenter, 0.014f,
        { 0.70f, 0.70f, 0.70f, 1.0f }, { 0.0f, 0.05f });
};

void ModelTestScene::DrawPart(Renderer& renderer, PrimitiveShape shape, const Vector3& position, const Vector3& scale, const Vector3& rotation, const ColorF& color)
{
    const Matrix4x4 viewProj =
        m_camera.ProjectionMatrix() *
        m_camera.ViewMatrix();

    // パーツごとの回転
    const Matrix4x4 partRotation =
        Matrix4x4::RotationY(rotation.y) *
        Matrix4x4::RotationX(rotation.x) *
        Matrix4x4::RotationZ(rotation.z);

    // パーツのローカル座標
    const Matrix4x4 localWorld =
        Matrix4x4::Translation(position) *
        partRotation *
        Matrix4x4::Scale(scale);

    // モデル全体の回転
    const Matrix4x4 baseRotation =
        Matrix4x4::RotationY(m_rotationAngle);

    // モデル全体の回転をパーツに適用
    const Matrix4x4 world =
        baseRotation * localWorld;

    renderer.Draw({
        shape,
        viewProj * world,
        Vector3::One,
        color,
        m_rotationAngle
    });
}

void ModelTestScene::drawEnemy0(Renderer& renderer)
{
    // 色
    const ColorF bodyColor = { 0.8f, 0.8f, 0.85f, 1.0f};
    const ColorF noseColor = { 0.9f, 0.1f, 0.1f, 1.0f};
    const ColorF wingColor = { 0.2f, 0.4f, 0.8f, 1.0f};

    // 胴体
    DrawPart(
        renderer,
        PrimitiveShape::Cylinder,
        { 0.0f, 0.0f, 0.0f },
        { 2.4f, 2.4f, 5.0f },
        { 0.0f, 0.0f, 0.0f },
        bodyColor
    );

    // 機首
    DrawPart(
        renderer,
        PrimitiveShape::Cone,
        { 0.0f, 0.0f, 4.0f },
        { 1.6f, 1.6f, 3.0f },
        { 0.0f, 0.0f, 0.0f },
        noseColor
    );

    // 左翼
    DrawPart(
        renderer,
        PrimitiveShape::Prism,
        { -4.0f, 0.0f, 0.0f },
        { 4.0f, 0.3f, 2.5f },
        { 0.0f, 0.0f, 0.0f },
        wingColor
    );

    // 右翼
    DrawPart(
        renderer,
        PrimitiveShape::Prism,
        { 4.0f, 0.0f, 0.0f },
        { 4.0f, 0.3f, 2.5f },
        { 0.0f, 0.0f, 0.0f },
        wingColor
    );
}