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

// LOL
#define INFINITY 1000000

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec2 o_Value;

layout(set = 1, binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform u_Uniforms
{
    int u_StepWidth;    
};

void main()
{
    vec2 texSize = textureSize(u_Texture, 0);
    vec2 texelSize = 1.0 / texSize;

    ivec2 uvInt = ivec2(v_TexCoords * texSize);

    float bestDist = INFINITY;
    vec2 bestCoord;

    for(int u = -1; u <= 1; u++)
    {
        for(int v = -1; v <= 1; v++)
        {
            ivec2 uv = uvInt + ivec2(u, v) * u_StepWidth;
            vec2 offsetPos = (texelFetch(u_Texture, uv, 0).rg + ENCODE_OFFSET) * texSize / ENCODE_SCALE;

            vec2 disp = uvInt - offsetPos;
            float dist = dot(disp, disp);

            // if offset position isn't a null position or is closer than the best
            // set as the new best and store the position
            if (offsetPos.y != -1.0 && dist < bestDist)
            {
                bestDist = dist;
                bestCoord = offsetPos;
            }
        }
    }

    if(bestDist == INFINITY)
        o_Value = vec2(-1.0);
    else
        o_Value = bestCoord * texelSize * ENCODE_SCALE - ENCODE_OFFSET;
}
