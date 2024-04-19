//////////////////////// Athena Panorama To Cubemap Shader ////////////////////////

// References:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance


#version 460 core
#pragma stage : compute

#include "Include/Common.glslh"

layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(set = 1, binding = 0) uniform sampler2D u_PanoramaTex;
layout(r11f_g11f_b10f, set = 1, binding = 1) uniform imageCube u_Cubemap;


vec2 SampleSphericalMap(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= vec2(1 / (2 * PI), 1 / PI);
    uv += 0.5;
    return uv;
}

void main()
{
    ivec3 unnormalizedTexCoords = ivec3(gl_GlobalInvocationID.xyz);
    vec3 direction = GetWorldDirectionFromCubeCoords(unnormalizedTexCoords, vec2(gl_NumWorkGroups * gl_WorkGroupSize));

    vec2 uv = SampleSphericalMap(direction);
    vec3 panoramaColor = texture(u_PanoramaTex, uv).rgb;

    // large values lead to artifacts
    panoramaColor = clamp(panoramaColor, 0.0, 150.0);

    imageStore(u_Cubemap, unnormalizedTexCoords, vec4(panoramaColor, 1));
}
