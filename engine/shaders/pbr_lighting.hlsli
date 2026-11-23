#ifndef BOREAL_PBR_LIGHTING_HLSLI
#define BOREAL_PBR_LIGHTING_HLSLI

#include "brdf.hlsli"

float3 calculatePBRLighting(float3 normal, float3 view_dir, float3 light_dir, float3 albedo, float2 material, float n_dot_l)
{
    return evaluateCookTorranceBRDF(normal, view_dir, light_dir, albedo, material.r, material.g) * n_dot_l;
}

#endif
