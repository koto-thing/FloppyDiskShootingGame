#include <d3dcompiler.h>
#include <vector>
#include <string>

#include "TextRenderingService.h"
#include "../../Engine/Diagnostics/Debug.h"
#include "../../Domain/ValueObjects/CharacterASCIIData.h"

// 埋め込みシェーダーコード (Typed Bufferからフォントを読み込む)
const char g_textShaderCode[] = R"(
cbuffer TransformBuffer : register(b0) {
    float4 u_posSize;       // xy: position, zw: size
    float4 u_Color;
    float4 u_uvOffsetScale; // x に文字コードが入っている
};

Buffer<uint> g_fontBuffer : register(t0);

struct VS_OUTPUT {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VS_OUTPUT VSTextMain(uint vID : SV_VertexID) {
    VS_OUTPUT output;
    float2 localPos;
    localPos.x = (float)(vID & 2) - 1.0f;
    localPos.y = (float)((vID & 1) << 1) - 1.0f;
    
    float2 u_position = u_posSize.xy;
    float2 u_size = u_posSize.zw;
    float2 finalPos = u_position + (localPos * u_size);
    output.pos = float4(finalPos, 0.0f, 1.0f);
    
    // rawUV: (vID & 2) ? 1.0f : 0.0f, (vID & 1) ? 0.0f : 1.0f
    output.uv = float2((vID & 2) ? 1.0f : 0.0f, (vID & 1) ? 0.0f : 1.0f);
    return output;
}

float4 PSTextMain(VS_OUTPUT input) : SV_TARGET {
    uint pixelX = (uint)clamp(input.uv.x * 8.0f, 0.0f, 7.0f);
    uint pixelY = (uint)clamp(input.uv.y * 8.0f, 0.0f, 7.0f);
    
    uint c = (uint)(u_uvOffsetScale.x + 0.5f);
    uint wordIndex = c * 2 + (pixelY / 4);
    uint word = g_fontBuffer[wordIndex];
    uint rowInWord = pixelY % 4;
    uint fontRow = (word >> (rowInWord * 8)) & 0xFF;
    
    uint bit = 0x80 >> pixelX;
    if ((fontRow & bit) == 0) {
        discard;
    }
    return u_Color;
}
)";

/**
 * @brief TextRenderingService を初期化する
 * @param device 
 * @param rtvFormat 
 * @return 
 */
bool TextRenderingService::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat) {
    if (!InitFontTexture(device))
        return false;
    
    if (!InitPipeline(device, rtvFormat))
        return false;
    
    return true;
}

/**
 * @brief フォントテクスチャを初期化する
 * @param device 
 * @return 
 */
bool TextRenderingService::InitFontTexture(ID3D12Device* device) {
    const int bufferSize = 1024;
    std::vector<unsigned char> fontData(bufferSize);
    for (int charIdx = 0; charIdx < 128; ++charIdx) {
        for (int row = 0; row < 8; ++row) {
            fontData[charIdx * 8 + row] = CharacterASCIIData::g_font[charIdx][row];
        }
    }

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = bufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_fontTexture)))) {
        return false;
    }

    void* pData = nullptr;
    m_fontTexture->Map(0, nullptr, &pData);
    memcpy(pData, fontData.data(), fontData.size());
    m_fontTexture->Unmap(0, nullptr);

    // SRVの作成 (Typed Buffer (Buffer<uint>) として作成)
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_fontSrvHeap)))) {
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = bufferSize / 4;
    srvDesc.Buffer.StructureByteStride = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(m_fontTexture.Get(), &srvDesc, m_fontSrvHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

/**
 * @brief パイプラインを初期化する
 * @param device 
 * @param rtvFormat 
 * @return 
 */
bool TextRenderingService::InitPipeline(ID3D12Device* device, DXGI_FORMAT rtvFormat) {
    // SRV範囲の定義 (ByteAddressBufferもSRVテーブルとしてバインドできる)
    D3D12_DESCRIPTOR_RANGE descRange = {};
    descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRange.NumDescriptors = 1;
    descRange.BaseShaderRegister = 0;
    descRange.RegisterSpace = 0;
    descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // ルートパラメータの定義
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    
    // 0: CBV
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // 1: SRV
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descRange;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    // サンプラーは不要になったため、ルートシグネチャから静的サンプラーを削除
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob))) {
        if (errorBlob) {
            Debug::LogError(static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }

    if (FAILED(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatureText)))) {
        return false;
    }

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    
    ComPtr<ID3DBlob> vsErrorBlob;
    HRESULT hrVS = D3DCompile(
        g_textShaderCode,
        sizeof(g_textShaderCode) - 1,
        "TextShaderSource",
        nullptr,
        nullptr,
        "VSTextMain",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vertexShader,
        &vsErrorBlob);
    if (FAILED(hrVS)) {
        if (vsErrorBlob) {
            MessageBoxA(NULL, (char*)vsErrorBlob->GetBufferPointer(), "Shader Compile Error (VSTextMain)", MB_OK);
        }
        return false;
    }
    
    ComPtr<ID3DBlob> psErrorBlob;
    HRESULT hrPS = D3DCompile(
        g_textShaderCode,
        sizeof(g_textShaderCode) - 1,
        "TextShaderSource",
        nullptr,
        nullptr,
        "PSTextMain",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShader,
        &psErrorBlob);
    if (FAILED(hrPS)) {
        if (psErrorBlob) {
            MessageBoxA(NULL, (char*)psErrorBlob->GetBufferPointer(), "Shader Compile Error (PSTextMain)", MB_OK);
        }
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = m_rootSignatureText.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = rtvFormat;
    psoDesc.SampleDesc.Count = 1;

    // 半透明ブレンドステート定義
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateText)))) {
        MessageBoxA(NULL, "CreateGraphicsPipelineState (Text) Failed", "Error", MB_OK);
        return false;
    }

    return true;
}

/**
 * @brief テキストを描画する
 * @param commandList 
 * @param cbvGpuAddress 
 * @param cbvCpuPtr 
 * @param text 
 * @param startX 
 * @param startY 
 * @param size 
 * @param color 
 * @param screenWidth 
 * @param screenHeight 
 */
void TextRenderingService::RenderText(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress,
    void* cbvCpuPtr,
    const char* text,
    float startX, float startY,
    float size,
    DirectX::XMFLOAT4 color,
    int screenWidth, int screenHeight
) {
    // パイプラインと記述子ヒープの設定
    commandList->SetGraphicsRootSignature(m_rootSignatureText.Get());
    commandList->SetPipelineState(m_pipelineStateText.Get());
    
    ID3D12DescriptorHeap* heaps[] = { m_fontSrvHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->SetGraphicsRootDescriptorTable(1, m_fontSrvHeap->GetGPUDescriptorHandleForHeapStart());
    
    float currentX = startX;
    float currentY = startY;
    int length = lstrlenA(text);
    
    // 外部から渡された定数バッファのポインタを扱いやすい型にキャスト
    TextConstantBufferData* currentCbCpu = reinterpret_cast<TextConstantBufferData*>(cbvCpuPtr);
    D3D12_GPU_VIRTUAL_ADDRESS currentCbGpu = cbvGpuAddress;
    
    for (int i = 0 ; i < length ; ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        if (c == '\n') {
            currentX = startX;
            currentY -= size * 2.0f;
            continue;
        }
        
        if (c >= 128) {
            c = '?';
        }
        
        // 定数バッファデータ書き込み (16バイトアラインされた u_posSize へパック)
        currentCbCpu->u_posSize = {
            currentX,
            currentY,
            size,
            size * static_cast<float>(screenWidth) / screenHeight
        };
        currentCbCpu->u_color = color;
        currentCbCpu->u_uvOffsetScale = { (float)c, 0.0f, 0.0f, 0.0f }; // x に文字コードを格納
        
        // コマンドリストに現在の文字の定数バッファをセット
        commandList->SetGraphicsRootConstantBufferView(0, currentCbGpu);
        commandList->DrawInstanced(4, 1, 0, 0);
        
        // 次の文字のためにポインタとGPUアドレスを進める
        currentCbCpu = reinterpret_cast<TextConstantBufferData*>(reinterpret_cast<char*>(currentCbCpu) + 256);
        currentCbGpu += 256;
        
        currentX += size * 1.5f; // 文字送り
    }
}
