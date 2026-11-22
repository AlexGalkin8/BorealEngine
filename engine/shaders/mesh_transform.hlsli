cbuffer CameraCB : register(b0)
{
    float4x4 gWorld;
    float4x4 gViewProjection;
}

float4 transformMeshPositionToClipSpace(float3 position)
{
    float4 world_position = mul(gWorld, float4(position, 1.0));
    precise float4 clip_position = mul(gViewProjection, world_position);
    return clip_position;
}
