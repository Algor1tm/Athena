#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

struct VertexOutput
{
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
    float u_MipLevel;
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

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

struct VertexOutput
{
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) in VertexOutput Input;


layout(binding = NORMAL_MAP_BINDER) uniform sampler2D u_NormalMap;

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


void main()
{
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

    out_Color = vec4(abs(normal), 1);
    out_EntityID = u_EntityID;
}
