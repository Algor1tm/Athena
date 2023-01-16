#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ModelViewProjection;
};

struct VertexOutput
{
	vec2 TexCoord;
    float LightScalar;
};

layout (location = 0) out VertexOutput Output;

void main()
{
    gl_Position = u_ModelViewProjection * vec4(a_Position, 1);

    Output.TexCoord = a_TexCoord;

    vec3 LightDirection = normalize(vec3(-0.25, -0.75, -1));
    Output.LightScalar = -dot(LightDirection, a_Normal);
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

layout(std140, binding = 2) uniform MaterialData
{
    vec3 u_Albedo;
    float u_Roughness;
    float u_Metalness;

	bool u_UseAlbedoTexture;
	bool u_UseNormalMap;
	bool u_UseRoughnessMap;
	bool u_UseMetalnessMap;
};

uniform sampler2D u_AlbedoTexture;
uniform sampler2D u_NormalMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_MetalnessMap;

struct VertexOutput
{
	vec2 TexCoord;
    float LightScalar;
};

layout (location = 0) in VertexOutput Input;

void main()
{
    vec3 LightColor = vec3(0.8, 0.8, 0.8);
    vec4 factor = vec4(LightColor, 1) * vec4(vec3(Input.LightScalar), 1);

    vec4 color = vec4(u_Albedo, 1);
    if(u_UseAlbedoTexture)
        color *= texture(u_AlbedoTexture, Input.TexCoord);

    out_Color = clamp(color * factor, 0.05, 1.);
    
    out_EntityID = 0;
}
