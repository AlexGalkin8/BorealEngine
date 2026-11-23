#ifndef BOREAL_BRDF_HLSLI
#define BOREAL_BRDF_HLSLI

#include "shader_constants.hlsli"

static const float kDielectricF0 = 0.04;
static const float kMinPbrRoughness = 0.05; // A finite roughness keeps GGX evaluable for mirror-like materials.
static const float kMinSpecularDenominator = 1e-6;

float3 fresnelSchlick(float3 f0, float v_dot_h)
{
    const float grazing = 1.0 - saturate(v_dot_h);
    const float grazing_squared = grazing * grazing;
    return f0 + (1.0 - f0) * (grazing_squared * grazing_squared * grazing);
}

// GGX alpha is perceptual roughness squared. Clamp roughness before converting.
float distributionGGX(float n_dot_h, float alpha)
{
    if (n_dot_h <= 0.0)
    {
        return 0.0;
    }

    const float cos_theta = saturate(n_dot_h);
    const float cos_theta_squared = cos_theta * cos_theta;
    const float alpha_squared = alpha * alpha;
    // Keep alpha squared separate so it survives rounding when N dot H is one.
    const float denominator = (1.0 - cos_theta_squared) + alpha_squared * cos_theta_squared;
    return alpha_squared / (kPi * denominator * denominator);
}

float maskingSmithGGX(float n_dot_direction, float alpha)
{
    const float cos_theta = saturate(n_dot_direction);
    if (cos_theta == 0.0)
    {
        return 0.0;
    }

    const float cos_theta_squared = cos_theta * cos_theta;
    const float alpha_squared = alpha * alpha;
    const float projected_roughness = sqrt(cos_theta_squared + alpha_squared * (1.0 - cos_theta_squared));
    return (2.0 * cos_theta) / (cos_theta + projected_roughness);
}

float geometrySmithGGX(float n_dot_v, float n_dot_l, float alpha)
{
    return maskingSmithGGX(n_dot_v, alpha) * maskingSmithGGX(n_dot_l, alpha);
}

// Directions are unit vectors pointing away from the surface in world space.
// The caller applies incident radiance, attenuation, and N dot L.
float3 evaluateCookTorranceBRDF(float3 normal, float3 view_dir, float3 light_dir, float3 albedo, float roughness, float metallic)
{
    const float n_dot_v = saturate(dot(normal, view_dir));
    const float n_dot_l = saturate(dot(normal, light_dir));
    const float3 half_sum = view_dir + light_dir;
    const float half_length_squared = dot(half_sum, half_sum);
    if (n_dot_v == 0.0 || n_dot_l == 0.0 || half_length_squared <= kMinHalfVectorLengthSquared)
    {
        return 0.0;
    }

    const float3 half_dir = half_sum * rsqrt(half_length_squared);
    const float clamped_roughness = max(saturate(roughness), kMinPbrRoughness);
    const float alpha = clamped_roughness * clamped_roughness;
    const float metalness = saturate(metallic);
    const float3 f0 = lerp(kDielectricF0, albedo, metalness);

    const float distribution = distributionGGX(dot(normal, half_dir), alpha);
    const float geometry = geometrySmithGGX(n_dot_v, n_dot_l, alpha);
    const float3 fresnel = fresnelSchlick(f0, dot(view_dir, half_dir));
    const float denominator = max(4.0 * n_dot_v * n_dot_l, kMinSpecularDenominator);
    const float3 specular = (distribution * geometry / denominator) * fresnel;
    const float3 diffuse = (1.0 - fresnel) * (1.0 - metalness) * albedo / kPi;
    return diffuse + specular;
}

#endif
