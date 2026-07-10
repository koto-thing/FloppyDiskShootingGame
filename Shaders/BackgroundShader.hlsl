struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 localPos : TEXCOORD0;
};

cbuffer TransformBuffer : register(b0)
{
    float4x4 u_wvpMatrix;
    float4 u_Color;
    float u_time;
    float u_shapeType;
    float u_rotAngle;
    float u_pad1;
};

float hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f);
}

// 宇宙空間 (背景の星空、遠くの無限遠の星)
float4 PSBackground(VS_OUTPUT input) : SV_TARGET
{
    float2 uv = input.localPos;
    
    // 深い宇宙のベース色
    float3 spaceColor = float3(0.01, 0.01, 0.03);
    
    // 星の描画 (静的な星、カメラのXZ移動で少し流れるとリアリティが出るが、背景は無限遠なのでゆっくり流す)
    float stars = 0.0;
    
    // 星のレイヤー
    for (int layer = 1; layer <= 3; ++layer)
    {
        float2 starUV = uv * (layer * 4.0);
        // カメラの左右移動 (u_position.x 等で間接的にスクロールさせる)
        starUV.x += u_Color.r * 0.0005 * layer; // 視差効果
        starUV.y += u_time * 0.002 * layer; // ゆっくり流れる
        
        float2 ipos = floor(starUV);
        float2 fpos = frac(starUV);
        float h = hash(ipos);
        
        if (h > 0.94)
        {
            // 瞬きアニメーション
            float pulse = sin(u_time * 0.05 + h * 6.283) * 0.4 + 0.6;
            float dist = length(fpos - float2(h, frac(h * 7.7)));
            stars += (smoothstep(0.05, 0.0, dist) * pulse) / float(layer);
        }
    }
    
    return float4(spaceColor + float3(stars, stars, stars), 1.0);
}
