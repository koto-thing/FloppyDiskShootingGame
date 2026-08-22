#pragma once
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Component.h"
#include "../../Engine/Scene/Transform.h"
#include "../../Engine/Scene/Tag.h"

class D3D12RenderingService;
class Collider;

/**
 * @brief ゲームオブジェクトの基底クラス (Unity風コンポーネントシステム)
 */
class GameObject {
public:
    GameObject();
    virtual ~GameObject();

    /** @brief オブジェクト名を取得する */
    const std::string& GetName() const { return m_name; }
    /** @brief オブジェクト名を設定する */
    void SetName(std::string name) { m_name = std::move(name); }
    /** @brief 破棄予約済みか取得する */
    bool IsPendingDestroy() const { return m_pendingDestroy; }
    /** @brief 破棄を予約済みにする */
    void MarkPendingDestroy() { m_pendingDestroy = true; }
    /** @brief Transformを取得する */
    Transform& transform() { return m_transform; }
    const Transform& transform() const { return m_transform; }
    /** @brief Active状態を設定する */
    void SetActive(bool active);
    /** @brief 自身のActive状態を取得する */
    bool ActiveSelf() const { return m_activeSelf; }
    /** @brief 階層を含むActive状態を取得する */
    bool ActiveInHierarchy() const;
    /** @brief タグを設定する */
    void SetTag(Tag tag) { m_tag = tag; }
    /** @brief タグを比較する */
    bool CompareTag(Tag tag) const { return m_tag == tag; }
    /** @brief レイヤーを設定する */
    void SetLayer(Layer layer) { m_layer = layer; }
    /** @brief レイヤーを取得する */
    Layer GetLayer() const { return m_layer; }
    
    // オブジェクトとコンポーネントの初期化
    virtual void Initialize(D3D12RenderingService& renderer);
    
    // 毎フレーム更新
    virtual void Tick();
    /** @brief Componentの終了処理を実行する */
    void Shutdown();
    
    // 描画処理
    virtual void RenderObject(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
    
    // 衝突判定
    void NotifyCollisionEnter(Collider& self, Collider& other);
    void NotifyCollisionStay(Collider& self, Collider& other);
    void NotifyCollisionExit(Collider& self, Collider& other);
    
    // セッター
    void SetPosition(const Vector3& position) { m_transform.SetPosition(position); }
    void SetRotation(const Quaternion& rotation) { m_transform.SetLocalRotation(rotation); }
    void SetScale(const Vector3& scale) { m_transform.SetLocalScale(scale); }
    
    // ゲッター
    Vector3 GetPosition() const { return m_transform.Position(); }
    const Quaternion& GetRotation() const { return m_transform.LocalRotation(); }
    const Vector3& GetScale() const { return m_transform.LocalScale(); }
    
    /** @brief Transformからワールド行列を取得する */
    const Matrix4x4& GetWorldMatrix() const { return m_transform.WorldMatrix(); }
    
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
    
    // 互換コンポーネント取得
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
    std::string m_name;
    Transform m_transform;
    
    std::vector<std::shared_ptr<Component>> m_components;
    D3D12RenderingService* m_renderer; // 初期化時に保持
    Tag m_tag;
    Layer m_layer;
    bool m_activeSelf;
    bool m_pendingDestroy;
    
private:
    friend class Component;
    void NotifyActiveChanged(bool active);
};
