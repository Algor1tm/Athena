//////////////////////// Athena Hi-Z Shader ////////////////////////

// References:
//    https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/XeGTAO.hlsli#L617

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D u_SourceDepth;
layout(r32f, set = 0, binding = 1) uniform writeonly image2D u_OutputDepth;


layout(push_constant) uniform u_Uniforms
{
    vec2 u_SourceSize;
	vec2 u_OutputSize;
    vec2 u_SourceTexelSize;
    vec2 u_OutputTexelSize;
};


void main()
{
	ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec2 uv = (pixelCoords + 0.5) * u_OutputTexelSize;

    // First pass only copy
    if(u_SourceSize == u_OutputSize)
    {
        float sourceDepth = texture(u_SourceDepth, uv).r;

#ifdef LINEAR_DEPTH
        float depth = LinearizeDepth(sourceDepth, u_Camera.FarClip, u_Camera.NearClip);
#else
        float depth = 1 - sourceDepth; // original depth buffer has reversed Z
#endif

        imageStore(u_OutputDepth, pixelCoords, vec4(depth, 0, 0, 1));
        return;
    }

	vec2 ratio = u_SourceSize / u_OutputSize;

    bool needExtraSampleX = ratio.x > 2;
    bool needExtraSampleY = ratio.y > 2;

    ivec2 readCoords = pixelCoords << 1; // ivec2(floor(vec2(pixelCoords) * ratio))

    float r1 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(0, 0)).r;
    float r2 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(1, 0)).r;
    float r3 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(1, 1)).r;
    float r4 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(0, 1)).r;

    float minDepth = min(r1, min(r2, min(r3, r4))); 

    if(needExtraSampleX)
    {
        r1 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(2, 0)).r;
        r2 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(2, 1)).r;
        minDepth = min(minDepth, min(r1, r2));
    }

    if(needExtraSampleY)
    {
        r1 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(0, 2)).r;
        r2 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(1, 2)).r;
        minDepth = min(minDepth, min(r1, r2));
    }

    if(needExtraSampleX && needExtraSampleY)
    {
        r1 = texelFetchOffset(u_SourceDepth, readCoords, 0, ivec2(2, 2)).r;
        minDepth = min(minDepth, r1);
    }

    imageStore(u_OutputDepth, pixelCoords, vec4(minDepth, 0, 0, 1));
}
