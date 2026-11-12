//////////////////////// Athena JumpFlood Init Shader////////////////////////

// References:
//     https://gist.github.com/bgolus/a18c1a3fc9af2d73cc19169a809eb195

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

#define SNORM16_MAX_FLOAT_MINUS_EPSILON (float(32768-2) / float(32768-1))
#define ENCODE_OFFSET vec2(1.0, SNORM16_MAX_FLOAT_MINUS_EPSILON)
#define ENCODE_SCALE vec2(2.0, 1.0 + SNORM16_MAX_FLOAT_MINUS_EPSILON)

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec2 o_EncodedPosition;

layout(set = 1, binding = 0) uniform sampler2D u_SilhouetteTexture;


void main()
{
    vec2 texSize = textureSize(u_SilhouetteTexture, 0);
    vec2 texelSize = 1.0 / texSize;

    ivec2 uvInt = ivec2(v_TexCoords * texSize);

    mat3 values;
    for(int u = -1; u <= 1; u++)
    {
        for(int v = -1; v <= 1; v++)
        {
            ivec2 uv = uvInt + ivec2(u, v);
            values[u + 1][v + 1] = texelFetch(u_SilhouetteTexture, uv, 0).r;
        }
    }

    // sobel to estimate edge direction
    vec2 dir = -vec2(
        values[0][0] + values[0][1] * 2.0 + values[0][2] - values[2][0] - values[2][1] * 2.0 - values[2][2],
        values[0][0] + values[1][0] * 2.0 + values[2][0] - values[0][2] - values[1][2] * 2.0 - values[2][2]
        );

    // if dir length is small, this is either a sub pixel dot or line
    // no way to estimate sub pixel edge, so output position
    if (abs(dir.x) <= 0.005 && abs(dir.y) <= 0.005)
    {
        o_EncodedPosition = vec2(-1.0);
        return;
    }

    dir = normalize(dir);

    vec2 offset = dir * (1.0 - values[1][1]);
    o_EncodedPosition = (uvInt + offset) * texelSize * ENCODE_SCALE - ENCODE_OFFSET;
}
