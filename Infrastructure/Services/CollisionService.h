#pragma once

#include <memory>
#include <vector>

#include "../../Domain/Entities/Collider.h"
#include "../../Domain/Entities/CircleCollider.h"
#include "../../Domain/Entities/AABBCollider.h"

class CollisionService {
public:
    void RegisterCollider(const std::shared_ptr<Collider>& collider);
    void UnregisterCollider(const Collider* collider);
    
    void UpdateService();
    void Clear();
    
private:
    bool CanCollide(const Collider& a, const Collider& b) const;
    bool CheckCollision(const Collider& a, const Collider& b) const;
    bool CheckCircleCircle(const CircleCollider& a, const CircleCollider& b) const;
    bool CheckAABBAABB(const AABBCollider& a, const AABBCollider& b) const;
    bool CheckCircleAABB(const CircleCollider& circle, const AABBCollider& aabb) const;
    
    std::vector<std::weak_ptr<Collider>> m_colliders;
};
