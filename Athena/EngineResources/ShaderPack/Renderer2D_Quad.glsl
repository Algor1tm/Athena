//////////////////////// Athena Renderer2D Quad shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in int a_TexIndex;
layout (location = 4) in float a_TilingFactor;

struct VertexInterpolators
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) out VertexInterpolators Interpolators;
layout (location = 3) flat out int v_TexIndex;

layout(push_constant) uniform CameraData
{
	mat4 u_ViewProjection;
};

void main()
{
	Interpolators.Color = a_Color;
	Interpolators.TexCoord = a_TexCoord;
	Interpolators.TilingFactor = a_TilingFactor;
	v_TexIndex = a_TexIndex;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}

#version 460 core
#pragma stage : fragment

struct VertexInterpolators
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) in VertexInterpolators Interpolators;
layout (location = 3) flat in int v_TexIndex;

layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 2) uniform sampler2D u_Texture;

void main()
{
	vec4 color = Interpolators.Color;
	color *= texture(u_Texture, Interpolators.TexCoord * Interpolators.TilingFactor);

	o_Color = color;
}
