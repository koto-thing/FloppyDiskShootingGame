#include "D3D12RenderingService.h"

namespace {
/** @brief 画像を使わず自機弾と敵弾を生成する埋め込みHLSL */
constexpr char PlayerShotShaderCode[] = R"hlsl(
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer ShotBuffer : register(b0)
{
    float4x4 u_transform;
    float4 u_color;
    float u_time;
    float u_shotType;
    float u_direction;
    float u_padding;
};

VS_OUTPUT VSPlayerShot(uint vertexId : SV_VertexID)
{
    VS_OUTPUT output;
    float2 localPosition;
    localPosition.x = float(vertexId & 2) - 1.0f;
    localPosition.y = float((vertexId & 1) << 1) - 1.0f;

    output.position = mul(float4(localPosition, 0.0f, 1.0f), u_transform);
    output.uv = localPosition;
    return output;
}

float4 PSPlayerShot(VS_OUTPUT input) : SV_TARGET
{
    float2 uv = input.uv;
    float pulse = 0.88f + sin(u_time * 0.18f) * 0.12f;
    float3 color;
    float alpha;

    if (u_shotType < 0.5f)
    {
        // HOMING: 明るい弾頭と後方へ細く消える青緑色の尾
        float headDistance = length((uv - float2(0.38f, 0.0f)) * float2(1.0f, 1.35f));
        float head = 1.0f - smoothstep(0.18f, 0.68f, headDistance);
        float tailWidth = 0.08f + saturate((uv.x + 1.0f) * 0.24f);
        float tail = (1.0f - smoothstep(tailWidth, tailWidth + 0.18f, abs(uv.y))) *
            saturate(1.0f - (uv.x - 0.10f) * 0.75f) * saturate(uv.x + 1.0f);
        float core = 1.0f - smoothstep(0.02f, 0.12f, abs(uv.y));
        alpha = saturate(head + tail * 0.72f);
        color = lerp(float3(0.02f, 0.45f, 0.95f), float3(0.75f, 1.0f, 1.0f),
            saturate(head + core * tail)) * pulse;
    }
    else if (u_shotType < 1.5f)
    {
        // PIERCING: 白い芯を紫の発光が包む細長いレーザー
        float endMask = 1.0f - smoothstep(0.72f, 1.0f, abs(uv.x));
        float core = 1.0f - smoothstep(0.02f, 0.13f, abs(uv.y));
        float glow = 1.0f - smoothstep(0.08f, 0.72f, abs(uv.y));
        alpha = saturate((core + glow * 0.65f) * endMask);
        color = lerp(float3(0.52f, 0.05f, 1.0f), float3(1.0f, 0.95f, 1.0f), core) * pulse;
    }
    else if (u_shotType < 2.5f)
    {
        // SPREAD: 中心核と発光リングを持つ黄色の小型光弾
        float distanceFromCenter = length(uv);
        float core = 1.0f - smoothstep(0.05f, 0.34f, distanceFromCenter);
        float body = 1.0f - smoothstep(0.30f, 0.78f, distanceFromCenter);
        float ring = 1.0f - smoothstep(0.035f, 0.13f, abs(distanceFromCenter - 0.58f));
        alpha = saturate(body + ring * 0.72f);
        color = lerp(float3(1.0f, 0.78f, 0.02f), float3(1.0f, 1.0f, 0.72f),
            saturate(core + ring * 0.35f)) * pulse;
    }
    else if (u_shotType < 3.5f)
    {
        // NORMAL: 機首中央から飛ぶ白青色の小型レーザー
        float endMask = 1.0f - smoothstep(0.74f, 1.0f, abs(uv.x));
        float core = 1.0f - smoothstep(0.02f, 0.19f, abs(uv.y));
        float glow = 1.0f - smoothstep(0.10f, 0.74f, abs(uv.y));
        alpha = saturate((core + glow * 0.48f) * endMask);
        color = lerp(float3(0.05f, 0.58f, 1.0f), float3(0.92f, 1.0f, 1.0f), core) * pulse;
    }
    else
    {
        // ENEMY: 白熱した芯を橙色の炎と赤い揺らぎが包む敵弾
        float2 p = float2(uv.x, uv.y * 1.45f);
        float edgeNoise = sin(p.x * 18.0f + u_time * 0.21f) * 0.035f +
            sin(p.x * 31.0f - u_time * 0.13f) * 0.018f;
        float distanceFromCore = length(float2(p.x * 0.82f, p.y + edgeNoise));
        float endMask = 1.0f - smoothstep(0.72f, 1.02f, abs(p.x));
        float outerGlow = (1.0f - smoothstep(0.28f, 0.92f, distanceFromCore)) * endMask;
        float flame = (1.0f - smoothstep(0.14f, 0.58f, distanceFromCore)) * endMask;
        float core = (1.0f - smoothstep(0.035f, 0.20f, distanceFromCore)) *
            (1.0f - smoothstep(0.42f, 0.82f, abs(p.x)));
        alpha = saturate(outerGlow * 0.52f + flame * 0.72f + core);
        color = lerp(float3(0.72f, 0.002f, 0.001f), float3(1.0f, 0.075f, 0.008f), flame);
        color = lerp(color, float3(1.0f, 0.82f, 0.62f), core) * pulse;
        if (u_shotType > 4.5f) color = 1.0f - saturate(color);
    }

    if (alpha < 0.01f) discard;
    return float4(color, alpha);
}
)hlsl";

/** @brief 敵命中時に火花と衝撃波を描く埋め込みHLSL */
constexpr char ExplosionShaderCode[] = R"hlsl(
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer ExplosionBuffer : register(b0)
{
    float4x4 u_transform;
    float4 u_color;
    float u_progress;
    float u_shapeType;
    float u_rotation;
    float u_padding;
};

VS_OUTPUT VSExplosion(uint vertexId : SV_VertexID)
{
    VS_OUTPUT output;
    float2 localPosition;
    localPosition.x = float(vertexId & 2) - 1.0f;
    localPosition.y = float((vertexId & 1) << 1) - 1.0f;
    output.position = mul(float4(localPosition, 0.0f, 1.0f), u_transform);
    output.uv = localPosition;
    return output;
}

float4 PSExplosion(VS_OUTPUT input) : SV_TARGET
{
    if (u_shapeType > 4.5f)
    {
        // ボム専用の青白い中心光と膨張する光輪を生成する
        float progress = saturate(u_progress);
        float distanceFromCenter = length(input.uv);
        float core = 1.0f - smoothstep(0.02f, 0.30f + progress * 0.22f, distanceFromCenter);
        float ringRadius = 0.16f + progress * 0.80f;
        float ring = 1.0f - smoothstep(0.025f, 0.10f, abs(distanceFromCenter - ringRadius));
        float alpha = saturate((core + ring) * (1.0f - progress * 0.82f));
        if (alpha < 0.01f) discard;
        float3 color = lerp(float3(0.02f, 0.20f, 1.0f), float3(0.72f, 0.94f, 1.0f), core);
        return float4(color, alpha);
    }

    if (u_shapeType > 3.5f)
    {
        // 迫撃砲着弾時に地表を走る高温の衝撃波を生成する
        float progress = saturate(u_progress);
        float2 uv = input.uv;
        float distanceFromCenter = length(float2(uv.x * 0.72f, uv.y * 2.45f));
        float ringRadius = 0.18f + progress * 0.82f;
        float ring = 1.0f - smoothstep(0.025f, 0.10f, abs(distanceFromCenter - ringRadius));
        float innerHeat = 1.0f - smoothstep(0.02f, 0.42f + progress * 0.22f, distanceFromCenter);
        float roughness = sin(atan2(uv.y, uv.x) * 17.0f + progress * 24.0f) * 0.5f + 0.5f;
        float alpha = saturate((ring * (0.70f + roughness * 0.30f) + innerHeat * 0.20f) *
            (1.0f - progress));
        if (alpha < 0.01f) discard;
        float3 color = lerp(float3(1.0f, 0.12f, 0.01f), float3(1.0f, 0.90f, 0.28f),
            saturate(innerHeat + ring * 0.45f));
        return float4(color, alpha);
    }

    if (u_shapeType > 2.5f)
    {
        // 戦艦下面から下向きに噴射する補助エンジン炎を生成する
        float2 uv = input.uv;
        float distanceFromNozzle = saturate((1.0f - uv.y) * 0.5f);
        float flicker = sin(u_progress * 19.0f + uv.y * 7.0f) * 0.08f +
            sin(u_progress * 31.0f - uv.y * 11.0f) * 0.04f;
        float flameLength = 0.90f + flicker;
        float width = lerp(0.72f, 0.05f, distanceFromNozzle) *
            (0.92f + sin(u_progress * 23.0f + distanceFromNozzle * 18.0f) * 0.08f);
        float body = 1.0f - smoothstep(width * 0.70f, width, abs(uv.x));
        float tip = 1.0f - smoothstep(flameLength - 0.12f, flameLength, distanceFromNozzle);
        float core = (1.0f - smoothstep(0.02f, max(0.06f, width * 0.46f), abs(uv.x))) *
            (1.0f - smoothstep(0.28f, 0.76f, distanceFromNozzle));
        float alpha = saturate(body * tip * (0.78f + core * 0.42f));
        if (alpha < 0.01f) discard;
        float3 color = lerp(float3(1.0f, 0.08f, 0.005f), float3(1.0f, 0.52f, 0.025f),
            1.0f - distanceFromNozzle);
        color = lerp(color, float3(1.0f, 0.96f, 0.62f), core);
        return float4(color, alpha);
    }

    if (u_shapeType > 1.5f)
    {
        // 撃破後半は膨張しながら上昇する黒煙を複数の塊で描く
        float progress = saturate(u_progress);
        float2 uv = input.uv;
        float rise = progress * 0.72f;
        float spread = 0.12f + progress * 0.34f;
        float2 p0 = uv - float2(-spread, -0.22f + rise);
        float2 p1 = uv - float2(spread * 0.75f, -0.05f + rise * 1.12f);
        float2 p2 = uv - float2(sin(progress * 13.0f) * 0.12f, 0.22f + rise * 0.82f);
        float smoke = 1.0f - smoothstep(0.22f, 0.58f, length(p0));
        smoke += 1.0f - smoothstep(0.20f, 0.54f, length(p1));
        smoke += 1.0f - smoothstep(0.18f, 0.50f, length(p2));
        float fade = smoothstep(0.02f, 0.14f, progress) * (1.0f - smoothstep(0.68f, 1.0f, progress));
        float alpha = saturate(smoke) * fade * 0.88f;
        if (alpha < 0.02f) discard;
        float shade = 0.025f + saturate(smoke) * 0.055f + progress * 0.035f;
        return float4(shade.xxx, alpha);
    }

    if (u_shapeType > 0.5f)
    {
        // 上昇速度の異なる円を重ね、テクスチャなしで煙の揺らぎを作る
        float time = u_progress;
        float2 uv = input.uv;
        float2 p0 = uv - float2(sin(time * 1.7f) * 0.16f, -0.38f + frac(time * 0.23f) * 1.35f);
        float2 p1 = uv - float2(cos(time * 1.3f + 1.8f) * 0.22f, -0.55f + frac(time * 0.19f + 0.42f) * 1.45f);
        float2 p2 = uv - float2(sin(time * 1.1f + 3.2f) * 0.18f, -0.48f + frac(time * 0.17f + 0.73f) * 1.40f);
        float smoke = (1.0f - smoothstep(0.20f, 0.55f, length(p0))) * 0.58f;
        smoke += (1.0f - smoothstep(0.18f, 0.50f, length(p1))) * 0.50f;
        smoke += (1.0f - smoothstep(0.16f, 0.46f, length(p2))) * 0.42f;
        float alpha = saturate(smoke) * saturate(1.0f - (uv.y + 0.25f) * 0.28f);
        if (alpha < 0.02f) discard;
        float shade = saturate(0.20f + uv.y * 0.13f + smoke * 0.18f);
        return float4(shade.xxx, alpha * 0.72f);
    }

    float distanceFromCenter = length(input.uv);
    float progress = saturate(u_progress);
    float core = 1.0f - smoothstep(0.03f, 0.34f + progress * 0.18f, distanceFromCenter);
    float ringRadius = 0.16f + progress * 0.78f;
    float ring = 1.0f - smoothstep(0.025f, 0.12f, abs(distanceFromCenter - ringRadius));
    float sparks = saturate(sin(atan2(input.uv.y, input.uv.x) * 9.0f + progress * 28.0f) * 0.5f + 0.5f);
    float alpha = saturate((core + ring * (0.75f + sparks * 0.25f)) * (1.0f - progress));
    if (alpha < 0.01f) discard;
    float3 color = lerp(float3(1.0f, 0.10f, 0.01f), float3(1.0f, 0.92f, 0.35f), core);
    return float4(color, alpha);
}
)hlsl";

/** @brief 瞬間発光して細く消えるレールガン軌跡の埋め込みHLSL */
constexpr char RailgunShaderCode[] = R"hlsl(
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer RailgunBuffer : register(b0)
{
    float4x4 u_transform;
    float4 u_color;
    float u_progress;
    float u_shapeType;
    float u_rotation;
    float u_padding;
};

VS_OUTPUT VSRailgun(uint vertexId : SV_VertexID)
{
    VS_OUTPUT output;
    float2 localPosition;
    localPosition.x = float(vertexId & 2) - 1.0f;
    localPosition.y = float((vertexId & 1) << 1) - 1.0f;
    output.position = mul(float4(localPosition, 0.0f, 1.0f), u_transform);
    output.uv = localPosition;
    return output;
}

float4 PSRailgun(VS_OUTPUT input) : SV_TARGET
{
    if (u_shapeType > 2.5f)
    {
        // チャージ進行に合わせて予告レーザーを濃くする
        float charge = saturate(u_progress);
        float endMask = 1.0f - smoothstep(0.94f, 1.0f, abs(input.uv.x));
        float lineMask = 1.0f - smoothstep(0.08f, 0.72f, abs(input.uv.y));
        float core = 1.0f - smoothstep(0.02f, 0.18f, abs(input.uv.y));
        float alpha = (lineMask * 0.34f + core * 0.46f) * endMask * charge;
        if (alpha < 0.01f) discard;
        return float4(lerp(float3(1.0f, 0.03f, 0.01f),
            float3(1.0f, 0.72f, 0.30f), core * charge), alpha);
    }

    if (u_shapeType > 1.5f)
    {
        // 加熱された空気が軌跡周辺で蛇行する蜃気楼状の揺らぎを作る
        float fade = pow(saturate(1.0f - u_progress), 1.35f);
        float wave0 = sin(input.uv.x * 42.0f + u_progress * 31.0f) * 0.12f;
        float wave1 = sin(input.uv.x * 67.0f - u_progress * 23.0f) * 0.07f;
        float upperBand = 1.0f - smoothstep(0.025f, 0.13f, abs(input.uv.y - 0.34f - wave0));
        float lowerBand = 1.0f - smoothstep(0.025f, 0.13f, abs(input.uv.y + 0.34f - wave1));
        float centerHaze = 1.0f - smoothstep(0.05f, 0.72f, abs(input.uv.y + wave0 * 0.35f));
        float endMask = 1.0f - smoothstep(0.88f, 1.0f, abs(input.uv.x));
        float alpha = saturate((upperBand + lowerBand) * 0.16f + centerHaze * 0.055f) * endMask * fade;
        if (alpha < 0.008f) discard;
        float shimmer = 0.5f + 0.5f * sin(input.uv.x * 83.0f + u_progress * 37.0f);
        return float4(lerp(float3(0.18f, 0.12f, 0.06f), float3(0.72f, 0.56f, 0.32f), shimmer), alpha);
    }

    if (u_shapeType > 0.5f)
    {
        // 発射予測位置を示す細い赤色レーザーポインタを弱く明滅させる
        float endMask = 1.0f - smoothstep(0.96f, 1.0f, abs(input.uv.x));
        float lineMask = 1.0f - smoothstep(0.05f, 0.24f, abs(input.uv.y));
        float pulse = 0.55f + 0.30f * sin(u_progress * 18.0f) * sin(u_progress * 18.0f);
        float alpha = lineMask * endMask * pulse;
        if (alpha < 0.01f) discard;
        return float4(1.0f, 0.015f, 0.005f, alpha);
    }

    // 発射直後の白い芯と橙色の残光を作り、短時間で急速に減衰させる
    float fade = pow(saturate(1.0f - u_progress), 2.4f);
    float endMask = 1.0f - smoothstep(0.90f, 1.0f, abs(input.uv.x));
    float core = 1.0f - smoothstep(0.015f, 0.075f, abs(input.uv.y));
    float glow = 1.0f - smoothstep(0.04f, 0.62f, abs(input.uv.y));
    float flicker = 0.88f + 0.12f * sin(input.uv.x * 91.0f + u_progress * 47.0f);
    float alpha = saturate(core + glow * 0.68f) * endMask * fade * flicker;
    if (alpha < 0.01f) discard;
    float3 color = lerp(float3(1.0f, 0.10f, 0.01f), float3(1.0f, 0.98f, 0.72f), core);
    return float4(color, alpha);
}
)hlsl";

/** @brief 2D描画でも共有する256バイト定数バッファの先頭部分 */
struct RendererTransformBufferData {
    DirectX::XMFLOAT4X4 u_wvpMatrix;
    DirectX::XMFLOAT4 u_Color;
    float u_time;
    float u_shapeType;
    float u_rotAngle;
    float u_pad[41];
};
}
#include "../../Engine/Diagnostics/Debug.h"
#include <cstdio>
#include <vector>

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
        Debug::LogError("D3D12RenderingService::InitD3D12 failed");
        MessageBoxA(NULL, "D3D12Renderer::Initialize - InitD3D12 Failed!", "Error", MB_OK);
        return false;
    }
    if (!InitializeLowResolutionRenderTarget()) {
        Debug::LogError("D3D12RenderingService::InitializeLowResolutionRenderTarget failed");
        return false;
    }
    if (!InitPipeline()) {
        Debug::LogError("D3D12RenderingService::InitPipeline failed");
        MessageBoxA(NULL, "D3D12Renderer::Initialize - InitPipeline Failed!", "Error", MB_OK);
        return false;
    }

    // スワップチェーン（バックバッファ）の実際のフォーマットを取得して渡す
    DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (m_renderTargets[0]) {
        rtvFormat = m_renderTargets[0]->GetDesc().Format;
    }

    // テキストレンダリングサービスの初期化
    if (!m_textRenderer.Initialize(m_device.Get(), rtvFormat)) {
        Debug::LogError("TextRenderingService::Initialize failed");
        MessageBoxA(NULL, "D3D12Renderer::Initialize - m_textRenderer.Initialize Failed!", "Error", MB_OK);
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
        Debug::LogHResult("D3D12CreateDevice failed for hardware and WARP adapters", hr);
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
    rtvHeapDesc.NumDescriptors = 3;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        MessageBoxA(NULL, "CreateDescriptorHeap (RTV) Failed", "Error", MB_OK);
        return false;
    }
    
    // Depth Stencil View用のDescriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 2;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)))) {
        MessageBoxA(NULL, "CreateDescriptorHeap (DSV) Failed", "Error", MB_OK);
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
    
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = static_cast<UINT64>(m_width);
    depthDesc.Height = static_cast<UINT>(m_height);
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;
    
    D3D12_HEAP_PROPERTIES depthHeapProps = {};
    depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    
    HRESULT depthHr = m_device->CreateCommittedResource(
        &depthHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&m_depthStencil)
    );
    
    if (FAILED(depthHr)) {
        MessageBoxA(NULL, "CreateCommittedResource (DepthStencil) Failed", "Error", MB_OK);
        return false;
    }
    
    // 
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // レトロ映像効果用の低解像度Depth Bufferを同じ定義から生成する
    depthDesc.Width = LOW_RES_WIDTH;
    depthDesc.Height = LOW_RES_HEIGHT;
    depthHr = m_device->CreateCommittedResource(
        &depthHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&m_lowResolutionDepthStencil));
    if (FAILED(depthHr)) {
        MessageBoxA(NULL, "CreateCommittedResource (Low Resolution DepthStencil) Failed", "Error", MB_OK);
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE lowResolutionDsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    lowResolutionDsvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_device->CreateDepthStencilView(m_lowResolutionDepthStencil.Get(), &dsvDesc, lowResolutionDsvHandle);

    D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvCpuData)))) {
        MessageBoxA(NULL, "ConstantBuffer Map Failed", "Error", MB_OK);
        return false;
    }

    return true;
}

/**
 * @brief 低解像度描画用のRenderTarget、Depth Buffer、Descriptorを生成する
 * @return 生成に成功した場合true
 */
bool D3D12RenderingService::InitializeLowResolutionRenderTarget() {
    // BackBuffer互換形式の低解像度RenderTargetを生成する
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = LOW_RES_WIDTH;
    textureDesc.Height = LOW_RES_HEIGHT;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = m_renderTargets[0]->GetDesc().Format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = textureDesc.Format;
    clearValue.Color[0] = 0.05f;
    clearValue.Color[1] = 0.05f;
    clearValue.Color[2] = 0.1f;
    clearValue.Color[3] = 1.0f;

    if (FAILED(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&m_lowResolutionRenderTarget)))) {
        MessageBoxA(NULL, "CreateCommittedResource (Low Resolution RenderTarget) Failed", "Error", MB_OK);
        return false;
    }

    // RTV Heapの3番目へ低解像度RenderTargetのRTVを生成する
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(2) * m_rtvDescriptorSize;
    m_device->CreateRenderTargetView(m_lowResolutionRenderTarget.Get(), nullptr, rtvHandle);

    // 拡大描画で参照するShader Visible SRVを生成する
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_lowResolutionSrvHeap)))) {
        MessageBoxA(NULL, "CreateDescriptorHeap (Low Resolution SRV) Failed", "Error", MB_OK);
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(
        m_lowResolutionRenderTarget.Get(),
        &srvDesc,
        m_lowResolutionSrvHeap->GetCPUDescriptorHandleForHeapStart());
    return true;
}

/**
 * D3D12 パイプラインを初期化する
 * @return 初期化に成功した場合：true、失敗した場合：false
 */
bool D3D12RenderingService::InitPipeline() {
    // ルートパラメータの定義 (CBVとSRV)
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // SRV用
    D3D12_DESCRIPTOR_RANGE descRange = { };
    descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRange.NumDescriptors = 1;
    descRange.BaseShaderRegister = 0;
    
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descRange;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    // 文字がボケないようにポイントサンプラーを定義
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = { };
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &samplerDesc;
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
    std::wstring pathUpscale = GetShaderFilePath(L"Shaders\\PixelUpscaleShader.hlsl");

    // 共通頂点シェーダーのコンパイル
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

    // 奥側のポリゴンを除外して閉じた3Dメッシュを正しく表示するPSOを作成する
    D3D12_GRAPHICS_PIPELINE_STATE_DESC model3DPsoDesc = psoDesc;
    model3DPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    model3DPsoDesc.DepthStencilState.DepthEnable = TRUE;
    model3DPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    model3DPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    model3DPsoDesc.DepthStencilState.StencilEnable = FALSE;
    model3DPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    
    HRESULT hrPSOModel3D = m_device->CreateGraphicsPipelineState(
        &model3DPsoDesc, IID_PPV_ARGS(&m_pipelineStateModel3D));
    if (FAILED(hrPSOModel3D)) {
        char buf[128];
        sprintf_s(buf, "CreateGraphicsPipelineState (Model3D) Failed with HRESULT: 0x%08X", hrPSOModel3D);
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

    /** @brief C++文字列から自機弾専用シェーダーをコンパイルする */
    ComPtr<ID3DBlob> playerShotVertexShader;
    ComPtr<ID3DBlob> playerShotPixelShader;
    error.Reset();
    const UINT shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    HRESULT hrPlayerShotVS = D3DCompile(
        PlayerShotShaderCode, sizeof(PlayerShotShaderCode) - 1, "EmbeddedPlayerShotShader",
        nullptr, nullptr, "VSPlayerShot", "vs_5_0", shaderCompileFlags, 0,
        &playerShotVertexShader, &error);
    if (FAILED(hrPlayerShotVS)) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Player Shot Shader Compile Error (VS)", MB_OK);
        return false;
    }

    error.Reset();
    HRESULT hrPlayerShotPS = D3DCompile(
        PlayerShotShaderCode, sizeof(PlayerShotShaderCode) - 1, "EmbeddedPlayerShotShader",
        nullptr, nullptr, "PSPlayerShot", "ps_5_0", shaderCompileFlags, 0,
        &playerShotPixelShader, &error);
    if (FAILED(hrPlayerShotPS)) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Player Shot Shader Compile Error (PS)", MB_OK);
        return false;
    }

    /** @brief 発光を重ねられる加算ブレンドの自機弾PSOを作成する */
    D3D12_GRAPHICS_PIPELINE_STATE_DESC playerShotPsoDesc = psoDesc;
    playerShotPsoDesc.VS = {
        playerShotVertexShader->GetBufferPointer(), playerShotVertexShader->GetBufferSize() };
    playerShotPsoDesc.PS = {
        playerShotPixelShader->GetBufferPointer(), playerShotPixelShader->GetBufferSize() };
    playerShotPsoDesc.BlendState.RenderTarget[0] = {
        TRUE, FALSE,
        D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    if (FAILED(m_device->CreateGraphicsPipelineState(
        &playerShotPsoDesc, IID_PPV_ARGS(&m_pipelineStatePlayerShot)))) {
        MessageBoxA(NULL, "CreateGraphicsPipelineState (PlayerShot) Failed",
            "PSO Creation Error", MB_OK);
        return false;
    }

    /** @brief C++文字列から命中爆発用シェーダーをコンパイルする */
    ComPtr<ID3DBlob> explosionVertexShader;
    ComPtr<ID3DBlob> explosionPixelShader;
    error.Reset();
    HRESULT hrExplosionVS = D3DCompile(
        ExplosionShaderCode, sizeof(ExplosionShaderCode) - 1, "EmbeddedExplosionShader",
        nullptr, nullptr, "VSExplosion", "vs_5_0", shaderCompileFlags, 0,
        &explosionVertexShader, &error);
    if (FAILED(hrExplosionVS)) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Explosion Shader Compile Error (VS)", MB_OK);
        return false;
    }

    error.Reset();
    HRESULT hrExplosionPS = D3DCompile(
        ExplosionShaderCode, sizeof(ExplosionShaderCode) - 1, "EmbeddedExplosionShader",
        nullptr, nullptr, "PSExplosion", "ps_5_0", shaderCompileFlags, 0,
        &explosionPixelShader, &error);
    if (FAILED(hrExplosionPS)) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Explosion Shader Compile Error (PS)", MB_OK);
        return false;
    }

    /** @brief 加算ブレンドで発光する爆発用PSOを作成する */
    D3D12_GRAPHICS_PIPELINE_STATE_DESC explosionPsoDesc = playerShotPsoDesc;
    explosionPsoDesc.VS = { explosionVertexShader->GetBufferPointer(), explosionVertexShader->GetBufferSize() };
    explosionPsoDesc.PS = { explosionPixelShader->GetBufferPointer(), explosionPixelShader->GetBufferSize() };
    explosionPsoDesc.BlendState.RenderTarget[0] = {
        TRUE, FALSE,
        D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    if (FAILED(m_device->CreateGraphicsPipelineState(
        &explosionPsoDesc, IID_PPV_ARGS(&m_pipelineStateExplosion)))) {
        MessageBoxA(NULL, "CreateGraphicsPipelineState (Explosion) Failed",
            "PSO Creation Error", MB_OK);
        return false;
    }

    /** @brief C++文字列からレールガン軌跡シェーダーをコンパイルする */
    ComPtr<ID3DBlob> railgunVertexShader;
    ComPtr<ID3DBlob> railgunPixelShader;
    error.Reset();
    HRESULT hrRailgunVS = D3DCompile(
        RailgunShaderCode, sizeof(RailgunShaderCode) - 1, "EmbeddedRailgunShader",
        nullptr, nullptr, "VSRailgun", "vs_5_0", shaderCompileFlags, 0,
        &railgunVertexShader, &error);
    if (FAILED(hrRailgunVS)) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Railgun Shader Compile Error (VS)", MB_OK);
        return false;
    }

    error.Reset();
    HRESULT hrRailgunPS = D3DCompile(
        RailgunShaderCode, sizeof(RailgunShaderCode) - 1, "EmbeddedRailgunShader",
        nullptr, nullptr, "PSRailgun", "ps_5_0", shaderCompileFlags, 0,
        &railgunPixelShader, &error);
    if (FAILED(hrRailgunPS)) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Railgun Shader Compile Error (PS)", MB_OK);
        return false;
    }

    // レールガン軌跡は加算ブレンドで背景へ発光を重ねる
    D3D12_GRAPHICS_PIPELINE_STATE_DESC railgunPsoDesc = playerShotPsoDesc;
    railgunPsoDesc.VS = {railgunVertexShader->GetBufferPointer(), railgunVertexShader->GetBufferSize()};
    railgunPsoDesc.PS = {railgunPixelShader->GetBufferPointer(), railgunPixelShader->GetBufferSize()};
    if (FAILED(m_device->CreateGraphicsPipelineState(
        &railgunPsoDesc, IID_PPV_ARGS(&m_pipelineStateRailgun)))) {
        MessageBoxA(NULL, "CreateGraphicsPipelineState (Railgun) Failed",
            "PSO Creation Error", MB_OK);
        return false;
    }

    // 黒煙は背景を暗く覆える通常アルファブレンドで合成する
    explosionPsoDesc.BlendState.RenderTarget[0] = {
        TRUE, FALSE,
        D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    if (FAILED(m_device->CreateGraphicsPipelineState(
        &explosionPsoDesc, IID_PPV_ARGS(&m_pipelineStateExplosionSmoke)))) {
        MessageBoxA(NULL, "CreateGraphicsPipelineState (Explosion Smoke) Failed",
            "PSO Creation Error", MB_OK);
        return false;
    }

    // Fullscreen Triangleで低解像度RenderTargetを拡大するPSOを生成する
    ComPtr<ID3DBlob> upscaleVertexShader;
    ComPtr<ID3DBlob> upscalePixelShader;
    error.Reset();
    if (FAILED(D3DCompileFromFile(
        pathUpscale.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", shaderCompileFlags, 0,
        &upscaleVertexShader, &error))) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Upscale Shader Compile Error (VS)", MB_OK);
        return false;
    }

    error.Reset();
    if (FAILED(D3DCompileFromFile(
        pathUpscale.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", shaderCompileFlags, 0,
        &upscalePixelShader, &error))) {
        if (error) MessageBoxA(NULL, static_cast<char*>(error->GetBufferPointer()),
            "Upscale Shader Compile Error (PS)", MB_OK);
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC upscalePsoDesc = psoDesc;
    upscalePsoDesc.VS = { upscaleVertexShader->GetBufferPointer(), upscaleVertexShader->GetBufferSize() };
    upscalePsoDesc.PS = { upscalePixelShader->GetBufferPointer(), upscalePixelShader->GetBufferSize() };
    upscalePsoDesc.BlendState.RenderTarget[0] = opaqueBlendDesc;
    if (FAILED(m_device->CreateGraphicsPipelineState(
        &upscalePsoDesc, IID_PPV_ARGS(&m_pipelineStateUpscale)))) {
        MessageBoxA(NULL, "CreateGraphicsPipelineState (Upscale) Failed", "PSO Creation Error", MB_OK);
        return false;
    }

    return true;
}

/**
 * フレームの描画を開始する
 */
void D3D12RenderingService::BeginFrame() {
    // フレーム途中のRenderTarget切替を避けるため、要求値をフレーム単位で確定する
    m_frameRetroEffectEnabled = m_retroEffectEnabled;
    // フレームごとに定数バッファのインデックスをリセット
    m_constantBufferCursor = 0;
    m_currentPipelineType = 0;
    m_sceneUpscaled = false;
    
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineStateObject.Get());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    const UINT renderWidth = m_frameRetroEffectEnabled ? LOW_RES_WIDTH : static_cast<UINT>(m_width);
    const UINT renderHeight = m_frameRetroEffectEnabled ? LOW_RES_HEIGHT : static_cast<UINT>(m_height);
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(renderWidth), static_cast<float>(renderHeight), 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(renderWidth), static_cast<LONG>(renderHeight) };
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_frameRetroEffectEnabled
        ? m_lowResolutionRenderTarget.Get() : m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = m_frameRetroEffectEnabled
        ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_frameRetroEffectEnabled
        ? m_rtvHeap->GetCPUDescriptorHandleForHeapStart() : GetRtvCpuDescriptorHandle();
    if (m_frameRetroEffectEnabled) rtvHandle.ptr += static_cast<SIZE_T>(2) * m_rtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    if (m_frameRetroEffectEnabled) {
        dsvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }
    constexpr float clearColor[] = { 0.05f, 0.05f, 0.1f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

/**
 * フレームの描画を終了する
 */
void D3D12RenderingService::EndFrame() {
    if (m_frameRetroEffectEnabled && !m_sceneUpscaled) DrawLowResolutionToBackBuffer();

    // BackBufferをPresent可能な状態へ戻す
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
 * @brief 低解像度RenderTargetをBackBufferへポイント拡大する
 * @return なし
 */
void D3D12RenderingService::DrawLowResolutionToBackBuffer() {
    // 低解像度RenderTargetをSRV、BackBufferをRTVへ遷移する
    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = m_lowResolutionRenderTarget.Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(_countof(barriers), barriers);

    // ウィンドウ全体へ低解像度TextureをFullscreen Triangleで拡大する
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, m_width, m_height };
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvCpuDescriptorHandle();
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_commandList->SetPipelineState(m_pipelineStateUpscale.Get());
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_lowResolutionSrvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    m_commandList->SetGraphicsRootDescriptorTable(
        1, m_lowResolutionSrvHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->DrawInstanced(3, 1, 0, 0);
    m_sceneUpscaled = true;
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
 * パイプラインステートを切り替える (0: Object, 1: Background, 2: SpellCircle, 3: Model3D, 4: PlayerShot, 5: Explosion)
 */
void D3D12RenderingService::SetPipelineState(int type) {
    /** @brief 文字描画後に元のパイプラインを復元するため選択を保持する */
    m_currentPipelineType = type;
    if (type == 0) {
        m_commandList->SetPipelineState(m_pipelineStateObject.Get());
    } else if (type == 1) {
        m_commandList->SetPipelineState(m_pipelineStateBackground.Get());
    } else if (type == 2) {
        m_commandList->SetPipelineState(m_pipelineStateSpellCircle.Get());
    } else if (type == 3) {
        m_commandList->SetPipelineState(m_pipelineStateModel3D.Get());
    } else if (type == 4) {
        m_commandList->SetPipelineState(m_pipelineStatePlayerShot.Get());
    } else if (type == 5) {
        m_commandList->SetPipelineState(m_pipelineStateExplosion.Get());
    }
}

/**
 * @brief Rendererのパイプライン識別子を既存のD3D12パイプラインへ変換する
 */
void D3D12RenderingService::SetPipeline(PipelineId pipeline) {
    switch (pipeline) {
    case PipelineId::Object:
        SetPipelineState(0);
        break;
    case PipelineId::Background:
        SetPipelineState(1);
        break;
    case PipelineId::SpellCircle:
        SetPipelineState(2);
        break;
    case PipelineId::Model3D:
        SetPipelineState(3);
        break;
    }
}

void D3D12RenderingService::SetCamera(const CameraMatrices& matrices, const Viewport& viewport) {
    m_cameraMatrices = matrices;
    m_cameraViewport = viewport;
    m_hasCamera = viewport.IsValid();
    if (m_hasCamera) {
        // ウィンドウ基準のカメラ領域を低解像度RenderTargetへ写像する
        const float scaleX = m_frameRetroEffectEnabled && !m_sceneUpscaled
            ? static_cast<float>(LOW_RES_WIDTH) / static_cast<float>(m_width) : 1.0f;
        const float scaleY = m_frameRetroEffectEnabled && !m_sceneUpscaled
            ? static_cast<float>(LOW_RES_HEIGHT) / static_cast<float>(m_height) : 1.0f;
        const LONG left = static_cast<LONG>(viewport.x * scaleX);
        const LONG top = static_cast<LONG>(viewport.y * scaleY);
        const LONG right = static_cast<LONG>((viewport.x + viewport.width) * scaleX);
        const LONG bottom = static_cast<LONG>((viewport.y + viewport.height) * scaleY);
        D3D12_VIEWPORT d3dViewport{
            static_cast<float>(left), static_cast<float>(top),
            static_cast<float>(right - left), static_cast<float>(bottom - top), 0.0f, 1.0f};
        D3D12_RECT scissor{left, top, right, bottom};
        m_commandList->RSSetViewports(1, &d3dViewport);
        m_commandList->RSSetScissorRects(1, &scissor);
    }
}

void D3D12RenderingService::ResetCamera() {
    // ワールド描画を確定し、以降のHUDをBackBufferへ直接描画する
    if (m_frameRetroEffectEnabled && !m_sceneUpscaled) DrawLowResolutionToBackBuffer();
    m_hasCamera = false;
    D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    D3D12_RECT scissor{0, 0, m_width, m_height};
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissor);
}

/**
 * @brief 2D描画用の定数バッファを書き込み、プロシージャル形状を描画する
 */
void D3D12RenderingService::DrawUiPrimitive(
    float x,
    float y,
    float width,
    float height,
    float z,
    const ColorF& color,
    float shapeType,
    UINT vertexCount) {
    if (m_constantBufferCursor >= MAX_CONSTANT_BUFFER_ELEMENTS) return;

    /** @brief 4頂点で構成するUIプリミティブ用の三角形ストリップを設定する */
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // NDC上の中心、サイズを既存シェーダーの単位形状へ変換する
    auto* cbData = reinterpret_cast<RendererTransformBufferData*>(
        reinterpret_cast<char*>(m_cbvCpuData) + static_cast<size_t>(m_constantBufferCursor) * 256);
    DirectX::XMMATRIX matrix = DirectX::XMMatrixScaling(width, height, 1.0f) *
        DirectX::XMMatrixTranslation(x, y, z);
    DirectX::XMStoreFloat4x4(&cbData->u_wvpMatrix, DirectX::XMMatrixTranspose(matrix));
    cbData->u_Color = {color.r, color.g, color.b, color.a};
    cbData->u_time = 0.0f;
    cbData->u_shapeType = shapeType;
    cbData->u_rotAngle = 0.0f;

    // 定数バッファをバインドして、形状に応じた頂点数を発行する
    const D3D12_GPU_VIRTUAL_ADDRESS address = m_constantBuffer->GetGPUVirtualAddress() +
        static_cast<UINT64>(m_constantBufferCursor) * 256;
    m_commandList->SetGraphicsRootConstantBufferView(0, address);
    m_commandList->DrawInstanced(vertexCount, 1, 0, 0);
    ++m_constantBufferCursor;
}

/**
 * @brief NDC矩形を描画する
 */
void D3D12RenderingService::DrawRect(const Rect& rect, const ColorF& color) {
    SetPipeline(PipelineId::Object);

    DrawUiPrimitive(
        rect.position.x,
        rect.position.y,
        rect.size.x,
        rect.size.y,
        0.0f,
        color,
        6.0f,
        4
    );
}

void D3D12RenderingService::DrawPrimitive3D(const Primitive3D& primitive) {
    if (m_constantBufferCursor >= MAX_CONSTANT_BUFFER_ELEMENTS) return;

    // 頂点配列の構成に応じてGPUのプリミティブトポロジーを切り替える
    const bool usesTriangleStrip = primitive.shape == PrimitiveShape::Plate ||
        primitive.shape == PrimitiveShape::Sprite2D;
    m_commandList->IASetPrimitiveTopology(
        usesTriangleStrip ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    struct PrimitiveBuffer {
        DirectX::XMFLOAT4X4 wvp;
        DirectX::XMFLOAT4 color;
        float time;
        float shapeType;
        float rotationAngle;
        float padding[41];
    };

    float shapeType = 1.0f;
    UINT vertexCount = 36;
    switch (primitive.shape) {
    case PrimitiveShape::Plate: shapeType = 0.0f; vertexCount = 4; break;
    case PrimitiveShape::Box: shapeType = 1.0f; vertexCount = 36; break;
    case PrimitiveShape::Sphere: shapeType = 7.0f; vertexCount = 576; break;
    case PrimitiveShape::Cylinder: shapeType = 3.0f; vertexCount = 96; break;
    case PrimitiveShape::Cone: shapeType = 4.0f; vertexCount = 48; break;
    case PrimitiveShape::Prism: shapeType = 5.0f; vertexCount = 24; break;
    case PrimitiveShape::Sprite2D: shapeType = 6.0f; vertexCount = 4; break;
    }

    auto* cbData = reinterpret_cast<PrimitiveBuffer*>(
        reinterpret_cast<char*>(m_cbvCpuData) + static_cast<size_t>(m_constantBufferCursor) * 256);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            cbData->wvp.m[row][column] = primitive.wvpMatrix.m[row][column];
        }
    }
    cbData->color = {primitive.color.r, primitive.color.g, primitive.color.b, primitive.color.a};
    cbData->time = 0.0f;
    cbData->shapeType = shapeType;
    cbData->rotationAngle = primitive.rotationAngle;
    const auto address = m_constantBuffer->GetGPUVirtualAddress() +
        static_cast<UINT64>(m_constantBufferCursor) * 256;
    m_commandList->SetGraphicsRootConstantBufferView(0, address);
    m_commandList->DrawInstanced(vertexCount, 1, 0, 0);
    ++m_constantBufferCursor;
}

/** @brief 埋め込みHLSLを使用して自機弾を描画する */
void D3D12RenderingService::DrawPlayerShot(const PlayerShotVisual& shot) {
    if (m_constantBufferCursor >= MAX_CONSTANT_BUFFER_ELEMENTS) return;

    /** @brief 自機弾の位置と大きさを定数バッファへ設定する */
    auto* cbData = reinterpret_cast<RendererTransformBufferData*>(
        reinterpret_cast<char*>(m_cbvCpuData) + static_cast<size_t>(m_constantBufferCursor) * 256);
    const DirectX::XMMATRIX matrix = DirectX::XMMatrixScaling(shot.size.x, shot.size.y, 1.0f) *
        DirectX::XMMatrixRotationZ(shot.direction) *
        DirectX::XMMatrixTranslation(shot.position.x, shot.position.y, 0.0f);
    DirectX::XMStoreFloat4x4(&cbData->u_wvpMatrix, DirectX::XMMatrixTranspose(matrix));
    cbData->u_Color = {1.0f, 1.0f, 1.0f, 1.0f};
    cbData->u_time = shot.time;
    cbData->u_shapeType = static_cast<float>(shot.type);
    cbData->u_rotAngle = shot.direction;

    /** @brief 自機弾専用PSOで4頂点の矩形を描画する */
    const int previousPipelineType = m_currentPipelineType;
    m_commandList->SetPipelineState(m_pipelineStatePlayerShot.Get());
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const D3D12_GPU_VIRTUAL_ADDRESS address = m_constantBuffer->GetGPUVirtualAddress() +
        static_cast<UINT64>(m_constantBufferCursor) * 256;
    m_commandList->SetGraphicsRootConstantBufferView(0, address);
    m_commandList->DrawInstanced(4, 1, 0, 0);
    ++m_constantBufferCursor;
    SetPipelineState(previousPipelineType);
}

/** @brief 埋め込みHLSLを使用して爆発エフェクトを描画する */
void D3D12RenderingService::DrawExplosion(const ExplosionVisual& explosion) {
    if (m_constantBufferCursor >= MAX_CONSTANT_BUFFER_ELEMENTS) return;

    // 3Dカメラ込みの変換行列と進行率を定数バッファへ設定する
    auto* cbData = reinterpret_cast<RendererTransformBufferData*>(
        reinterpret_cast<char*>(m_cbvCpuData) + static_cast<size_t>(m_constantBufferCursor) * 256);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            cbData->u_wvpMatrix.m[row][column] = explosion.wvpMatrix.m[row][column];
        }
    }
    cbData->u_Color = {1.0f, 1.0f, 1.0f, 1.0f};
    cbData->u_time = explosion.progress;
    cbData->u_shapeType = static_cast<float>(explosion.effectType);
    cbData->u_rotAngle = 0.0f;

    // 爆発専用PSOで画面正対クアッドを発行する
    const int previousPipelineType = m_currentPipelineType;
    m_commandList->SetPipelineState((explosion.effectType == 0 || explosion.effectType == 3 ||
        explosion.effectType == 4 || explosion.effectType == 5) ?
        m_pipelineStateExplosion.Get() : m_pipelineStateExplosionSmoke.Get());
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const D3D12_GPU_VIRTUAL_ADDRESS address = m_constantBuffer->GetGPUVirtualAddress() +
        static_cast<UINT64>(m_constantBufferCursor) * 256;
    m_commandList->SetGraphicsRootConstantBufferView(0, address);
    m_commandList->DrawInstanced(4, 1, 0, 0);
    ++m_constantBufferCursor;
    SetPipelineState(previousPipelineType);
}

/** @brief 埋め込みHLSLを使用してレールガン軌跡を描画する */
void D3D12RenderingService::DrawRailgun(const RailgunVisual& railgun) {
    if (m_constantBufferCursor >= MAX_CONSTANT_BUFFER_ELEMENTS) return;

    // 3D空間の軌跡Transformと消滅進行率を定数バッファへ設定する
    auto* cbData = reinterpret_cast<RendererTransformBufferData*>(
        reinterpret_cast<char*>(m_cbvCpuData) + static_cast<size_t>(m_constantBufferCursor) * 256);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            cbData->u_wvpMatrix.m[row][column] = railgun.wvpMatrix.m[row][column];
        }
    }
    cbData->u_Color = {1.0f, 1.0f, 1.0f, 1.0f};
    cbData->u_time = railgun.progress;
    cbData->u_shapeType = static_cast<float>(railgun.effectType);
    cbData->u_rotAngle = 0.0f;

    // 専用加算ブレンドPSOで軌跡クアッドを描画する
    const int previousPipelineType = m_currentPipelineType;
    m_commandList->SetPipelineState(m_pipelineStateRailgun.Get());
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const D3D12_GPU_VIRTUAL_ADDRESS address = m_constantBuffer->GetGPUVirtualAddress() +
        static_cast<UINT64>(m_constantBufferCursor) * 256;
    m_commandList->SetGraphicsRootConstantBufferView(0, address);
    m_commandList->DrawInstanced(4, 1, 0, 0);
    ++m_constantBufferCursor;
    SetPipelineState(previousPipelineType);
}

/**
 * @brief NDC円を照準用プロシージャルシェーダーで描画する
 */
void D3D12RenderingService::DrawCircle(const Circle& circle, const ColorF& color) {
    SetPipeline(PipelineId::SpellCircle);
    const float diameter = circle.radius < 0.0f ? -circle.radius * 2.0f : circle.radius * 2.0f;
    DrawUiPrimitive(circle.center.x, circle.center.y, diameter, diameter, 0.0f,
                    color, 6.0f, 4);
}

/**
 * Siv3D風の簡易テキスト描画インターフェース
 */
void D3D12RenderingService::RenderText(const char* text, DirectX::XMFLOAT2 position, float size,
                                        DirectX::XMFLOAT4 color, float characterSpacing) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    
    const size_t length = std::strlen(text);
    
    // 定数バッファの範囲外アクセスを防止する
    if (length > MAX_CONSTANT_BUFFER_ELEMENTS - m_constantBufferCursor) {
        Debug::LogWarning("RenderText: constant buffer capacity exceeded");
        
        return;
    }
    
    const UINT startIndex = m_constantBufferCursor;
    
    D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = 
        m_constantBuffer->GetGPUVirtualAddress() +
            static_cast<UINT64>(startIndex) * 256;
    
    void* cbvCpuPtr = 
        reinterpret_cast<char*>(m_cbvCpuData) + 
            static_cast<size_t>(startIndex) * 256;
    
    m_textRenderer.RenderText(
        m_commandList.Get(),
        cbvGpuAddress,
        cbvCpuPtr,
        text,
        position.x,
        position.y,
        size,
        color,
        characterSpacing,
        m_width,
        m_height
    );
    
    // 次のテキストが使用する位置を自動更新
    m_constantBufferCursor += static_cast<UINT>(length);
    /** @brief フォント専用ルートシグネチャとPSOが後続のモデルやUIへ漏れないよう復元する */
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    SetPipelineState(m_currentPipelineType);
}

/**
 * @brief Rendererの文字コマンドを既存の文字描画処理へ転送する
 */
void D3D12RenderingService::DrawTextCommand(
    std::string_view text,
    const Vector2& position,
    float size,
    const ColorF& color,
    float characterSpacing) {
    // ResetCameraを使わないUI専用Sceneでも文字だけはBackBuffer解像度を維持する
    if (m_frameRetroEffectEnabled && !m_sceneUpscaled) DrawLowResolutionToBackBuffer();
    std::string ownedText(text);
    RenderText(ownedText.c_str(), {position.x, position.y}, size, {color.r, color.g, color.b, color.a},
               characterSpacing);
}

