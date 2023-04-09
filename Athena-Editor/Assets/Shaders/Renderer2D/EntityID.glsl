#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_EntityID;


layout(std140, binding = RENDERER2D_CAMERA_BUFFER_BINDER) uniform Camera
{
	mat4 u_ViewProjection;
};

layout (location = 0) flat out int v_EntityID;


void main()
{
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1);
}


#type FRAGMENT_SHADER
#version 430 core

layout(location = 0) out int out_EntityID;

layout (location = 0) flat in int v_EntityID;


void main()
{
	out_EntityID = v_EntityID;
}
