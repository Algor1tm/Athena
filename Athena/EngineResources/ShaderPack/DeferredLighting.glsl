//////////////////////// Athena Deferred Lighting Shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoords;

layout(location = 0) out vec2 v_TexCoords;

void main()
{
    gl_Position = vec4(a_Position, 0, 1);
    v_TexCoords = a_TexCoords;
}

#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"
#include "Include/Shadows.glslh"

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 9) uniform sampler2D u_SceneDepth;
layout(set = 1, binding = 10) uniform sampler2D u_SceneAlbedo;
layout(set = 1, binding = 11) uniform sampler2D u_SceneNormalsEmission;
layout(set = 1, binding = 12) uniform sampler2D u_SceneRoughnessMetalness;

layout(set = 1, binding = 13) uniform sampler2D u_BRDF_LUT;
layout(set = 1, binding = 14) uniform samplerCube u_EnvironmentMap;
layout(set = 1, binding = 15) uniform samplerCube u_IrradianceMap;


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

vec3 ComputeDirectionalLightRadiance(DirectionalLight light)
{
    vec3 radiance = light.Color.rgb * light.Intensity;
    return radiance;
}

vec3 ComputePointLightRadiance(PointLight light, vec3 worldPos)
{
    float dist = length(worldPos - light.Position);

    if(dist >= light.Radius)
        return vec3(0.0);

    float attenuationFactor = dist / light.Radius;

    float factor2 = attenuationFactor * attenuationFactor;
    float attenuation = (1.0 - factor2) * (1.0 - factor2) / (1 + light.FallOff * factor2);
    attenuation = clamp(attenuation, 0.0, 1.0);

    vec3 radiance = light.Color.rgb * light.Intensity * attenuation;
    return radiance;
}

vec3 ComputeSpotLightRadiance(SpotLight light, vec3 worldPos)
{
    float dist = length(worldPos - light.Position);

    if(dist >= light.Range)
        return vec3(0.0);

    vec3 lightToPixel = normalize(worldPos - light.Position);
    float theta = dot(lightToPixel, light.Direction);

    if(theta < light.SpotAngle)
        return vec3(0.0);

    float distFactor = dist / light.Range;

    float distFactor2 = distFactor * distFactor;
    float rangeAttenuation = (1.0 - distFactor2) * (1.0 - distFactor2) / (1 + light.RangeFallOff * distFactor2);
    rangeAttenuation = clamp(rangeAttenuation, 0.0, 1.0);
    
    float thetaFactor = 1.0 - (theta - light.SpotAngle) / (1.0 - light.SpotAngle);

    float thetaFactor2 = thetaFactor * thetaFactor;
    float innerAttenuation = (1.0 - thetaFactor2) * (1.0 - thetaFactor2) / (1 + light.InnerFallOff * thetaFactor2);
    innerAttenuation = clamp(innerAttenuation, 0.0, 1.0);

    float attenuation = rangeAttenuation * innerAttenuation;

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

vec3 GetLightComplexityDebugColor(uint count)
{
    if(count < 2)
        return vec3(0, 0.2, 1);
    if(count < 3)
        return vec3(0, 1, 1);
    if(count <= 4)
        return vec3(0, 1, 0);
    if(count <= 8)
        return vec3(1, 1, 0);
    if(count <= 12)
        return vec3(1, 0.4, 0);
    if(count < MAX_POINT_LIGHT_COUNT_PER_TILE)
        return vec3(1.0, 0.05, 0);

    return vec3(0, 0, 0);
}


void main()
{
    // Unpack GBuffer
    float depth = texture(u_SceneDepth, v_TexCoords).r;
    if(depth == 0.0)
        discard;

    vec4 clipSpace = vec4(v_TexCoords * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = u_Camera.InverseProjection * clipSpace;
    viewPos /= viewPos.w;
    vec3 worldPos = vec3(u_Camera.InverseView * viewPos);

    vec4 normalEmission = texture(u_SceneNormalsEmission, v_TexCoords);
    vec3 normal = normalEmission.rgb * 2.0 - 1.0;
    float emission = normalEmission.a;

    vec3 albedo = texture(u_SceneAlbedo, v_TexCoords).rgb;

    vec2 rm = texture(u_SceneRoughnessMetalness, v_TexCoords).rg;
    float roughness = rm.r;
    float metalness = rm.g;
    
    // Compute lightning from all light sources
    vec3 viewDir = normalize(u_Camera.Position - worldPos);
    float distanceFromCamera = distance(u_Camera.Position, worldPos);
    vec3 totalIrradiance = vec3(0.0);
    
    vec2 screenUV = vec2(gl_FragCoord);

    for (int i = 0; i < g_DirectionalLightCount; ++i)
    {
        DirectionalLight light = g_DirectionalLights[i];
        vec3 dir = light.Direction;
        vec3 radiance = ComputeDirectionalLightRadiance(light);
        
        float shadow = 0.0;
        if(bool(light.CastShadows))
            shadow = ComputeDirectionalLightShadow(light.LightSize, dir, normal, worldPos, distanceFromCamera, u_Camera.View, screenUV);
        
        if(shadow < 1.0)
            totalIrradiance += (1 - shadow) * LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
    }
    
    ivec2 tileID = ivec2(gl_FragCoord.xy / LIGHT_TILE_SIZE);
	uint tileIndex = tileID.y * u_Renderer.ViewportTilesCount.x + tileID.x;
    TileVisibleLights tileData = u_VisibleLights[tileIndex];

#if 0
    //for (int j = 0; j < g_PointLightCount; ++j)
    //{
    //    PointLight light = g_PointLights[j];
#else
    for (int j = 0; j < tileData.LightCount; ++j)
    {
        uint lightIndex = tileData.LightIndices[j];
        PointLight light = g_PointLights[lightIndex];
#endif
        vec3 radiance = ComputePointLightRadiance(light, worldPos);
        
        if(radiance != vec3(0.0))
        {
            vec3 dir = normalize(worldPos - light.Position);
            totalIrradiance += LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
        }
    }

    for (int j = 0; j < g_SpotLightCount; ++j)
    {
        SpotLight light = g_SpotLights[j];
        vec3 radiance = ComputeSpotLightRadiance(light, worldPos);
        
        if(radiance != vec3(0.0))
        {
            vec3 dir = normalize(worldPos - light.Position);
            totalIrradiance += LightContribution(dir, radiance, normal, viewDir, albedo.rgb, metalness, roughness);
        }
    }
    
    vec3 ambient = IBL(normal, viewDir, albedo.rgb, metalness, roughness);
    vec3 emissionColor = albedo.rgb * emission;

    vec3 hdrColor = totalIrradiance + ambient + emissionColor;
    
    if(bool(u_Renderer.DebugShadowCascades))
    {
        vec3 cascadeDebugColor = GetCascadeDebugColor(worldPos, u_Camera.View);
        hdrColor = mix(hdrColor, cascadeDebugColor, 0.3);
    }
    else if(bool(u_Renderer.DebugLightComplexity))
    {
        if(tileData.LightCount != 0)
        {
            vec3 color = GetLightComplexityDebugColor(tileData.LightCount);
            hdrColor = mix(hdrColor, vec3(color), 0.65);
        }   
    }

    o_Color = vec4(hdrColor, 1.0);
}
