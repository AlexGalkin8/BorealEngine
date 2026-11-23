#include "fullscreen_triangle.hlsli"

Texture2D<float4> gSceneLighting : register(t0);
Texture2D<float4> gAlbedo : register(t1);
Texture2D<float4> gWorldNormal : register(t2);

cbuffer Postprocess : register(b0)
{
    float gExposure;
    uint gDebugView;
}

float3 encodeSrgb(float3 linear_color)
{
    return lerp(12.92 * linear_color,
        1.055 * pow(linear_color, 1.0 / 2.4) - 0.055, step(0.0031308, linear_color));
}

float4 pixelShader(float4 position : SV_POSITION) : SV_Target
{
    const int3 coordinates = int3(int2(position.xy), 0);
    if (gDebugView == 1)
    {
        return float4(encodeSrgb(saturate(gAlbedo.Load(coordinates).rgb)), 1.0);
    }
    if (gDebugView == 2)
    {
        const float coverage = gAlbedo.Load(coordinates).a;
        const float3 normal = gWorldNormal.Load(coordinates).xyz;
        return float4(saturate(normal * 0.5 + 0.5) * coverage, 1.0);
    }

    const float3 hdr = max(gSceneLighting.Load(coordinates).rgb, 0.0);
    const float3 exposed = hdr * gExposure;
    const float3 linear_color = gDebugView == 3 ? saturate(hdr) : exposed / (1.0 + exposed);
    return float4(encodeSrgb(linear_color), 1.0);
}
