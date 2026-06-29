// Constant buffer data structure
cbuffer TransformBuffer : register(b0)
{
    float2 u_position; // Object center position (-1.0 to 1.0)
    float2 u_size;     // Object size (width, height)
    float4 u_Color;    // Object color (RGBA)
};

// Vertex shader output structure
struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

// Vertex shader
VS_OUTPUT VSMain(uint vID : SV_VertexID)
{
    VS_OUTPUT output;
    
    // vID: 0 = Bottom-Left, 1 = Top-Left, 2 = Bottom-Right, 3 = Top-Right
    float2 localPos;
    localPos.x = (float) (vID & 2) - 1.0f;
    localPos.y = (float) ((vID & 1) << 1) - 1.0f;

    // Apply size and shift position
    float2 finalPos = u_position + (localPos * u_size);
    
    output.pos = float4(finalPos, 0.0f, 1.0f);
    output.color = u_Color;
    return output;
}

// Pixel shader
float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}