//////////////////////// Athena SSR Shader ////////////////////////

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

layout(local_size_x = 8, local_size_y = 4) in;

layout(rg8, set = 1, binding = 2) uniform image2D u_Output;

layout(set = 1, binding = 3) uniform sampler2D u_HiZBuffer;
layout(set = 1, binding = 4) uniform sampler2D u_SceneDepth;
layout(set = 1, binding = 5) uniform sampler2D u_SceneNormals;
layout(set = 1, binding = 6) uniform sampler2D u_SceneRoughnessMetalness;


void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);

	// Compute

	imageStore(u_Output, pixelCoords, vec4(1.0));
}
