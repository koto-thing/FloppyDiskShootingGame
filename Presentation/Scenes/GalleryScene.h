#pragma once

#include <cstdint>
#include <array>
#include <memory>

#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/Graphics/Camera3D.h"
#include "../Gameplay/SideScrollingShooter.h"

class Button;

/** @brief 3Dモデルとゲーム音声を鑑賞するシーン */
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
#ifdef FLOPPY_AUDIO_TESTS
    friend void RunGalleryAudioTests();
#endif
    /** @brief カテゴリを変更し試聴を停止する @param category カテゴリ番号 @return なし */
    void SelectCategory(int category);
    /** @brief 音声を循環選択し試聴を停止する @param step 移動量 @return なし */
    void SelectAudio(int step);
    /** @brief 選択中の音声を先頭から再生する @return なし */
    void PlayAudio();
    /** @brief BGMと効果音とボイスの試聴を停止する @return なし */
    void StopAudio();
    /** @brief 選択カテゴリの音声数を取得する @return 音声数、モデル表示中は0 */
    int AudioCount() const;
    /** @brief 選択中の音声名を取得する @return 表示名、モデル表示中は空文字列 */
    const char* AudioName() const;
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

    /** @brief 曲を維持して背景ステージを循環選択する @param step 移動量 @return なし */
    void SelectBackground(int step);

    std::unique_ptr<SideScrollingShooter> m_background;
    std::array<std::unique_ptr<Button>, 2> m_backgroundButtons;
    int m_backgroundStage = 1;
    int m_backgroundFrame = 0;
    Camera3D m_camera;
    std::unique_ptr<Button> m_returnButton;
    std::array<std::unique_ptr<Button>, 4> m_categoryButtons;
    std::array<std::unique_ptr<Button>, 4> m_audioButtons;
    int m_category = 0;
    std::array<int, 4> m_audioIndices {};
    std::uint32_t m_galleryUnlocks = 0;
    int m_exhibitIndex = 0;
    int m_animationIndex = 0;
    float m_animationTime = 0.0f;
    float m_orbitYaw = -0.65f;
    float m_orbitPitch = 0.22f;
    float m_zoom = 1.0f;
    bool m_playing = true;
    // 0: 全表示、1: 試聴パネル、2: 再生操作のみ、3: 全非表示
    int m_uiHideLevel = 0;
};
