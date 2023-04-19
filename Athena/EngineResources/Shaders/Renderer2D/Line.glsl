#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec4 a_Color;


layout(std140, binding = RENDERER2D_CAMERA_BUFFER_BINDER) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) out VertexOutput Output;


void main()
{
	Output.Color = a_Color;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) in VertexOutput Input;


void main()
{
	out_Color = Input.Color;
}
