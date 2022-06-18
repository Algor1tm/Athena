#type VERTEX_SHADER
#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_TexCoord;
			
out vec2 v_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
			
void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 330 core
			
layout(location = 0) out vec4 out_Color;
in vec2 v_TexCoord;
uniform sampler2D u_Texture;

void main()
{
	out_Color = texture(u_Texture, v_TexCoord);
}
