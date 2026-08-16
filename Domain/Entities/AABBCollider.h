#pragma once

#include "Collider.h"
#include "GameObject.h"

class AABBCollider final : public Collider {
public:
    explicit AABBCollider(const DirectX::XMFLOAT2& halfSize)
        : Collider(ColliderType::AABB),
          m_halfSize(halfSize) {
        
    }

    /**
     * @brief コライダーの半サイズを設定する
     * @param halfSize 半サイズ (X方向, Y方向)
     */
    void SetHalfSize(const DirectX::XMFLOAT2& halfSize) {
        m_halfSize = halfSize;
    }
    
    /**
     * @brief ワールド空間におけるコライダーの半サイズを取得する
     * @return ワールド空間における半サイズ (X方向, Y方向)
     */
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