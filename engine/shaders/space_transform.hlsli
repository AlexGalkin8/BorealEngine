#ifndef BOREAL_SPACE_TRANSFORM_HLSLI
#define BOREAL_SPACE_TRANSFORM_HLSLI

// SV_Position uses pixel centers and a top-left origin. D3D depth is already in [0, 1].
float3 reconstructWorldPosition(float2 pixel_center, float depth, float2 target_size, float4x4 inverse_view_projection)
{
    const float2 uv = pixel_center / target_size;
    const float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    const float4 world = mul(inverse_view_projection, float4(ndc, depth, 1.0));
    return world.xyz / world.w;
}

#endif
