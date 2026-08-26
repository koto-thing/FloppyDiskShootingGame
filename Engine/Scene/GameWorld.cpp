#include "GameWorld.h"

#include <algorithm>

GameObject& GameWorld::CreateGameObject(std::string_view name) {
    auto object = std::make_unique<GameObject>();
    object->SetName(std::string(name));
    GameObject& result = *object;
    m_objects.push_back(std::move(object));
    return result;
}

void GameWorld::Destroy(GameObject& object) {
    if (!object.IsPendingDestroy()) {
        object.MarkPendingDestroy();
        m_pendingDestroy.push_back(&object);
    }
}

GameObject* GameWorld::Find(std::string_view name) const {
    for (const auto& object : m_objects) {
        if (!object->IsPendingDestroy() && object->GetName() == name) return object.get();
    }
    return nullptr;
}

void GameWorld::Tick() { for (auto& object : m_objects) object->Tick(); FlushPendingChanges(); }
void GameWorld::Dispose() { m_objects.clear(); m_pendingDestroy.clear(); }

void GameWorld::FlushPendingChanges() {
    m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(), [this](const auto& object) {
        if (!object->IsPendingDestroy()) return false;
        object->Dispose();
        return true;
    }), m_objects.end());
    m_pendingDestroy.clear();
}
