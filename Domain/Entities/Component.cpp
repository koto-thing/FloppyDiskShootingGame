#include "Component.h"
#include "GameObject.h"

/**
 * @brief コンポーネントの有効状態を設定する
 * @param enabled 有効にする場合true
 */
void Component::SetEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
}

/** @brief 所属GameObject上で更新可能か判定する */
bool Component::IsActiveAndEnabled() const {
    return m_enabled && m_gameObject != nullptr && m_gameObject->ActiveInHierarchy();
}

/** @brief 所属GameObjectを参照する */
GameObject& Component::gameObject() { return *m_gameObject; }
/** @brief 所属GameObjectのTransformを参照する */
Transform& Component::transform() { return m_gameObject->transform(); }
