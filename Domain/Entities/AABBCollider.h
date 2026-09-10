#pragma once

#include "Collider.h"
#include "GameObject.h"

class AABBCollider final : public Collider {
public:
    /**
     * @brief 軸平行境界箱コライダーを生成する
     * @param halfSize コライダーの半サイズ
     */
    explicit AABBCollider(const Vector2& halfSize)
        : Collider(ColliderType::AABB),
          m_halfSize(halfSize) {
        
    }

    /**
     * @brief コライダーの半サイズを設定する
     * @param halfSize 半サイズ (X方向, Y方向)
     */
    void SetHalfSize(const Vector2& halfSize) {
        m_halfSize = halfSize;
    }
    
    /**
     * @brief ワールド空間におけるコライダーの半サイズを取得する
     * @return ワールド空間における半サイズ (X方向, Y方向)
     */
    Vector2 GetWorldHalfSize() const {
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
    Vector2 m_halfSize;
};
