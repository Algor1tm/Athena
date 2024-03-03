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

layout(set = 1, binding = 0) uniform sampler2D u_SceneHDRColor;
layout(set = 1, binding = 1) uniform sampler2D u_BloomTexture;

layout(r11f_g11f_b10f, set = 0, binding = 1) uniform writeonly image2D u_BloomTextureMip;


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


// x -> threshold, yzw -> (threshold - knee, 2.0 * knee, 0.25 * knee)
// Curve = (threshold - knee, knee * 2.0, knee * 0.25)
vec4 QuadraticThreshold(vec4 color, float threshold, vec3 curve)
{
    const float epsilon = 1.0e-4;

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

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
    ivec2 baseIndex = ivec2(gl_WorkGroupID) * GROUP_SIZE - FILTER_RADIUS;
    bool firstPass = u_ReadMipLevel == 0;

    // The first (TILE_PIXEL_COUNT - GROUP_THREAD_COUNT) threads load at most 2 texel values
    for (int i = int(gl_LocalInvocationIndex); i < TILE_PIXEL_COUNT; i += GROUP_THREAD_COUNT)
    {
        vec2 uv       = (vec2(baseIndex) + 0.5) * u_TexelSize;
        vec2 uvOffset = vec2(i % TILE_SIZE, i / TILE_SIZE) * u_TexelSize;
        
        // On first pass read from scene texture
        vec4 color;
        if(firstPass)
        {
            color = texture(u_SceneHDRColor, uv + uvOffset);
        }
        else
        {
            color = textureLod(u_BloomTexture, uv + uvOffset, u_ReadMipLevel);
        }

        StorePixel(i, color);
    }

    memoryBarrierShared();
    barrier();

    uint center = (gl_LocalInvocationID.x + FILTER_RADIUS) + (gl_LocalInvocationID.y + FILTER_RADIUS) * TILE_SIZE;
    
    vec4 A = LoadPixel(center - TILE_SIZE - 1);
    vec4 B = LoadPixel(center - TILE_SIZE    );
    vec4 C = LoadPixel(center - TILE_SIZE + 1);
    vec4 F = LoadPixel(center - 1            );
    vec4 G = LoadPixel(center                );
    vec4 H = LoadPixel(center + 1            );
    vec4 K = LoadPixel(center + TILE_SIZE - 1);
    vec4 L = LoadPixel(center + TILE_SIZE    );
    vec4 M = LoadPixel(center + TILE_SIZE + 1);

    vec4 D = (A + B + G + F) * 0.25;
    vec4 E = (B + C + H + G) * 0.25;
    vec4 I = (F + G + L + K) * 0.25;
    vec4 J = (G + H + M + L) * 0.25;

    vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);

    vec4 color  = KarisAverage((D + E + I + J) * div.x);
         color += KarisAverage((A + B + G + F) * div.y);
         color += KarisAverage((B + C + H + G) * div.y);
         color += KarisAverage((F + G + L + K) * div.y);
         color += KarisAverage((G + H + M + L) * div.y);

    if (firstPass)
    {
        vec3 curve = vec3(u_Threshold - u_Knee, 2.0 * u_Knee, 0.25 * u_Knee);
        color = QuadraticThreshold(color, u_Threshold, curve);
    }

    imageStore(u_BloomTextureMip, pixelCoords, color);
}
