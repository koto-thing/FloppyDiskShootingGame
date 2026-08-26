#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <string>

#include "TextRenderingService.h"
#include "../../Engine/Graphics/IRenderBackend.h"

using Microsoft::WRL::ComPtr;

class D3D12RenderingService : public IRenderBackend {
public:
    D3D12RenderingService();
    ~D3D12RenderingService();

    bool Initialize(HWND hwnd, int width, int height);
    void Cleanup();

    void BeginFrame() override;
    void EndFrame() override;
    void SyncFrame();

    ID3D12Device* GetDevice() const { return m_device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return m_pipelineStateObject.Get(); }
    ID3D12Resource* GetConstantBuffer() const { return m_constantBuffer.Get(); }
    void* GetCbvCpuData() const { return m_cbvCpuData; }
    UINT GetFrameIndex() const { return m_frameIndex; }
    UINT GetDescriptorHandleIncrementSize() const { return m_rtvDescriptorSize; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCpuDescriptorHandle() const;

    /** @brief 2D円を既存のプロシージャルシェーダーで描画する */
    void DrawCircle(const Circle& circle, const ColorF& color) override;
    /** @brief 2D矩形を既存のプロシージャルシェーダーで描画する */
    void DrawRect(const Rect& rect, const ColorF& color) override;
    void DrawPrimitive3D(const Primitive3D& primitive) override;
    /** @brief Rendererのパイプライン識別子をD3D12ステートへ変換する */
    void SetPipeline(PipelineId pipeline) override;
    void SetCamera(const CameraMatrices& matrices, const Viewport& viewport) override;
    void ResetCamera() override;
    /** @brief 画面幅を取得する */
    int Width() const override { return m_width; }
    /** @brief 画面高さを取得する */
    int Height() const override { return m_height; }
    /** @brief 画面のアスペクト比を取得する */
    float AspectRatio() const override { return m_height == 0 ? 1.0f : static_cast<float>(m_width) / static_cast<float>(m_height); }

    // パイプラインステートの切り替え (0: Object, 1: Background, 2: SpellCircle)
    void SetPipelineState(int type);

    TextRenderingService& GetTextRenderer() { return m_textRenderer; }
    const TextRenderingService& GetTextRenderer() const { return m_textRenderer; }

    // Siv3D風の簡易テキスト描画インターフェース
    void DrawTextCommand(std::string_view text, const Vector2& position, float size, const ColorF& color,
                         float characterSpacing) override;
    void RenderText(const char* text, DirectX::XMFLOAT2 position, float size, DirectX::XMFLOAT4 color,
                    float characterSpacing = 0.0f);

private:
    void DrawUiPrimitive(float x, float y, float width, float height, float z, const ColorF& color, float shapeType, UINT vertexCount);

    bool InitD3D12(HWND hwnd, int width, int height);
    bool InitPipeline();

    int m_width;
    int m_height;

    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12Resource> m_renderTargets[2];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    
    // 3つの個別パイプラインステート
    ComPtr<ID3D12PipelineState> m_pipelineStateObject;
    ComPtr<ID3D12PipelineState> m_pipelineStateBackground;
    ComPtr<ID3D12PipelineState> m_pipelineStateSpellCircle;

    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;
    HANDLE m_fenceEvent;
    UINT m_frameIndex;
    UINT m_rtvDescriptorSize;
    
    // --- フォントレンダラー ---
    TextRenderingService m_textRenderer;

    static constexpr UINT MAX_CONSTANT_BUFFER_ELEMENTS = 2048;
    UINT m_constantBufferCursor = 0;                           // 現在のフレームで次に使用する定数バッファ番号
    ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_cbvCpuData;
    CameraMatrices m_cameraMatrices {};
    Viewport m_cameraViewport {};
    bool m_hasCamera = false;
};
