#include "GameWorld.h"

#include <algorithm>

/** @brief 名前付きGameObjectを生成して所有対象へ追加する */
GameObject& GameWorld::CreateGameObject(std::string_view name) {
    auto object = std::make_unique<GameObject>();
    object->SetName(std::string(name));
    GameObject& result = *object;
    m_objects.push_back(std::move(object));
    return result;
}

/** @brief GameObjectをフレーム末尾で破棄するよう予約する */
void GameWorld::Destroy(GameObject& object) {
    if (!object.IsPendingDestroy()) {
        object.MarkPendingDestroy();
        m_pendingDestroy.push_back(&object);
    }
}

/** @brief 名前で破棄予約前のGameObjectを検索する */
GameObject* GameWorld::Find(std::string_view name) const {
    for (const auto& object : m_objects) {
        if (!object->IsPendingDestroy() && object->GetName() == name) return object.get();
    }
    return nullptr;
}

/** @brief 所有GameObjectを更新して保留変更を反映する */
void GameWorld::Tick() { for (auto& object : m_objects) object->Tick(); FlushPendingChanges(); }
/** @brief 所有GameObjectのリソースを解放する */
void GameWorld::Dispose() { m_objects.clear(); m_pendingDestroy.clear(); }

/** @brief 破棄予約済みのGameObjectを所有対象から除去する */
void GameWorld::FlushPendingChanges() {
    m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(), [this](const auto& object) {
        if (!object->IsPendingDestroy()) return false;
        object->Dispose();
        return true;
    }), m_objects.end());
    m_pendingDestroy.clear();
}
