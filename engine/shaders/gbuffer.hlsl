#include "static_samplers.hlsli"
#include "mesh_transform.hlsli"
#include "shader_constants.hlsli"

Texture2D<float4> gBaseColorTexture : register(t0);
Texture2D<float4> gNormalTexture : register(t1);
Texture2D<float4> gMetallicRoughnessTexture : register(t2);

cbuffer MaterialCB : register(b1)
{
    float4 gBaseColorFactor;
    float gNormalScale;
    float gRoughnessFactor;
    float gMetallicFactor;
    uint gHasMetallicRoughnessTexture;
}

struct VS_INPUT
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 normal_texcoord : TEXCOORD1;
    float2 metallic_roughness_texcoord : TEXCOORD2;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float2 normal_texcoord : TEXCOORD2;
    float3 world_normal : TEXCOORD3;
    float4 world_tangent : TEXCOORD4;
    float2 metallic_roughness_texcoord : TEXCOORD5;
    float4 color : COLOR0;
};

struct GBUFFER_OUTPUT
{
    float4 base_color : SV_Target0;
    float4 world_normal : SV_Target1;
    float2 material : SV_Target2;
};

VS_OUTPUT vertexShader(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = transformMeshPositionToClipSpace(input.position);
    output.texcoord = input.texcoord;
    output.world_position = mul(gWorld, float4(input.position, 1.0)).xyz;
    output.normal_texcoord = input.normal_texcoord;
    output.metallic_roughness_texcoord = input.metallic_roughness_texcoord;
    const float3 world_x = mul(gWorld, float4(1.0, 0.0, 0.0, 0.0)).xyz;
    const float3 world_y = mul(gWorld, float4(0.0, 1.0, 0.0, 0.0)).xyz;
    const float3 world_z = mul(gWorld, float4(0.0, 0.0, 1.0, 0.0)).xyz;
    const float3 normal_x = cross(world_y, world_z);
    const float world_sign = sign(dot(world_x, normal_x));
    // Cofactors give the inverse-transpose direction under nonuniform scaling.
    output.world_normal = world_sign * (input.normal.x * normal_x
        + input.normal.y * cross(world_z, world_x) + input.normal.z * cross(world_x, world_y));
    output.world_tangent = float4(mul((float3x3)gWorld, input.tangent.xyz), input.tangent.w * world_sign);
    output.color = input.color;
    return output;
}

float3 getWorldNormal(VS_OUTPUT input)
{
    const float3 position_dx = ddx(input.world_position);
    const float3 position_dy = ddy(input.world_position);
    const float2 uv_dx = ddx(input.normal_texcoord);
    const float2 uv_dy = ddy(input.normal_texcoord);
    const float3 geometric_normal = cross(position_dx, position_dy);
    const float3 base_normal = dot(input.world_normal, input.world_normal) > 0.0
        ? normalize(input.world_normal) : geometric_normal * rsqrt(max(dot(geometric_normal, geometric_normal), kMinGeometricNormalLengthSquared));
    const float facing = dot(base_normal, geometric_normal) < 0.0 ? -1.0 : 1.0;
    if (gNormalScale == 0.0)
    {
        return facing * base_normal;
    }

    float3 tangent = input.world_tangent.xyz;
    float handedness = input.world_tangent.w;
    if (handedness == 0.0)
    {
        const float uv_sign = sign(uv_dx.x * uv_dy.y - uv_dx.y * uv_dy.x);
        tangent = uv_sign * (uv_dy.y * position_dx - uv_dx.y * position_dy);
        const float3 bitangent = uv_sign * (uv_dx.x * position_dy - uv_dy.x * position_dx);
        handedness = sign(dot(cross(base_normal, tangent), bitangent));
    }
    tangent -= base_normal * dot(base_normal, tangent);
    if (dot(tangent, tangent) <= 0.0 || handedness == 0.0)
    {
        return facing * base_normal;
    }

    tangent = normalize(tangent);
    const float3 bitangent = cross(base_normal, tangent) * sign(handedness);
    float3 mapped = gNormalTexture.SampleGrad(gLinearWrapSampler, input.normal_texcoord, uv_dx, uv_dy).xyz * 2.0 - 1.0;
    mapped.xy *= gNormalScale;
    const float3 world_normal = tangent * mapped.x + bitangent * mapped.y + base_normal * mapped.z;
    return dot(world_normal, world_normal) > 0.0 ? facing * normalize(world_normal) : facing * base_normal;
}


GBUFFER_OUTPUT pixelShader(VS_OUTPUT input)
{
    GBUFFER_OUTPUT output;
    output.base_color = float4(gBaseColorTexture.Sample(gLinearWrapSampler, input.texcoord).rgb * input.color.rgb * gBaseColorFactor.rgb, 1.0);
    output.world_normal = float4(getWorldNormal(input), 0.0);
    output.material = float2(gRoughnessFactor, gMetallicFactor);
    if (gHasMetallicRoughnessTexture != 0)
    {
        output.material *= gMetallicRoughnessTexture.Sample(gLinearWrapSampler, input.metallic_roughness_texcoord).gb;
    }
    return output;
}
