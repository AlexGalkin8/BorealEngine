#include "blinn_phong_lighting.hlsli"
#include "diffuse_lighting.hlsli"
#include "pbr_lighting.hlsli"
#include "fullscreen_triangle.hlsli"
#include "space_transform.hlsli"
#include "shader_constants.hlsli"

Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gWorldNormal : register(t1);

Texture2D<float> gDepth : register(t2);
Texture2D<float2> gMaterial : register(t3);

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
    uint gLightingModel;
    float3 gLightGain;
    uint gHasDirectionalLight;
    float4x4 gInverseViewProjection;
    uint gPointLightCount;
    float3 gCameraPosition;
    PointLight gPointLights[4];
}

// Match render::ELightingModel.
static const uint kLightingDiffuse = 0;
static const uint kLightingBlinnPhong = 1;
static const uint kLightingPBR = 2;

float3 evaluateLightResponse(uint lighting_model, float3 normal, float3 view_dir, float3 light_dir,
    float3 albedo, float2 material, float3 incident_radiance)
{
    const float n_dot_l = saturate(dot(normal, light_dir));
    if (n_dot_l == 0.0)
    {
        return 0.0;
    }

    switch (lighting_model)
    {
    case kLightingBlinnPhong:
        return incident_radiance * calculateBlinnPhongLighting(normal, view_dir, light_dir, albedo, n_dot_l);
    case kLightingPBR:
        return incident_radiance * calculatePBRLighting(normal, view_dir, light_dir, albedo, material, n_dot_l);
    default:
        return incident_radiance * calculateDiffuseLighting(albedo, n_dot_l);
    }
}

float4 shadeDeferred(float4 position, uint lighting_model)
{
    const int3 pixel_position = int3(int2(position.xy), 0);
    const float4 albedo = gAlbedo.Load(pixel_position);
    const float3 normal = gWorldNormal.Load(pixel_position).xyz;

    const float normal_length_squared = dot(normal, normal);
    if (albedo.a == 0.0 || normal_length_squared <= 0.0) // Skip uncovered pixels and degenerate normals
    {
        return float4(0.0, 0.0, 0.0, albedo.a);
    }

    const float3 unit_normal = normal * rsqrt(normal_length_squared);
    uint width, height;
    gDepth.GetDimensions(width, height);
    const float3 world_position = reconstructWorldPosition(position.xy, gDepth.Load(pixel_position),
        float2(width, height), gInverseViewProjection);
    const float3 to_camera = gCameraPosition - world_position;
    const float view_distance_squared = dot(to_camera, to_camera);
    const float3 view_dir = to_camera * rsqrt(max(view_distance_squared, kMinViewVectorLengthSquared));
    const float2 material = lighting_model == kLightingPBR ? gMaterial.Load(pixel_position) : float2(0.0, 0.0);

    float3 lighting = 0.0;
    if (gHasDirectionalLight != 0)
    {
        lighting = evaluateLightResponse(lighting_model, unit_normal, view_dir, gDirectionToLight, albedo.rgb, material, gLightGain);
    }

    for (uint index = 0; index < gPointLightCount; ++index)
    {
        const PointLight light = gPointLights[index];
        const float3 to_light = light.position - world_position;
        const float distance_squared = dot(to_light, to_light);
        const float radius_squared = light.radius * light.radius;
        const float cutoff = saturate(1.0 - distance_squared / radius_squared);
        if (distance_squared <= 0.0 || cutoff == 0.0) // Skip degenerate lights and lights that don't reach the pixel
        {
            continue;
        }

        const float3 light_dir = to_light * rsqrt(distance_squared);
        const float attenuation = cutoff * cutoff / max(distance_squared, 0.01);
        lighting += evaluateLightResponse(lighting_model, unit_normal, view_dir, light_dir,albedo.rgb, material, light.light_gain * attenuation);
    }
    return float4(lighting, albedo.a);
}

float4 pixelShader(float4 position : SV_POSITION) : SV_Target
{
    switch (gLightingModel)
    {
    case kLightingBlinnPhong:
        return shadeDeferred(position, kLightingBlinnPhong);
    case kLightingPBR:
        return shadeDeferred(position, kLightingPBR);
    default:
        return shadeDeferred(position, kLightingDiffuse);
    }
}
