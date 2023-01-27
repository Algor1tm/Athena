#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;

out vec3 TexCoords;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    mat4 u_Transform;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
    int u_EntityID;
};

void main()
{
    TexCoords = a_Position;

    vec4 pos = u_ProjectionMatrix * u_ViewMatrix * vec4(a_Position, 1);
    gl_Position = pos.xyww;
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

in vec3 TexCoords;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    mat4 u_Transform;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
    int u_EntityID;
};

layout(binding = 0) uniform samplerCube u_Skybox;

void main()
{
    vec3 envColor = textureLod(u_Skybox, TexCoords, u_SkyboxLOD).rgb;

    envColor = vec3(1.0) - exp(-envColor * u_Exposure);
    envColor = pow(envColor, vec3(1.0 / 2.2)); 

    out_Color = vec4(envColor, 1);

    out_EntityID = -1;
}