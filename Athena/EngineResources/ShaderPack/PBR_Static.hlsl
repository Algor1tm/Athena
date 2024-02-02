//////////////////////// Athena PBR Shader ////////////////////////

#include "Buffers.hlsli"
#include "Common.hlsli"


struct Vertex
{
    float3 Position;
    float2 TexCoords;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
};

struct Interpolators
{
    float4 OutPosition : SV_POSITION;
    float3 WorldPos;
    float2 TexCoords;
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
    
    float4 worldPos = mul(float4(vertex.Position, 1.0), u_Transform);
    output.OutPosition = mul(mul(worldPos, u_Camera.View), u_Camera.Projection);
    output.WorldPos = worldPos.xyz;
    output.TexCoords = vertex.TexCoords;
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


float3 LightContribution(float3 lightDirection, float3 lightRadiance, float3 normal, float3 viewDir, float3 albedo, float metalness, float roughness)
{
    float3 reflectivityAtZeroIncidence = float3(0.04);
    reflectivityAtZeroIncidence = lerp(reflectivityAtZeroIncidence, albedo, metalness);

    float3 negativeLightDir = -lightDirection;
    float3 halfWayVector = normalize(viewDir + negativeLightDir);

    float NDF = DistributionGGX(normal, halfWayVector, roughness);
    float G = GeometrySmith(normal, viewDir, negativeLightDir, roughness);
    float3 F = FresnelShlick(max(dot(halfWayVector, viewDir), 0.0), reflectivityAtZeroIncidence);

    float3 reflectedLight = F;
    float3 absorbedLight = float3(1.0) - reflectedLight;
        
    absorbedLight *= 1.0 - metalness;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, negativeLightDir), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float normalDotLightDir = max(dot(normal, negativeLightDir), 0.0);

    return (absorbedLight * albedo / PI + specular) * lightRadiance * normalDotLightDir;
}

float3 ComputeDirectionalLightRadiance(int index)
{
    DirectionalLight light = g_DirectionalLights[index];
    float3 radiance = light.Color.rgb * light.Intensity;
    return radiance;
}

float3 ComputePointLightRadiance(int index, float3 worldPos)
{
    PointLight light = g_PointLights[index];
    
    float distance = length(worldPos - light.Position);
    float attenuationFactor = distance / light.Radius;

    float attenuation = 0.0;
    if (attenuationFactor < 1.0)
    {
        float factor2 = attenuationFactor * attenuationFactor;
        attenuation = (1.0 - factor2) * (1.0 - factor2) / (1 + light.FallOff * attenuationFactor);
        attenuation = clamp(attenuation, 0.0, 1.0);
    }

    float3 radiance = light.Color.rgb * light.Intensity * attenuation;
    return radiance;
}


Fragment FSMain(Interpolators input)
{
    Fragment output;
    
    // Get PBR parameters
    float4 albedo = u_Albedo;
    if (u_UseAlbedoMap)
        albedo *= u_AlbedoMap.Sample(u_AlbedoMapSampler, input.TexCoords);
    
    float3 normal = input.Normal;
    if(u_UseNormalMap)
    {
        normal = u_NormalMap.Sample(u_NormalMapSampler, input.TexCoords).rgb;
        normal = normal * 2 - 1;
        normal = mul(normal, input.TBN);
    }
    
    float roughness = u_UseRoughnessMap ? u_RoughnessMap.Sample(u_RoughnessMapSampler, input.TexCoords).r : u_Roughness;
    float metalness = u_UseMetalnessMap ? u_MetalnessMap.Sample(u_MetalnessMapSampler, input.TexCoords).r : u_Metalness;
    
    // Compute lighting from all light sources
    float3 viewDir = normalize(u_Camera.Position.xyz - input.WorldPos.xyz);
    float3 totalIrradiance = float3(0.0);
    
    for (int i = 0; i < g_DirectionalLightCount; ++i)
    {
        float3 dir = normalize(g_DirectionalLights[i].Direction);
        float3 radiance = ComputeDirectionalLightRadiance(i);
        
        totalIrradiance += LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
    }
    
    for (int j = 0; j < g_PointLightCount; ++j)
    {
        float3 dir = normalize(input.WorldPos.xyz - g_PointLights[j].Position);
        float3 radiance = ComputePointLightRadiance(j, input.WorldPos.xyz);
        
        totalIrradiance += LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
    }
    
    const float3 ambientDiffuse = float3(0.5, 0.5, 0.5);
    const float3 ambientSpecular = float3(0.0, 0.0, 0.0);
    
    float3 ambient = albedo.rgb * ambientDiffuse + ambientSpecular;
    float3 emission = albedo.rgb * u_Emission;
    
    float3 hdrColor = totalIrradiance + ambient + emission;
    
    output.Color = float4(hdrColor, albedo.a);
    return output;
}
