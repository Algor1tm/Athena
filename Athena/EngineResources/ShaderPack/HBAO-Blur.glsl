//////////////////////// Athena SSAO Denoise Shader ////////////////////////

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
layout(location = 0) out float o_AOValue;

layout(set = 1, binding = 0) uniform sampler2D u_AOTexture;


void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_AOTexture, 0));

    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(u_AOTexture, v_TexCoords + offset).r;
        }
    }

    o_AOValue = result / (4.0 * 4.0);
}
