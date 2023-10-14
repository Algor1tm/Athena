#type VERTEX_SHADER
#version 450

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec4 a_Color;

layout (location = 0) out vec4 Color;


void main() 
{
    gl_Position = vec4(a_Position, 0.0, 1.0);
    Color = a_Color;
}


#type FRAGMENT_SHADER
#version 450

layout(location = 0) in vec4 Color;

layout(location = 0) out vec4 out_Color;

void main() 
{
    out_Color = Color;
}
