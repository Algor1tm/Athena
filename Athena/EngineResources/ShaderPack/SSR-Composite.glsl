//////////////////////// Athena SSR-Composite Shader ////////////////////////

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

layout(local_size_x = 8, local_size_y = 4) in;

layout(rgba16f, set = 1, binding = 2) uniform image2D u_SceneColorOutput;
layout(set = 1, binding = 3) uniform sampler2D u_SSRReflectedUV;


void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec3 color = imageLoad(u_SceneColorOutput, pixelCoords).rgb;

	// Composite

	imageStore(u_SceneColorOutput, pixelCoords, vec4(color, 1.0));
}
