#include "GameObject.h"

#include "../Interfaces/ICollisionReceiver.h"

GameObject::GameObject()
    : m_name("GameObject"), m_renderer(nullptr), m_tag(Tag::Untagged), m_layer(Layer::Default),
      m_activeSelf(true), m_pendingDestroy(false) {
}

GameObject::~GameObject() { Shutdown(); }

void GameObject::Initialize(D3D12RenderingService& renderer) {
    m_renderer = &renderer;
    for (auto& component : m_components) component->Initialize(renderer);
}

void GameObject::SetActive(bool active) {
    if (m_activeSelf == active) return;
    m_activeSelf = active;
    NotifyActiveChanged(active);
}

bool GameObject::ActiveInHierarchy() const { return m_activeSelf; }

void GameObject::NotifyActiveChanged(bool) {}

void GameObject::Tick() {
    if (m_pendingDestroy || !ActiveInHierarchy()) return;
    for (auto& component : m_components) if (component->IsActiveAndEnabled()) component->Tick();
}

void GameObject::Shutdown() {
    for (auto& component : m_components) {
        component->Shutdown();
    }
    m_components.clear();
}

void GameObject::RenderObject(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
    if (!ActiveInHierarchy()) return;
    for (auto& component : m_components) if (component->IsActiveAndEnabled()) component->Render(renderer, viewMatrix, projMatrix);
}

void GameObject::NotifyCollisionEnter(Collider& self, Collider& other) {
    for (auto& component : m_components) if (auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get())) receiver->OnCollisionEnter(self, other);
}

void GameObject::NotifyCollisionStay(Collider& self, Collider& other) {
    for (auto& component : m_components) if (auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get())) receiver->OnCollisionStay(self, other);
}

void GameObject::NotifyCollisionExit(Collider& self, Collider& other) {
    for (auto& component : m_components) if (auto* receiver = dynamic_cast<ICollisionReceiver*>(component.get())) receiver->OnCollisionExit(self, other);
}
