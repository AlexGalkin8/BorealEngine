Texture2D<float4> gBaseColor : register(t0);

float4 vertexShader(uint vertex_id : SV_VertexID) : SV_POSITION
{
    const float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    return float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

float4 pixelShader(float4 position : SV_POSITION) : SV_Target
{
    const float4 base_color = gBaseColor.Load(int3(int2(position.xy), 0));
    const float3 linear_color = saturate(base_color.rgb);
    const float3 display_color = lerp(12.92 * linear_color,
        1.055 * pow(linear_color, 1.0 / 2.4) - 0.055, step(0.0031308, linear_color));
    return float4(lerp(float3(0.1, 0.1, 0.1), display_color, base_color.a), 1.0);
}
