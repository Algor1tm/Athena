//////////////////////// Athena HBAO Blur Shader ////////////////////////

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

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec2 o_AODepth;

#include "Include/HBAO.glslh"

layout(set = 1, binding = 0) uniform sampler2D u_AODepth;

#define KERNEL_RADIUS 3

float BlurFunction(vec2 uv, float r, float centerAO, float centerDepth, inout float totalW)
{
    vec2 aoz = texture(u_AODepth, uv).xy;
    float c = aoz.x;
    float d = aoz.y;
  
    const float blurSigma = float(KERNEL_RADIUS) * 0.5;
    const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);
  
    float ddiff = (d - centerDepth) * u_HBAO.BlurSharpness;
    float w = exp2(-r * r * blurFalloff - ddiff * ddiff);
    totalW += w;

    return c * w;
}

void main()
{
    vec2 aoz = texture(u_AODepth, v_TexCoords).xy;
    float centerAO = aoz.x;
    float centerDepth = aoz.y;
 
    float totalAO = centerAO;
    float totalW = 1.0;

    vec2 direction = vec2(u_HBAO.InvResolution.x, 0);

    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        vec2 uv = v_TexCoords + direction * r;
        totalAO += BlurFunction(uv, r, centerAO, centerDepth, totalW);  
    }
  
    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        vec2 uv = v_TexCoords - direction * r;
        totalAO += BlurFunction(uv, r, centerAO, centerDepth, totalW);  
    }

    o_AODepth = vec2(totalAO / totalW, centerDepth);
}
