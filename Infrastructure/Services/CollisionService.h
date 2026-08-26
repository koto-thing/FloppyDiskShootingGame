#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "../../Domain/Entities/Collider.h"
#include "../../Domain/Entities/CircleCollider.h"
#include "../../Domain/Entities/AABBCollider.h"

class CollisionService {
public:
    void RegisterCollider(const std::shared_ptr<Collider>& collider);
    void UnregisterCollider(const Collider* collider);
    
    void Tick();
    void Clear();
    
private:
    enum class CollisionEvent { Enter, Stay, Exit };

    struct CollisionPair {
        const Collider* first = nullptr;
        const Collider* second = nullptr;

        CollisionPair() = default;
        CollisionPair(const Collider* lhs, const Collider* rhs);
        bool operator==(const CollisionPair& other) const {
            return first == other.first && second == other.second;
        }
    };

    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair& pair) const;
    };

    bool CanCollide(const Collider& a, const Collider& b) const;
    bool CheckCollision(const Collider& a, const Collider& b) const;
    bool CheckCircleCircle(const CircleCollider& a, const CircleCollider& b) const;
    bool CheckAABBAABB(const AABBCollider& a, const AABBCollider& b) const;
    bool CheckCircleAABB(const CircleCollider& circle, const AABBCollider& aabb) const;
    std::shared_ptr<Collider> FindLiveCollider(const Collider* collider) const;
    void Dispatch(const CollisionPair& pair, CollisionEvent event) const;
    
    std::vector<std::weak_ptr<Collider>> m_colliders;
    std::unordered_set<CollisionPair, CollisionPairHash> m_previousPairs;
    std::unordered_set<CollisionPair, CollisionPairHash> m_currentPairs;
};
