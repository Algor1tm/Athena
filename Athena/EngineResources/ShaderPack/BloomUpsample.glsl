//////////////////////// Athena Bloom Downsample Shader ////////////////////////

// References:
//   https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/


#version 460 core
#pragma stage : compute

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 1, binding = 0) uniform sampler2D u_BloomTexture;
layout(set = 1, binding = 1) uniform sampler2D u_DirtTexture;

layout(r11f_g11f_b10f, set = 0, binding = 0) uniform image2D u_BloomTextureMip;


layout(push_constant) uniform u_BloomData
{
	float u_Intensity;
    float u_Threshold;
    float u_Knee;
    float u_DirtIntensity;
    vec2 u_TexelSize;
    uint u_ReadMipLevel;
};

vec3 Sample(vec2 uv, float xOff, float yOff)
{
    return textureLod(u_BloomTexture, uv + vec2(xOff, yOff) * u_TexelSize, u_ReadMipLevel).rgb;
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) * u_TexelSize;
    bool lastPass = u_ReadMipLevel == 1;

    // 9-tap tent filter
    /*--------------------------
              | 1  2  1 |
     1 / 16 * | 2  4  2 |
              | 1  2  1 |
    --------------------------*/

    vec3 s;
    s  = Sample(uv, -2, -2);
    s += Sample(uv, -2,  0) * 2.0;
    s += Sample(uv, -2,  2);

    s += Sample(uv,  0, -2) * 2.0;
    s += Sample(uv,  0,  0) * 4.0;
    s += Sample(uv,  0,  2) * 2.0;

    s += Sample(uv,  2, -2);
    s += Sample(uv,  2,  0) * 2.0;
    s += Sample(uv,  2,  2);

    vec3 bloom = s * (1.0 / 16.0) * u_Intensity;
    vec3 outPixel = bloom;

    if(!lastPass)
        outPixel += imageLoad(u_BloomTextureMip, pixelCoords).rgb;

    // Apply dirt texture in last pass
    if (lastPass)
    {
        vec2 uv  = (vec2(pixelCoords) + vec2(0.5, 0.5)) * u_TexelSize;
        outPixel += texture(u_DirtTexture, uv).rgb * u_DirtIntensity * bloom;
    }

    imageStore(u_BloomTextureMip, pixelCoords, vec4(outPixel, 1.0));
}
