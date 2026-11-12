//////////////////////// Athena SMAA-Blending Shader ////////////////////////

#version 460 core
#pragma stage : vertex

#define SMAA_INCLUDE_PS 0
#include "Include/SMAA.glslh"

layout(location = 0) in vec2 a_Position;

layout(location = 0) out vec2 v_TexCoords;
layout(location = 1) out vec4 v_Offset;

void main()
{
    v_TexCoords = vec2( (gl_VertexIndex << 1) & 2, (gl_VertexIndex) & 2 );
    gl_Position = vec4( v_TexCoords * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0), 0.0, 1.0 );

    SMAANeighborhoodBlendingVS(v_TexCoords, v_Offset);
}

#version 460 core
#pragma stage : fragment

#include "Include/SMAA.glslh"

layout(location = 0) in vec2 v_TexCoords;
layout(location = 1) in vec4 v_Offset;

layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 4) uniform sampler2D u_SceneColor;
layout(set = 1, binding = 5) uniform sampler2D u_BlendTex;

void main()
{
    o_Color = SMAANeighborhoodBlendingPS(v_TexCoords, v_Offset, u_SceneColor, u_BlendTex);
}
