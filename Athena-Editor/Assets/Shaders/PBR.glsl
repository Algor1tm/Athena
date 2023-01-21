#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewProjection;
    mat4 u_Transform;
    int u_EntityID;
};

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) out VertexOutput Output;
layout (location = 6) flat out int v_EntityID;

void main()
{
    gl_Position = u_ViewProjection *  u_Transform * vec4(a_Position, 1);

    Output.WorldPos = vec3(u_Transform * vec4(a_Position, 1));
    Output.TexCoord = a_TexCoord;
    Output.Normal = vec3(u_Transform * vec4(a_Normal, 0));

    vec3 T = normalize(vec3(u_Transform * vec4(a_Tangent, 0)));
    vec3 N = Output.Normal;
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(vec3(u_Transform * vec4(a_Bitangent, 0)));
    Output.TBN = mat3(T, B, N);

    v_EntityID = u_EntityID;
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

layout(std140, binding = 2) uniform MaterialData
{
    vec4 u_Albedo;
    float u_Roughness;
    float u_Metalness;
    float u_AmbientOcclusion;

	int u_UseAlbedoTexture;
	int u_UseNormalMap;
	int u_UseRoughnessMap;
	int u_UseMetalnessMap;
    int u_UseAmbientOcclusionMap;
};

layout(binding = 0) uniform sampler2D u_AlbedoTexture;
layout(binding = 1) uniform sampler2D u_NormalMap;
layout(binding = 2) uniform sampler2D u_RoughnessMap;
layout(binding = 3) uniform sampler2D u_MetalnessMap;
layout(binding = 4) uniform sampler2D u_AmbientOcclusionMap;

layout(std140, binding = 3) uniform DirectionalLight
{
    vec4 Color;
    vec3 Direction;
    float Intensity;
} Light;

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) in VertexOutput Input;
layout (location = 6) flat in int v_EntityID;

void main()
{
    vec4 albedo = u_Albedo;
    if(bool(u_UseAlbedoTexture))
        albedo *= texture(u_AlbedoTexture, Input.TexCoord);

    vec3 normal;
    if(bool(u_UseNormalMap))
    {
        normal = texture(u_NormalMap, Input.TexCoord).rgb;
        normal = normal * 2 - 1;
        normal = normalize(Input.TBN * normal);
    }
    else
    {
        normal = Input.Normal;
    }

    float diff = -dot(normalize(Light.Direction), normal);
    vec3 diffuse = Light.Intensity * diff * Light.Color.rgb;

    out_Color = max(albedo * vec4(diffuse, 1), albedo / 10);
    //out_Color = vec4(normal, 1);
    //out_Color = vec4(texture(u_NormalMap, Input.TexCoord).rgb, 1);
    out_EntityID = v_EntityID;
}
