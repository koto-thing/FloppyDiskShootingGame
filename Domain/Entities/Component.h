#pragma once
#include <DirectXMath.h>

class GameObject;
class Transform;
class D3D12RenderingService;

/**
 * @brief GameObject に機能を追加するための基底コンポーネントクラス
 */
class Component {
public:
    Component() : m_gameObject(nullptr), m_enabled(true) {}
    virtual ~Component() {}

    // コンポーネント初期化
    virtual void Initialize(D3D12RenderingService& renderer) { (void)renderer; }
    
    // 状態更新
    virtual void Tick() {}

    /** @brief Componentの有効状態を取得する */
    bool Enabled() const { return m_enabled; }
    /** @brief Componentの有効状態を変更する */
    void SetEnabled(bool enabled);
    /** @brief 所属GameObject上で更新可能か取得する */
    bool IsActiveAndEnabled() const;
    /** @brief コンポーネントが保持するリソースを解放する */
    virtual void Dispose() {}
    
    // 描画処理
    virtual void Render(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
        (void)renderer; (void)viewMatrix; (void)projMatrix;
    }

    void SetGameObject(GameObject* gameObject) { m_gameObject = gameObject; }
    GameObject* GetGameObject() const { return m_gameObject; }
    /** @brief 所属GameObjectを参照する */
    GameObject& gameObject();
    /** @brief 所属GameObjectのTransformを参照する */
    Transform& transform();

protected:
    friend class GameObject;
    GameObject* m_gameObject; // 所属する GameObject への弱参照
    bool m_enabled;
};
