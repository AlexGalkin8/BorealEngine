#ifndef BOREAL_DIFFUSE_LIGHTING_HLSLI
#define BOREAL_DIFFUSE_LIGHTING_HLSLI

float3 calculateDiffuseLighting(float3 albedo, float n_dot_l)
{
    return albedo * n_dot_l;
}

#endif
