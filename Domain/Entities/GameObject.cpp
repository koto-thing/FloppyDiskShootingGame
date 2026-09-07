#include "GameObject.h"

#include "../Interfaces/ICollisionReceiver.h"

/** @brief GameObjectを生成する */
GameObject::GameObject()
    : m_name("GameObject"), m_renderer(nullptr), m_tag(Tag::Untagged), m_layer(Layer::Default),
      m_activeSelf(true), m_pendingDestroy(false) {
}

/** @brief GameObjectを破棄する */
GameObject::~GameObject() { Dispose(); }

/** @brief GameObjectと所属コンポーネントを初期化する */
void GameObject::Initialize(D3D12RenderingService& renderer) {
    // 描画サービスを保持して所属コンポーネントを初期化する
    m_renderer = &renderer;
    for (auto& component : m_components) component->Initialize(renderer);
}

/** @brief GameObjectの有効状態を設定する */
void GameObject::SetActive(bool active) {
    if (m_activeSelf == active) return;
    m_activeSelf = active;
    NotifyActiveChanged(active);
}

/** @brief 階層を含むGameObjectの有効状態を取得する */
bool GameObject::ActiveInHierarchy() const { return m_activeSelf; }

/** @brief 有効状態の変更を所属コンポーネントへ通知する */
void GameObject::NotifyActiveChanged(bool) {}

/** @brief 有効な所属コンポーネントを更新する */
void GameObject::Tick() {
    if (m_pendingDestroy || !ActiveInHierarchy()) return;
    for (auto& component : m_components) if (component->IsActiveAndEnabled()) component->Tick();
}

/** @brief 所属コンポーネントを破棄する */
void GameObject::Dispose() {
    for (auto& component : m_components) {
        component->Dispose();
    }
    m_components.clear();
}

/** @brief 有効な所属コンポーネントを描画する */
void GameObject::RenderObject(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
    if (!ActiveInHierarchy()) return;
    for (auto& component : m_components) if (component->IsActiveAndEnabled()) component->Render(renderer, viewMatrix, projMatrix);
}

/** @brief 衝突開始を受信可能なコンポーネントへ通知する */
void GameObject::NotifyCollisionEnter(Collider& self, Collider& other) {
    for (auto& component : m_components) if (auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get())) receiver->OnCollisionEnter(self, other);
}

/** @brief 衝突継続を受信可能なコンポーネントへ通知する */
void GameObject::NotifyCollisionStay(Collider& self, Collider& other) {
    for (auto& component : m_components) if (auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get())) receiver->OnCollisionStay(self, other);
}

/** @brief 衝突終了を受信可能なコンポーネントへ通知する */
void GameObject::NotifyCollisionExit(Collider& self, Collider& other) {
    for (auto& component : m_components) if (auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get())) receiver->OnCollisionExit(self, other);
}
