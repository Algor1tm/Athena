#version 460

layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube u_Skybox;
layout(r11f_g11f_b10f, binding = 1) uniform imageCube u_IrradianceMap;


#define PI 3.14159265359

vec3 CubeCoordToWorld(ivec3 cubeCoord)
{
    vec2 texCoord = vec2(cubeCoord.xy) / vec2(gl_NumWorkGroups * gl_WorkGroupSize);
    texCoord = texCoord  * 2.0 - 1.0;
    vec3 direction;
    switch(cubeCoord.z)
    {
        case 0: direction = vec3(1.0, -texCoord.yx); break;
        case 1: direction = vec3(-1.0, -texCoord.y, texCoord.x); break; 
        case 2: direction = vec3(texCoord.x, 1.0, texCoord.y); break; 
        case 3: direction = vec3(texCoord.x, -1.0, -texCoord.y); break;
        case 4: direction = vec3(texCoord.x, -texCoord.y, 1.0); break; 
        case 5: direction = vec3(-texCoord.xy, -1.0); break; 
    }

    return normalize(direction);
}

void main() 
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    vec3 direction = CubeCoordToWorld(texelCoord);

    const float maxFloat = 3.402823466 * pow(10.0, 1.65);

    // the sample direction equals the hemisphere's orientation 
    vec3 normal = direction;

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

            irradiance += clamp(texture(u_Skybox, sampleVec).rgb, 0, maxFloat) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    imageStore(u_IrradianceMap, texelCoord, vec4(irradiance, 1));
}
