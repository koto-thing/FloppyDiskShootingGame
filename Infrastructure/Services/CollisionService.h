#pragma once

#include <memory>
#include <vector>

#include "../../Domain/Entities/Collider.h"

class CollisionService {
public:
    void RegisterCollider(const std::shared_ptr<Collider>& collider);
    void UnregisterCollider(const Collider* collider);
    
    void UpdateService();
    void Clear();
    
private:
    bool CanCollide(const Collider& a, const Collider& b) const;
    bool CheckCollision(const Collider& a, const Collider& b) const;
    
    std::vector<std::weak_ptr<Collider>> m_colliders;
};
