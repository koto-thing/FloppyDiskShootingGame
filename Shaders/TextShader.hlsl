cbuffer TransformBuffer : register(b0) {
    float2 u_position;
    float2 u_size;
    float4 u_Color;
    float4 u_uvOffsetScale;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct VS_OUTPUT {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VS_OUTPUT VSTextMain(uint vID : SV_VertexID) {
    VS_OUTPUT output;
    float2 localPos;
    localPos.x = (float)(vID & 2) - 1.0f;
    localPos.y = (float)((vID & 1) << 1) - 1.0f;
    
    float2 finalPos = u_position + (localPos * u_size);
    output.pos = float4(finalPos, 0.0f, 1.0f);
    
    // 頂点IDからUV（0.0～1.0）を生成し、文字に応じたUV範囲に変形
    float2 rawUV = float2((vID & 2) ? 1.0f : 0.0f, (vID & 1) ? 0.0f : 1.0f);
    output.uv = u_uvOffsetScale.xy + (rawUV * u_uvOffsetScale.zw);
    return output;
}

float4 PSTextMain(VS_OUTPUT input) : SV_TARGET {
    // Rチャンネルから文字の有無をサンプリング
    float alpha = g_texture.Sample(g_sampler, input.uv).r;
    clip(alpha - 0.5f);
    return u_Color;
}