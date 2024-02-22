//////////////////////// Athena PBR Shader ////////////////////////

#version 460 core
#pragma stage : vertex

#include "Include/Buffers.glslh"


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

struct VertexInterpolators
{
    vec3 WorldPos;
    vec2 TexCoords;
    vec3 Normal;
    mat3 TBN;
};

layout(location = 0) out VertexInterpolators Interpolators;

layout(push_constant) uniform MaterialData
{
    mat4 u_Transform;
    vec4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_Emission;

    uint u_UseAlbedoMap;
    uint u_UseNormalMap;
    uint u_UseRoughnessMap;
    uint u_UseMetalnessMap;
};


void main()
{
    vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
    gl_Position = u_Camera.ViewProjection * worldPos;

    Interpolators.WorldPos = worldPos.xyz;
    Interpolators.TexCoords = a_TexCoords;
    Interpolators.Normal = normalize(u_Transform * vec4(a_Normal, 0)).xyz;

    vec3 T = normalize(u_Transform * vec4(a_Tangent, 0)).xyz;
    vec3 B = normalize(u_Transform * vec4(a_Bitangent, 0)).xyz;
    vec3 N =  Interpolators.Normal;
    T = normalize(T - dot(T, N) * N);
    
    Interpolators.TBN = mat3(T, B, N);
}

#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"
#include "Include/Shadows.glslh"

struct VertexInterpolators
{
    vec3 WorldPos;
    vec2 TexCoords;
    vec3 Normal;
    mat3 TBN;
};

layout(location = 0) in VertexInterpolators Interpolators;
layout(location = 0) out vec4 o_Color;


layout(push_constant) uniform MaterialData
{
    mat4 u_Transform;
    vec4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_Emission;

    uint u_UseAlbedoMap;
    uint u_UseNormalMap;
    uint u_UseRoughnessMap;
    uint u_UseMetalnessMap;
};

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_RoughnessMap;
layout(set = 0, binding = 3) uniform sampler2D u_MetalnessMap;

layout(set = 1, binding = 5) uniform sampler2D u_BRDF_LUT;
layout(set = 1, binding = 6) uniform samplerCube u_EnvironmentMap;
layout(set = 1, binding = 7) uniform samplerCube u_IrradianceMap;


vec3 LightContribution(vec3 lightDirection, vec3 lightRadiance, vec3 normal, vec3 viewDir, vec3 albedo, float metalness, float roughness)
{
    vec3 reflectivityAtZeroIncidence = vec3(0.04);
    reflectivityAtZeroIncidence = mix(reflectivityAtZeroIncidence, albedo, metalness);

    vec3 negativeLightDir = -lightDirection;
    vec3 halfWayVector = normalize(viewDir + negativeLightDir);

    float NDF = DistributionGGX(normal, halfWayVector, roughness);
    float G = GeometrySmith(normal, viewDir, negativeLightDir, roughness);
    vec3 F = FresnelShlick(max(dot(halfWayVector, viewDir), 0.0), reflectivityAtZeroIncidence);

    vec3 reflectedLight = F;
    vec3 absorbedLight = vec3(1.0) - reflectedLight;
        
    absorbedLight *= 1.0 - metalness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, negativeLightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float normalDotLightDir = max(dot(normal, negativeLightDir), 0.0);

    return (absorbedLight * albedo / PI + specular) * lightRadiance * normalDotLightDir;
}

vec3 ComputeDirectionalLightRadiance(int index)
{
    DirectionalLight light = g_DirectionalLights[index];
    vec3 radiance = light.Color.rgb * light.Intensity;
    return radiance;
}

vec3 ComputePointLightRadiance(int index, vec3 worldPos)
{
    PointLight light = g_PointLights[index];
    
    float dist = length(worldPos - light.Position);
    float attenuationFactor = dist / light.Radius;

    float attenuation = 0.0;
    if (attenuationFactor < 1.0)
    {
        float factor2 = attenuationFactor * attenuationFactor;
        attenuation = (1.0 - factor2) * (1.0 - factor2) / (1 + light.FallOff * attenuationFactor);
        attenuation = clamp(attenuation, 0.0, 1.0);
    }

    vec3 radiance = light.Color.rgb * light.Intensity * attenuation;
    return radiance;
}

vec3 IBL(vec3 normal, vec3 viewDir, vec3 albedo, float metalness, float roughness)
{    
    vec3 reflectivityAtZeroIncidence = vec3(0.04);
    reflectivityAtZeroIncidence = mix(reflectivityAtZeroIncidence, albedo, metalness);

    float NdotV = max(dot(normal, viewDir), 0.0);

    vec3 reflectedLight = FresnelSchlickRoughness(NdotV, reflectivityAtZeroIncidence, roughness); 
    vec3 absorbedLight = 1.0 - reflectedLight;
    absorbedLight *= 1.0 - metalness;

    vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
    vec3 diffuseIBL = absorbedLight * irradiance;

    vec3 reflectedVec = reflect(-viewDir, normal); 

    float lod = u_Renderer.EnvironmentLOD + roughness * (MAX_SKYBOX_MAP_LOD - u_Renderer.EnvironmentLOD);
    vec3 envMapReflectedColor = textureLod(u_EnvironmentMap, reflectedVec, lod).rgb;  
    vec2 envBRDF = texture(u_BRDF_LUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = envMapReflectedColor * (reflectedLight * envBRDF.x + envBRDF.y);

    vec3 ambient = (diffuseIBL * albedo + specularIBL) * u_Renderer.EnvironmentIntensity;
    return ambient;
}


void main()
{
    // Get PBR parameters
    vec4 albedo = u_Albedo;
    if (bool(u_UseAlbedoMap))
        albedo *= texture(u_AlbedoMap, Interpolators.TexCoords);
    
    vec3 normal = Interpolators.Normal;
    if(bool(u_UseNormalMap))
    {
        normal = texture(u_NormalMap, Interpolators.TexCoords).rgb;
        normal = normal * 2 - 1;
        normal = Interpolators.TBN * normal;
    }
    
    float roughness = bool(u_UseRoughnessMap) ? texture(u_RoughnessMap, Interpolators.TexCoords).r : u_Roughness;
    float metalness = bool(u_UseMetalnessMap) ? texture(u_MetalnessMap, Interpolators.TexCoords).r : u_Metalness;
    
    // Compute lightning from all light sources
    vec3 viewDir = normalize(u_Camera.Position - Interpolators.WorldPos);
    float distanceFromCamera = distance(u_Camera.Position, Interpolators.WorldPos);
    vec3 totalIrradiance = vec3(0.0);
    
    for (int i = 0; i < g_DirectionalLightCount; ++i)
    {
        vec3 dir = normalize(g_DirectionalLights[i].Direction);
        vec3 radiance = ComputeDirectionalLightRadiance(i);
        
        float shadow = ComputeDirectionalLightShadow(dir, normal, Interpolators.WorldPos, distanceFromCamera, u_Camera.View);
        
        if(shadow < 1.0)
            totalIrradiance += (1 - shadow) * LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
    }
    
    for (int j = 0; j < g_PointLightCount; ++j)
    {
        vec3 dir = normalize(Interpolators.WorldPos - g_PointLights[j].Position);
        vec3 radiance = ComputePointLightRadiance(j, Interpolators.WorldPos);
        
        totalIrradiance += LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
    }
    
    // Compute image based lightning
    vec3 ambient = IBL(normal, viewDir, albedo.rgb, metalness, roughness);

    vec3 emission = albedo.rgb * u_Emission;
    vec3 hdrColor = totalIrradiance + ambient + emission;
    
    if(bool(u_Renderer.DebugShadowCascades))
    {
        //vec3 cascadeDebugColor = GetCascadeDebugColor(Interpolators.WorldPos, u_Camera.View);
        //hdrColor.rgb = mix(hdrColor.rgb, cascadeDebugColor, 0.5);

        float value = PenumbraDebug(Interpolators.WorldPos, u_Camera.View);
        hdrColor.rgb = vec3(value);

        //float value = FindBlockerDebug(Interpolators.WorldPos, u_Camera.View);
        //hdrColor.rgb = vec3(value);
    }

    o_Color = vec4(hdrColor, albedo.a);
}
