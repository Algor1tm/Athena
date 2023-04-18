#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_Position;

struct VertexOutput
{
    vec3 TexCoords;
};

layout (location = 0) out VertexOutput Output;


layout(std140, binding = CAMERA_BUFFER_BINDER) uniform CameraData
{
	mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    vec4 Position;
    float NearClip;
	float FarClip;
} u_Camera;


void main()
{
    Output.TexCoords = a_Position;

    vec4 pos = u_Camera.ProjectionMatrix * u_Camera.ViewMatrix * vec4(a_Position, 1);
    gl_Position = pos.xyww;
}


#type FRAGMENT_SHADER
#version 460 core

layout(location = 0) out vec4 out_Color;

struct VertexOutput
{
    vec3 TexCoords;
};

layout (location = 0) in VertexOutput Input;


layout(std140, binding = ENVMAP_BUFFER_BINDER) uniform EnvMapData
{
	float LOD;
    float Intensity;
} u_EnvMapData;

layout(binding = ENVIRONMENT_MAP_BINDER) uniform samplerCube u_EnvironmentMap;


void main()
{
    vec3 envColor = textureLod(u_EnvironmentMap, Input.TexCoords, u_EnvMapData.LOD).rgb;

    out_Color = vec4(envColor, 1);
}
