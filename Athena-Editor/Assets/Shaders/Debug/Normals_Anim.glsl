#type VERTEX_SHADER
#version 430 core

#define MAX_NUM_BONES_PER_VERTEX 4
#define MAX_NUM_BONES 512

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
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
};

layout(std140, binding = 2) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
};

struct VertexOutput
{
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout(std430, binding = 5) readonly buffer BoneTransforms
{
    mat4 g_Bones[MAX_NUM_BONES];
};

layout (location = 0) out VertexOutput Output;

void main()
{
    mat4 boneTransform = g_Bones[a_BoneIDs[0]] * a_Weights[0];
    for(int i = 1; i < MAX_NUM_BONES_PER_VERTEX; ++i)
    {
        boneTransform += g_Bones[a_BoneIDs[i]] * a_Weights[i];
    }

    mat4 fullTransform = u_Transform * boneTransform;
    vec4 animatedPos = fullTransform * vec4(a_Position, 1);

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * animatedPos;

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

layout(binding = 1) uniform sampler2D u_NormalMap;

struct VertexOutput
{
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) in VertexOutput Input;

void main()
{
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

    out_Color = vec4(abs(normal), 1);
    out_EntityID = u_EntityID;
}
