#type VERTEX_SHADER
#version 460 core


layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;


layout(std140, binding = CAMERA_BUFFER_BINDER) uniform CameraData
{
	mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    vec4 Position;
    float NearClip;
	float FarClip;
} u_Camera;

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 Transform;
    int ID;
    bool IsAnimated;
} u_Entity;

layout(std430, binding = BONES_BUFFER_BINDER) readonly buffer BoneTransforms
{
    mat4 g_Bones[MAX_NUM_BONES];
};


void main()
{
    mat4 fullTransform = u_Entity.Transform;

    if(bool(u_Entity.IsAnimated))
    {
        mat4 boneTransform = g_Bones[a_BoneIDs[0]] * a_Weights[0];
        for(int i = 1; i < MAX_NUM_BONES_PER_VERTEX; ++i)
        {
            boneTransform += g_Bones[a_BoneIDs[i]] * a_Weights[i];
        }

       fullTransform *= boneTransform;
    }

    vec4 transformedPos = fullTransform * vec4(a_Position, 1);

    gl_Position = u_Camera.ProjectionMatrix * u_Camera.ViewMatrix * transformedPos;
}

#type GEOMETRY_SHADER
#version 460 core

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
#version 460 core

layout(location = 0) out vec4 out_Color;

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 Transform;
    int ID;
    bool IsAnimated;
} u_Entity;


void main()
{
    out_Color = vec4(0.2, 0.9, 0.3, 1);
}
