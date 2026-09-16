#pragma once
#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"
#include <array>
#include <memory>

/** @brief 自動マッチングと部屋コードを使う協力プレイ画面 */
class OnlineLobbyScene : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief 保存設定とボタンを初期化する @return なし */
    void Initialize() override;
    /** @brief 部屋コードとボタン操作を処理する @return なし */
    void ProcessInput() override;
    /** @brief 開始設定をゲームへ渡す @return なし */
    void Tick() override;
    /** @brief 未開始の接続を終了する @return なし */
    void Dispose() override;
    /** @brief ロビーを描画する @param renderer 描画先 @return なし */
    void Render(Renderer& renderer) override;
private:
    std::array<std::unique_ptr<Button>, 15> m_buttons;
    bool m_automatic = true;
    std::string m_code;
};
