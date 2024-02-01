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
    float4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_Emission;

    uint u_UseAlbedoMap;
    uint u_UseNormalMap;
    uint u_UseRoughnessMap;
    uint u_UseMetalnessMap;
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

[[vk::combinedImageSampler]]
Texture2D u_AlbedoMap : register(t0, space0);
SamplerState u_AlbedoMapSampler : register(s0, space0);

[[vk::combinedImageSampler]]
Texture2D u_NormalMap : register(t1, space0);
SamplerState u_NormalMapSampler : register(s1, space0);

[[vk::combinedImageSampler]]
Texture2D u_RoughnessMap : register(t2, space0);
SamplerState u_RoughnessMapSampler : register(s2, space0);

[[vk::combinedImageSampler]]
Texture2D u_MetalnessMap : register(t3, space0);
SamplerState u_MetalnessMapSampler : register(s3, space0);


Fragment FSMain(Interpolators input)
{
    Fragment output;
    
    // Get PBR parameters
    float4 albedo = u_Albedo;
    if (u_UseAlbedoMap)
        albedo *= u_AlbedoMap.Sample(u_AlbedoMapSampler, input.TexCoord);
    
    float3 normal = input.Normal;
    if(u_UseNormalMap)
    {
        normal = u_NormalMap.Sample(u_NormalMapSampler, input.TexCoord).rgb;
        normal = normal * 2 - 1;
        normal = mul(normal, input.TBN);
    }
    
    float roughness = u_UseRoughnessMap ? u_RoughnessMap.Sample(u_RoughnessMapSampler, input.TexCoord).r : u_Roughness;
    float metalness = u_UseMetalnessMap ? u_MetalnessMap.Sample(u_MetalnessMapSampler, input.TexCoord).r : u_Metalness;
    float emission = u_Emission;
    
    output.Color = albedo;
    
    return output;
}
