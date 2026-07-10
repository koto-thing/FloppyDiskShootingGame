#include "GameObject.h"

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

void GameObject::Initialize(D3D12RenderingService& renderer) {
    m_renderer = &renderer;
    
    // 既存のコンポーネントをすべて初期化
    for (auto& component : m_components) {
        component->Initialize(renderer);
    }
}

void GameObject::UpdateObject() {
    // ワールド行列の更新を確定
    GetWorldMatrix();
    
    // コンポーネントの更新
    for (auto& component : m_components) {
        component->Update();
    }
}

void GameObject::RenderObject(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
    // すべてのコンポーネントの描画処理を呼び出す
    for (auto& component : m_components) {
        component->Render(renderer, viewMatrix, projMatrix);
    }
}

const DirectX::XMMATRIX& GameObject::GetWorldMatrix() {
    if (m_isDirty) {
        UpdateWorldMatrix();
    }
    return m_worldMatrix;
}

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