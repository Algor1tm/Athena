#type VERTEX_SHADER
#version 460 core

#define MAX_NUM_BONES_PER_VERTEX 4
#define MAX_NUM_BONES 512

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

layout(location = 0) out vec3 WorldPosViewSpace;

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

    WorldPosViewSpace = vec3(u_ViewMatrix * transformedPos);
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * transformedPos;
}

#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

layout(location = 0) in vec3 WorldPosViewSpace;

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
    bool u_Animated;
};

struct CascadeSplitInfo
{
    float SplitDepth;
    vec2 FrustumPlanes;
    float _Padding;
};

layout(std140, binding = SHADOWS_BUFFER_BINDER) uniform ShadowsData
{
    mat4 u_LightViewProjMatrices[SHADOW_CASCADES_COUNT];
    mat4 u_LightViewMatrices[SHADOW_CASCADES_COUNT];
    CascadeSplitInfo u_CascadeSplits[SHADOW_CASCADES_COUNT];
	float u_MaxShadowDistance;
	float u_FadeOut;
	float u_LightSize;
	bool u_SoftShadows;
};


void main()
{
    float depthValue = abs(WorldPosViewSpace.z);

    int layer = SHADOW_CASCADES_COUNT;
    for(int i = 0; i < SHADOW_CASCADES_COUNT; ++i)
    {
        if(depthValue < u_CascadeSplits[i].SplitDepth)
        {
            layer = i;
            break;
        }
    }

    vec3 color = vec3(0.0, 0.0, 0.0);

    if(layer == 0)
        color = vec3(1.0, 0.0, 0.0);
    else if(layer == 1)
        color = vec3(0.0, 1.0, 0.0);
    else if(layer == 2)
        color = vec3(0.0, 0.0, 1.0);
    else if(layer == 3)
        color = vec3(1.0, 1.0, 0.0);
    else if(layer == 4)
        color = vec3(0.0, 1.0, 1.0);
    else if(layer == 5)
        color = vec3(1.0, 0.0, 1.0);
    else if(layer == 6)
        color = vec3(0.5, 0.5, 0.5);

    out_Color = vec4(color, 1);
    out_EntityID = u_EntityID;
}
