#pragma once

#include "Component.h"
#include "GameObject.h"
#include "../ValueObjects/ColliderType.h"
#include "../ValueObjects/CollisionLayer.h"

class Collider : public Component {
public:
    explicit Collider(ColliderType type)
        : m_type(type) {
        
    }
    
    virtual ~Collider() = default;

    /**
     * @brief コライダーの種類を取得する
     * @return ColliderType コライダーの種類
     */
    ColliderType GetColliderType() const {
        return m_type;
    }

    /**
     * @brief コライダーの衝突レイヤーを設定する
     * @param layer 衝突レイヤー
     */
    void SetLayer(CollisionLayer layer) {
        m_layer = layer;
    }

    /**
     * @brief コライダーの衝突レイヤーを取得する
     * @return CollisionLayer 衝突レイヤー
     */
    CollisionLayer GetLayer() const {
        return m_layer;
    }

    /**
     * @brief コライダーの衝突マスクを設定する
     * @param mask 衝突マスク (ビットフラグ)
     */
    void SetCollisionMask(std::uint32_t mask) {
        m_collisionMask = mask;
    }

    /**
     * @brief コライダーの衝突マスクを取得する
     * @return std::uint32_t 衝突マスク (ビットフラグ)
     */
    std::uint32_t GetCollisionMask() const {
        return m_collisionMask;
    }

    /**
     * @brief コライダーの有効/無効を設定する
     * @param enabled true: 有効, false: 無効
     */
    void SetEnabled(bool enabled) {
        m_enabled = enabled;
    }

    /**
     * @brief コライダーが有効かどうかを取得する
     * @return true: 有効, false: 無効
     */
    bool IsEnabled() const {
        return m_enabled;
    }

    /**
     * @brief コライダーのオフセットを設定する
     * @param offset オフセット値 (XMFLOAT2)
     */
    void SetOffset(const DirectX::XMFLOAT2& offset) {
        m_offset = offset;
    }

    /**
     * @brief コライダーのオフセットを取得する
     * @return DirectX::XMFLOAT2 オフセット値
     */
    DirectX::XMFLOAT2 GetWorldPosition() const {
        if (m_gameObject == nullptr) {
            return m_offset;
        } 
        
        const auto& position = m_gameObject->GetPosition();
        
        return {
        position.x + m_offset.x,
        position.y + m_offset.y,
        };
    }
    
private:
    ColliderType m_type;
    CollisionLayer m_layer = CollisionLayer::NONE;
    std::uint32_t m_collisionMask = 0;
    DirectX::XMFLOAT2 m_offset = { 0.0f, 0.0f };
    bool m_enabled = true;
};
