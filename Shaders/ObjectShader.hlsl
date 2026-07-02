struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 localPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

cbuffer TransformBuffer : register(b0)
{
    float3 u_position;
    float u_pad1;
    float3 u_size;
    float u_pad2;
    float4 u_Color;
    float u_time;
    float u_shapeType; // 0: Plate (XZ面), 1: Cube, 2: Pyramid (戦闘機), 3: Sprite2D (XY面/UI)
};

// 単位立方体の36頂点 (Triangle List)
static const float3 cubeVertices[36] = {
    // Front face
    float3(-0.5, -0.5, -0.5), float3(-0.5,  0.5, -0.5), float3( 0.5,  0.5, -0.5),
    float3(-0.5, -0.5, -0.5), float3( 0.5,  0.5, -0.5), float3( 0.5, -0.5, -0.5),
    // Back face
    float3(-0.5, -0.5,  0.5), float3( 0.5,  0.5,  0.5), float3(-0.5,  0.5,  0.5),
    float3(-0.5, -0.5,  0.5), float3(-0.5,  0.5,  0.5), float3( 0.5, -0.5,  0.5),
    // Left face
    float3(-0.5, -0.5,  0.5), float3(-0.5,  0.5,  0.5), float3(-0.5,  0.5, -0.5),
    float3(-0.5, -0.5,  0.5), float3(-0.5,  0.5, -0.5), float3(-0.5, -0.5, -0.5),
    // Right face
    float3( 0.5, -0.5, -0.5), float3( 0.5,  0.5, -0.5), float3( 0.5,  0.5,  0.5),
    float3( 0.5, -0.5, -0.5), float3( 0.5,  0.5,  0.5), float3( 0.5, -0.5,  0.5),
    // Top face
    float3(-0.5,  0.5, -0.5), float3( 0.5,  0.5, -0.5), float3( 0.5,  0.5,  0.5),
    float3(-0.5,  0.5, -0.5), float3( 0.5,  0.5,  0.5), float3(-0.5,  0.5,  0.5),
    // Bottom face
    float3(-0.5, -0.5,  0.5), float3( 0.5, -0.5,  0.5), float3( 0.5, -0.5, -0.5),
    float3(-0.5, -0.5,  0.5), float3( 0.5, -0.5, -0.5), float3(-0.5, -0.5, -0.5)
};

static const float3 cubeNormals[36] = {
    float3(0,0,-1), float3(0,0,-1), float3(0,0,-1), float3(0,0,-1), float3(0,0,-1), float3(0,0,-1),
    float3(0,0, 1), float3(0,0, 1), float3(0,0, 1), float3(0,0, 1), float3(0,0, 1), float3(0,0, 1),
    float3(-1,0,0), float3(-1,0,0), float3(-1,0,0), float3(-1,0,0), float3(-1,0,0), float3(-1,0,0),
    float3( 1,0,0), float3( 1,0,0), float3( 1,0,0), float3( 1,0,0), float3( 1,0,0), float3( 1,0,0),
    float3(0, 1,0), float3(0, 1,0), float3(0, 1,0), float3(0, 1,0), float3(0, 1,0), float3(0, 1,0),
    float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0)
};

// 三角錐 (Pyramid) 18頂点 (底面6頂点＋側面12頂点)
static const float3 pyramidVertices[18] = {
    // Bottom Face (2 triangles)
    float3(-0.5, -0.3, -0.5), float3( 0.5, -0.3,  0.5), float3( 0.5, -0.3, -0.5),
    float3(-0.5, -0.3, -0.5), float3(-0.5, -0.3,  0.5), float3( 0.5, -0.3,  0.5),
    // Front Face (tip is 0, 0.5, 0)
    float3(-0.5, -0.3, -0.5), float3( 0.0,  0.5,  0.0), float3( 0.5, -0.3, -0.5),
    // Back Face
    float3(-0.5, -0.3,  0.5), float3( 0.5, -0.3,  0.5), float3( 0.0,  0.5,  0.0),
    // Left Face
    float3(-0.5, -0.3, -0.5), float3(-0.5, -0.3,  0.5), float3( 0.0,  0.5,  0.0),
    // Right Face
    float3( 0.5, -0.3, -0.5), float3( 0.0,  0.5,  0.0), float3( 0.5, -0.3,  0.5)
};

static const float3 pyramidNormals[18] = {
    float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0),
    float3(0, 0.447, -0.894), float3(0, 0.447, -0.894), float3(0, 0.447, -0.894),
    float3(0, 0.447,  0.894), float3(0, 0.447,  0.894), float3(0, 0.447,  0.894),
    float3(-0.894, 0.447, 0), float3(-0.894, 0.447, 0), float3(-0.894, 0.447, 0),
    float3( 0.894, 0.447, 0), float3( 0.894, 0.447, 0), float3( 0.894, 0.447, 0)
};

VS_OUTPUT VSMain(uint vID : SV_VertexID)
{
    VS_OUTPUT output;
    float3 localPos = float3(0,0,0);
    float3 normal = float3(0,1,0);

    if (u_shapeType < 0.5) {
        // 0: Plate (XZ平面、地面グリッドなど)
        float2 rawPos;
        rawPos.x = float(vID & 2) - 1.0f;
        rawPos.y = float((vID & 1) << 1) - 1.0f;
        localPos = float3(rawPos.x, 0.0f, rawPos.y); 
        output.localPos = rawPos;
        normal = float3(0, 1, 0);
    }
    else if (u_shapeType < 1.5) {
        // 1: Cube
        localPos = cubeVertices[vID % 36];
        normal = cubeNormals[vID % 36];
        output.localPos = localPos.xy;
    }
    else if (u_shapeType < 2.5) {
        // 2: Pyramid (Arwing/敵機など)
        localPos = pyramidVertices[vID % 18];
        normal = pyramidNormals[vID % 18];
        output.localPos = localPos.xy;
    }
    else {
        // 3: Sprite2D (XY平面、UIや爆発スプライトなど)
        float2 rawPos;
        rawPos.x = float(vID & 2) - 1.0f;
        rawPos.y = float((vID & 1) << 1) - 1.0f;
        localPos = float3(rawPos.x, rawPos.y, 0.0f);
        output.localPos = rawPos;
        normal = float3(0, 0, -1);
    }

    // 3Dスケール
    float3 worldPos = u_position + localPos * u_size;

    // C++側ですでにカメラ相対 (View空間) に変換されていることを前提とする
    float3 viewPos = worldPos;

    float fov = 1.35f;
    float aspect = 800.0f / 600.0f;
    float zNear = 0.5f;

    if (viewPos.z <= zNear) {
        // クリッピング
        output.pos = float4(0, 0, -10.0, 1.0);
        output.color = float4(0, 0, 0, 0);
        return output;
    }

    // 透視投影
    output.pos.x = (viewPos.x / viewPos.z) * fov / aspect;
    output.pos.y = (viewPos.y / viewPos.z) * fov;
    output.pos.z = saturate((viewPos.z - zNear) / 800.0f);
    output.pos.w = 1.0f;

    // ライト計算 (フラットシェーディング平行光源)
    float3 lightDir = normalize(float3(0.4, 0.8, -0.4));
    float diff = saturate(dot(normal, lightDir)) * 0.45 + 0.55;

    output.color = float4(u_Color.rgb * diff, u_Color.a);
    output.normal = normal;
    return output;
}

float4 PSObject(VS_OUTPUT input) : SV_TARGET
{
    if (input.color.a < 0.01) discard;
    return input.color;
}
