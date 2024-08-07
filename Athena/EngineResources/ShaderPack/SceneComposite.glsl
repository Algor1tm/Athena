//////////////////////// Athena Composite Shader ////////////////////////

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

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 4) uniform sampler2D u_SceneHDRColor;
layout(set = 1, binding = 5) uniform sampler2D u_BloomTexture;


#define TONEMAP_MODE_NONE 0
#define TONEMAP_MODE_ACES_FILMIC 1
#define TONEMAP_MODE_ACES_TRUE 2


layout(push_constant) uniform u_Uniforms
{
    uint u_Mode;
    float u_Exposure;
    bool u_EnableBloom;
};


// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 Tonemap_ACES_FILMIC(vec3 hdrColor, float exposure)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    hdrColor *= exposure * 0.6;

    return clamp((hdrColor * (a * hdrColor + b)) / (hdrColor * (c * hdrColor + d) + e), 0.0, 1.0);
}

// https://www.shadertoy.com/view/XsGfWV
vec3 Tonemap_ACES_TRUE(vec3 hdrColor, float exposure)
{
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

    hdrColor *= exposure * 0.6;

	vec3 v = m1 * hdrColor;    
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);	
}

vec3 Tonemap(vec3 hdrColor)
{
    if(u_Mode == TONEMAP_MODE_ACES_FILMIC)
        return Tonemap_ACES_FILMIC(hdrColor, u_Exposure);
    
    if(u_Mode == TONEMAP_MODE_ACES_TRUE)
        return Tonemap_ACES_TRUE(hdrColor, u_Exposure);

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

#if FILM_GRAIN
    float grain = FilmGrain();
    float grainStrength = 0.05;

    color += vec3(grain * grainStrength);
#endif

    o_Color = vec4(color, 1.0);
}
