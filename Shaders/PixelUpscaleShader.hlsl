Texture2D sceneTexture : register(t0);
SamplerState pointSampler : register(s0);

static const float COLOR_LEVELS = 16.0;
static const float DITHER_STRENGTH = 0.5;
static const float BAYER_4X4[16] =
{
     0.0 / 16.0,  8.0 / 16.0,  2.0 / 16.0, 10.0 / 16.0,
    12.0 / 16.0,  4.0 / 16.0, 14.0 / 16.0,  6.0 / 16.0,
     3.0 / 16.0, 11.0 / 16.0,  1.0 / 16.0,  9.0 / 16.0,
    15.0 / 16.0,  7.0 / 16.0, 13.0 / 16.0,  5.0 / 16.0
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

/**
 * @brief 表示値のRGBを指定段階数へ量子化する
 * @param color 0～1の表示値RGB
 * @return 0～1へ収めた量子化後のRGB
 */
float3 QuantizeColor(float3 color)
{
    // BackBufferはUNORMなので、表示値のまま等間隔に量子化する
    float intervals = COLOR_LEVELS - 1.0;
    return floor(saturate(color) * intervals + 0.5) / intervals;
}

/**
 * @brief 低解像度texel座標に対応する4x4 Bayer閾値を取得する
 * @param pixelPosition 低解像度RenderTargetのtexel座標
 * @return 0～1のBayer閾値
 */
float GetBayer4x4(int2 pixelPosition)
{
    int2 matrixPosition = pixelPosition & 3;
    return BAYER_4X4[matrixPosition.y * 4 + matrixPosition.x];
}

/**
 * @brief 量子化前のRGBへ中心化したBayer閾値を加える
 * @param color 表示値の量子化前RGB
 * @param pixelPosition 低解像度RenderTargetのtexel座標
 * @return 0～1へ収めたディザ適用後のRGB
 */
float3 ApplyBayerDither(float3 color, int2 pixelPosition)
{
    // 暗部と明部ではパターンを消し、階調補助が必要な中間輝度だけへ適用する
    float3 midtoneMask = smoothstep(0.03, 0.15, color) *
        (1.0 - smoothstep(0.85, 0.98, color));
    float3 dither = (GetBayer4x4(pixelPosition) + 0.5 / 16.0 - 0.5) *
        (DITHER_STRENGTH / (COLOR_LEVELS - 1.0)) * midtoneMask;
    return saturate(color + dither);
}

/**
 * @brief 拡大率と明るさに応じた走査線と緩やかな周辺減光を適用する
 * @param color 量子化後のRGB
 * @param uv 画面内の正規化座標
 * @param textureHeight 低解像度Textureの高さ
 * @return RGBへ掛ける明度倍率
 */
float ArcadeBrightness(float3 color, float2 uv, float textureHeight)
{
    // 画素幅で正弦波を平均し、等倍以下では走査線を消して干渉縞を抑える
    // 位相は2倍表示の画素が波のゼロ点に乗らない向きにする
    float row = uv.y * textureHeight;
    float footprint = max(fwidth(row), 0.0001);
    float phaseWidth = 3.14159265 * footprint;
    float filteredWave = sin(6.2831853 * row) * sin(phaseWidth) / phaseWidth;

    // 明るい画素ほど走査線を弱め、敵弾や爆発の明度を保つ
    float depth = 0.22 * (1.0 - 0.8 * max(color.r, max(color.g, color.b)));
    float scanline = 1.0 - depth * smoothstep(1.0, 2.0, 1.0 / footprint) *
        (0.5 + 0.5 * filteredWave);

    // 画面端でも最大8%の減光に留め、端を通る敵弾の明度を保つ
    float2 centered = uv * 2.0 - 1.0;
    return scanline * (1.0 - 0.04 * dot(centered, centered));
}

/**
 * @brief Fullscreen Triangleの頂点とUVを生成する
 * @param vertexId 頂点ID
 * @return クリップ座標とUV
 */
VS_OUTPUT VSMain(uint vertexId : SV_VertexID)
{
    VS_OUTPUT output;

    // 画面全体を覆う単一三角形を生成する
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    output.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    output.uv = uv;
    return output;
}

/**
 * @brief 低解像度Textureを減色してアーケード風に拡大する
 * @param input 補間されたクリップ座標とUV
 * @return 減色と走査線と周辺減光を適用した色
 */
float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    // 低解像度Textureのtexel座標を基準にディザを固定する
    uint textureWidth;
    uint textureHeight;
    sceneTexture.GetDimensions(textureWidth, textureHeight);
    int2 pixelPosition = int2(input.uv * float2(textureWidth, textureHeight));

    // Point Sampling後にディザ、量子化の順でRGBへ適用し、元のAlphaを維持する
    float4 sampled = sceneTexture.Sample(pointSampler, input.uv);
    float3 color = QuantizeColor(ApplyBayerDither(saturate(sampled.rgb), pixelPosition));
    return float4(color * ArcadeBrightness(color, input.uv, textureHeight), sampled.a);
}
