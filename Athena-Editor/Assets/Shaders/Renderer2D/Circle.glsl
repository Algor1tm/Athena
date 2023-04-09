#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_WorldPosition;
layout (location = 2) in vec3 a_LocalPosition;
layout (location = 3) in vec4 a_Color;
layout (location = 4) in float a_Thickness;
layout (location = 5) in float a_Fade;


layout(std140, binding = RENDERER2D_CAMERA_BUFFER_BINDER) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
	float Fade;
};

layout (location = 0) out VertexOutput Output;


void main()
{
	Output.LocalPosition = a_LocalPosition;
	Output.Color = a_Color;
	Output.Thickness = a_Thickness;
	Output.Fade = a_Fade;

	gl_Position = u_ViewProjection * vec4(a_WorldPosition, 1);
}


#type FRAGMENT_SHADER
#version 430 core
			
layout(location = 0) out vec4 out_Color;

struct VertexOutput
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
	float Fade;
};

layout (location = 0) in VertexOutput Input;


void main()
{
	float distance = 1.0 - length(Input.LocalPosition.xy);
	float circle = smoothstep(0.0, Input.Fade, distance);
	circle *= smoothstep(Input.Thickness + Input.Fade, Input.Thickness, distance);

	out_Color = Input.Color;
	out_Color.a *= circle;
}
