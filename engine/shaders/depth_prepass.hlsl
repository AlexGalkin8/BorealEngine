#include "mesh_transform.hlsli"

float4 vertexShader(float3 position : POSITION) : SV_POSITION
{
    return transformMeshPositionToClipSpace(position);
}
