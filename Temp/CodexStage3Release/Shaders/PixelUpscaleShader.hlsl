Texture2D sceneTexture : register(t0);
SamplerState pointSampler : register(s0);

static const float COLOR_LEVELS = 6.0;
static const float COLOR_GAMMA = 2.2;
static const float DITHER_STRENGTH = 0.03;
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
 * @brief RGBを指定段階数へ量子化する
 * @param color 0～1のRGB
 * @param levels チャンネルごとの段階数
 * @return 0～1へ収めた量子化後のRGB
 */
float3 QuantizeColor(float3 color, float levels)
{
    // 暗部を知覚色空間へ広げてから量子化し、黒潰れを抑える
    float3 perceptualColor = pow(saturate(color), 1.0 / COLOR_GAMMA);
    float intervals = levels - 1.0;
    float3 quantizedColor = floor(perceptualColor * intervals + 0.5) / intervals;
    return saturate(pow(quantizedColor, COLOR_GAMMA));
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
 * @param color 量子化前のRGB
 * @param pixelPosition 低解像度RenderTargetのtexel座標
 * @param strength ディザ強度
 * @return 0～1へ収めたディザ適用後のRGB
 */
float3 ApplyBayerDither(float3 color, int2 pixelPosition, float strength)
{
    // 暗部と明部ではパターンを消し、階調補助が必要な中間輝度だけへ適用する
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    float midtoneMask = smoothstep(0.08, 0.25, luminance) *
        (1.0 - smoothstep(0.75, 0.95, luminance));
    float dither = (GetBayer4x4(pixelPosition) - 0.5) * strength * midtoneMask;
    return saturate(color + dither);
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
 * @brief 低解像度Textureをポイントサンプリングする
 * @param input 補間されたクリップ座標とUV
 * @return サンプリングした色
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
    float3 ditheredColor = ApplyBayerDither(sampled.rgb, pixelPosition, DITHER_STRENGTH);
    return float4(QuantizeColor(ditheredColor, COLOR_LEVELS), sampled.a);
}
