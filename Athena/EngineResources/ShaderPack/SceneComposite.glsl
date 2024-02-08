//////////////////////// Athena Composite Shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoords;

layout(location = 0) out vec2 v_TexCoords;

void main()
{
    gl_Position = vec4(a_Position, 0, 1);
    v_TexCoords = a_TexCoords;
}

#version 460 core
#pragma stage : fragment

#include "Buffers.glslh"

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 0) uniform sampler2D u_SceneHDRColor;


void main()
{
    vec3 hdrColor = texture(u_SceneHDRColor, v_TexCoords).rgb;
    
    vec3 color = vec3(1.0) - exp(-hdrColor * u_Scene.Exposure);
    color = pow(color, vec3(1.0 / u_Scene.Gamma));
    o_Color = vec4(color, 1.0);
}
