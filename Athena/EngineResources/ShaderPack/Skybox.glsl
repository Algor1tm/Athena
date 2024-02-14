//////////////////////// Athena Skybox shader ////////////////////////

#version 460 core
#pragma stage : vertex

#include "Include/Buffers.glslh"

layout(location = 0) in vec3 a_Position;
layout(location = 0) out vec3 v_TexCoords;

void main()
{
    v_TexCoords = a_Position;

    vec4 pos = u_Camera.Projection * u_Camera.RotationView * vec4(a_Position, 1.0);
    gl_Position = pos.xyww;
}


#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"

layout(location = 0) in vec3 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 2) uniform samplerCube u_EnvironmentMap;

void main()
{
    vec3 color = textureLod(u_EnvironmentMap, v_TexCoords, u_Scene.EnvironmentLOD).rgb * u_Scene.EnvironmentIntensity;
    o_Color = vec4(color, 1);
}
