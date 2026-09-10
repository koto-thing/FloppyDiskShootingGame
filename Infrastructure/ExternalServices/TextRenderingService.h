#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <dxgiformat.h>

using Microsoft::WRL::ComPtr;

class TextRenderingService {
public:
    /**
     * @brief テキスト描画サービスを生成する
     */
    TextRenderingService() = default;

    /**
     * @brief テキスト描画サービスを破棄する
     */
    ~TextRenderingService() = default;

    /**
     * @brief テキスト描画リソースを初期化する
     * @param device 使用するD3D12デバイス
     * @param rtvFormat レンダーターゲットのフォーマット
     * @return 初期化に成功した場合true
     */
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);

    /**
     * @brief テキスト描画コマンドを発行する
     * @param commandList 使用するコマンドリスト
     * @param cbvGpuAddress 定数バッファのGPUアドレス
     * @param cbvCpuPtr 定数バッファのCPUアドレス
     * @param text 描画する文字列
     * @param startX 描画開始位置のX座標
     * @param startY 描画開始位置のY座標
     * @param size 文字サイズ
     * @param color 文字色
     * @param characterSpacing 文字間隔
     * @param screenWidth 画面幅
     * @param screenHeight 画面高さ
     */
    void RenderText(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress,
        void* cbvCpuPtr,
        const char* text,
        float startX, float startY,
        float size,
        DirectX::XMFLOAT4 color,
        float characterSpacing,
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

    /**
     * @brief フォントテクスチャを初期化する
     * @param device 使用するD3D12デバイス
     * @return 初期化に成功した場合true
     */
    bool InitFontTexture(ID3D12Device* device);

    /**
     * @brief テキスト描画用パイプラインを初期化する
     * @param device 使用するD3D12デバイス
     * @param rtvFormat レンダーターゲットのフォーマット
     * @return 初期化に成功した場合true
     */
    bool InitPipeline(ID3D12Device* device, DXGI_FORMAT rtvFormat);
};
