#type VERTEX_SHADER
#version 430 core

layout (location = 0) in vec3 a_Position;

out vec3 LocalPos;

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
    LocalPos = a_Position;  
    gl_Position =  u_ProjectionMatrix * u_ViewMatrix * vec4(a_Position, 1.0);
}

#type FRAGMENT_SHADER
#version 430 core

out vec4 out_Color;
in vec3 LocalPos;

layout(binding = 0) uniform samplerCube u_EnvironmentMap;

const float PI = 3.14159265359;

void main()
{		
    const float maxFloat = 3.402823466 * pow(10.0, 1.65);

    // the sample direction equals the hemisphere's orientation 
    vec3 normal = normalize(LocalPos);

    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0; 
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

            irradiance += clamp(textureLod(u_EnvironmentMap, sampleVec, 0).rgb, 0, maxFloat) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    out_Color = vec4(irradiance, 1.0);
}
