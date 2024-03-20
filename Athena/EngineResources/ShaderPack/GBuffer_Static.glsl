//////////////////////// Athena G-Buffer Shader ////////////////////////

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

layout(push_constant) uniform u_MaterialData
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

#include "Include/Common.glslh"

struct VertexInterpolators
{
    vec3 WorldPos;
    vec2 TexCoords;
    vec3 Normal;
    mat3 TBN;
};

layout(location = 0) in VertexInterpolators Interpolators;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_PositionEmission;
layout(location = 2) out vec4 o_Normals;
layout(location = 3) out vec2 o_RoughnessMetalness;

layout(push_constant) uniform u_MaterialData
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


void main()
{
    vec4 albedo = u_Albedo;
    if (bool(u_UseAlbedoMap))
        albedo *= texture(u_AlbedoMap, Interpolators.TexCoords);
    
    vec3 normal = normalize(Interpolators.Normal);
    if(bool(u_UseNormalMap))
    {
        normal = texture(u_NormalMap, Interpolators.TexCoords).rgb;
        normal = normal * 2 - 1;
        normal = normalize(Interpolators.TBN * normal);
    }
    
    float roughness = bool(u_UseRoughnessMap) ? texture(u_RoughnessMap, Interpolators.TexCoords).r : u_Roughness;
    float metalness = bool(u_UseMetalnessMap) ? texture(u_MetalnessMap, Interpolators.TexCoords).r : u_Metalness;

    o_Albedo = vec4(albedo.rgb, 1.0);
    o_PositionEmission.xyz = Interpolators.WorldPos;
    o_PositionEmission.w = u_Emission;
    o_Normals.xy = PackNormal(normal);
    o_RoughnessMetalness.r = roughness;
    o_RoughnessMetalness.g = metalness;
}
