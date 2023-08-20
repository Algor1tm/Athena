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
layout(rgba16f, binding = 1) uniform writeonly image2D u_HDRFramebufferDownsampled;


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


// x -> threshold, yzw -> (threshold - knee, 2.0 * knee, 0.25 * knee)

const float epsilon = 1.0e-4;

// Curve = (threshold - knee, knee * 2.0, knee * 0.25)
vec4 QuadraticThreshold(vec4 color, float threshold, vec3 curve)
{
	// Pixel brightness
    float br = max(color.r, max(color.g, color.b));

    // Under-threshold part: quadratic curve
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve.
    color *= max(rq, br - threshold) / max(br, epsilon);

    return color;
}

float Luma(vec3 c)
{
    return dot(c, vec3(0.2126729, 0.7151522, 0.0721750));
}

// [Karis2013] proposed reducing the dynamic range before averaging
vec4 KarisAverage(vec4 c)
{
    return c / (1.0 + Luma(c.rgb));
}

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
	ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
    ivec2 baseIndex   = ivec2(gl_WorkGroupID) * GROUP_SIZE - FILTER_RADIUS;

    // The first (TILE_PIXEL_COUNT - GROUP_THREAD_COUNT) threads load at most 2 texel values
    for (int i = int(gl_LocalInvocationIndex); i < TILE_PIXEL_COUNT; i += GROUP_THREAD_COUNT)
    {
        vec2 uv        = (vec2(baseIndex) + 0.5) * u_BloomData.TexelSize;
        vec2 uvOffset = vec2(i % TILE_SIZE, i / TILE_SIZE) * u_BloomData.TexelSize;
        
        vec4 color = textureLod(u_HDRFramebufferOriginal, uv + uvOffset, u_BloomData.MipLevel);
        Store_lds(i, color);
    }

    memoryBarrierShared();
    barrier();

    // Based on [Jimenez14] http://goo.gl/eomGso
    // center texel
    uint sm_idx = (gl_LocalInvocationID.x + FILTER_RADIUS) + (gl_LocalInvocationID.y + FILTER_RADIUS) * TILE_SIZE;

    vec4 A = Load_lds(sm_idx - TILE_SIZE - 1);
    vec4 B = Load_lds(sm_idx - TILE_SIZE    );
    vec4 C = Load_lds(sm_idx - TILE_SIZE + 1);
    vec4 F = Load_lds(sm_idx - 1            );
    vec4 G = Load_lds(sm_idx                );
    vec4 H = Load_lds(sm_idx + 1            );
    vec4 K = Load_lds(sm_idx + TILE_SIZE - 1);
    vec4 L = Load_lds(sm_idx + TILE_SIZE    );
    vec4 M = Load_lds(sm_idx + TILE_SIZE + 1);

    vec4 D = (A + B + G + F) * 0.25;
    vec4 E = (B + C + H + G) * 0.25;
    vec4 I = (F + G + L + K) * 0.25;
    vec4 J = (G + H + M + L) * 0.25;

    vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);

    vec4 color =  KarisAverage((D + E + I + J) * div.x);
         color += KarisAverage((A + B + G + F) * div.y);
         color += KarisAverage((B + C + H + G) * div.y);
         color += KarisAverage((F + G + L + K) * div.y);
         color += KarisAverage((G + H + M + L) * div.y);

	if (u_BloomData.EnableThreshold)
    {
        vec3 curve = vec3(u_BloomData.Threshold - u_BloomData.Knee, 2.0 * u_BloomData.Knee, 0.25 * u_BloomData.Knee);
        color = QuadraticThreshold(color, u_BloomData.Threshold, curve);
    }

	imageStore(u_HDRFramebufferDownsampled, pixelCoords, color);
}
