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


layout(push_constant) uniform u_BloomData
{
	float u_Intensity;
    float u_Threshold;
    float u_Knee;
    float u_DirtIntensity;
    vec2 u_TexelSize;
    uint u_ReadMipLevel;
};

vec3 Sample(vec2 uv, int xOff, int yOff)
{
    if(u_ReadMipLevel == 0)
        return texture(u_SceneHDRColor, uv + vec2(xOff, yOff) * u_TexelSize).rgb;

    return textureLod(u_BloomTexture, uv + vec2(xOff, yOff) * u_TexelSize, u_ReadMipLevel).rgb;
}

shared vec3 s_Pixels[TILE_PIXEL_COUNT];

void StorePixel(uint idx, vec3 color)
{
    s_Pixels[idx] = color;
}

vec3 LoadPixel(uint center, int xOff, int yOff)
{
    uint idx = center + xOff + yOff * TILE_SIZE;
    return s_Pixels[idx];
}

// x -> threshold, yzw -> (threshold - knee, 2.0 * knee, 0.25 * knee)
// Curve = (threshold - knee, knee * 2.0, knee * 0.25)
vec3 QuadraticThreshold(vec3 color, float threshold, vec3 curve)
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
vec3 KarisAverage(vec3 c)
{
    return c / (1.0 + Luma(c));
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
    ivec2 baseIndex = ivec2(gl_WorkGroupID) * GROUP_SIZE - FILTER_RADIUS;
    vec2 uv = (vec2(baseIndex) + 0.5) * u_TexelSize;

    // The first (TILE_PIXEL_COUNT - GROUP_THREAD_COUNT) threads load at most 2 texel values
    for (int i = int(gl_LocalInvocationIndex); i < TILE_PIXEL_COUNT; i += GROUP_THREAD_COUNT)
    {
        int xOff = i % TILE_SIZE;
        int yOff = i / TILE_SIZE;

        vec3 color = Sample(uv, xOff, yOff);
        StorePixel(i, color);
    }

    memoryBarrierShared();
    barrier();

    // 36-texel downsample (13 bilinear fetches)
    /*--------------------------
          A    B    C  
             D    E
          F    G    H
             I    J
          K    L    M
    --------------------------*/


    bool firstPass = u_ReadMipLevel == 0;
    uint center = (gl_LocalInvocationID.x + FILTER_RADIUS) + (gl_LocalInvocationID.y + FILTER_RADIUS) * TILE_SIZE;

    vec3 A = LoadPixel(center, -1, -1);
    vec3 B = LoadPixel(center,  0, -1);
    vec3 C = LoadPixel(center,  1, -1);

    vec3 F = LoadPixel(center, -1,  0);
    vec3 G = LoadPixel(center,  0,  0);
    vec3 H = LoadPixel(center,  1,  0);

    vec3 K = LoadPixel(center, -1,  1);
    vec3 L = LoadPixel(center,  0,  1);
    vec3 M = LoadPixel(center,  1,  1);

    // Seems that these values are not exactly correct, but despite this bloom result is ok
    vec3 D = (A + B + G + F) * 0.25;
    vec3 E = (B + C + H + G) * 0.25;
    vec3 I = (F + G + L + K) * 0.25;
    vec3 J = (G + H + M + L) * 0.25;

    vec3 red    = 0.5   * (D + E + J + I) * 0.25;
    vec3 yellow = 0.125 * (G + F + A + B) * 0.25;
    vec3 green  = 0.125 * (G + B + C + H) * 0.25;
    vec3 blue   = 0.125 * (G + H + L + M) * 0.25;
    vec3 purple = 0.125 * (G + L + K + F) * 0.25;

    vec3 result;

    // Apply KarisAverage and Threshold in first pass
    if(firstPass)
    {
        red    = KarisAverage(red);
        yellow = KarisAverage(yellow);
        green  = KarisAverage(green);
        blue   = KarisAverage(blue);
        purple = KarisAverage(purple);
        
        result = red + yellow + green + blue + purple;

        vec3 curve = vec3(u_Threshold - u_Knee, 2.0 * u_Knee, 0.25 * u_Knee);
        result = QuadraticThreshold(result, u_Threshold, curve);
    }
    else
    {
        result = red + yellow + green + blue + purple;

#if 0
	    result  = G * 0.125;
	    result += (A + C + K + M) * 0.03125;
	    result += (B + F + H + L) * 0.0625;
	    result += (D + E + I + J) * 0.125;
#endif
    }

    imageStore(u_BloomTextureMip, pixelCoords, vec4(result, 1.0));
}
