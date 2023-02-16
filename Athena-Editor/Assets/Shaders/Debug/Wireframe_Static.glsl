#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
};

layout(std140, binding = 2) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
};

void main()
{
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * u_Transform * vec4(a_Position, 1);
}

#type GEOMETRY_SHADER
#version 430 core

layout (triangles) in;
layout (line_strip, max_vertices = 3) out;

void main()
{
    for (int i = 0; i < gl_in.length(); i++)
    {
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}


#type FRAGMENT_SHADER
#version 430 core

layout(std140, binding = 2) uniform EntityData
{
    mat4 u_Transform;
    int u_EntityID;
};

layout(location = 0) out vec4 out_Color;
layout(location = 1) out int out_EntityID;

void main()
{
    out_Color = vec4(0.2, 0.9, 0.3, 1);
    out_EntityID = u_EntityID;
}
