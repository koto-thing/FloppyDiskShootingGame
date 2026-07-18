#include "MeshRenderer.h"
#include "GameObject.h"
#include "../../Infrastructure/ExternalServices/D3D12RenderingService.h"

// 定数バッファ構造体 (HLSLアライメント)
struct RendererTransformBuffer {
    DirectX::XMFLOAT4X4 u_wvpMatrix; // 64
    DirectX::XMFLOAT4 u_Color;       // 16
    float u_time;                    // 4
    float u_shapeType;               // 4
    float u_rotAngle;                // 4
    float u_pad[41];                 // 164 (計256バイト)
};

MeshRenderer::MeshRenderer() 
    : m_mesh(nullptr),
      m_color(1.0f, 1.0f, 1.0f, 1.0f),
      m_cbvCpuData(nullptr) {
}

MeshRenderer::~MeshRenderer() {
    if (m_constantBuffer && m_cbvCpuData) {
        m_constantBuffer->Unmap(0, nullptr);
    }
}

/**
 * @brief MeshRenderer を初期化する
 * @param renderer D3D12RenderingService の参照
 */
void MeshRenderer::Initialize(D3D12RenderingService& renderer) {
    ID3D12Device* device = renderer.GetDevice();
    if (!device) return;
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = 256; // アライメント
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (SUCCEEDED(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)))) {
        
        D3D12_RANGE readRange = { 0, 0 };
        m_constantBuffer->Map(0, &readRange, &m_cbvCpuData);
    }
}

void MeshRenderer::Render(D3D12RenderingService& renderer, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
    if (!m_constantBuffer || !m_cbvCpuData || !m_mesh || !m_gameObject) return;

    // 親 GameObject のワールド行列の取得
    DirectX::XMMATRIX world = m_gameObject->GetWorldMatrix();

    // WVPの合成
    DirectX::XMMATRIX wvpMatrix = world * viewMatrix * projMatrix;

    // 定数バッファへのデータコピー
    RendererTransformBuffer* cbData = reinterpret_cast<RendererTransformBuffer*>(m_cbvCpuData);
    if (cbData) {
        DirectX::XMStoreFloat4x4(&cbData->u_wvpMatrix, DirectX::XMMatrixTranspose(wvpMatrix));
        cbData->u_Color = m_color;
        cbData->u_time = 0.0f;
        cbData->u_shapeType = static_cast<float>(m_mesh->GetShapeType());
        // GameObjectのY回転角度を自転角度として送る
        cbData->u_rotAngle = DirectX::XMConvertToRadians(m_gameObject->GetRotation().y);
    }

    // 定数バッファのバインドと描画
    ID3D12GraphicsCommandList* cmdList = renderer.GetCommandList();
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    cmdList->DrawInstanced(m_mesh->GetVertexCount(), 1, 0, 0);
}
