#include "CollisionService.h"

#include <algorithm>
#include <cmath>
#include <functional>

CollisionService::CollisionPair::CollisionPair(const Collider* lhs, const Collider* rhs) {
    if (std::less<const Collider*>{}(rhs, lhs)) {
        first = rhs;
        second = lhs;
    } else {
        first = lhs;
        second = rhs;
    }
}

std::size_t CollisionService::CollisionPairHash::operator()(const CollisionPair& pair) const {
    const std::size_t firstHash = std::hash<const Collider*>{}(pair.first);
    const std::size_t secondHash = std::hash<const Collider*>{}(pair.second);
    return firstHash ^ (secondHash + 0x9e3779b9u + (firstHash << 6) + (firstHash >> 2));
}

void CollisionService::RegisterCollider(const std::shared_ptr<Collider>& collider) {
    if (!collider) {
        return;
    }
    for (const auto& existing : m_colliders) {
        if (const auto locked = existing.lock(); locked && locked.get() == collider.get()) return;
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
    std::erase_if(m_previousPairs, [collider](const CollisionPair& pair) {
        return pair.first == collider || pair.second == collider;
    });
    std::erase_if(m_currentPairs, [collider](const CollisionPair& pair) {
        return pair.first == collider || pair.second == collider;
    });
}

void CollisionService::Tick() {
    std::erase_if(
        m_colliders,
        [](const auto& collider) {
            return collider.expired();
        }
    );

    m_currentPairs.clear();

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

            const CollisionPair pair(a.get(), b.get());
            m_currentPairs.insert(pair);
            Dispatch(pair, m_previousPairs.contains(pair) ? CollisionEvent::Stay : CollisionEvent::Enter);
        }
    }

    for (const CollisionPair& pair : m_previousPairs) {
        if (!m_currentPairs.contains(pair) && FindLiveCollider(pair.first) && FindLiveCollider(pair.second)) {
            Dispatch(pair, CollisionEvent::Exit);
        }
    }

    m_previousPairs = m_currentPairs;
}

void CollisionService::Clear() {
    m_colliders.clear();
    m_previousPairs.clear();
    m_currentPairs.clear();
}

std::shared_ptr<Collider> CollisionService::FindLiveCollider(const Collider* collider) const {
    for (const auto& weakCollider : m_colliders) {
        if (const auto locked = weakCollider.lock(); locked && locked.get() == collider) return locked;
    }
    return nullptr;
}

void CollisionService::Dispatch(const CollisionPair& pair, CollisionEvent event) const {
    const auto first = FindLiveCollider(pair.first);
    const auto second = FindLiveCollider(pair.second);
    if (!first || !second) return;

    GameObject* firstObject = first->GetGameObject();
    GameObject* secondObject = second->GetGameObject();
    if (firstObject == nullptr || secondObject == nullptr) return;

    switch (event) {
    case CollisionEvent::Enter:
        firstObject->NotifyCollisionEnter(*first, *second);
        secondObject->NotifyCollisionEnter(*second, *first);
        break;
    case CollisionEvent::Stay:
        firstObject->NotifyCollisionStay(*first, *second);
        secondObject->NotifyCollisionStay(*second, *first);
        break;
    case CollisionEvent::Exit:
        firstObject->NotifyCollisionExit(*first, *second);
        secondObject->NotifyCollisionExit(*second, *first);
        break;
    }
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
