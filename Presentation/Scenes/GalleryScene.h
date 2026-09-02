#pragma once

#include <cstdint>
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/Graphics/Camera3D.h"

class Button;

/** @brief ゲーム中に解放した3Dモデルと動作を鑑賞するシーン */
class GalleryScene final : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief 保存済み解放状態と鑑賞カメラを初期化する */
    void Initialize() override;
    /** @brief 展示選択、カメラ、アニメーション操作を受け付ける */
    void ProcessInput() override;
    /** @brief 再生中アニメーションの時間を更新する */
    void Tick() override;
    /** @brief ギャラリーが保持するUIを解放する */
    void Dispose() override;
    /**
     * @brief 選択中の展示モデルと説明UIを描画する
     * @param renderer 描画先Renderer
     */
    void Render(Renderer& renderer) override;

private:
    /** @brief 選択中の展示に合わせてアニメーションと視点を初期化する */
    void ResetExhibit();
    /**
     * @brief 選択中の展示が解放済みか判定する
     * @return 解放済みの場合true
     */
    bool IsUnlocked() const;
    /**
     * @brief 選択中展示のモデルを描画する
     * @param renderer 描画先Renderer
     */
    void RenderExhibit(Renderer& renderer) const;
    /**
     * @brief 展示名、説明、解放状態、操作案内を描画する
     * @param renderer 描画先Renderer
     */
    void RenderUi(Renderer& renderer) const;

    Camera3D m_camera;
    std::unique_ptr<Button> m_returnButton;
    std::uint32_t m_galleryUnlocks = 0;
    int m_exhibitIndex = 0;
    int m_animationIndex = 0;
    float m_animationTime = 0.0f;
    float m_orbitYaw = -0.65f;
    float m_orbitPitch = 0.22f;
    float m_zoom = 1.0f;
    bool m_playing = true;
};
