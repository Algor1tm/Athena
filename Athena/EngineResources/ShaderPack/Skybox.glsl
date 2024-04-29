//////////////////////// Athena Skybox shader ////////////////////////

#version 460 core
#pragma stage : vertex

#include "Include/Buffers.glslh"

layout(location = 0) in vec2 a_Position;
layout(location = 0) out vec3 v_TexCoords;

void main()
{
    vec2 uv = vec2( (gl_VertexIndex << 1) & 2, (gl_VertexIndex) & 2 );
    vec4 ndcPos = vec4( uv * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0), 0.0, 1.0 );

    gl_Position = ndcPos;
    v_TexCoords = vec3(u_Camera.InverseViewProjection * ndcPos);
}


#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"

layout(location = 0) in vec3 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 2) uniform samplerCube u_EnvironmentMap;

void main()
{
    vec3 color = textureLod(u_EnvironmentMap, v_TexCoords, u_Renderer.EnvironmentLOD).rgb * u_Renderer.EnvironmentIntensity;
    o_Color = vec4(color, 1);
}
