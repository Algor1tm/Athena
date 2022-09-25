//#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_TexCoord;

layout(std140, binding = 1) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	vec4 invPosition = vec4(a_Position.xy, -a_Position.z, 1.f);

	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	gl_Position = u_ViewProjection * invPosition;
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout (binding = 0) uniform sampler2D u_Texture[32];

struct FragmentInput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) in FragmentInput Input;

void main()
{
	out_Color = Input.Color;
	//out_Color = texture(u_Texture[0], Input.TexCoord);
	//out_Color = vec4(Input.TexCoord, 0.f, 1.f);
}
