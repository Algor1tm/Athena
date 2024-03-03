//////////////////////// Athena Renderer2D Circle shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout (location = 0) in vec3 a_WorldPosition;
layout (location = 1) in vec3 a_LocalPosition;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in float a_Thickness;
layout (location = 4) in float a_Fade;

struct VertexInterpolators
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
	float Fade;
};

layout (location = 0) out VertexInterpolators Interpolators;

layout(push_constant) uniform u_CameraData
{
	mat4 u_ViewProjection;
};

void main()
{
	Interpolators.LocalPosition = a_LocalPosition;
	Interpolators.Color = a_Color;
	Interpolators.Thickness = a_Thickness;
	Interpolators.Fade = a_Fade;

	gl_Position = u_ViewProjection * vec4(a_WorldPosition, 1);
}

#version 460 core
#pragma stage : fragment

struct VertexInterpolators
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
	float Fade;
};

layout (location = 0) in VertexInterpolators Interpolators;
layout(location = 0) out vec4 o_Color;

void main()
{
	float distance = 1.0 - length(Interpolators.LocalPosition.xy);
	float circle = smoothstep(0.0, Interpolators.Fade, distance);
	circle *= smoothstep(Interpolators.Thickness + Interpolators.Fade, Interpolators.Thickness, distance);

	o_Color = Interpolators.Color;
	o_Color.a *= circle;
}