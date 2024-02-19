//////////////////////// Athena Directional Light Shadow Map Shader////////////////////////

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

    uint u_BonesOffset;
};


void main()
{
    mat4 bonesTransform = g_Bones[u_BonesOffset + a_BoneIDs[0]] * a_Weights[0];
    for(int i = 1; i < MAX_NUM_BONES_PER_VERTEX; ++i)
    {
        bonesTransform += g_Bones[u_BonesOffset + a_BoneIDs[i]] * a_Weights[i];
    }

    mat4 transform = u_Transform * bonesTransform;

    gl_Position = transform * vec4(a_Position, 1.0);
}

#version 460 core
#pragma stage : geometry


layout(triangles, invocations = SHADOW_CASCADES_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;
    
layout(std140, set = 1, binding = 0) uniform u_ShadowsData
{
    mat4 u_DirLightViewProjection[SHADOW_CASCADES_COUNT];
};

void main()
{          
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = u_DirLightViewProjection[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }

    EndPrimitive();
} 
