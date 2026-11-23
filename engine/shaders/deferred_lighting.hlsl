#include "fullscreen_triangle.hlsli"

Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gWorldNormal : register(t1);

cbuffer DirectionalLight : register(b0)
{
    float3 gDirectionToLight;
    float gDirectionPadding;
    float3 gLightGain;
    float gGainPadding;
}

float4 pixelShader(float4 position : SV_POSITION) : SV_Target
{
    const int3 coordinates = int3(int2(position.xy), 0);
    const float4 albedo = gAlbedo.Load(coordinates);
    const float3 normal = gWorldNormal.Load(coordinates).xyz;
    const float normal_length_squared = dot(normal, normal);
    if (albedo.a == 0.0 || normal_length_squared <= 0.0)
    {
        return float4(0.0, 0.0, 0.0, albedo.a);
    }

    const float cosine = max(dot(normal * rsqrt(normal_length_squared), gDirectionToLight), 0.0);
    return float4(albedo.rgb * gLightGain * cosine, albedo.a);
}
