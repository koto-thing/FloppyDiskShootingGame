#include "CollisionService.h"

void CollisionService::RegisterCollider(const std::shared_ptr<Collider>& collider) {
    if (!collider) {
        return;
    }
    
    m_colliders.emplace_back(collider);
}

void CollisionService::UnregisterCollider(const Collider* collider) {
    std::erase_if(
        m_colliders,
        [collider](const auto& weakCollider) {
            const auto locked = weakCollider.lock();
            
            return !locked || locked.get() == collider;
        }
    );
}

void CollisionService::UpdateService() {
    std::erase_if(
        m_colliders,
        [](const auto& collider) {
            return collider.expired();
        }
    );

    for (std::size_t i = 0; i < m_colliders.size(); ++i) {
        const auto a = m_colliders[i].lock();

        if (!a || !a->IsEnabled()) {
            continue;
        }

        for (std::size_t j = i + 1; j < m_colliders.size(); ++j) {
            const auto b = m_colliders[j].lock();

            if (!b || !b->IsEnabled()) {
                continue;
            }

            if (!CanCollide(*a, *b)) {
                continue;
            }

            if (!CheckCollision(*a, *b)) {
                continue;
            }

            GameObject* objectA = a->GetGameObject();
            GameObject* objectB = b->GetGameObject();

            if (objectA != nullptr) {
                objectA->NotifyCollisionEnter(
                    *a,
                    *b
                );
            }

            if (objectB != nullptr) {
                objectB->NotifyCollisionEnter(
                    *b,
                    *a
                );
            }
        }
    }
}

void CollisionService::Clear() {
    m_colliders.clear();
}

bool CollisionService::CanCollide(const Collider& a, const Collider& b) const {
    const auto aLayer = static_cast<std::uint32_t>(a.GetLayer());
    const auto bLayer = static_cast<std::uint32_t>(b.GetLayer());
    
    const bool aTargetsB = (a.GetCollisionMask() & bLayer) != 0;
    const bool bTargetsA = (b.GetCollisionMask() & aLayer) != 0;
    
    return aTargetsB && bTargetsA;
}

bool CollisionService::CheckCollision(const Collider& a,const Collider& b) const {
    if (a.GetColliderType() == ColliderType::CIRCLE &&
        b.GetColliderType() == ColliderType::CIRCLE) {
        return CheckCircleCircle(
            static_cast<const CircleColliderComponent&>(a),
            static_cast<const CircleColliderComponent&>(b)
        );
    }

    if (a.GetColliderType() == ColliderType::AABB &&
        b.GetColliderType() == ColliderType::AABB) {
        return CheckAABBAABB(
            static_cast<const AABBColliderComponent&>(a),
            static_cast<const AABBColliderComponent&>(b)
        );
    }

    if (a.GetColliderType() == ColliderType::CIRCLE &&
        b.GetColliderType() == ColliderType::AABB) {
        return CheckCircleAABB(
            static_cast<const CircleColliderComponent&>(a),
            static_cast<const AABBColliderComponent&>(b)
        );
    }

    if (a.GetColliderType() == ColliderType::AABB &&
        b.GetColliderType() == ColliderType::CIRCLE) {
        return CheckCircleAABB(
            static_cast<const CircleColliderComponent&>(b),
            static_cast<const AABBColliderComponent&>(a)
        );
    }

    return false;
}
