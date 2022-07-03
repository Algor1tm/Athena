#type VERTEX_SHADER
#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_TexCoord;

out vec4 v_Color;			
out vec2 v_TexCoord;

uniform mat4 u_ViewProjection;

void main()
{
	v_TexCoord = a_TexCoord;
	v_Color = a_Color;
	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 330 core
			
layout(location = 0) out vec4 out_Color;

in vec4 v_Color;
in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture;
uniform float u_tilingFactor;

void main()
{
	//out_Color = texture(u_Texture, v_TexCoord * u_tilingFactor) * u_Color;
	out_Color = v_Color;
}
