#pragma once

#include "Collider.h"
#include "GameObject.h"

class AABBCollider final : public Collider {
public:
    explicit AABBCollider(const DirectX::XMFLOAT2& halfSize)
        : Collider(ColliderType::AABB),
          m_halfSize(halfSize) {
        
    }
    
    void SetHalfSize(const DirectX::XMFLOAT2& halfSize) {
        m_halfSize = halfSize;
    }
    
    DirectX::XMFLOAT2 GetWorldHalfSize() const {
        if (m_gameObject == nullptr) {
            return m_halfSize;
        }
        
        const auto& scale = m_gameObject->GetScale();
        
        return {
            m_halfSize.x * std::abs(scale.x),
            m_halfSize.y * std::abs(scale.y),
        };
    }
    
private:
    DirectX::XMFLOAT2 m_halfSize;
};