//////////////////////// Athena G-Buffer Shader ////////////////////////

#version 460 core
#pragma stage : vertex

#include "Include/Buffers.glslh"

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in ivec4 a_BoneIDs;
layout(location = 6) in vec4 a_Weights;

layout(location = 7) in vec3 a_TRow0;
layout(location = 8) in vec3 a_TRow1;
layout(location = 9) in vec3 a_TRow2;
layout(location = 10) in vec3 a_TRow3;

struct VertexInterpolators
{
    vec2 TexCoords;
    vec3 Normal;
    mat3 TBN;
};

layout(location = 0) out VertexInterpolators Interpolators;

layout(push_constant) uniform u_MaterialData
{
    uint u_BonesOffset;

    uint u_UseAlbedoMap;
    uint u_UseNormalMap;
    uint u_UseRoughnessMap;

    vec4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_Emission;

    uint u_UseMetalnessMap;
};


void main()
{
    mat4 worldTransform = GetTransform(a_TRow0, a_TRow1, a_TRow2, a_TRow3);
    mat4 transform = worldTransform * GetBonesTransform(u_BonesOffset, a_BoneIDs, a_Weights);

    vec4 worldPos = transform * vec4(a_Position, 1.0);
    gl_Position = u_Camera.ViewProjection * worldPos;

    Interpolators.TexCoords = a_TexCoords;
    Interpolators.Normal = normalize(transform * vec4(a_Normal, 0)).xyz;

    vec3 T = normalize(transform * vec4(a_Tangent, 0)).xyz;
    vec3 B = normalize(transform * vec4(a_Bitangent, 0)).xyz;
    vec3 N =  Interpolators.Normal;
    T = normalize(T - dot(T, N) * N);
    
    Interpolators.TBN = mat3(T, B, N);
}

#version 460 core
#pragma stage : fragment

#include "Include/Common.glslh"

struct VertexInterpolators
{
    vec2 TexCoords;
    vec3 Normal;
    mat3 TBN;
};

layout(location = 0) in VertexInterpolators Interpolators;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_NormalsEmission;
layout(location = 2) out vec2 o_RoughnessMetalness;

layout(push_constant) uniform u_MaterialData
{
    uint u_BonesOffset;

    uint u_UseAlbedoMap;
    uint u_UseNormalMap;
    uint u_UseRoughnessMap;

    vec4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_Emission;

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
    o_NormalsEmission.rgb = normal * 0.5 + 0.5;
    o_NormalsEmission.a = u_Emission;
    o_RoughnessMetalness.r = roughness;
    o_RoughnessMetalness.g = metalness;
}
