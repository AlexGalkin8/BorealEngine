#include "static_samplers.hlsli"
#include "mesh_transform.hlsli"

#ifdef USE_TEXTURE
Texture2D    gBaseColorTexture : register(t0);
#endif

struct VS_INPUT
{
    float3 position : POSITION;
#ifdef USE_VERTEX_COLOR
    float4 color : COLOR;
#else
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
#endif
};


struct VS_OUTPUT
{
    float4 position : SV_POSITION;
#ifdef USE_VERTEX_COLOR
    float4 color : COLOR;
#else
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
#endif
};


VS_OUTPUT vertexShader(VS_INPUT input)
{
    VS_OUTPUT output;

    output.position = transformMeshPositionToClipSpace(input.position);

#ifdef USE_VERTEX_COLOR
    output.color = input.color;
#else
    output.texcoord = input.texcoord;
    output.color = input.color;
#endif

    return output;
}


float4 pixelShader(VS_OUTPUT input) : SV_Target
{
#ifdef USE_VERTEX_COLOR
    return input.color;
#elif defined(USE_TEXTURE)
    float4 tex_color = gBaseColorTexture.Sample(gLinearWrapSampler, input.texcoord);
    return tex_color * input.color;
#else
    return input.color;
#endif
}
