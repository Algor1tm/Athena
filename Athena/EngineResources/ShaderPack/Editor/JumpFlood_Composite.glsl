//////////////////////// Athena JumpFlood Pass Shader////////////////////////

// References:
//     https://gist.github.com/bgolus/a18c1a3fc9af2d73cc19169a809eb195

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

#define SNORM16_MAX_FLOAT_MINUS_EPSILON (float(32768-2) / float(32768-1))
#define ENCODE_OFFSET vec2(1.0, SNORM16_MAX_FLOAT_MINUS_EPSILON)
#define ENCODE_SCALE vec2(2.0, 1.0 + SNORM16_MAX_FLOAT_MINUS_EPSILON)

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform u_Uniforms
{
    vec4 u_OutlineColor;
    float u_OutlineWidth;    
};

void main()
{
    vec2 texSize = textureSize(u_Texture, 0);
    vec2 texelSize = 1.0 / texSize;

    ivec2 uvInt = ivec2(v_TexCoords * texSize);

    vec2 encodedPos = texelFetch(u_Texture, uvInt, 0).rg;

    //if(encodedPos.y == -1.0)
    //{
    //    o_Color = vec4(0.0);
    //    return;
    //}

    vec2 nearestPos = (encodedPos + ENCODE_OFFSET) * texSize / ENCODE_SCALE;
    vec2 currentPos = uvInt;

    float dist = length(nearestPos - currentPos);

    // + 1.0 is because encoded nearest position is half a pixel inset
    // not + 0.5 because we want the anti-aliased edge to be aligned between pixels
    // distance is already in pixels, so this is already perfectly anti-aliased!
    float outline = clamp(u_OutlineWidth - dist + 1.0, 0.0, 1.0);

    vec4 color = u_OutlineColor;
    color.a *= outline;

    o_Color = color;
}
