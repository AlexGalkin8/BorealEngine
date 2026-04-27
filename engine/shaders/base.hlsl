cbuffer CameraCB : register(b0)
{
    float4x4 gWorld;
    float4x4 gViewProjection;
}

struct VS_INPUT
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};


struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};


VS_OUTPUT vertexShader(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 world_pos = mul(gWorld, float4(input.position, 1.0));
    output.position = mul(gViewProjection, world_pos);
    output.texcoord = input.texcoord;
    return output;
}


float4 pixelShader(VS_OUTPUT input) : SV_Target
{
    return float4(1.0, 0.5, 0.0, 1.0);
}
