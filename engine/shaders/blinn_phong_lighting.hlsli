#ifndef BOREAL_BLINN_PHONG_LIGHTING_HLSLI
#define BOREAL_BLINN_PHONG_LIGHTING_HLSLI

#include "shader_constants.hlsli"

// Peak specular strength before the light and surface-angle factors.
static const float kBlinnPhongSpecularStrength = 0.04;

// Higher exponents produce a narrower highlight.
static const float kBlinnPhongShininess = 32.0;

float3 calculateBlinnPhongLighting(float3 normal, float3 view_dir, float3 light_dir, float3 albedo, float n_dot_l)
{
    const float3 half_sum = view_dir + light_dir;
    const float length_squared = dot(half_sum, half_sum);
    const float n_dot_h = saturate(dot(normal, half_sum * rsqrt(max(length_squared, kMinHalfVectorLengthSquared))));
    const float specular = dot(normal, view_dir) > 0.0 ? kBlinnPhongSpecularStrength * pow(n_dot_h, kBlinnPhongShininess) : 0.0;
    return (albedo + specular) * n_dot_l;
}

#endif
