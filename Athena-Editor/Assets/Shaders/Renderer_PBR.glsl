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

struct VertexOutput
{
	vec2 TexCoord;
    float LightScalar;
};

layout (location = 0) in VertexOutput Input;

void main()
{
    vec3 LightColor = vec3(0.8, 0.8, 0.8);
    vec3 Color = vec3(0.8, 0.8, 0.8);

    out_Color = clamp(vec4(Color * LightColor * vec3(Input.LightScalar), 1), 0.05, 1.);
    out_EntityID = 0;
}
