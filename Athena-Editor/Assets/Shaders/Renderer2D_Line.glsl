#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in int a_EntityID;


layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) out VertexOutput Output;
layout (location = 1) flat out int v_EntityID;

void main()
{
	Output.Color = a_Color;
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
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
layout (location = 1) flat in int v_EntityID;

void main()
{
	out_Color = Input.Color;
	out_EntityID = v_EntityID;
}
