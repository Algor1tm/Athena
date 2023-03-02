#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_Position;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_LightSpaceMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
};

layout(std140, binding = 2) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
    bool u_Animated;
};

void main()
{
   gl_Position = u_LightSpaceMatrix * u_Transform * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 460 core

void main()
{

}
