//////////////////////// Athena HBAO Shader ////////////////////////

// Refernces:
//  https://github.com/nvpro-samples/gl_ssao
//  https://developer.download.nvidia.com/presentations/2008/SIGGRAPH/HBAO_SIG08b.pdf

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"
#include "Include/HBAO.glslh"

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(set = 1, binding = 2) uniform sampler2DArray u_DepthLayers;
layout(set = 1, binding = 3) uniform sampler2D u_SceneNormals;

layout(rg16f, set = 1, binding = 4) uniform image2D u_Output;

uint s_DepthLayer = gl_GlobalInvocationID.z;


vec3 GetViewPos(vec2 uv)
{
    vec2 clipSpace = 2.0 * uv - 1.0;
    vec2 viewSpace = clipSpace / vec2(u_Camera.Projection[0][0], u_Camera.Projection[1][1]);
    float viewDepth = texture(u_DepthLayers, vec3(uv, s_DepthLayer)).r;

    return vec3(viewSpace, viewDepth);
}

float ComputeCoarseAO(vec2 fullResUV, vec3 jitter, vec3 pos, vec3 normal)
{
    return 0.0;
}

void main()
{
    vec2 pixelCoords = gl_GlobalInvocationID.xy * 4.0 + u_HBAO.Float2Offsets[s_DepthLayer].xy;
    vec2 uv = pixelCoords * (u_HBAO.InvQuarterResolution / 4.0);

	// Get view space position and normal
    vec3 pos = GetViewPos(uv);
    vec3 normal = texture(u_SceneNormals, uv).rgb * 2.0 - 1.0;
    normal = vec3(u_Camera.View * vec4(normal, 0));

    vec3 jitter = u_HBAO.Jitters[s_DepthLayer].xyz;

    float ao = ComputeCoarseAO(uv, jitter, pos, normal);
    ao = pow(ao, u_HBAO.Intensity);

    imageStore(u_Output, ivec2(pixelCoords), vec4(ao, pos.z, 0, 1));
}
