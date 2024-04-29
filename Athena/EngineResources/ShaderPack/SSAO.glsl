//////////////////////// Athena SSAO Shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout(location = 0) in vec2 a_Position;
layout(location = 0) out vec2 v_TexCoords;

void main()
{
    v_TexCoords = vec2( (gl_VertexIndex << 1) & 2, (gl_VertexIndex) & 2 );
    gl_Position = vec4( v_TexCoords * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0), 0.0, 1.0 );
}

#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

#define NUM_SAMPLES 64

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out float o_AOValue;

layout(set = 1, binding = 2) uniform sampler2D u_SceneDepth;
layout(set = 1, binding = 3) uniform sampler2D u_SceneNormals;

layout(std140, set = 1, binding = 5) uniform u_AOData
{
    vec4 u_SamplesKernel[NUM_SAMPLES];  
    vec4 u_KernelNoise[16];
    float u_Intensity;
    float u_Radius;
    float u_Bias;
    float _Pad0;
};

void main()
{
    float depth = texture(u_SceneDepth, v_TexCoords).r;
    if(depth == 0.0)
        discard;

    // Get view space position and normal
    vec3 viewPos = ViewPositionFromDepth(v_TexCoords, depth, u_Camera.InverseProjection);
    vec3 normal = texture(u_SceneNormals, v_TexCoords).rgb * 2.0 - 1.0;
    normal = vec3(u_Camera.View * vec4(normal, 0));

    ivec2 uvInt = ivec2(v_TexCoords * u_Renderer.ViewportSize);
    ivec2 coords = ivec2(uvInt.x % 4, uvInt.y % 4);
    int index = 4 * coords.y + coords.x;
    vec3 randomVec = vec3(u_KernelNoise[index].xy, 0.0);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);    // from tangent to view space matrix

    float occlusion = 0.0;
    for(int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec3 samplePos = TBN * u_SamplesKernel[i].xyz;
        samplePos = viewPos + samplePos * u_Radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = u_Camera.Projection * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(u_SceneDepth, offset.xy).r;
        sampleDepth = -LinearizeDepth(sampleDepth, u_Camera.FarClip, u_Camera.NearClip);

        if(sampleDepth >= samplePos.z + u_Bias)
        {
            float rangeCheck = smoothstep(0.0, 1.0, u_Radius / abs(viewPos.z - sampleDepth));
            occlusion += rangeCheck;
        }
    }

    occlusion = 1.0 - (occlusion / NUM_SAMPLES) * u_Intensity;

    o_AOValue = occlusion;
}
