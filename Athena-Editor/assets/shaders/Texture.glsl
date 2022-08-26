#type VERTEX_SHADER
#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in float a_TexIndex;
layout (location = 4) in float a_TilingFactor;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;

uniform mat4 u_ViewProjection;

void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_TilingFactor = a_TilingFactor;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 330 core
			
layout(location = 0) out vec4 out_Color;
layout(location = 1) out vec4 out_Color2;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;

uniform sampler2D u_Texture[32];

void main()
{
	// Does not work on AMD cards
	//out_Color = texture(u_Texture[int(v_TexIndex)], v_TexCoord * v_TilingFactor) * v_Color;

	vec4 TexColor = v_Color;
	switch(int(v_TexIndex))
	{
		case 0: TexColor *= texture(u_Texture[0], v_TexCoord * v_TilingFactor); break;
		case 1: TexColor *= texture(u_Texture[1], v_TexCoord * v_TilingFactor); break;
		case 2: TexColor *= texture(u_Texture[2], v_TexCoord * v_TilingFactor); break;
		case 3: TexColor *= texture(u_Texture[3], v_TexCoord * v_TilingFactor); break;
		case 4: TexColor *= texture(u_Texture[4], v_TexCoord * v_TilingFactor); break;
		case 5: TexColor *= texture(u_Texture[5], v_TexCoord * v_TilingFactor); break;
		case 6: TexColor *= texture(u_Texture[6], v_TexCoord * v_TilingFactor); break;
		case 7: TexColor *= texture(u_Texture[7], v_TexCoord * v_TilingFactor); break;
		case 8: TexColor *= texture(u_Texture[8], v_TexCoord * v_TilingFactor); break;
		case 9: TexColor *= texture(u_Texture[9], v_TexCoord * v_TilingFactor); break;
		case 10: TexColor *= texture(u_Texture[10], v_TexCoord * v_TilingFactor); break;
		case 11: TexColor *= texture(u_Texture[11], v_TexCoord * v_TilingFactor); break;
		case 12: TexColor *= texture(u_Texture[12], v_TexCoord * v_TilingFactor); break;
		case 13: TexColor *= texture(u_Texture[13], v_TexCoord * v_TilingFactor); break;
		case 14: TexColor *= texture(u_Texture[14], v_TexCoord * v_TilingFactor); break;
		case 15: TexColor *= texture(u_Texture[15], v_TexCoord * v_TilingFactor); break;
		case 16: TexColor *= texture(u_Texture[16], v_TexCoord * v_TilingFactor); break;
		case 17: TexColor *= texture(u_Texture[17], v_TexCoord * v_TilingFactor); break;
		case 18: TexColor *= texture(u_Texture[18], v_TexCoord * v_TilingFactor); break;
		case 19: TexColor *= texture(u_Texture[19], v_TexCoord * v_TilingFactor); break;
		case 20: TexColor *= texture(u_Texture[20], v_TexCoord * v_TilingFactor); break;
		case 21: TexColor *= texture(u_Texture[21], v_TexCoord * v_TilingFactor); break;
		case 22: TexColor *= texture(u_Texture[22], v_TexCoord * v_TilingFactor); break;
		case 23: TexColor *= texture(u_Texture[23], v_TexCoord * v_TilingFactor); break;
		case 24: TexColor *= texture(u_Texture[24], v_TexCoord * v_TilingFactor); break;
		case 25: TexColor *= texture(u_Texture[25], v_TexCoord * v_TilingFactor); break;
		case 26: TexColor *= texture(u_Texture[26], v_TexCoord * v_TilingFactor); break;
		case 27: TexColor *= texture(u_Texture[27], v_TexCoord * v_TilingFactor); break;
		case 28: TexColor *= texture(u_Texture[28], v_TexCoord * v_TilingFactor); break;
		case 29: TexColor *= texture(u_Texture[29], v_TexCoord * v_TilingFactor); break;
		case 30: TexColor *= texture(u_Texture[30], v_TexCoord * v_TilingFactor); break;
		case 31: TexColor *= texture(u_Texture[31], v_TexCoord * v_TilingFactor); break;
	}
	out_Color = TexColor;

	out_Color2 = vec4(1, 0, 0, 1);
}
