#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <dxgiformat.h>

using Microsoft::WRL::ComPtr;

class TextRenderingService {
public:
    TextRenderingService() = default;
    ~TextRenderingService() = default;
    
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);
    
    void RenderText(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress,
        void* cbvCpuPtr,
        const char* text,
        float startX, float startY,
        float size,
        DirectX::XMFLOAT4 color,
        int screenWidth, int screenHeight
    );
    
private:
    ComPtr<ID3D12Resource> m_fontTexture;
    ComPtr<ID3D12DescriptorHeap> m_fontSrvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineStateText;
    ComPtr<ID3D12RootSignature> m_rootSignatureText;
    
    struct TextConstantBufferData {
        DirectX::XMFLOAT4 u_posSize;      // xy = position, zw = size
        DirectX::XMFLOAT4 u_color;
        DirectX::XMFLOAT4 u_uvOffsetScale;
    };
    
    bool InitFontTexture(ID3D12Device* device);
    bool InitPipeline(ID3D12Device* device, DXGI_FORMAT rtvFormat);
};
