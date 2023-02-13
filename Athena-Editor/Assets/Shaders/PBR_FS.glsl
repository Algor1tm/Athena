//#version 430 core

#define PI 3.14159265358979323846

#define MAX_SKYBOX_MAP_LOD 10

#define MAX_DIRECTIONAL_LIGHT_COUNT 32
#define MAX_POINT_LIGHT_COUNT 32


layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
};

layout(std140, binding = 2) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
};

layout(std140, binding = 3) uniform MaterialData
{
    vec4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_AmbientOcclusion;

	int u_UseAlbedoMap;
	int u_UseNormalMap;
	int u_UseRoughnessMap;
	int u_UseMetalnessMap;
    int u_UseAmbientOcclusionMap;
};

struct DirectionalLight
{
    vec4 Color;
    vec3 Direction;
    float Intensity;
};

struct PointLight
{
    vec4 Color;
    vec3 Position;
    float Intensity;
    float Radius;
    float FallOff;
};

layout(std430, binding = 4) readonly buffer LightBuffer
{
    DirectionalLight g_DirectionalLightBuffer[MAX_DIRECTIONAL_LIGHT_COUNT];
    int g_DirectionalLightCount;

    PointLight g_PointLightBuffer[MAX_POINT_LIGHT_COUNT];
    int g_PointLightCount;
};

layout(binding = 0) uniform sampler2D u_AlbedoMap;
layout(binding = 1) uniform sampler2D u_NormalMap;
layout(binding = 2) uniform sampler2D u_RoughnessMap;
layout(binding = 3) uniform sampler2D u_MetalnessMap;
layout(binding = 4) uniform sampler2D u_AmbientOcclusionMap;

layout(binding = 5) uniform samplerCube u_SkyboxMap;
layout(binding = 6) uniform samplerCube u_IrradianceMap;
layout(binding = 7) uniform sampler2D   u_BRDF_LUT;

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) in VertexOutput Input;


float DistributionGGX(vec3 normal, vec3 halfWay, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float normalDotHalfWay  = max(dot(normal, halfWay), 0.0);
    float normalDotHalfWay2 = normalDotHalfWay * normalDotHalfWay;
	
    float numerator = a2;
    float denominator = (normalDotHalfWay2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
	
    return numerator / denominator;
}

float GeometrySchlickGGX(float normalDotView, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float numerator = normalDotView;
    float denominator = normalDotView * (1.0 - k) + k;
	
    return numerator / denominator;
}

float GeometrySmith(vec3 normal, vec3 view, vec3 lightDir, float roughness)
{
    float normalDotView = max(dot(normal, view), 0.0);
    float normalDotLightDir = max(dot(normal, lightDir), 0.0);
    float ggx2 = GeometrySchlickGGX(normalDotView, roughness);
    float ggx1 = GeometrySchlickGGX(normalDotLightDir, roughness);
	
    return ggx1 * ggx2;
}

vec3 FresnelShlick(float cosHalfWayAndView, vec3 reflectivityAtZeroIncidence)
{
    return reflectivityAtZeroIncidence + (1.0 - reflectivityAtZeroIncidence) * pow(clamp(1.0 - cosHalfWayAndView, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosHalfWayAndView, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosHalfWayAndView, 0.0, 1.0), 5.0);
}  

vec3 ComputeRadiance(vec3 lightDirection, vec3 lightRadiance, vec3 normal, vec3 viewVector, vec3 albedo, float metalness, float roughness, vec3 reflectivityAtZeroIncidence)
{
    vec3 negativeLightDir = -lightDirection;
    vec3 halfWayVector = normalize(viewVector + negativeLightDir);

    float NDF = DistributionGGX(normal, halfWayVector, roughness);
    float G = GeometrySmith(normal, viewVector, negativeLightDir, roughness);
    vec3 F = FresnelShlick(max(dot(halfWayVector, viewVector), 0.0), reflectivityAtZeroIncidence);

    vec3 reflectedLight = F;
    vec3 absorbedLight = vec3(1.0) - reflectedLight;
        
    absorbedLight *= 1.0 - metalness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewVector), 0.0) * max(dot(normal, negativeLightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float normalDotLightDir = max(dot(normal, negativeLightDir), 0.0);

    return (absorbedLight * albedo / PI + specular) * lightRadiance * normalDotLightDir;
}


void main()
{
    vec4 albedo = u_Albedo;
    if(bool(u_UseAlbedoMap))
        albedo *= texture(u_AlbedoMap, Input.TexCoord);

    vec3 normal;
    if(bool(u_UseNormalMap))
    {
        normal = texture(u_NormalMap, Input.TexCoord).rgb;
        normal = normal * 2 - 1;
        normal = normalize(Input.TBN * normal);
    }
    else
    {
        normal = normalize(Input.Normal);
    }

    float roughness;
    if(bool(u_UseRoughnessMap))
        roughness = texture(u_RoughnessMap, Input.TexCoord).r;
    else
        roughness = u_Roughness;

    float metalness;
    if(bool(u_UseMetalnessMap))
        metalness = texture(u_MetalnessMap, Input.TexCoord).r;
    else
        metalness = u_Metalness;

    float ambientOcclusion;
    if(bool(u_UseAmbientOcclusionMap))
        ambientOcclusion = texture(u_AmbientOcclusionMap, Input.TexCoord).r;
    else
        ambientOcclusion = u_AmbientOcclusion;
    
    vec3 reflectivityAtZeroIncidence = vec3(0.04);
    reflectivityAtZeroIncidence = mix(reflectivityAtZeroIncidence, albedo.rgb, metalness);

    vec3 viewVector = normalize(u_CameraPosition.xyz - Input.WorldPos);

    vec3 totalIrradiance = vec3(0.0);
    

    for(int i = 0; i < g_DirectionalLightCount; ++i)
    {
        vec3 lightDirection = normalize(g_DirectionalLightBuffer[i].Direction);
        vec3 lightRadiance = vec3(g_DirectionalLightBuffer[i].Color) * g_DirectionalLightBuffer[i].Intensity;

        totalIrradiance += ComputeRadiance(lightDirection, lightRadiance, normal, viewVector, albedo.rgb, metalness, roughness, reflectivityAtZeroIncidence);
    }

    for(int i = 0; i < g_PointLightCount; ++i)
    {
        float distance = length(Input.WorldPos - g_PointLightBuffer[i].Position);

        vec3 lightDirection = (Input.WorldPos - g_PointLightBuffer[i].Position) / (distance * distance);

        float factor = distance / g_PointLightBuffer[i].Radius;

        float attenuation = 0.0;
        if(factor < 1.0)
        {
            float factor2 = factor * factor;
            attenuation = (1.0 - factor2) * (1.0 - factor2) / (1 + g_PointLightBuffer[i].FallOff * factor);
            attenuation = clamp(attenuation, 0.0, 1.0);
        }

        vec3 lightRadiance = g_PointLightBuffer[i].Color.rgb * g_PointLightBuffer[i].Intensity * attenuation;

        totalIrradiance += ComputeRadiance(lightDirection, lightRadiance, normal, viewVector, albedo.rgb, metalness, roughness, reflectivityAtZeroIncidence);
    }


    vec3 reflectedVec = reflect(-viewVector, normal); 

    float NdotV = max(dot(normal, viewVector), 0.0);

    vec3 reflectedLight = FresnelSchlickRoughness(NdotV, reflectivityAtZeroIncidence, roughness); 
    vec3 absorbedLight = 1.0 - reflectedLight;
    absorbedLight *= 1.0 - metalness;

    vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo.rgb;

    vec3 prefilteredColor = textureLod(u_SkyboxMap, reflectedVec, roughness * MAX_SKYBOX_MAP_LOD).rgb;  
    vec2 envBRDF = texture(u_BRDF_LUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (reflectedLight * envBRDF.x + envBRDF.y);

    vec3 ambient = (absorbedLight * diffuse + specular) * ambientOcclusion;

    vec3 color = ambient + totalIrradiance;

    color = vec3(1.0) - exp(-color * u_Exposure);
    color = pow(color, vec3(1.0/2.2)); 

    out_Color = vec4(color, albedo.a);

    out_EntityID = u_EntityID;
}
