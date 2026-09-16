#pragma once
#include "../../Application/Interfaces/IScene.h"
#include "../../Domain/ValueObjects/SceneSharedData.h"
#include "../../Domain/ValueObjects/SceneType.h"
#include "../../Engine/UI/Button.h"
#include <array>
#include <memory>

/** @brief Steamフレンド招待と2人協力プレイの準備画面 */
class SteamLobbyScene : public IScene<SceneType, SceneSharedData> {
public:
    /** @brief ロビーへの接続と操作ボタンを初期化する @return なし */
    void Initialize() override;
    /** @brief ロビー操作を処理する @return なし */
    void ProcessInput() override;
    /** @brief 同期開始時にゲームへ遷移する @return なし */
    void Tick() override;
    /** @brief 未開始ロビーを解放する @return なし */
    void Dispose() override;
    /** @brief ロビーの状態を描画する @param renderer 描画先 @return なし */
    void Render(Renderer& renderer) override;
private:
    std::array<std::unique_ptr<Button>, 6> m_buttons;
};
