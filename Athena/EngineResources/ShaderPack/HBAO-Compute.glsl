//////////////////////// Athena HBAO Shader ////////////////////////

// Refernces:
//  https://github.com/nvpro-samples/gl_ssao
//  https://developer.download.nvidia.com/presentations/2008/SIGGRAPH/HBAO_SIG08b.pdf

#version 460 core
#pragma stage : compute

#include "Include/Common.glslh"
#include "Include/HBAO.glslh"

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(set = 1, binding = 2) uniform sampler2DArray u_DepthLayers;
layout(set = 1, binding = 3) uniform sampler2D u_SceneNormals;

layout(rg16f, set = 1, binding = 4) uniform image2D u_Output;

#define NUM_STEPS 4
#define NUM_DIRECTIONS  8

uint s_DepthLayer = gl_GlobalInvocationID.z;


vec2 RotateDirection(vec2 dir, vec2 cosSin)
{
  return vec2(dir.x * cosSin.x - dir.y * cosSin.y,
              dir.x * cosSin.y + dir.y * cosSin.x);
}

vec3 GetViewPos(vec2 uv)
{
    float viewDepth = texture(u_DepthLayers, vec3(uv, s_DepthLayer)).r;
    vec2 viewUV = uv * u_HBAO.ProjInfo.xy + u_HBAO.ProjInfo.zw;
    viewUV *= viewDepth;

    return vec3(viewUV, -viewDepth);
}

float FallOff(float distanceSquare)
{
  return distanceSquare * u_HBAO.NegInvR2 + 1.0;
}

// project horizon vector onto normal
float ComputeAO(vec3 pos, vec3 normal, vec3 samplePos)
{
    vec3 horizonVec = samplePos - pos;
    float distanceSquared = dot(horizonVec, horizonVec);
    float NdotH = dot(normal, horizonVec) / sqrt(distanceSquared);

    return clamp(NdotH - u_HBAO.Bias, 0, 1) * clamp(FallOff(distanceSquared), 0, 1);
}

float ComputeCoarseAO(vec2 fullResUV, float radiusPixels, vec3 jitter, vec3 pos, vec3 normal)
{
    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float stepSizePixels = radiusPixels / (NUM_STEPS + 1);

    const float alpha = 2.0 * PI / NUM_DIRECTIONS;
    float ao = 0;

    for (float directionIndex = 0; directionIndex < NUM_DIRECTIONS; ++directionIndex)
    {
        float angle = alpha * directionIndex;

        // Compute normalized 2D direction
        vec2 direction = RotateDirection(vec2(cos(angle), sin(angle)), jitter.xy);

        // Jitter starting sample within the first step
        float rayPixels = (jitter.z * stepSizePixels + 1.0);

        for (float stepIndex = 0; stepIndex < NUM_STEPS; ++stepIndex)
        {
            vec2 sampleUV = round(rayPixels * direction) * u_HBAO.InvQuarterResolution + fullResUV;
            vec3 samplePos = GetViewPos(sampleUV);

            rayPixels += stepSizePixels;

            ao += ComputeAO(pos, normal, samplePos);
        }
    }
    
    ao *= u_HBAO.AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
    return clamp(1.0 - ao * 2.0, 0, 1);
}

void main()
{
    vec2 pixelCoords = gl_GlobalInvocationID.xy * 4.0 + u_HBAO.Float2Offsets[s_DepthLayer].xy;
    vec2 uv = pixelCoords * (u_HBAO.InvQuarterResolution / 4.0);

	// Get view space position and normal
    vec3 pos = GetViewPos(uv);

    vec3 normal = texelFetch(u_SceneNormals, ivec2(pixelCoords), 0).xyz;
    if(normal != vec3(0.0))
        normal = normal * 2.0 - 1.0;

    vec3 jitter = u_HBAO.Jitters[s_DepthLayer].xyz;
    float radiusPixels = u_HBAO.RadiusToScreen / abs(pos.z);

    float ao = ComputeCoarseAO(uv, radiusPixels, jitter, pos, normal);
    ao = pow(ao, u_HBAO.Intensity);

    imageStore(u_Output, ivec2(pixelCoords), vec4(ao, pos.z, 0, 1));
}
