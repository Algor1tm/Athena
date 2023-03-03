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


layout(std140, binding = SCENE_BUFFER_BINDER) uniform SceneData
{
	mat4 u_LightSpaceMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
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

    gl_Position = u_LightSpaceMatrix * transformedPos;
}


#type FRAGMENT_SHADER
#version 460 core

void main()
{

}
