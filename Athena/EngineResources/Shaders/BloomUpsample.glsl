#type COMPUTE_SHADER
#version 460

#define GROUP_SIZE         8
#define GROUP_THREAD_COUNT (GROUP_SIZE * GROUP_SIZE)
#define FILTER_SIZE        3
#define FILTER_RADIUS      (FILTER_SIZE / 2)
#define TILE_SIZE          (GROUP_SIZE + 2 * FILTER_RADIUS)
#define TILE_PIXEL_COUNT   (TILE_SIZE * TILE_SIZE)

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE) in;

layout(binding = 0)			 uniform sampler2D u_HDRFramebufferOriginal;
layout(rgba16f, binding = 1) uniform image2D u_HDRFramebufferDownsampled;

layout(binding = 2)			 uniform sampler2D u_DirtTexture;


layout(std140, binding = BLOOM_BUFFER_BINDER) uniform BloomData
{
	float Intensity;
    float Threshold;
    float Knee;
    float DirtIntensity;
    vec2 TexelSize;
    bool EnableThreshold;
    int MipLevel;
} u_BloomData;


shared float sm_r[TILE_PIXEL_COUNT];
shared float sm_g[TILE_PIXEL_COUNT];
shared float sm_b[TILE_PIXEL_COUNT];

void Store_lds(int idx, vec4 c)
{
    sm_r[idx] = c.r;
    sm_g[idx] = c.g;
    sm_b[idx] = c.b;
}

vec4 Load_lds(uint idx)
{
    return vec4(sm_r[idx], sm_g[idx], sm_b[idx], 1.0);
}

void main()
{
	ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec2  baseIndex   = ivec2(gl_WorkGroupID) * GROUP_SIZE - FILTER_RADIUS;

    // The first (TILE_PIXEL_COUNT - GROUP_THREAD_COUNT) threads load at most 2 texel values
    for (int i = int(gl_LocalInvocationIndex); i < TILE_PIXEL_COUNT; i += GROUP_THREAD_COUNT)
    {
        vec2 uv        = (baseIndex + 0.5) * u_BloomData.TexelSize;
        vec2 uvOffset = vec2(i % TILE_SIZE, i / TILE_SIZE) * u_BloomData.TexelSize;
        
        vec4 color = textureLod(u_HDRFramebufferOriginal, (uv + uvOffset), u_BloomData.MipLevel);
        Store_lds(i, color);
    }

    memoryBarrierShared();
    barrier();

    // center texel
    uint sm_idx = (gl_LocalInvocationID.x + FILTER_RADIUS) + (gl_LocalInvocationID.y + FILTER_RADIUS) * TILE_SIZE;

    // Based on [Jimenez14] http://goo.gl/eomGso
    vec4 s;
    s =  Load_lds(sm_idx - TILE_SIZE - 1);
    s += Load_lds(sm_idx - TILE_SIZE    ) * 2.0;
    s += Load_lds(sm_idx - TILE_SIZE + 1);
	
    s += Load_lds(sm_idx - 1) * 2.0;
    s += Load_lds(sm_idx    ) * 4.0;
    s += Load_lds(sm_idx + 1) * 2.0;
	
    s += Load_lds(sm_idx + TILE_SIZE - 1);
    s += Load_lds(sm_idx + TILE_SIZE    ) * 2.0;
    s += Load_lds(sm_idx + TILE_SIZE + 1);

    vec4 bloom = s * (1.0 / 16.0);

	vec4 outPixel = imageLoad(u_HDRFramebufferDownsampled, pixelCoords);
	     outPixel += bloom * u_BloomData.Intensity;

    if (u_BloomData.MipLevel == 1)
    {
        vec2  uv  = (vec2(pixelCoords) + vec2(0.5, 0.5)) * u_BloomData.TexelSize;
        outPixel += texture(u_DirtTexture, uv) * u_BloomData.DirtIntensity * bloom * u_BloomData.Intensity;
    }

	imageStore(u_HDRFramebufferDownsampled, pixelCoords, outPixel);
}
