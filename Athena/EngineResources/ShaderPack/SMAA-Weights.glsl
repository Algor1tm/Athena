//////////////////////// Athena SMAA-Weights Shader ////////////////////////

#version 460 core
#pragma stage : vertex


#define SMAA_INCLUDE_PS 0
#include "Include/SMAA.glslh"

layout(location = 0) in vec2 a_Position;

layout(location = 0) out vec2 v_TexCoords;
layout(location = 1) out vec2 v_PixCoords;
layout(location = 2) out vec4 v_Offsets[3];

void main()
{
    v_TexCoords = vec2( (gl_VertexIndex << 1) & 2, (gl_VertexIndex) & 2 );
    gl_Position = vec4( v_TexCoords * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0), 0.0, 1.0 );

    SMAABlendingWeightCalculationVS(v_TexCoords, v_PixCoords, v_Offsets);
}

#version 460 core
#pragma stage : fragment

#include "Include/SMAA.glslh"

layout(location = 0) in vec2 v_TexCoords;
layout(location = 1) in vec2 v_PixCoords;
layout(location = 2) in vec4 v_Offsets[3];

layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 4) uniform sampler2D u_Edges;
layout(set = 1, binding = 5) uniform sampler2D u_AreaTex;
layout(set = 1, binding = 6) uniform sampler2D u_SearchTex;

void main()
{
    o_Color = SMAABlendingWeightCalculationPS(v_TexCoords, v_PixCoords, v_Offsets, u_Edges, u_AreaTex, u_SearchTex, vec4(0));
}
