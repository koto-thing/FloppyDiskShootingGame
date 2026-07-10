#pragma once
#include "Component.h"
#include "Mesh.h"
#include <wrl/client.h>
#include <d3d12.h>

/**
 * @brief GameObject にメッシュ描画機能を追加するコンポーネント (UnityのMeshRendererに相当)
 */
class MeshRenderer : public Component {
public:
    MeshRenderer();
    virtual ~MeshRenderer();

    virtual void Initialize(D3D12RenderingService& renderer) override;
    virtual void Render(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) override;

    // セッター
    void SetMesh(std::shared_ptr<Mesh> mesh) { m_mesh = mesh; }
    void SetColor(const DirectX::XMFLOAT4& color) { m_color = color; }

    // ゲッター
    std::shared_ptr<Mesh> GetMesh() const { return m_mesh; }
    const DirectX::XMFLOAT4& GetColor() const { return m_color; }

private:
    std::shared_ptr<Mesh> m_mesh;
    DirectX::XMFLOAT4 m_color;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_cbvCpuData; // 定数バッファ書き込み用 CPU Map アドレス
};
