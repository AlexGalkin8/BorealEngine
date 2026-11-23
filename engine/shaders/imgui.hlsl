#include "static_samplers.hlsli"

cbuffer ImGuiTransform : register(b0)
{
    float2 gScale;
    float2 gOffset;
}

Texture2D<float4> gTexture : register(t0);

struct VS_INPUT
{
    float2 position : POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

VS_OUTPUT vertexShader(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position * gScale + gOffset, 0.0, 1.0);
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}

float4 pixelShader(VS_OUTPUT input) : SV_Target
{
    return input.color * gTexture.Sample(gLinearClampSampler, input.texcoord);
}
