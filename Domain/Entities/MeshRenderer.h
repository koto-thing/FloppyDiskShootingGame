#pragma once
#include "Component.h"
#include "Mesh.h"
#include <memory>
#include <wrl/client.h>
#include <d3d12.h>

/**
 * @brief GameObject にメッシュ描画機能を追加するコンポーネント (UnityのMeshRendererに相当)
 */
class MeshRenderer : public Component {
public:
    /** @brief メッシュ描画コンポーネントを生成する */
    MeshRenderer();
    /** @brief メッシュ描画コンポーネントを破棄する */
    virtual ~MeshRenderer();

    /**
     * @brief 描画リソースを初期化する
     * @param renderer 描画サービス
     */
    virtual void Initialize(D3D12RenderingService& renderer) override;
    /**
     * @brief メッシュを描画する
     * @param renderer 描画サービス
     * @param viewMatrix ビュー行列
     * @param projMatrix 投影行列
     */
    virtual void Render(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) override;

    /** @brief 描画対象メッシュを設定する */
    void SetMesh(std::shared_ptr<Mesh> mesh) { m_mesh = mesh; }
    /** @brief 描画色を設定する */
    void SetColor(const DirectX::XMFLOAT4& color) { m_color = color; }

    /** @brief 描画対象メッシュを取得する */
    std::shared_ptr<Mesh> GetMesh() const { return m_mesh; }
    /** @brief 描画色を取得する */
    const DirectX::XMFLOAT4& GetColor() const { return m_color; }

private:
    std::shared_ptr<Mesh> m_mesh;
    DirectX::XMFLOAT4 m_color;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_cbvCpuData; // 定数バッファ書き込み用 CPU Map アドレス
};
