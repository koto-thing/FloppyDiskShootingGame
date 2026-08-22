#include "Component.h"
#include "GameObject.h"

void Component::SetEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
}

bool Component::IsActiveAndEnabled() const {
    return m_enabled && m_gameObject != nullptr && m_gameObject->ActiveInHierarchy();
}

GameObject& Component::gameObject() { return *m_gameObject; }
Transform& Component::transform() { return m_gameObject->transform(); }
