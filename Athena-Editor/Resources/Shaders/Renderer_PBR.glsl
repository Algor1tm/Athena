#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ModelViewProjection;
};

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) out VertexOutput Output;

void main()
{
    gl_Position = u_ModelViewProjection * vec4(a_Position, 1);
    Output.Color = a_Color;
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) in VertexOutput Input;

void main()
{
    out_Color = Input.Color;
    out_EntityID = 0;
}
