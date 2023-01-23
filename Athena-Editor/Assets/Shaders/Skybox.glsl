#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;

out vec3 TexCoords;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewProjection;
    vec4 u_CameraPosition;
    mat4 u_Transform;
    int u_EntityID;
};

void main()
{
    vec3 uv = a_Position;
    TexCoords = vec3(uv.x, uv.y, -uv.z);
    vec4 pos = u_ViewProjection * vec4(a_Position, 1);
    gl_Position = pos.xyww;
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

in vec3 TexCoords;

layout(binding = 0) uniform samplerCube u_Skybox;

void main()
{
    out_Color = texture(u_Skybox, TexCoords);
    //out_Color = vec4(TexCoords, 1);
    out_EntityID = -1;
}
