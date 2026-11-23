float4 vertexShader(uint vertex_id : SV_VertexID) : SV_POSITION
{
    const float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    return float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}
