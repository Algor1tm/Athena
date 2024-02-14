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

layout(set = 0, binding = 0) uniform sampler2D u_Textures[32];


void main()
{
	vec4 color = Interpolators.Color;
	vec2 texCoord = Interpolators.TexCoord * Interpolators.TilingFactor;

	switch(int(v_TexIndex))
	{
		case 0:  color *= texture(u_Textures[ 0], texCoord); break;
		case 1:  color *= texture(u_Textures[ 1], texCoord); break;
		case 2:  color *= texture(u_Textures[ 2], texCoord); break;
		case 3:  color *= texture(u_Textures[ 3], texCoord); break;
		case 4:  color *= texture(u_Textures[ 4], texCoord); break;
		case 5:  color *= texture(u_Textures[ 5], texCoord); break;
		case 6:  color *= texture(u_Textures[ 6], texCoord); break;
		case 7:  color *= texture(u_Textures[ 7], texCoord); break;
		case 8:  color *= texture(u_Textures[ 8], texCoord); break;
		case 9:  color *= texture(u_Textures[ 9], texCoord); break;
		case 10: color *= texture(u_Textures[10], texCoord); break;
		case 11: color *= texture(u_Textures[11], texCoord); break;
		case 12: color *= texture(u_Textures[12], texCoord); break;
		case 13: color *= texture(u_Textures[13], texCoord); break;
		case 14: color *= texture(u_Textures[14], texCoord); break;
		case 15: color *= texture(u_Textures[15], texCoord); break;
		case 16: color *= texture(u_Textures[16], texCoord); break;
		case 17: color *= texture(u_Textures[17], texCoord); break;
		case 18: color *= texture(u_Textures[18], texCoord); break;
		case 19: color *= texture(u_Textures[19], texCoord); break;
		case 20: color *= texture(u_Textures[20], texCoord); break;
		case 21: color *= texture(u_Textures[21], texCoord); break;
		case 22: color *= texture(u_Textures[22], texCoord); break;
		case 23: color *= texture(u_Textures[23], texCoord); break;
		case 24: color *= texture(u_Textures[24], texCoord); break;
		case 25: color *= texture(u_Textures[25], texCoord); break;
		case 26: color *= texture(u_Textures[26], texCoord); break;
		case 27: color *= texture(u_Textures[27], texCoord); break;
		case 28: color *= texture(u_Textures[28], texCoord); break;
		case 29: color *= texture(u_Textures[29], texCoord); break;
		case 30: color *= texture(u_Textures[30], texCoord); break;
		case 31: color *= texture(u_Textures[31], texCoord); break;
	}

	o_Color = color;
}
