#type VERTEX_SHADER
#version 460 core


layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) out VertexOutput Output;


layout(std140, binding = SCENE_BUFFER_BINDER) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_NearClip;
	float u_FarClip;
    float u_SkyboxLOD;
	float u_Exposure;
};

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
    bool u_Animated;
};


layout(std430, binding = BONES_BUFFER_BINDER) readonly buffer BoneTransforms
{
    mat4 g_Bones[MAX_NUM_BONES];
};


void main()
{
    mat4 fullTransform = u_Transform;

    if(bool(u_Animated))
    {
        mat4 boneTransform = g_Bones[a_BoneIDs[0]] * a_Weights[0];
        for(int i = 1; i < MAX_NUM_BONES_PER_VERTEX; ++i)
        {
            boneTransform += g_Bones[a_BoneIDs[i]] * a_Weights[i];
        }

       fullTransform *= boneTransform;
    }

    vec4 transformedPos = fullTransform * vec4(a_Position, 1);

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * transformedPos;

    Output.WorldPos = vec3(transformedPos);
    Output.TexCoord = a_TexCoord;
    Output.Normal = normalize(vec3(fullTransform * vec4(a_Normal, 0)));

    vec3 T = normalize(vec3(fullTransform * vec4(a_Tangent, 0)));
    vec3 N = Output.Normal;
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(vec3(fullTransform * vec4(a_Bitangent, 0)));
    Output.TBN = mat3(T, B, N);
}


#type FRAGMENT_SHADER
#version 460 core


layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) in VertexOutput Input;


layout(std140, binding = SCENE_BUFFER_BINDER) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_NearClip;
	float u_FarClip;
    float u_SkyboxLOD;
	float u_Exposure;
};

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
    bool u_Animated;
};

layout(std140, binding = MATERIAL_BUFFER_BINDER) uniform MaterialData
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

layout(std140, binding = SHADOWS_BUFFER_BINDER) uniform ShadowsData
{
    vec4 u_CascadePlaneDistances;
	float u_MaxShadowDistance;
	float u_FadeOut;
	float u_LightSize;
	bool u_SoftShadows;
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

layout(std430, binding = LIGHT_BUFFER_BINDER) readonly buffer LightBuffer
{
    mat4 g_DirectionalLightSpaceMatrices[SHADOW_CASCADES_COUNT];
    DirectionalLight g_DirectionalLightBuffer[MAX_DIRECTIONAL_LIGHT_COUNT];
    int g_DirectionalLightCount;

    PointLight g_PointLightBuffer[MAX_POINT_LIGHT_COUNT];
    int g_PointLightCount;
};

layout(binding = ALBEDO_MAP_BINDER)     uniform sampler2D u_AlbedoMap;
layout(binding = NORMAL_MAP_BINDER)     uniform sampler2D u_NormalMap;
layout(binding = ROUGHNESS_MAP_BINDER)  uniform sampler2D u_RoughnessMap;
layout(binding = METALNESS_MAP_BINDER)  uniform sampler2D u_MetalnessMap;
layout(binding = AO_MAP_BINDER)         uniform sampler2D u_AmbientOcclusionMap;

layout(binding = SKYBOX_MAP_BINDER)     uniform samplerCube u_SkyboxMap;
layout(binding = IRRADIANCE_MAP_BINDER) uniform samplerCube u_IrradianceMap;
layout(binding = BRDF_LUT_BINDER)       uniform sampler2D u_BRDF_LUT;

layout(binding = SHADOW_MAP_BINDER)     uniform sampler2DArray u_ShadowMap;



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

vec3 LightContribution(vec3 lightDirection, vec3 lightRadiance, vec3 normal, vec3 viewVector, vec3 albedo, float metalness, float roughness, vec3 reflectivityAtZeroIncidence)
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

float ComputeShadow(vec3 normal, vec3 lightDir, float distanceFromCamera)
{
    float fade = 0.0;
    if(distanceFromCamera > u_MaxShadowDistance)
    {
        // TODO: fade out effect
        fade = 1.0;
    }


    vec4 worldPosViewSpace = u_ViewMatrix * vec4(Input.WorldPos, 1.0);
    float depthValue = abs(worldPosViewSpace.z);

    int layer = SHADOW_CASCADES_COUNT;
    for(int i = 0; i < SHADOW_CASCADES_COUNT; ++i)
    {
        if(depthValue < u_CascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }

    vec4 lightSpacePosition = g_DirectionalLightSpaceMatrices[layer] * vec4(Input.WorldPos, 1.0);

    vec3 projCoords = 0.5 * lightSpacePosition.xyz / lightSpacePosition.w + 0.5;
    float currentDepth = projCoords.z;

    if(currentDepth > 1.0) return 0.0;

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    bias *= layer == SHADOW_CASCADES_COUNT ? 1.0 / (u_FarClip * 0.5) : 1.0 / u_CascadePlaneDistances[layer] * 0.5;

    float closestDepth = texture(u_ShadowMap, vec3(projCoords.xy, layer)).r;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return (1 - fade) * shadow;
}


void main()
{
    ////////////////// PBR TEXTURES //////////////////
    vec4 albedo = u_Albedo;
    if(bool(u_UseAlbedoMap))
        albedo *= texture(u_AlbedoMap, Input.TexCoord);

    vec3 normal;
    if(bool(u_UseNormalMap))
    {
        normal = texture(u_NormalMap, Input.TexCoord).rgb;
        normal = normal * 2 - 1;
        normal = Input.TBN * normal;
    }
    else
    {
        normal = Input.Normal;
    }

    float roughness = bool(u_UseRoughnessMap) ? texture(u_RoughnessMap, Input.TexCoord).r : u_Roughness;
    float metalness = bool(u_UseMetalnessMap) ? texture(u_MetalnessMap, Input.TexCoord).r : u_Metalness;
    float ambientOcclusion = bool(u_UseAmbientOcclusionMap) ? texture(u_AmbientOcclusionMap, Input.TexCoord).r : 1.0;
    
    vec3 reflectivityAtZeroIncidence = vec3(0.04);
    reflectivityAtZeroIncidence = mix(reflectivityAtZeroIncidence, albedo.rgb, metalness);

    vec3 viewVector = normalize(u_CameraPosition.xyz - Input.WorldPos);

    vec3 totalIrradiance = vec3(0.0);

    ////////////////// DIRECTIONAL LIGHTS //////////////////
    for(int i = 0; i < g_DirectionalLightCount; ++i)
    {
        vec3 lightDirection = normalize(g_DirectionalLightBuffer[i].Direction);
        vec3 lightRadiance = vec3(g_DirectionalLightBuffer[i].Color) * g_DirectionalLightBuffer[i].Intensity;

        float distanceFromCamera = distance(u_CameraPosition.xyz, Input.WorldPos);
        float shadow = ComputeShadow(normal, -lightDirection, distanceFromCamera);
        
        totalIrradiance += (1 - shadow) * LightContribution(lightDirection, lightRadiance, normal, viewVector, albedo.rgb, metalness, roughness, reflectivityAtZeroIncidence);
    }

    ////////////////// POINT LIGHTS //////////////////
    for(int i = 0; i < g_PointLightCount; ++i)
    {
        float distanceFromCamera = length(Input.WorldPos - g_PointLightBuffer[i].Position);
        vec3 lightDirection = (Input.WorldPos - g_PointLightBuffer[i].Position) / (distanceFromCamera * distanceFromCamera);
        float factor = distanceFromCamera / g_PointLightBuffer[i].Radius;

        float attenuation = 0.0;
        if(factor < 1.0)
        {
            float factor2 = factor * factor;
            attenuation = (1.0 - factor2) * (1.0 - factor2) / (1 + g_PointLightBuffer[i].FallOff * factor);
            attenuation = clamp(attenuation, 0.0, 1.0);
        }

        vec3 lightRadiance = g_PointLightBuffer[i].Color.rgb * g_PointLightBuffer[i].Intensity * attenuation;

        totalIrradiance += LightContribution(lightDirection, lightRadiance, normal, viewVector, albedo.rgb, metalness, roughness, reflectivityAtZeroIncidence);
    }

   ////////////////// ENVIRONMENT MAP LIGHTNING //////////////////
    float NdotV = max(dot(normal, viewVector), 0.0);

    vec3 reflectedLight = FresnelSchlickRoughness(NdotV, reflectivityAtZeroIncidence, roughness); 
    vec3 absorbedLight = 1.0 - reflectedLight;
    absorbedLight *= 1.0 - metalness;

    vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
    vec3 diffuseIBL = absorbedLight * irradiance;

    vec3 reflectedVec = reflect(-viewVector, normal); 

    vec3 prefilteredColor = textureLod(u_SkyboxMap, reflectedVec, roughness * MAX_SKYBOX_MAP_LOD).rgb;  
    vec2 envBRDF = texture(u_BRDF_LUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = prefilteredColor * (reflectedLight * envBRDF.x + envBRDF.y);


    ////////////////// MAIN COLOR //////////////////
    vec3 ambient = (diffuseIBL * albedo.rgb + specularIBL) * ambientOcclusion;
    vec3 color = ambient + totalIrradiance;

    ////////////////// TONE MAPPING //////////////////
    color = vec3(1.0) - exp(-color * u_Exposure);

    ////////////////// GAMMA CORRECTION //////////////////
    color = pow(color, vec3(1.0 / 2.2)); 


    out_Color = vec4(color, albedo.a);
    out_EntityID = u_EntityID;
}
