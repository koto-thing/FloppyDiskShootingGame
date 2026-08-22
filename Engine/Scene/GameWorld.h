#pragma once

#include "../../Domain/Entities/GameObject.h"
#include <memory>
#include <string_view>
#include <vector>

/**
 * @brief 実行中のGameObjectを所有し、ライフサイクルを進行させる
 */
class GameWorld {
public:
    /** @brief 名前付きGameObjectを生成する */
    GameObject& CreateGameObject(std::string_view name = "GameObject");
    /** @brief GameObjectをフレーム末尾で破棄するよう予約する */
    void Destroy(GameObject& object);
    /** @brief 名前でGameObjectを検索する */
    GameObject* Find(std::string_view name) const;
    /** @brief 所有オブジェクトを更新する */
    void Tick();
    /** @brief 破棄待ちオブジェクトを解放する */
    void Shutdown();
    /** @brief 予約された生成・破棄を反映する */
    void FlushPendingChanges();

private:
    std::vector<std::unique_ptr<GameObject>> m_objects;
    std::vector<GameObject*> m_pendingDestroy;
};
