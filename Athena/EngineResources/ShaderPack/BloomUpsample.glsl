//////////////////////// Athena Bloom Downsample Shader ////////////////////////

// References:
//   https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/


#version 460 core
#pragma stage : compute

#define GROUP_SIZE         8
#define GROUP_THREAD_COUNT (GROUP_SIZE * GROUP_SIZE)
#define FILTER_SIZE        3
#define FILTER_RADIUS      (FILTER_SIZE / 2)
#define TILE_SIZE          (GROUP_SIZE + 2 * FILTER_RADIUS)
#define TILE_PIXEL_COUNT   (TILE_SIZE * TILE_SIZE)

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE) in;

layout(set = 1, binding = 0) uniform sampler2D u_BloomTexture;
layout(set = 1, binding = 1) uniform sampler2D u_DirtTexture;

layout(r11f_g11f_b10f, set = 0, binding = 1) uniform image2D u_BloomTextureMip;


layout(push_constant) uniform BloomData
{
	float u_Intensity;
    float u_Threshold;
    float u_Knee;
    float u_DirtIntensity;
    vec2 u_TexelSize;
    uint u_ReadMipLevel;
};

shared vec3 s_Pixels[TILE_PIXEL_COUNT];

void StorePixel(int idx, vec4 color)
{
    s_Pixels[idx] = color.rgb;
}

vec4 LoadPixel(uint idx)
{
    return vec4(s_Pixels[idx], 1.0);
}


void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec2 baseIndex = ivec2(gl_WorkGroupID) * GROUP_SIZE - FILTER_RADIUS;
    bool lastPass = u_ReadMipLevel == 1;

    // The first (TILE_PIXEL_COUNT - GROUP_THREAD_COUNT) threads load at most 2 texel values
    for (int i = int(gl_LocalInvocationIndex); i < TILE_PIXEL_COUNT; i += GROUP_THREAD_COUNT)
    {
        vec2 uv        = (baseIndex + 0.5) * u_TexelSize;
        vec2 uvOffset = vec2(i % TILE_SIZE, i / TILE_SIZE) * u_TexelSize;
        
        vec4 color = textureLod(u_BloomTexture, (uv + uvOffset), u_ReadMipLevel);
        StorePixel(i, color);
    }

    memoryBarrierShared();
    barrier();

    uint center = (gl_LocalInvocationID.x + FILTER_RADIUS) + (gl_LocalInvocationID.y + FILTER_RADIUS) * TILE_SIZE;

    vec4 s;
    s  = LoadPixel(center - TILE_SIZE - 1);
    s += LoadPixel(center - TILE_SIZE    ) * 2.0;
    s += LoadPixel(center - TILE_SIZE + 1);

    s += LoadPixel(center - 1) * 2.0;
    s += LoadPixel(center    ) * 4.0;
    s += LoadPixel(center + 1) * 2.0;

    s += LoadPixel(center + TILE_SIZE - 1);
    s += LoadPixel(center + TILE_SIZE    ) * 2.0;
    s += LoadPixel(center + TILE_SIZE + 1);

    vec4 bloom = s * (1.0 / 16.0) * u_Intensity;
    vec4 outPixel = bloom;

    if(!lastPass)
        outPixel += imageLoad(u_BloomTextureMip, pixelCoords);

    // Apply dirt texture in last pass
    if (lastPass)
    {
        vec2 uv  = (vec2(pixelCoords) + vec2(0.5, 0.5)) * u_TexelSize;
        outPixel += texture(u_DirtTexture, uv) * u_DirtIntensity * bloom;
    }

    imageStore(u_BloomTextureMip, pixelCoords, outPixel);
}
