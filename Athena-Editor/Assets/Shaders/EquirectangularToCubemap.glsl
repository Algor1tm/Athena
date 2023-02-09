#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;

out vec3 LocalPos;

layout(std140, binding = 1) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_SkyboxLOD;
	float u_Exposure;
};

void main()
{
    LocalPos = a_Position;  
    gl_Position =  u_ProjectionMatrix * u_ViewMatrix * vec4(a_Position, 1.0);
}

#type FRAGMENT_SHADER
#version 430 core

out vec4 out_Color;
in vec3 LocalPos;

uniform sampler2D u_EquirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 pos)
{
    vec2 uv = vec2(atan(pos.z, pos.x), asin(pos.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(LocalPos));
    vec3 color = texture(u_EquirectangularMap, uv).rgb;

    out_Color = vec4(color, 1.0);
}