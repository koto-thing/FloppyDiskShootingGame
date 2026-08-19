struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 localPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
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

// 三角柱 (Triangular Prism: 24頂点)
static const float3 prismVertices[24] =
{
    float3(-0.5, 0.5, -0.5), float3(0.0, 0.5, 0.5), float3(0.5, 0.5, -0.5),
    float3(-0.5, -0.5, -0.5), float3(0.5, -0.5, -0.5), float3(0.0, -0.5, 0.5),
    float3(-0.5, -0.5, -0.5), float3(-0.5, 0.5, -0.5), float3(0.5, 0.5, -0.5),
    float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, -0.5), float3(0.5, -0.5, -0.5),
    float3(0.0, -0.5, 0.5), float3(0.0, 0.5, 0.5), float3(-0.5, 0.5, -0.5),
    float3(0.0, -0.5, 0.5), float3(-0.5, 0.5, -0.5), float3(-0.5, -0.5, -0.5),
    float3(0.5, -0.5, -0.5), float3(0.5, 0.5, -0.5), float3(0.0, 0.5, 0.5),
    float3(0.5, -0.5, -0.5), float3(0.0, 0.5, 0.5), float3(0.0, -0.5, 0.5)
};

static const float3 prismNormals[24] =
{
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, 0, -1), float3(0, 0, -1), float3(0, 0, -1),
    float3(0, 0, -1), float3(0, 0, -1), float3(0, 0, -1),
    float3(-0.8944, 0, 0.4472), float3(-0.8944, 0, 0.4472), float3(-0.8944, 0, 0.4472),
    float3(-0.8944, 0, 0.4472), float3(-0.8944, 0, 0.4472), float3(-0.8944, 0, 0.4472),
    float3(0.8944, 0, 0.4472), float3(0.8944, 0, 0.4472), float3(0.8944, 0, 0.4472),
    float3(0.8944, 0, 0.4472), float3(0.8944, 0, 0.4472), float3(0.8944, 0, 0.4472)
};
// 円柱 (Cylinder: 72頂点)
static const float3 cylinderVertices[72] =
{
    float3(0.5000, -0.5, 0.0000), float3(0.5000, 0.5, 0.0000), float3(0.3536, 0.5, 0.3536),
    float3(0.5000, -0.5, 0.0000), float3(0.3536, 0.5, 0.3536), float3(0.3536, -0.5, 0.3536),
    float3(0.3536, -0.5, 0.3536), float3(0.3536, 0.5, 0.3536), float3(0.0000, 0.5, 0.5000),
    float3(0.3536, -0.5, 0.3536), float3(0.0000, 0.5, 0.5000), float3(0.0000, -0.5, 0.5000),
    float3(0.0000, -0.5, 0.5000), float3(0.0000, 0.5, 0.5000), float3(-0.3536, 0.5, 0.3536),
    float3(0.0000, -0.5, 0.5000), float3(-0.3536, 0.5, 0.3536), float3(-0.3536, -0.5, 0.3536),
    float3(-0.3536, -0.5, 0.3536), float3(-0.3536, 0.5, 0.3536), float3(-0.5000, 0.5, 0.0000),
    float3(-0.3536, -0.5, 0.3536), float3(-0.5000, 0.5, 0.0000), float3(-0.5000, -0.5, 0.0000),
    float3(-0.5000, -0.5, 0.0000), float3(-0.5000, 0.5, 0.0000), float3(-0.3536, 0.5, -0.3536),
    float3(-0.5000, -0.5, 0.0000), float3(-0.3536, 0.5, -0.3536), float3(-0.3536, -0.5, -0.3536),
    float3(-0.3536, -0.5, -0.3536), float3(-0.3536, 0.5, -0.3536), float3(0.0000, 0.5, -0.5000),
    float3(-0.3536, -0.5, -0.3536), float3(0.0000, 0.5, -0.5000), float3(0.0000, -0.5, -0.5000),
    float3(0.0000, -0.5, -0.5000), float3(0.0000, 0.5, -0.5000), float3(0.3536, 0.5, -0.3536),
    float3(0.0000, -0.5, -0.5000), float3(0.3536, 0.5, -0.3536), float3(0.3536, -0.5, -0.3536),
    float3(0.3536, -0.5, -0.3536), float3(0.3536, 0.5, -0.3536), float3(0.5000, 0.5, 0.0000),
    float3(0.3536, -0.5, -0.3536), float3(0.5000, 0.5, 0.0000), float3(0.5000, -0.5, 0.0000),
    float3(0, 0.5, 0), float3(0.5000, 0.5, 0.0000), float3(0.3536, 0.5, 0.3536),
    float3(0, 0.5, 0), float3(0.3536, 0.5, 0.3536), float3(0.0000, 0.5, 0.5000),
    float3(0, 0.5, 0), float3(0.0000, 0.5, 0.5000), float3(-0.3536, 0.5, 0.3536),
    float3(0, 0.5, 0), float3(-0.3536, 0.5, 0.3536), float3(-0.5000, 0.5, 0.0000),
    float3(0, -0.5, 0), float3(0.3536, -0.5, 0.3536), float3(0.5000, -0.5, 0.0000),
    float3(0, -0.5, 0), float3(0.0000, -0.5, 0.5000), float3(0.3536, -0.5, 0.3536),
    float3(0, -0.5, 0), float3(-0.3536, -0.5, 0.3536), float3(0.0000, -0.5, 0.5000),
    float3(0, -0.5, 0), float3(-0.5000, -0.5, 0.0000), float3(-0.3536, -0.5, 0.3536)
};

static const float3 cylinderNormals[72] =
{
    float3(1.0000, 0.0, 0.0000), float3(1.0000, 0.0, 0.0000), float3(0.7071, 0.0, 0.7071),
    float3(1.0000, 0.0, 0.0000), float3(0.7071, 0.0, 0.7071), float3(0.7071, 0.0, 0.7071),
    float3(0.7071, 0.0, 0.7071), float3(0.7071, 0.0, 0.7071), float3(0.0000, 0.0, 1.0000),
    float3(0.7071, 0.0, 0.7071), float3(0.0000, 0.0, 1.0000), float3(0.0000, 0.0, 1.0000),
    float3(0.0000, 0.0, 1.0000), float3(0.0000, 0.0, 1.0000), float3(-0.7071, 0.0, 0.7071),
    float3(0.0000, 0.0, 1.0000), float3(-0.7071, 0.0, 0.7071), float3(-0.7071, 0.0, 0.7071),
    float3(-0.7071, 0.0, 0.7071), float3(-0.7071, 0.0, 0.7071), float3(-1.0000, 0.0, 0.0000),
    float3(-0.7071, 0.0, 0.7071), float3(-1.0000, 0.0, 0.0000), float3(-1.0000, 0.0, 0.0000),
    float3(-1.0000, 0.0, 0.0000), float3(-1.0000, 0.0, 0.0000), float3(-0.7071, 0.0, -0.7071),
    float3(-1.0000, 0.0, 0.0000), float3(-0.7071, 0.0, -0.7071), float3(-0.7071, 0.0, -0.7071),
    float3(-0.7071, 0.0, -0.7071), float3(-0.7071, 0.0, -0.7071), float3(0.0000, 0.0, -1.0000),
    float3(-0.7071, 0.0, -0.7071), float3(0.0000, 0.0, -1.0000), float3(0.0000, 0.0, -1.0000),
    float3(0.0000, 0.0, -1.0000), float3(0.0000, 0.0, -1.0000), float3(0.7071, 0.0, -0.7071),
    float3(0.0000, 0.0, -1.0000), float3(0.7071, 0.0, -0.7071), float3(0.7071, 0.0, -0.7071),
    float3(0.7071, 0.0, -0.7071), float3(0.7071, 0.0, -0.7071), float3(1.0000, 0.0, 0.0000),
    float3(0.7071, 0.0, -0.7071), float3(1.0000, 0.0, 0.0000), float3(1.0000, 0.0, 0.0000),
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0)
};

// 円錐 (Cone: 48頂点)
static const float3 coneVertices[48] =
{
    float3(0.5000, -0.5, 0.0000), float3(0.0, 0.5, 0.0), float3(0.3536, -0.5, 0.3536),
    float3(0.3536, -0.5, 0.3536), float3(0.0, 0.5, 0.0), float3(0.0000, -0.5, 0.5000),
    float3(0.0000, -0.5, 0.5000), float3(0.0, 0.5, 0.0), float3(-0.3536, -0.5, 0.3536),
    float3(-0.3536, -0.5, 0.3536), float3(0.0, 0.5, 0.0), float3(-0.5000, -0.5, 0.0000),
    float3(-0.5000, -0.5, 0.0000), float3(0.0, 0.5, 0.0), float3(-0.3536, -0.5, -0.3536),
    float3(-0.3536, -0.5, -0.3536), float3(0.0, 0.5, 0.0), float3(0.0000, -0.5, -0.5000),
    float3(0.0000, -0.5, -0.5000), float3(0.0, 0.5, 0.0), float3(0.3536, -0.5, -0.3536),
    float3(0.3536, -0.5, -0.3536), float3(0.0, 0.5, 0.0), float3(0.5000, -0.5, 0.0000),
    float3(0, -0.5, 0), float3(0.3536, -0.5, 0.3536), float3(0.5000, -0.5, 0.0000),
    float3(0, -0.5, 0), float3(0.0000, -0.5, 0.5000), float3(0.3536, -0.5, 0.3536),
    float3(0, -0.5, 0), float3(-0.3536, -0.5, 0.3536), float3(0.0000, -0.5, 0.5000),
    float3(0, -0.5, 0), float3(-0.5000, -0.5, 0.0000), float3(-0.3536, -0.5, 0.3536),
    float3(0, -0.5, 0), float3(-0.3536, -0.5, -0.3536), float3(-0.5000, -0.5, 0.0000),
    float3(0, -0.5, 0), float3(0.0000, -0.5, -0.5000), float3(-0.3536, -0.5, -0.3536),
    float3(0, -0.5, 0), float3(0.3536, -0.5, -0.3536), float3(0.0000, -0.5, -0.5000),
    float3(0, -0.5, 0), float3(0.5000, -0.5, 0.0000), float3(0.3536, -0.5, -0.3536)
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
        // 2: Pyramid
        localPos = pyramidVertices[vID % 18];
        normal = pyramidNormals[vID % 18];
        output.localPos = localPos.xy;
    }
    else if (u_shapeType < 3.5)
    {
// 3: Cylinder (96頂点)
        localPos = cylinderVertices[vID % 72];
        normal = cylinderNormals[vID % 72];
        output.localPos = localPos.xz;
    }
    else if (u_shapeType < 4.5)
    {
        // 4: Cone (48頂点) - 先端は (0, 0.5, 0) [+Y方向]
        uint idx = vID % 48;
        localPos = coneVertices[idx];

        if (idx < 24)
        {
            // Y軸を高さ方向とする側面法線
            float2 dirXZ = normalize(localPos.xz);
            normal = normalize(float3(dirXZ.x * 1.0f, 0.5f, dirXZ.y * 1.0f));
        }
        else
        {
            // 底面法線 (-Y方向)
            normal = float3(0.0f, -1.0f, 0.0f);
        }
        output.localPos = localPos.xz;
    }
    else if (u_shapeType < 5.5)
    {
        // 5: 三角柱 (24頂点)
        localPos = prismVertices[vID % 24];
        normal = prismNormals[vID % 24];
        output.localPos = localPos.xy;
    }
    else {
        // 6: Sprite2D (XY平面、UIや爆発スプライトなど)
        float2 rawPos;
        rawPos.x = float(vID & 2) - 1.0f;
        rawPos.y = float((vID & 1) << 1) - 1.0f;
        localPos = float3(rawPos.x, rawPos.y, 0.0f);
        output.localPos = rawPos;
        normal = float3(0, 0, -1);
    }
    
    // WVP行列による座標変換
    output.pos = mul(float4(localPos, 1.0f), u_wvpMatrix);

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
