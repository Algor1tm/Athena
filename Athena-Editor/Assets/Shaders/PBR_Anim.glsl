#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    mat4 u_Transform;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
    int u_EntityID;
};

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

#define MAX_BONES 512

layout(std430, binding = 4) readonly buffer BoneTransforms
{
    mat4 g_Bones[MAX_BONES];
};

layout (location = 0) out VertexOutput Output;

void main()
{
    mat4 boneTransform = g_Bones[a_BoneIDs[0]] * a_Weights[0];
    boneTransform     += g_Bones[a_BoneIDs[1]] * a_Weights[1];
    boneTransform     += g_Bones[a_BoneIDs[2]] * a_Weights[2];
    boneTransform     += g_Bones[a_BoneIDs[3]] * a_Weights[3];

    mat4 fullTransform = u_Transform * boneTransform;
    vec4 animatedPos = fullTransform * vec4(a_Position, 1);

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * animatedPos;

    Output.WorldPos = vec3(animatedPos);
    Output.TexCoord = a_TexCoord;
    Output.Normal = normalize(vec3(fullTransform * vec4(a_Normal, 0)));

    vec3 T = normalize(vec3(fullTransform * vec4(a_Tangent, 0)));
    vec3 N = Output.Normal;
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(vec3(fullTransform * vec4(a_Bitangent, 0)));
    Output.TBN = mat3(T, B, N);
}


#type FRAGMENT_SHADER
#version 430 core
#extension GL_ARB_shading_language_include : require

#include "/PBR_FS.glsl"
