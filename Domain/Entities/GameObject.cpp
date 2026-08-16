#include "GameObject.h"

#include "../Interfaces/ICollisionReceiver.h"

GameObject::GameObject() 
    : m_position(0.0f, 0.0f, 0.0f),
      m_rotation(0.0f, 0.0f, 0.0f),
      m_scale(1.0f, 1.0f, 1.0f),
      m_worldMatrix(DirectX::XMMatrixIdentity()),
      m_isDirty(true),
      m_renderer(nullptr) {
}

GameObject::~GameObject() {
    m_components.clear();
}

/**
 * @brief GameObject とそのコンポーネントを初期化する
 * @param renderer D3D12RenderingService の参照
 */
void GameObject::Initialize(D3D12RenderingService& renderer) {
    m_renderer = &renderer;
    
    // 既存のコンポーネントをすべて初期化
    for (auto& component : m_components) {
        component->Initialize(renderer);
    }
}

/**
 * @brief GameObject とそのコンポーネントを更新する
 */
void GameObject::UpdateObject() {
    // ワールド行列の更新を確定
    GetWorldMatrix();
    
    // コンポーネントの更新
    for (auto& component : m_components) {
        component->Update();
    }
}

/**
 * @brief GameObject とそのコンポーネントを描画する
 * @param renderer D3D12RenderingService の参照
 * @param viewMatrix ビュー行列
 * @param projMatrix プロジェクション行列
 */
void GameObject::RenderObject(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
    // すべてのコンポーネントの描画処理を呼び出す
    for (auto& component : m_components) {
        component->Render(renderer, viewMatrix, projMatrix);
    }
}

/**
 * @brief 衝突イベントをコンポーネントに通知する
 * @param self 衝突したコライダー自身
 * @param other 衝突した相手のコライダー
 */
void GameObject::NotifyCollisionEnter(Collider& self, Collider& other) {
    for (auto& component : m_components) {
        auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get());
        if (receiver != nullptr) {
            receiver->OnCollisionEnter(self, other);
        }
    }
}

/**
 * @brief 衝突中のイベントをコンポーネントに通知する
 * @param self 衝突中のコライダー自身
 * @param other 衝突中の相手のコライダー
 */
void GameObject::NotifyCollisionStay(Collider& self, Collider& other) {
    for (auto& component : m_components) {
        auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get());
        if (receiver != nullptr) {
            receiver->OnCollisionStay(self, other);
        }
    }
}

/**
 * @brief 衝突終了のイベントをコンポーネントに通知する
 * @param self 衝突終了したコライダー自身
 * @param other 衝突終了した相手のコライダー
 */
void GameObject::NotifyCollisionExit(Collider& self, Collider& other) {
    for (auto& component : m_components) {
        auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get());
        if (receiver != nullptr) {
            receiver->OnCollisionExit(self, other);
        }
    }
}

/**
 * @brief ワールド行列を取得する。必要に応じて更新される。
 * @return ワールド行列 (DirectX::XMMATRIX)
 */
const DirectX::XMMATRIX& GameObject::GetWorldMatrix() {
    if (m_isDirty) {
        UpdateWorldMatrix();
    }
    return m_worldMatrix;
}

/**
 * @brief ワールド行列を更新する。位置、回転、スケールの変更があった場合に呼び出される。
 */
void GameObject::UpdateWorldMatrix() {
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    
    DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
        DirectX::XMConvertToRadians(m_rotation.x),
        DirectX::XMConvertToRadians(m_rotation.y),
        DirectX::XMConvertToRadians(m_rotation.z)
    );
    
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    
    // SRTの順で乗算
    m_worldMatrix = scale * rotation * translation;
    m_isDirty = false;
}