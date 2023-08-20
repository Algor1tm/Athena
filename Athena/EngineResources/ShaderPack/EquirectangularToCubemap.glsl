#type COMPUTE_SHADER
#version 460

layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D u_EquirectangularMap;
layout(r11f_g11f_b10f, binding = 1) uniform imageCube u_Skybox;


vec2 SampleSphericalMap(vec3 pos)
{
    vec2 uv = vec2(atan(pos.z, pos.x), asin(pos.y));
    uv *= vec2(1 / (2 * PI), 1 / PI);
    uv += 0.5;
    return uv;
}

vec3 CubeCoordToWorld(ivec3 cubeCoord)
{
    vec2 texCoord = vec2(cubeCoord.xy)  / vec2(gl_NumWorkGroups * gl_WorkGroupSize);
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

    vec2 uv = SampleSphericalMap(direction);
    vec3 color = texture(u_EquirectangularMap, uv).rgb;
	
    imageStore(u_Skybox, texelCoord, vec4(color, 1));
}
