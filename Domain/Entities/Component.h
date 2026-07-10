#pragma once
#include <DirectXMath.h>
#include <memory>

class GameObject;
class D3D12RenderingService;

/**
 * @brief GameObject に機能を追加するための基底コンポーネントクラス
 */
class Component {
public:
    Component() : m_gameObject(nullptr) {}
    virtual ~Component() {}

    // コンポーネント初期化
    virtual void Initialize(D3D12RenderingService& renderer) { (void)renderer; }
    
    // 毎フレーム更新
    virtual void Update() {}
    
    // 描画処理
    virtual void Render(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
        (void)renderer; (void)viewMatrix; (void)projMatrix;
    }

    void SetGameObject(GameObject* gameObject) { m_gameObject = gameObject; }
    GameObject* GetGameObject() const { return m_gameObject; }

protected:
    GameObject* m_gameObject; // 所属する GameObject への弱参照
};
