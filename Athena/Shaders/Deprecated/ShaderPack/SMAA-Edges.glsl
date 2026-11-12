//////////////////////// Athena SMAA-Edges Shader ////////////////////////

#version 460 core
#pragma stage : vertex

#define SMAA_INCLUDE_PS 0
#include "Include/SMAA.glslh"

layout(location = 0) in vec2 a_Position;

layout(location = 0) out vec2 v_TexCoords;
layout(location = 1) out vec4 v_Offsets[3];

void main()
{
    v_TexCoords = vec2( (gl_VertexIndex << 1) & 2, (gl_VertexIndex) & 2 );
    gl_Position = vec4( v_TexCoords * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0), 0.0, 1.0 );

    SMAAEdgeDetectionVS(v_TexCoords, v_Offsets);
}

#version 460 core
#pragma stage : fragment

#include "Include/SMAA.glslh"

layout(location = 0) in vec2 v_TexCoords;
layout(location = 1) in vec4 v_Offsets[3];

layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 4) uniform sampler2D u_Texture;

void main()
{
    vec2 edges = SMAALumaEdgeDetectionPS(v_TexCoords, v_Offsets, u_Texture);
    o_Color = vec4(edges, 0, 1);
}
