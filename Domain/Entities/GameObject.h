#pragma once
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "Component.h"

class D3D12RenderingService;
class Collider;

/**
 * @brief ゲームオブジェクトの基底クラス (Unity風コンポーネントシステム)
 */
class GameObject {
public:
    GameObject();
    virtual ~GameObject();
    
    // オブジェクトとコンポーネントの初期化
    virtual void Initialize(D3D12RenderingService& renderer);
    
    // 毎フレーム更新
    virtual void UpdateObject();
    
    // 描画処理
    virtual void RenderObject(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
    
    // 衝突判定
    void NotifyCollisionEnter(Collider& self, Collider& other);
    void NotifyCollisionStay(Collider& self, Collider& other);
    void NotifyCollisionExit(Collider& self, Collider& other);
    
    // セッター
    void SetPosition(const DirectX::XMFLOAT3& position) { m_position = position; m_isDirty = true; }
    void SetRotation(const DirectX::XMFLOAT3& rotation) { m_rotation = rotation; m_isDirty = true; }
    void SetScale(const DirectX::XMFLOAT3& scale) { m_scale = scale; m_isDirty = true; }
    
    // ゲッター
    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    const DirectX::XMFLOAT3& GetRotation() const { return m_rotation; }
    const DirectX::XMFLOAT3& GetScale() const { return m_scale; }
    
    const DirectX::XMMATRIX& GetWorldMatrix();
    
    // Unity風コンポーネント追加
    template <typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        auto component = std::make_shared<T>(std::forward<Args>(args)...);
        component->SetGameObject(this);
        m_components.push_back(component);
        if (m_renderer) {
            component->Initialize(*m_renderer);
        }
        return component;
    }
    
    // Unity風コンポーネント取得
    template <typename T>
    std::shared_ptr<T> GetComponent() {
        for (auto& component : m_components) {
            auto casted = std::dynamic_pointer_cast<T>(component);
            if (casted) {
                return casted;
            }
        }
        return nullptr;
    }
    
protected:
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_rotation;
    DirectX::XMFLOAT3 m_scale;
    
    DirectX::XMMATRIX m_worldMatrix;
    bool m_isDirty;
    
    std::vector<std::shared_ptr<Component>> m_components;
    D3D12RenderingService* m_renderer; // 初期化時に保持
    
private:
    void UpdateWorldMatrix();
};
