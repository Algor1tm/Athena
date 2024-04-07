//////////////////////// Athena Bloom Downsample Shader ////////////////////////

// References:
//   https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/


#version 460 core
#pragma stage : compute

layout(local_size_x = 8, local_size_y = 4) in;

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

vec3 Sample(vec2 uv, float xOff, float yOff)
{
    if(u_ReadMipLevel == 0)
        return texture(u_SceneHDRColor, uv + vec2(xOff, yOff) * u_TexelSize).rgb;

    return textureLod(u_BloomTexture, uv + vec2(xOff, yOff) * u_TexelSize, u_ReadMipLevel).rgb;
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) * u_TexelSize;
    bool firstPass = u_ReadMipLevel == 0;

    // 36-texel downsample (13 bilinear fetches)
    /*--------------------------
          A    B    C  
             D    E
          F    G    H
             I    J
          K    L    M
    --------------------------*/

    vec3 A = Sample(uv, -1, -1);
    vec3 B = Sample(uv,  0, -1);
    vec3 C = Sample(uv,  1, -1);

    vec3 F = Sample(uv, -1,  0);
    vec3 G = Sample(uv,  0,  0);
    vec3 H = Sample(uv,  1,  0);

    vec3 K = Sample(uv, -1,  1);
    vec3 L = Sample(uv,  0,  1);
    vec3 M = Sample(uv,  1,  1);

    vec3 D = Sample(uv, -0.5, -0.5);
    vec3 E = Sample(uv, -0.5,  0.5);
    vec3 I = Sample(uv,  0.5, -0.5);
    vec3 J = Sample(uv,  0.5,  0.5);

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
