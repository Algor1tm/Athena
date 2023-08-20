#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoords;

struct VertexOutput
{
    vec2 TexCoords;
};

layout (location = 0) out VertexOutput Output;


void main()
{
    Output.TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 0, 1);
}


#type FRAGMENT_SHADER
#version 460 core

layout(location = 0) out vec4 out_Color;

struct VertexOutput
{
    vec2 TexCoords;
};

layout (location = 0) in VertexOutput Input;

layout(binding = 0) uniform sampler2D u_HDRFramebuffer;
layout(binding = 1) uniform sampler2D u_DepthBuffer;


layout(std140, binding = SCENE_BUFFER_BINDER) uniform SceneData
{
	float Exposure;
    float Gamma;
} u_Scene;


void main()
{
    vec3 hdrColor = texture(u_HDRFramebuffer, Input.TexCoords).rgb;
    float depth = texture(u_DepthBuffer, Input.TexCoords).r;

    vec3 color = vec3(1.0) - exp(-hdrColor * u_Scene.Exposure);
    color = pow(color, vec3(1.0 / u_Scene.Gamma)); 

    gl_FragDepth = depth;
    out_Color = vec4(color, 1);
}
