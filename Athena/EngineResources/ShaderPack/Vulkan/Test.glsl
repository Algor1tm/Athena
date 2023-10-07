#type VERTEX_SHADER
#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Color;

layout(location = 0) out vec3 fragColor;


void main() 
{
    gl_Position = vec4(a_Position, 1.0);
    fragColor = a_Color;
}


#type FRAGMENT_SHADER
#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(fragColor, 1.0);
}
