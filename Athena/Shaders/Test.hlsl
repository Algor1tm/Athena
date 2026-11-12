#include "TestHeader.hlsl"

VSOutput VSMain(uint VertexID : SV_VertexID)
{
    VSOutput output;

    half2 uv = half2((VertexID << 1) & 2, VertexID & 2);

    output.Position = float4(
        uv * float2(2.0, -2.0) + float2(-1.0, 1.0),
        0.0,
        1.0
    );

    return output;
}

cbuffer TestBuffer : register(b0, space0)
{
    float4 Color;
};

float4 PSMain(VSOutput input) : SV_Target0
{
    return Color; // Magenta
}
