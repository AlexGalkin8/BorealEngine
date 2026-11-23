#include "fullscreen_triangle.hlsli"

Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gWorldNormal : register(t1);

Texture2D<float> gDepth : register(t2);

struct PointLight
{
    float3 position;
    float radius;
    float3 light_gain;
    float padding;
};

cbuffer Lighting : register(b0)
{
    float3 gDirectionToLight;
    float gDirectionPadding;
    float3 gLightGain;
    float gGainPadding;
    float4x4 gInverseViewProjection;
    uint gPointLightCount;
    uint3 gLightPadding;
    PointLight gPointLights[4];
}

float3 reconstructWorldPosition(float2 pixel_center, float depth)
{
    uint width, height;
    gDepth.GetDimensions(width, height);
    const float2 uv = pixel_center / float2(width, height);
    const float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    const float4 world = mul(gInverseViewProjection, float4(ndc, depth, 1.0));
    return world.xyz / world.w;
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

    const float3 unit_normal = normal * rsqrt(normal_length_squared);
    float3 lighting = gLightGain * max(dot(unit_normal, gDirectionToLight), 0.0);
    if (gPointLightCount > 0)
    {
        const float3 world_position = reconstructWorldPosition(position.xy, gDepth.Load(coordinates));
        for (uint index = 0; index < gPointLightCount; ++index)
        {
            const PointLight light = gPointLights[index];
            const float3 to_light = light.position - world_position;
            const float distance_squared = dot(to_light, to_light);
            const float radius_squared = light.radius * light.radius;
            const float cutoff = saturate(1.0 - distance_squared / radius_squared);
            const float3 light_dir = to_light * rsqrt(max(distance_squared, 1e-6));
            const float cosine = max(dot(unit_normal, light_dir), 0.0);
            lighting += light.light_gain * cosine * cutoff * cutoff / max(distance_squared, 0.01);
        }
    }
    return float4(albedo.rgb * lighting, albedo.a);
}
