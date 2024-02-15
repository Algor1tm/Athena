//////////////////////// Athena Renderer2D Quad shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in int a_TexIndex;

struct VertexInterpolators
{
	vec4 Color;
	vec2 TexCoords;
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
	Interpolators.TexCoords = a_TexCoords;
	v_TexIndex = a_TexIndex;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}

#version 460 core
#pragma stage : fragment

struct VertexInterpolators
{
	vec4 Color;
	vec2 TexCoords;
};

layout (location = 0) in VertexInterpolators Interpolators;
layout (location = 3) flat in int v_TexIndex;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform sampler2D u_Textures[32];


void main()
{
	vec4 color = Interpolators.Color;

	switch(int(v_TexIndex))
	{
		case 0:  color *= texture(u_Textures[ 0], Interpolators.TexCoords); break;
		case 1:  color *= texture(u_Textures[ 1], Interpolators.TexCoords); break;
		case 2:  color *= texture(u_Textures[ 2], Interpolators.TexCoords); break;
		case 3:  color *= texture(u_Textures[ 3], Interpolators.TexCoords); break;
		case 4:  color *= texture(u_Textures[ 4], Interpolators.TexCoords); break;
		case 5:  color *= texture(u_Textures[ 5], Interpolators.TexCoords); break;
		case 6:  color *= texture(u_Textures[ 6], Interpolators.TexCoords); break;
		case 7:  color *= texture(u_Textures[ 7], Interpolators.TexCoords); break;
		case 8:  color *= texture(u_Textures[ 8], Interpolators.TexCoords); break;
		case 9:  color *= texture(u_Textures[ 9], Interpolators.TexCoords); break;
		case 10: color *= texture(u_Textures[10], Interpolators.TexCoords); break;
		case 11: color *= texture(u_Textures[11], Interpolators.TexCoords); break;
		case 12: color *= texture(u_Textures[12], Interpolators.TexCoords); break;
		case 13: color *= texture(u_Textures[13], Interpolators.TexCoords); break;
		case 14: color *= texture(u_Textures[14], Interpolators.TexCoords); break;
		case 15: color *= texture(u_Textures[15], Interpolators.TexCoords); break;
		case 16: color *= texture(u_Textures[16], Interpolators.TexCoords); break;
		case 17: color *= texture(u_Textures[17], Interpolators.TexCoords); break;
		case 18: color *= texture(u_Textures[18], Interpolators.TexCoords); break;
		case 19: color *= texture(u_Textures[19], Interpolators.TexCoords); break;
		case 20: color *= texture(u_Textures[20], Interpolators.TexCoords); break;
		case 21: color *= texture(u_Textures[21], Interpolators.TexCoords); break;
		case 22: color *= texture(u_Textures[22], Interpolators.TexCoords); break;
		case 23: color *= texture(u_Textures[23], Interpolators.TexCoords); break;
		case 24: color *= texture(u_Textures[24], Interpolators.TexCoords); break;
		case 25: color *= texture(u_Textures[25], Interpolators.TexCoords); break;
		case 26: color *= texture(u_Textures[26], Interpolators.TexCoords); break;
		case 27: color *= texture(u_Textures[27], Interpolators.TexCoords); break;
		case 28: color *= texture(u_Textures[28], Interpolators.TexCoords); break;
		case 29: color *= texture(u_Textures[29], Interpolators.TexCoords); break;
		case 30: color *= texture(u_Textures[30], Interpolators.TexCoords); break;
		case 31: color *= texture(u_Textures[31], Interpolators.TexCoords); break;
	}

	o_Color = color;
}
