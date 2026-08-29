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
    m_camera.SetViewport({0, 0, renderer.Width(), renderer.Height()});
    /** @brief 奥側の面を除外する3Dモデル用パイプラインを選択する */
    renderer.SetPipeline(PipelineId::Model3D);
    renderer.SetCamera(m_camera);

    /** @brief 床面を描画する */
    const Matrix4x4 floorWorld = Matrix4x4::Translation({0.0f, -1.5f, 0.0f}) *
        Matrix4x4::Scale({4.0f, 1.0f, 4.0f});
    renderer.Draw({
        PrimitiveShape::Plate,
        m_camera.ProjectionMatrix() * m_camera.ViewMatrix() * floorWorld,
        Vector3::One,
        {0.12f, 0.16f, 0.22f, 1.0f}
    });

    /** @brief モデルを描画する */
// --- 共通パラメータの設定 ---
    const Vector3 player = { 0.0f, 0.0f, 0.0f }; // 機体中心
    const float rollFactor = 0.0f;              // ロール係数（必要に応じて変更）
    const Matrix4x4 viewProj = m_camera.ProjectionMatrix() * m_camera.ViewMatrix();
    const Matrix4x4 baseRotation = Matrix4x4::RotationY(m_rotationAngle);

    // カラー定義
    const ColorF noseCol = { 0.90f, 0.20f, 0.20f, 1.0f }; // 赤系（ノーズ）
    const ColorF pCol = { 0.80f, 0.80f, 0.80f, 1.0f }; // 白・グレー系（メイン）
    const ColorF rearCol = { 0.60f, 0.60f, 0.65f, 1.0f }; // ダークグレー系（リア）
    const ColorF wingCol = { 0.20f, 0.50f, 0.90f, 1.0f }; // 青系（主翼）
    const ColorF tailCol = { 0.90f, 0.70f, 0.10f, 1.0f }; // 黄系（尾翼）

    // 描画用ローカルラムダ（コード重複を減らし規則を統一）
    auto DrawPart = [&](PrimitiveShape shape, const Vector3& pos, const Vector3& scale, const Vector3& rot, const ColorF& color) {
        // 位置移動とスケールを設定し、全体回転(Y軸)を適用
        const Matrix4x4 partRotation = Matrix4x4::RotationY(rot.y) * Matrix4x4::RotationX(rot.x) * Matrix4x4::RotationZ(rot.z);
        const Matrix4x4 localWorld = Matrix4x4::Translation(pos) * partRotation * Matrix4x4::Scale(scale);
        const Matrix4x4 world = baseRotation * localWorld;

        renderer.Draw({
            shape,
            viewProj * world,
            Vector3::One,
            color,
            m_rotationAngle
            });
        };

    // A. 機首ノーズ
    DrawPart(
        PrimitiveShape::Cone,
        { player.x, player.y, player.z},
        { 1.6f, 1.6f, 4.0f },
        { 90.0f, 0.0f, 0.0f },
        noseCol
    );

    /** @brief 3D描画後にUI座標系へ戻して操作説明を重ねる */
    renderer.ResetCamera();
    renderer.DrawText("3D MODEL TEST", TextAlign::TopCenter, 0.035f, ColorF::White(), {0.0f, -0.08f});
    renderer.DrawText("CYLINDER - AUTO ROTATE", TextAlign::BottomCenter, 0.018f,
                      {0.70f, 0.82f, 0.95f, 1.0f}, {0.0f, 0.12f});
    renderer.DrawText("ESC: BACK TO TITLE", TextAlign::BottomCenter, 0.014f,
                      {0.70f, 0.70f, 0.70f, 1.0f}, {0.0f, 0.05f});
}

void drawModel(float m_rotationAngle, Renderer renderer, Camera3D m_camera)
{
    const Matrix4x4 cylinderWorld = Matrix4x4::RotationY(m_rotationAngle) *
        Matrix4x4::Scale({ 2.0f, 3.0f, 2.0f });
    renderer.Draw({
        PrimitiveShape::Cylinder,
        m_camera.ProjectionMatrix() * m_camera.ViewMatrix() * cylinderWorld,
        Vector3::One,
        {0.30f, 0.78f, 1.0f, 1.0f},
        m_rotationAngle
        });
}
