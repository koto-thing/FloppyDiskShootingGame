struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 localPos : TEXCOORD0;
};

cbuffer TransformBuffer : register(b0)
{
    float3 u_position;
    float u_pad1;
    float3 u_size;
    float u_pad2;
    float4 u_Color;
    float u_time;
    float u_shapeType;
};

// SFレトロレーダー / 照準HUDエフェクト
float4 PSSpellCircle(VS_OUTPUT input) : SV_TARGET
{
    float2 uv = input.localPos;
    float d = length(uv);
    if (d > 1.0) discard;
    
    float theta = atan2(uv.y, uv.x);
    float pulse = sin(u_time * 0.05) * 0.1 + 0.9;
    
    // ロックオンカーソル風の同心円と照準十字線
    float circle1 = smoothstep(0.02, 0.0, abs(d - 0.8));
    float circle2 = smoothstep(0.015, 0.0, abs(d - 0.4));
    
    // 十字架線 (中心部は空ける)
    float crossLine = (smoothstep(0.015, 0.0, abs(uv.x)) + smoothstep(0.015, 0.0, abs(uv.y))) 
                      * step(0.15, d) * step(d, 0.9);
                      
    // 外周の目盛り
    float ticks = smoothstep(0.02, 0.0, abs(d - 0.82)) * step(0.85, frac(theta * 8.0 / 6.283));
    
    float pattern = saturate(circle1 + circle2 + crossLine + ticks);
    if (pattern < 0.05) discard;
    
    return float4(u_Color.rgb * pulse, pattern * u_Color.a);
}
