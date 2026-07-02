#include "D3D12RenderingService.h"
#include <cstdio>

// .exeが存在する絶対パスを基準にシェーダーファイルへのフルパスを解決する
std::wstring GetShaderFilePath(const wchar_t* fileName) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0'; // 実行ファイル名部分を切り捨て
    }
    return std::wstring(exePath) + fileName;
}

/**
 * D3D12Renderer クラスのコンストラクタ
 */
D3D12RenderingService::D3D12RenderingService() 
    : m_width(800), m_height(600), m_fenceValue(0), m_fenceEvent(nullptr), m_frameIndex(0), m_rtvDescriptorSize(0), m_cbvCpuData(nullptr) {
}

/**
 * D3D12Renderer クラスのデストラクタ
 */
D3D12RenderingService::~D3D12RenderingService() {
    Cleanup();
}

/**
 * D3D12Renderer を初期化する
 * @param hwnd ウィンドウハンドル
 * @param width 幅
 * @param height 高さ
 * @return 初期化に成功した場合：true、失敗した場合：false
 */
bool D3D12RenderingService::Initialize(HWND hwnd, int width, int height) {
    m_width = width;
    m_height = height;

    if (!InitD3D12(hwnd, width, height)) {
        MessageBoxA(NULL, "D3D12Renderer::Initialize - InitD3D12 Failed!", "Error", MB_OK);
        return false;
    }
    if (!InitPipeline()) {
        MessageBoxA(NULL, "D3D12Renderer::Initialize - InitPipeline Failed!", "Error", MB_OK);
        return false;
    }

    return true;
}

/**
 * D3D12Renderer をクリーンアップする
 */
void D3D12RenderingService::Cleanup() {
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    if (m_constantBuffer) {
        m_constantBuffer->Unmap(0, nullptr);
    }
}

// Embedded shader code to avoid file loading issues
const char g_shaderCode[] = R"(
cbuffer TransformBuffer : register(b0)
{
    float2 u_position; // Object center position (-1.0 to 1.0)
    float2 u_size;     // Object size (width, height)
    float4 u_Color;    // Object color (RGBA)
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VS_OUTPUT VSMain(uint vID : SV_VertexID)
{
    VS_OUTPUT output;
    
    // vID: 0 = Bottom-Left, 1 = Top-Left, 2 = Bottom-Right, 3 = Top-Right
    float2 localPos;
    localPos.x = (float) (vID & 2) - 1.0f;
    localPos.y = (float) ((vID & 1) << 1) - 1.0f;

    // Apply size and shift position
    float2 finalPos = u_position + (localPos * u_size);
    
    output.pos = float4(finalPos, 0.0f, 1.0f);
    output.color = u_Color;
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}
)";

/**
 * D3D12Renderer を初期化する
 * @param hwnd ウィンドウハンドル
 * @param width 幅
 * @param height 高さ
 * @return 初期化に成功した場合：true、失敗した場合：false
 */
bool D3D12RenderingService::InitD3D12(HWND hwnd, int width, int height) {
#if defined(_DEBUG)
    // Enable D3D12 debug layer for debug builds to catch issues early
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        MessageBoxA(NULL, "CreateDXGIFactory1 Failed", "Error", MB_OK);
        return false;
    }

    // ハードウェアデバイスの作成を試みる
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
    if (FAILED(hr)) {
        // 失敗した場合は WARP デバイス (ソフトウェアレンダラー) の作成を試みる
        ComPtr<IDXGIAdapter> warpAdapter;
        if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)))) {
            hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        }
    }
    if (FAILED(hr)) {
        MessageBoxA(NULL, "D3D12CreateDevice Failed (Hardware & WARP)", "Error", MB_OK);
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
        MessageBoxA(NULL, "CreateCommandQueue Failed", "Error", MB_OK);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(factory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &swapChain))) {
        MessageBoxA(NULL, "CreateSwapChain Failed", "Error", MB_OK);
        return false;
    }
    if (FAILED(swapChain.As(&m_swapChain))) {
        MessageBoxA(NULL, "SwapChain As IDXGISwapChain3 Failed", "Error", MB_OK);
        return false;
    }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        MessageBoxA(NULL, "CreateDescriptorHeap (RTV) Failed", "Error", MB_OK);
        return false;
    }

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT n = 0; n < 2; ++n) {
        if (FAILED(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])))) {
            MessageBoxA(NULL, "SwapChain GetBuffer Failed", "Error", MB_OK);
            return false;
        }
        m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
    if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) {
        MessageBoxA(NULL, "CreateCommandAllocator Failed", "Error", MB_OK);
        return false;
    }
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) {
        MessageBoxA(NULL, "CreateCommandList Failed", "Error", MB_OK);
        return false;
    }
    m_commandList->Close();
    
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
        MessageBoxA(NULL, "CreateFence Failed", "Error", MB_OK);
        return false;
    }
    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        MessageBoxA(NULL, "CreateEvent for Fence Failed", "Error", MB_OK);
        return false;
    }
    
    UINT elementCount = 2048; 
    UINT bufferSize = elementCount * 256;

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

    // 【修正】末尾の余分な閉じ括弧 ')' を削除
    hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer));
        
    if (FAILED(hr)) {
        MessageBoxA(NULL, "CreateCommittedResource (ConstantBuffer) Failed", "Error", MB_OK);
        return false;
    }

    D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvCpuData)))) {
        MessageBoxA(NULL, "ConstantBuffer Map Failed", "Error", MB_OK);
        return false;
    }

    return true;
}

/**
 * D3D12 パイプラインを初期化する
 * @return 初期化に成功した場合：true、失敗した場合：false
 */
bool D3D12RenderingService::InitPipeline() {
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        MessageBoxA(NULL, "D3D12SerializeRootSignature Failed", "Error", MB_OK);
        return false;
    }
    if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
        MessageBoxA(NULL, "CreateRootSignature Failed", "Error", MB_OK);
        return false;
    }

    // 各シェーダーファイルへの絶対パスを取得
    std::wstring pathObject = GetShaderFilePath(L"Shaders\\ObjectShader.hlsl");
    std::wstring pathBackground = GetShaderFilePath(L"Shaders\\BackgroundShader.hlsl");
    std::wstring pathSpellCircle = GetShaderFilePath(L"Shaders\\SpellCircleShader.hlsl");

    // 1. 共通頂点シェーダーのコンパイル (ObjectShader.hlsl からコンパイル)
    ComPtr<ID3DBlob> vertexShader;
    error.Reset();
    HRESULT hrVS = D3DCompileFromFile(
        pathObject.c_str(),
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vertexShader,
        &error);
    if (FAILED(hrVS)) {
        if (error) {
            MessageBoxA(NULL, (char*)error->GetBufferPointer(), "Shader Compile Error (VS)", MB_OK);
        } else {
            char buf[128];
            sprintf_s(buf, "D3DCompileFromFile (VS) Failed with HRESULT: 0x%08X", hrVS);
            MessageBoxA(NULL, buf, "Shader Compile Error", MB_OK);
        }
        // 【修正】未定義の `msg` を使わず、上で整形した `buf` などを使ってエラーを表示するように統一
        return false;
    }
    
    // --- 各ピクセルシェーダーのコンパイルとPSO作成 ---

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    
    // 半透明ブレンドステート定義 (Object と SpellCircle 用)
    const D3D12_RENDER_TARGET_BLEND_DESC alphaBlendDesc = {
        TRUE, FALSE,
        D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    // 不透明ブレンドステート定義 (Background 用)
    const D3D12_RENDER_TARGET_BLEND_DESC opaqueBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    // A. Object 用 PSO 作成 (PSObject)
    ComPtr<ID3DBlob> pixelShaderObj;
    error.Reset();
    HRESULT hrPSObj = D3DCompileFromFile(
        pathObject.c_str(),
        nullptr,
        nullptr,
        "PSObject",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShaderObj,
        &error);
    if (FAILED(hrPSObj)) {
        if (error) {
            MessageBoxA(NULL, (char*)error->GetBufferPointer(), "Shader Compile Error (PSObject)", MB_OK);
        } else {
            char buf[128];
            sprintf_s(buf, "D3DCompileFromFile (PSObject) Failed with HRESULT: 0x%08X", hrPSObj);
            MessageBoxA(NULL, buf, "Shader Compile Error", MB_OK);
        }
        return false;
    }
    psoDesc.PS = { pixelShaderObj->GetBufferPointer(), pixelShaderObj->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0] = alphaBlendDesc;
    HRESULT hrPSOObj = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateObject));
    if (FAILED(hrPSOObj)) {
        char buf[128];
        sprintf_s(buf, "CreateGraphicsPipelineState (Object) Failed with HRESULT: 0x%08X", hrPSOObj);
        MessageBoxA(NULL, buf, "PSO Creation Error", MB_OK);
        return false;
    }

    // B. Background 用 PSO 作成 (PSBackground)
    ComPtr<ID3DBlob> pixelShaderBG;
    error.Reset();
    HRESULT hrPSBG = D3DCompileFromFile(
        pathBackground.c_str(),
        nullptr,
        nullptr,
        "PSBackground",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShaderBG,
        &error);
    if (FAILED(hrPSBG)) {
        if (error) {
            MessageBoxA(NULL, (char*)error->GetBufferPointer(), "Shader Compile Error (PSBackground)", MB_OK);
        } else {
            char buf[128];
            sprintf_s(buf, "D3DCompileFromFile (PSBackground) Failed with HRESULT: 0x%08X", hrPSBG);
            MessageBoxA(NULL, buf, "Shader Compile Error", MB_OK);
        }
        return false;
    }
    psoDesc.PS = { pixelShaderBG->GetBufferPointer(), pixelShaderBG->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0] = opaqueBlendDesc;
    HRESULT hrPSOBG = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateBackground));
    if (FAILED(hrPSOBG)) {
        char buf[128];
        sprintf_s(buf, "CreateGraphicsPipelineState (Background) Failed with HRESULT: 0x%08X", hrPSOBG);
        MessageBoxA(NULL, buf, "PSO Creation Error", MB_OK);
        return false;
    }

    // C. SpellCircle 用 PSO 作成 (PSSpellCircle)
    ComPtr<ID3DBlob> pixelShaderSC;
    error.Reset();
    HRESULT hrPSSC = D3DCompileFromFile(
        pathSpellCircle.c_str(),
        nullptr,
        nullptr,
        "PSSpellCircle",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShaderSC,
        &error);
    if (FAILED(hrPSSC)) {
        if (error) {
            MessageBoxA(NULL, (char*)error->GetBufferPointer(), "Shader Compile Error (PSSpellCircle)", MB_OK);
        } else {
            char buf[128];
            sprintf_s(buf, "D3DCompileFromFile (PSSpellCircle) Failed with HRESULT: 0x%08X", hrPSSC);
            MessageBoxA(NULL, buf, "Shader Compile Error", MB_OK);
        }
        return false;
    }
    psoDesc.PS = { pixelShaderSC->GetBufferPointer(), pixelShaderSC->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0] = alphaBlendDesc;
    HRESULT hrPSOSC = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateSpellCircle));
    if (FAILED(hrPSOSC)) {
        char buf[128];
        sprintf_s(buf, "CreateGraphicsPipelineState (SpellCircle) Failed with HRESULT: 0x%08X", hrPSOSC);
        MessageBoxA(NULL, buf, "PSO Creation Error", MB_OK);
        return false;
    }

    return true;
}

/**
 * フレームの描画を開始する
 */
void D3D12RenderingService::BeginFrame() {
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineStateObject.Get());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, m_width, m_height };
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvCpuDescriptorHandle();
    const float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

/**
 * フレームの描画を終了する
 */
void D3D12RenderingService::EndFrame() {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    m_commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    m_swapChain->Present(1, 0);

    SyncFrame();
}

/**
 * フレームの同期を行う
 */
void D3D12RenderingService::SyncFrame() {
    const UINT64 fence = m_fenceValue;
    if (FAILED(m_commandQueue->Signal(m_fence.Get(), fence))) return;
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fence) {
        if (FAILED(m_fence->SetEventOnCompletion(fence, m_fenceEvent))) return;
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

/**
 * D3D12Renderer のレンダーターゲットビューの CPU ディスクリプタハンドルを取得する
 * @return レンダーターゲットビュー of CPU descriptor handle
 */
D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderingService::GetRtvCpuDescriptorHandle() const {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;
    return rtvHandle;
}

/**
 * パイプラインステートを切り替える (0: Object, 1: Background, 2: SpellCircle)
 */
void D3D12RenderingService::SetPipelineState(int type) {
    if (type == 0) {
        m_commandList->SetPipelineState(m_pipelineStateObject.Get());
    } else if (type == 1) {
        m_commandList->SetPipelineState(m_pipelineStateBackground.Get());
    } else if (type == 2) {
        m_commandList->SetPipelineState(m_pipelineStateSpellCircle.Get());
    }
}