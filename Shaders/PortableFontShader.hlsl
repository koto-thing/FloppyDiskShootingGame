// Portable editions use the same 8x8 ASCII glyphs without a typed buffer texture
cbuffer FontBuffer : register(b0)
{
    float4 u_posSize;
    float4 u_color;
    float4 u_rows0;
    float4 u_rows1;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

/**
 * @brief 文字のTriangle Strip頂点を生成する
 * @param vertexId 頂点ID
 * @return クリップ座標と文字内UV
 */
VS_OUTPUT VSMain(uint vertexId : SV_VertexID)
{
    // DirectX版と同じ左下、左上、右下、右上の順で生成する
    float2 localPosition = float2(float(vertexId & 2) - 1.0,
        float((vertexId & 1) << 1) - 1.0);
    VS_OUTPUT output;
    output.position = float4(u_posSize.xy + localPosition * u_posSize.zw, 0.0, 1.0);
    output.uv = float2((vertexId & 2) ? 1.0 : 0.0, (vertexId & 1) ? 0.0 : 1.0);
    return output;
}

/**
 * @brief CPUから受け取った8行のビットマップで文字を描画する
 * @param input 補間された文字内UV
 * @return 文字の色
 */
float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    // 各行を正確に表せる0～255のfloatで転送し、WebGL 2のUBOで共有する
    uint x = uint(clamp(input.uv.x * 8.0, 0.0, 7.0));
    uint y = uint(clamp(input.uv.y * 8.0, 0.0, 7.0));
    uint row = uint(y < 4 ? u_rows0[y] : u_rows1[y - 4]);
    if ((row & (0x80 >> x)) == 0) discard;
    return u_color;
}
