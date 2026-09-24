
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
    if (u_shapeType > 4.5f)
    {
        // 金色レーザーの白熱した芯を脈動させる
        float endMask = 1.0f - smoothstep(0.90f, 1.0f, abs(input.uv.x));
        float core = 1.0f - smoothstep(0.015f, 0.085f, abs(input.uv.y));
        float innerGlow = 1.0f - smoothstep(0.04f, 0.52f, abs(input.uv.y));
        float pulse = 0.84f + 0.16f * sin(u_progress * 6.283185f);
        float alpha = saturate(core + innerGlow * 0.76f) * endMask * pulse;
        if (alpha < 0.01f) discard;
        float3 color = lerp(float3(1.0f, 0.42f, 0.015f),
            float3(1.0f, 1.0f, 0.82f), core);
        return float4(color, alpha);
    }

    if (u_shapeType > 3.5f)
    {
        // 金色レーザーの外周へ広い加算発光を作る
        float endMask = 1.0f - smoothstep(0.86f, 1.0f, abs(input.uv.x));
        float halo = 1.0f - smoothstep(0.04f, 0.92f, abs(input.uv.y));
        float ripple = 0.82f + 0.18f * sin(input.uv.x * 54.0f + u_progress * 6.283185f);
        float alpha = halo * endMask * ripple * 0.38f;
        if (alpha < 0.008f) discard;
        return float4(1.0f, 0.55f, 0.035f, alpha);
    }

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
