#pragma once

#include "Collider.h"

class CircleCollider final : public Collider {
public:
    explicit CircleCollider(float radius) 
        : Collider(ColliderType::CIRCLE),
          m_radius(radius) {
        
    }

    /**
     * @brief コライダーの半径を設定する
     * @param radius 半径
     */
    void SetRadius(float radius) {
        m_radius = radius;
    }

    /**
     * @brief コライダーの半径を取得する
     * @return 半径
     */
    float GetRadius() {
        return m_radius;
    }

    /**
     * @brief ワールド座標系でのコライダーの半径を取得する
     * @return ワールド座標系での半径
     */
    float GetWorldRadius() const {
        if (m_gameObject == nullptr) {
            return m_radius;
        }
        
        const auto& scale = m_gameObject->GetScale();
        
        const float maxScale = std::max(
            std::abs(scale.x),
            std::abs(scale.y)
        );
        
        return m_radius * maxScale;
    }
    
private:
    float m_radius;
};
