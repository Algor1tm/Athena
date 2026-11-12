//////////////////////// Athena JumpFlood Mesh Fill Shader////////////////////////

#version 460 core
#pragma stage : vertex

#include "../Include/Buffers.glslh"

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

layout(location = 5) in vec3 a_TRow0;
layout(location = 6) in vec3 a_TRow1;
layout(location = 7) in vec3 a_TRow2;
layout(location = 8) in vec3 a_TRow3;


void main()
{
    mat4 transform = GetTransform(a_TRow0, a_TRow1, a_TRow2, a_TRow3);

    vec4 worldPos = transform * vec4(a_Position, 1.0);
    gl_Position = u_Camera.ViewProjection * worldPos;
}

#version 460 core
#pragma stage : fragment

layout(location = 0) out float o_Value;

void main()
{
    o_Value = 1.0;
}
