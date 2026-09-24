#include <cstdio>
#define EMBEDDED_HLSL(...) #__VA_ARGS__
constexpr char PlayerShotShaderCode[] = EMBEDDED_HLSL(
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
        // HOMING
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
        if (u_shotType > 5.5f)
        {
            // PLAYER NORMAL: 敵通常弾の炎形状を青い発光へ差し替える
            color = lerp(float3(0.001f, 0.08f, 0.78f), float3(0.02f, 0.42f, 1.0f), flame);
            color = lerp(color, float3(0.68f, 0.94f, 1.0f), core) * pulse;
        }
        else if (u_shotType > 4.5f) color = 1.0f - saturate(color);
    }

    alpha *= saturate(u_color.a);
    if (alpha < 0.01f) discard;
    return float4(color, alpha);
}
);
constexpr char ExplosionShaderCode[] = EMBEDDED_HLSL(
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
);
constexpr char RailgunShaderCode[] = EMBEDDED_HLSL(
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
);
int main() {
FILE* PlayerShotShaderCodeFile=nullptr; fopen_s(&PlayerShotShaderCodeFile,"Temp/BombIntroShaders/PlayerShotShaderCode.after.hlsl","wb"); if(!PlayerShotShaderCodeFile) return 1; fwrite(PlayerShotShaderCode,1,sizeof(PlayerShotShaderCode)-1,PlayerShotShaderCodeFile); fclose(PlayerShotShaderCodeFile);
FILE* ExplosionShaderCodeFile=nullptr; fopen_s(&ExplosionShaderCodeFile,"Temp/BombIntroShaders/ExplosionShaderCode.after.hlsl","wb"); if(!ExplosionShaderCodeFile) return 1; fwrite(ExplosionShaderCode,1,sizeof(ExplosionShaderCode)-1,ExplosionShaderCodeFile); fclose(ExplosionShaderCodeFile);
FILE* RailgunShaderCodeFile=nullptr; fopen_s(&RailgunShaderCodeFile,"Temp/BombIntroShaders/RailgunShaderCode.after.hlsl","wb"); if(!RailgunShaderCodeFile) return 1; fwrite(RailgunShaderCode,1,sizeof(RailgunShaderCode)-1,RailgunShaderCodeFile); fclose(RailgunShaderCodeFile);
}
