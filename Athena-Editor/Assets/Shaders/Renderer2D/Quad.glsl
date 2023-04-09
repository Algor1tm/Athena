#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in float a_TexIndex;
layout (location = 5) in float a_TilingFactor;


layout(std140, binding = RENDERER2D_CAMERA_BUFFER_BINDER) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) flat out float v_TexIndex;


void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	Output.TilingFactor = a_TilingFactor;
	v_TexIndex = a_TexIndex;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 430 core
			
layout(location = 0) out vec4 out_Color;

struct FragmentInput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) in FragmentInput Input;
layout (location = 3) flat in float v_TexIndex;


layout (binding = 0) uniform sampler2D u_Texture[32];

void main()
{
	vec4 TexColor = Input.Color;

	switch(int(v_TexIndex))
	{
		case 0:  TexColor *= texture(u_Texture[ 0], Input.TexCoord * Input.TilingFactor); break;
		case 1:  TexColor *= texture(u_Texture[ 1], Input.TexCoord * Input.TilingFactor); break;
		case 2:  TexColor *= texture(u_Texture[ 2], Input.TexCoord * Input.TilingFactor); break;
		case 3:  TexColor *= texture(u_Texture[ 3], Input.TexCoord * Input.TilingFactor); break;
		case 4:  TexColor *= texture(u_Texture[ 4], Input.TexCoord * Input.TilingFactor); break;
		case 5:  TexColor *= texture(u_Texture[ 5], Input.TexCoord * Input.TilingFactor); break;
		case 6:  TexColor *= texture(u_Texture[ 6], Input.TexCoord * Input.TilingFactor); break;
		case 7:  TexColor *= texture(u_Texture[ 7], Input.TexCoord * Input.TilingFactor); break;
		case 8:  TexColor *= texture(u_Texture[ 8], Input.TexCoord * Input.TilingFactor); break;
		case 9:  TexColor *= texture(u_Texture[ 9], Input.TexCoord * Input.TilingFactor); break;
		case 10: TexColor *= texture(u_Texture[10], Input.TexCoord * Input.TilingFactor); break;
		case 11: TexColor *= texture(u_Texture[11], Input.TexCoord * Input.TilingFactor); break;
		case 12: TexColor *= texture(u_Texture[12], Input.TexCoord * Input.TilingFactor); break;
		case 13: TexColor *= texture(u_Texture[13], Input.TexCoord * Input.TilingFactor); break;
		case 14: TexColor *= texture(u_Texture[14], Input.TexCoord * Input.TilingFactor); break;
		case 15: TexColor *= texture(u_Texture[15], Input.TexCoord * Input.TilingFactor); break;
		case 16: TexColor *= texture(u_Texture[16], Input.TexCoord * Input.TilingFactor); break;
		case 17: TexColor *= texture(u_Texture[17], Input.TexCoord * Input.TilingFactor); break;
		case 18: TexColor *= texture(u_Texture[18], Input.TexCoord * Input.TilingFactor); break;
		case 19: TexColor *= texture(u_Texture[19], Input.TexCoord * Input.TilingFactor); break;
		case 20: TexColor *= texture(u_Texture[20], Input.TexCoord * Input.TilingFactor); break;
		case 21: TexColor *= texture(u_Texture[21], Input.TexCoord * Input.TilingFactor); break;
		case 22: TexColor *= texture(u_Texture[22], Input.TexCoord * Input.TilingFactor); break;
		case 23: TexColor *= texture(u_Texture[23], Input.TexCoord * Input.TilingFactor); break;
		case 24: TexColor *= texture(u_Texture[24], Input.TexCoord * Input.TilingFactor); break;
		case 25: TexColor *= texture(u_Texture[25], Input.TexCoord * Input.TilingFactor); break;
		case 26: TexColor *= texture(u_Texture[26], Input.TexCoord * Input.TilingFactor); break;
		case 27: TexColor *= texture(u_Texture[27], Input.TexCoord * Input.TilingFactor); break;
		case 28: TexColor *= texture(u_Texture[28], Input.TexCoord * Input.TilingFactor); break;
		case 29: TexColor *= texture(u_Texture[29], Input.TexCoord * Input.TilingFactor); break;
		case 30: TexColor *= texture(u_Texture[30], Input.TexCoord * Input.TilingFactor); break;
		case 31: TexColor *= texture(u_Texture[31], Input.TexCoord * Input.TilingFactor); break;
	}

	out_Color = TexColor;
}
