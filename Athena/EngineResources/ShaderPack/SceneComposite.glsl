//////////////////////// Athena Composite Shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoords;

layout(location = 0) out vec2 v_TexCoords;

void main()
{
    gl_Position = vec4(a_Position, 0, 1);
    v_TexCoords = a_TexCoords;
}

#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 4) uniform sampler2D u_SceneHDRColor;
layout(set = 1, binding = 5) uniform sampler2D u_BloomTexture;


#define TONEMAP_MODE_NONE 0
#define TONEMAP_MODE_ACES 1
#define TONEMAP_MODE_EXPOSURE 2


layout(push_constant) uniform u_Uniforms
{
    uint u_Mode;
    float u_Exposure;
    bool u_EnableBloom;
};


vec3 Tonemap_ACES(vec3 hdrColor)
{
#if 1 // https://www.shadertoy.com/view/XsGfWV

    mat3 m1 = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
	);

	mat3 m2 = mat3(
        1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
	);

	vec3 v = m1 * hdrColor;    
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);	

#else // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/

    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return clamp((hdrColor * (a * hdrColor + b)) / (hdrColor * (c * hdrColor + d) + e), 0.0, 1.0);

#endif
}

vec3 Tonemap_Exposure(vec3 hdrColor, float exposure)
{
    return vec3(1.0) - exp(-hdrColor * exposure);
}

vec3 Tonemap(vec3 hdrColor)
{
    if(u_Mode == TONEMAP_MODE_EXPOSURE)
        return Tonemap_Exposure(hdrColor, u_Exposure);
    
    if(u_Mode == TONEMAP_MODE_ACES)
        return Tonemap_ACES(hdrColor);

    return clamp(hdrColor, 0.0, 1.0);
};

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0 / DISPLAY_GAMMA));
}

float FilmGrain()
{
    float noise = (fract(sin(dot(v_TexCoords, vec2(12.9898,78.233) * 2.0)) * 43758.5453));
    return -noise;
}


void main()
{
    vec3 hdrColor = texture(u_SceneHDRColor, v_TexCoords).rgb;

    // Composite Bloom texture
    if(u_EnableBloom)
        hdrColor += texture(u_BloomTexture, v_TexCoords).rgb;

    vec3 color = Tonemap(hdrColor);
    color = GammaCorrect(color);

    float grain = FilmGrain();
    float grainStrength = 0.05;

    //color += vec3(grain * grainStrength);

    o_Color = vec4(color, 1.0);
}
