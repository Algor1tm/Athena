#include "Buffers.hlsli"

struct Vertex
{
    float3 Position;
    float2 TexCoord;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
};

struct Interpolators
{
    float4 WorldPos : SV_POSITION;
    float2 TexCoord;
    float3 Normal;
    float3x3 TBN;
};

[[vk::push_constant]]
cbuffer MaterialData
{
    float4x4 u_Transform;
};


Interpolators VSMain(Vertex vertex)
{
    Interpolators output;
    
    output.WorldPos = mul(mul(mul(float4(vertex.Position, 1.0), u_Transform), u_Camera.View), u_Camera.Proj);
    output.TexCoord = vertex.TexCoord;
    output.Normal = normalize(mul(float4(vertex.Normal, 0), u_Transform).xyz);

    float3 T = normalize(mul(float4(vertex.Tangent, 0), u_Transform).xyz);
    float3 B = normalize(mul(float4(vertex.Bitangent, 0), u_Transform).xyz);
    float3 N = output.Normal;
    T = normalize(T - dot(T, N) * N);
    
    output.TBN = float3x3(T, B, N);
    
    return output;
}


struct Fragment
{
    float4 Color : SV_TARGET0;
};

Fragment FSMain(Interpolators input)
{
    Fragment output;
    
    input.Normal = input.Normal * 0.5 + 0.5;
    output.Color = float4(input.Normal, 1.0);
    return output;
}
