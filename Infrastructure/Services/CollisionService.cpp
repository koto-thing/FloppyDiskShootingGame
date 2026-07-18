#include "CollisionService.h"

#include <algorithm>
#include <cmath>

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
            static_cast<const CircleCollider&>(a),
            static_cast<const CircleCollider&>(b)
        );
    }

    if (a.GetColliderType() == ColliderType::AABB &&
        b.GetColliderType() == ColliderType::AABB) {
        return CheckAABBAABB(
            static_cast<const AABBCollider&>(a),
            static_cast<const AABBCollider&>(b)
        );
    }

    if (a.GetColliderType() == ColliderType::CIRCLE &&
        b.GetColliderType() == ColliderType::AABB) {
        return CheckCircleAABB(
            static_cast<const CircleCollider&>(a),
            static_cast<const AABBCollider&>(b)
        );
    }

    if (a.GetColliderType() == ColliderType::AABB &&
        b.GetColliderType() == ColliderType::CIRCLE) {
        return CheckCircleAABB(
            static_cast<const CircleCollider&>(b),
            static_cast<const AABBCollider&>(a)
        );
    }

    return false;
}

bool CollisionService::CheckCircleCircle(
    const CircleCollider& a,
    const CircleCollider& b
) const {
    const auto aPosition = a.GetWorldPosition();
    const auto bPosition = b.GetWorldPosition();
    const float deltaX = aPosition.x - bPosition.x;
    const float deltaY = aPosition.y - bPosition.y;
    const float radiusSum = std::abs(a.GetWorldRadius()) +
                            std::abs(b.GetWorldRadius());

    return deltaX * deltaX + deltaY * deltaY <= radiusSum * radiusSum;
}

bool CollisionService::CheckAABBAABB(
    const AABBCollider& a,
    const AABBCollider& b
) const {
    const auto aPosition = a.GetWorldPosition();
    const auto bPosition = b.GetWorldPosition();
    const auto aHalfSize = a.GetWorldHalfSize();
    const auto bHalfSize = b.GetWorldHalfSize();

    return std::abs(aPosition.x - bPosition.x) <=
               std::abs(aHalfSize.x) + std::abs(bHalfSize.x) &&
           std::abs(aPosition.y - bPosition.y) <=
               std::abs(aHalfSize.y) + std::abs(bHalfSize.y);
}

bool CollisionService::CheckCircleAABB(
    const CircleCollider& circle,
    const AABBCollider& aabb
) const {
    const auto circlePosition = circle.GetWorldPosition();
    const auto boxPosition = aabb.GetWorldPosition();
    const auto halfSize = aabb.GetWorldHalfSize();
    const float halfWidth = std::abs(halfSize.x);
    const float halfHeight = std::abs(halfSize.y);
    const float radius = std::abs(circle.GetWorldRadius());

    const float closestX = std::clamp(
        circlePosition.x,
        boxPosition.x - halfWidth,
        boxPosition.x + halfWidth
    );
    const float closestY = std::clamp(
        circlePosition.y,
        boxPosition.y - halfHeight,
        boxPosition.y + halfHeight
    );
    const float deltaX = circlePosition.x - closestX;
    const float deltaY = circlePosition.y - closestY;

    return deltaX * deltaX + deltaY * deltaY <= radius * radius;
}
