/*
shapeType = 0 → XZ平面の板
shapeType = 1 → 四角柱
shapeType = 2 → ピラミッド
shapeType = 3 → 円柱
shapeType = 4 → 円錐
shapeType = 5 → 三角柱
shapeType = 6 → XY平面の板
shapeType = 7 → 球体
*/

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 localPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float shapeType : TEXCOORD2;
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

static const float LIGHT_LEVELS = 4.0;
static const float MIN_BRIGHTNESS = 0.2;

// 単位立方体の36頂点 (Triangle List)
static const float3 cubeVertices[36] = {
    // Front face
    float3(-0.5, -0.5, -0.5), float3(-0.5,  0.5, -0.5), float3( 0.5,  0.5, -0.5),
    float3(-0.5, -0.5, -0.5), float3( 0.5,  0.5, -0.5), float3( 0.5, -0.5, -0.5),
    // Back face
    float3(-0.5, -0.5,  0.5), float3( 0.5,  0.5,  0.5), float3(-0.5,  0.5,  0.5),
    float3(-0.5, -0.5,  0.5), float3( 0.5, -0.5,  0.5), float3( 0.5,  0.5,  0.5),
    // Left face
    float3(-0.5, -0.5,  0.5), float3(-0.5,  0.5,  0.5), float3(-0.5,  0.5, -0.5),
    float3(-0.5, -0.5,  0.5), float3(-0.5,  0.5, -0.5), float3(-0.5, -0.5, -0.5),
    // Right face
    float3( 0.5, -0.5, -0.5), float3( 0.5,  0.5, -0.5), float3( 0.5,  0.5,  0.5),
    float3( 0.5, -0.5, -0.5), float3( 0.5,  0.5,  0.5), float3( 0.5, -0.5,  0.5),
    // Top face
    float3(-0.5,  0.5, -0.5), float3( 0.5,  0.5,  0.5), float3( 0.5,  0.5, -0.5),
    float3(-0.5,  0.5, -0.5), float3(-0.5,  0.5,  0.5), float3( 0.5,  0.5,  0.5),
    // Bottom face
    float3(-0.5, -0.5,  0.5), float3( 0.5, -0.5, -0.5), float3( 0.5, -0.5,  0.5),
    float3(-0.5, -0.5,  0.5), float3(-0.5, -0.5, -0.5), float3( 0.5, -0.5, -0.5)
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
// 円柱 (Cylinder: 96頂点 = 側面48 + 上面24 + 底面24)
static const float3 cylinderVertices[96] =
{
    // --- 側面 (48頂点 / 16三角形) ---
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

    /** @brief 上面の三角形を外向きの+Y法線と一致する頂点順で定義する */
    float3(0, 0.5, 0), float3(0.3536, 0.5, 0.3536), float3(0.5000, 0.5, 0.0000),
    float3(0, 0.5, 0), float3(0.0000, 0.5, 0.5000), float3(0.3536, 0.5, 0.3536),
    float3(0, 0.5, 0), float3(-0.3536, 0.5, 0.3536), float3(0.0000, 0.5, 0.5000),
    float3(0, 0.5, 0), float3(-0.5000, 0.5, 0.0000), float3(-0.3536, 0.5, 0.3536),
    float3(0, 0.5, 0), float3(-0.3536, 0.5, -0.3536), float3(-0.5000, 0.5, 0.0000),
    float3(0, 0.5, 0), float3(0.0000, 0.5, -0.5000), float3(-0.3536, 0.5, -0.3536),
    float3(0, 0.5, 0), float3(0.3536, 0.5, -0.3536), float3(0.0000, 0.5, -0.5000),
    float3(0, 0.5, 0), float3(0.5000, 0.5, 0.0000), float3(0.3536, 0.5, -0.3536),

    /** @brief 底面の三角形を外向きの-Y法線と一致する頂点順で定義する */
    float3(0, -0.5, 0), float3(0.5000, -0.5, 0.0000), float3(0.3536, -0.5, 0.3536),
    float3(0, -0.5, 0), float3(0.3536, -0.5, 0.3536), float3(0.0000, -0.5, 0.5000),
    float3(0, -0.5, 0), float3(0.0000, -0.5, 0.5000), float3(-0.3536, -0.5, 0.3536),
    float3(0, -0.5, 0), float3(-0.3536, -0.5, 0.3536), float3(-0.5000, -0.5, 0.0000),
    float3(0, -0.5, 0), float3(-0.5000, -0.5, 0.0000), float3(-0.3536, -0.5, -0.3536),
    float3(0, -0.5, 0), float3(-0.3536, -0.5, -0.3536), float3(0.0000, -0.5, -0.5000),
    float3(0, -0.5, 0), float3(0.0000, -0.5, -0.5000), float3(0.3536, -0.5, -0.3536),
    float3(0, -0.5, 0), float3(0.3536, -0.5, -0.3536), float3(0.5000, -0.5, 0.0000)
};

static const float3 cylinderNormals[96] =
{
    // 側面 48頂点の法線
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

    // 上面 24頂点の法線 (すべて上向き +Y)
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),
    float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0),

    // 底面 24頂点の法線 (すべて下向き -Y)
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
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
// 円錐 (Cone: 48頂点) の法線データ
static const float3 coneNormals[48] =
{
    // --- 側面 (24頂点 / 8三角形) ---
    float3(0.8944, 0.4472, 0.0000), float3(0.8944, 0.4472, 0.0000), float3(0.8944, 0.4472, 0.0000),
    float3(0.6325, 0.4472, 0.6325), float3(0.6325, 0.4472, 0.6325), float3(0.6325, 0.4472, 0.6325),
    float3(0.0000, 0.4472, 0.8944), float3(0.0000, 0.4472, 0.8944), float3(0.0000, 0.4472, 0.8944),
    float3(-0.6325, 0.4472, 0.6325), float3(-0.6325, 0.4472, 0.6325), float3(-0.6325, 0.4472, 0.6325),
    float3(-0.8944, 0.4472, 0.0000), float3(-0.8944, 0.4472, 0.0000), float3(-0.8944, 0.4472, 0.0000),
    float3(-0.6325, 0.4472, -0.6325), float3(-0.6325, 0.4472, -0.6325), float3(-0.6325, 0.4472, -0.6325),
    float3(0.0000, 0.4472, -0.8944), float3(0.0000, 0.4472, -0.8944), float3(0.0000, 0.4472, -0.8944),
    float3(0.6325, 0.4472, -0.6325), float3(0.6325, 0.4472, -0.6325), float3(0.6325, 0.4472, -0.6325),

    // --- 底面 (24頂点 / 8三角形: すべて下向き -Y) ---
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0),
    float3(0, -1, 0), float3(0, -1, 0), float3(0, -1, 0)
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
        localPos = cylinderVertices[vID % 96];
        normal = cylinderNormals[vID % 96];
        output.localPos = localPos.xy;
    }
    else if (u_shapeType < 4.5)
    {
        // 4: Cone (48頂点) 
        localPos = coneVertices[vID % 48];
        normal = coneNormals[vID % 48];
        output.localPos = localPos.xy;
    }
    else if (u_shapeType < 5.5)
    {
        // 5: 三角柱 (24頂点)
        localPos = prismVertices[vID % 24];
        normal = prismNormals[vID % 24];
        output.localPos = localPos.xy;
    }
    else if (u_shapeType < 6.5) {
        // 6: Sprite2D (XY平面、UIや爆発スプライトなど)
        float2 rawPos;
        rawPos.x = float(vID & 2) - 1.0f;
        rawPos.y = float((vID & 1) << 1) - 1.0f;
        localPos = float3(rawPos.x, rawPos.y, 0.0f);
        output.localPos = rawPos;
        normal = float3(0, 0, -1);
    }
    else {
        // 7: Sphere (8緯度帯 x 12経度帯を頂点IDから生成)
        const uint slices = 12;
        const uint triangleVertex = vID % 6;
        const uint quad = vID / 6;
        const uint stack = quad / slices;
        const uint slice = quad % slices;
        const float theta0 = 3.14159265f * float(stack) / 8.0f;
        const float theta1 = 3.14159265f * float(stack + 1) / 8.0f;
        const float phi0 = 6.28318531f * float(slice) / float(slices);
        const float phi1 = 6.28318531f * float(slice + 1) / float(slices);
        const float3 points[4] = {
            float3(sin(theta0) * cos(phi0), cos(theta0), sin(theta0) * sin(phi0)),
            float3(sin(theta1) * cos(phi0), cos(theta1), sin(theta1) * sin(phi0)),
            float3(sin(theta1) * cos(phi1), cos(theta1), sin(theta1) * sin(phi1)),
            float3(sin(theta0) * cos(phi1), cos(theta0), sin(theta0) * sin(phi1))
        };
        const uint indices[6] = {0, 2, 1, 0, 3, 2};
        normal = points[indices[triangleVertex]];
        localPos = normal * 0.5f;
        output.localPos = localPos.xy;
    }
    
    // WVP行列による座標変換
    output.pos = mul(float4(localPos, 1.0f), u_wvpMatrix);

    // Y軸回転を法線へ反映する
    float sinAngle;
    float cosAngle;
    sincos(u_rotAngle, sinAngle, cosAngle);
    float3 rotatedNormal = float3(
        normal.x * cosAngle + normal.z * sinAngle,
        normal.y,
        -normal.x * sinAngle + normal.z * cosAngle);
    output.color = u_Color;
    output.normal = rotatedNormal;
    output.shapeType = u_shapeType;
    return output;
}

float4 PSObject(VS_OUTPUT input) : SV_TARGET
{
    if (input.color.a < 0.01) discard;

    // Sprite2Dはライティングせず、3Dプリミティブだけをピクセル単位で量子化する
    float brightness = 1.0;
    if (input.shapeType < 5.5) {
        float3 lightDir = normalize(float3(0.4, 0.8, -0.4));
        float diffuse = saturate(dot(normalize(input.normal), lightDir));
        float quantizedDiffuse = floor(diffuse * (LIGHT_LEVELS - 1.0) + 0.5) / (LIGHT_LEVELS - 1.0);
        brightness = lerp(MIN_BRIGHTNESS, 1.0, quantizedDiffuse);
    }

    return float4(input.color.rgb * brightness, input.color.a);
}
