//////////////////////// Athena Hi-Z Shader ////////////////////////

// References:
//    https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/XeGTAO.hlsli#L617

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 1, binding = 2) uniform sampler2D u_SourceNDCDepth;

layout(r32f, set = 1, binding = 3) uniform writeonly image2D u_Output0;
layout(r32f, set = 1, binding = 4) uniform writeonly image2D u_Output1;
layout(r32f, set = 1, binding = 5) uniform writeonly image2D u_Output2;
layout(r32f, set = 1, binding = 6) uniform writeonly image2D u_Output3;
layout(r32f, set = 1, binding = 7) uniform writeonly image2D u_Output4;

shared float s_ScratchDepths[8][8];

#define RADIUS         0.5 * 1.457
#define FALLOFF_RANGE  0.615

float DepthMipFilter(float depth0, float depth1, float depth2, float depth3)
{
    float maxDepth = max( max( depth0, depth1 ), max( depth2, depth3 ) );

    const float depthRangeScaleFactor = 0.75;
    const float effectRadius = depthRangeScaleFactor * RADIUS;
    const float falloffRange = FALLOFF_RANGE * effectRadius;

    const float falloffFrom = effectRadius * (1.0 - FALLOFF_RANGE);

    const float falloffMul = -1.0 / ( falloffRange );
    const float falloffAdd = falloffFrom / ( falloffRange ) + 1.0;

    float weight0 = clamp( (maxDepth - depth0) * falloffMul + falloffAdd, 0, 1 );
    float weight1 = clamp( (maxDepth - depth1) * falloffMul + falloffAdd, 0, 1 );
    float weight2 = clamp( (maxDepth - depth2) * falloffMul + falloffAdd, 0, 1 );
    float weight3 = clamp( (maxDepth - depth3) * falloffMul + falloffAdd, 0, 1 );

    float weightSum = weight0 + weight1 + weight2 + weight3;
    return (weight0 * depth0 + weight1 * depth1 + weight2 * depth2 + weight3 * depth3) / weightSum;
}

void main()
{
    const ivec2 baseCoord = ivec2(gl_GlobalInvocationID);
    const ivec2 pixelCoord = baseCoord * 2;

    vec4 depths4 = textureGatherOffset(u_SourceNDCDepth, pixelCoord * u_Renderer.InverseViewportSize, ivec2(1, 1), 0);

    float depth0 = LinearizeDepth(depths4.w, u_Camera.FarClip, u_Camera.NearClip);
    float depth1 = LinearizeDepth(depths4.z, u_Camera.FarClip, u_Camera.NearClip);
    float depth2 = LinearizeDepth(depths4.x, u_Camera.FarClip, u_Camera.NearClip);
    float depth3 = LinearizeDepth(depths4.y, u_Camera.FarClip, u_Camera.NearClip);

    // Mip 0
    imageStore(u_Output0, pixelCoord + ivec2(0, 0), vec4(depth0, 0, 0, 1));
    imageStore(u_Output0, pixelCoord + ivec2(1, 0), vec4(depth1, 0, 0, 1));
    imageStore(u_Output0, pixelCoord + ivec2(0, 1), vec4(depth2, 0, 0, 1));
    imageStore(u_Output0, pixelCoord + ivec2(1, 1), vec4(depth3, 0, 0, 1));

    // Mip 1
    float dm1 = DepthMipFilter( depth0, depth1, depth2, depth3 );
    imageStore(u_Output1, baseCoord, vec4(dm1, 0, 0, 1));
    s_ScratchDepths[ gl_LocalInvocationID.x][ gl_LocalInvocationID.y] = dm1;

    groupMemoryBarrier();
    barrier();

     // MIP 2
    if(gl_LocalInvocationID.xy % ivec2(2) == ivec2(0))
    {
        float inTL = s_ScratchDepths[gl_LocalInvocationID.x + 0][gl_LocalInvocationID.y + 0];
        float inTR = s_ScratchDepths[gl_LocalInvocationID.x + 1][gl_LocalInvocationID.y + 0];
        float inBL = s_ScratchDepths[gl_LocalInvocationID.x + 0][gl_LocalInvocationID.y + 1];
        float inBR = s_ScratchDepths[gl_LocalInvocationID.x + 1][gl_LocalInvocationID.y + 1];

        float dm2 = DepthMipFilter(inTL, inTR, inBL, inBR);
        imageStore(u_Output2, baseCoord / 2, vec4(dm2, 0, 0, 1));
        s_ScratchDepths[ gl_LocalInvocationID.x ][ gl_LocalInvocationID.y ] = dm2;
    }

    groupMemoryBarrier();
    barrier();

    // MIP 3
    if(gl_LocalInvocationID.xy % ivec2(4) == ivec2(0))
    {
        float inTL = s_ScratchDepths[gl_LocalInvocationID.x + 0][gl_LocalInvocationID.y + 0];
        float inTR = s_ScratchDepths[gl_LocalInvocationID.x + 2][gl_LocalInvocationID.y + 0];
        float inBL = s_ScratchDepths[gl_LocalInvocationID.x + 0][gl_LocalInvocationID.y + 2];
        float inBR = s_ScratchDepths[gl_LocalInvocationID.x + 2][gl_LocalInvocationID.y + 2];

        float dm3 = DepthMipFilter(inTL, inTR, inBL, inBR);
        imageStore(u_Output3, baseCoord / 4, vec4(dm3, 0, 0, 1));
        s_ScratchDepths[ gl_LocalInvocationID.x ][ gl_LocalInvocationID.y ] = dm3;
    }

    groupMemoryBarrier();
    barrier();

    // MIP 4
    if(gl_LocalInvocationID.xy % ivec2(8) == ivec2(0))
    {
        float inTL = s_ScratchDepths[gl_LocalInvocationID.x + 0][gl_LocalInvocationID.y + 0];
        float inTR = s_ScratchDepths[gl_LocalInvocationID.x + 4][gl_LocalInvocationID.y + 0];
        float inBL = s_ScratchDepths[gl_LocalInvocationID.x + 0][gl_LocalInvocationID.y + 4];
        float inBR = s_ScratchDepths[gl_LocalInvocationID.x + 4][gl_LocalInvocationID.y + 4];

        float dm4 = DepthMipFilter(inTL, inTR, inBL, inBR);
        imageStore(u_Output4, baseCoord / 8, vec4(dm4, 0, 0, 1));
    }
}
