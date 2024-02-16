//////////////////////// Athena Environment Mip Filter Shader ////////////////////////

// References:
// https://learnopengl.com/PBR/IBL/Specular-IBL


#version 460 core
#pragma stage : compute

#include "Include/Common.glslh"

#define SAMPLE_COUNT 1024

layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(set = 1, binding = 0) uniform samplerCube u_EnvironmentMap;
layout(r11f_g11f_b10f, set = 0, binding = 0) uniform imageCube u_EnvironmentMipImage;

layout(push_constant) uniform MipData
{
    uint u_MipLevel;
};

void main() 
{
    ivec3 unnormalizedTexCoords = ivec3(gl_GlobalInvocationID.xyz);
    vec2 texCoords = vec2(unnormalizedTexCoords.xy) / vec2(gl_NumWorkGroups * gl_WorkGroupSize);

    vec3 direction = ImageCubeCoordsToWorldDirection(texCoords, unnormalizedTexCoords.z);
    direction.y = -direction.y;

    float roughness = float(u_MipLevel) / MAX_SKYBOX_MAP_LOD;

    vec3 N = direction;    
    vec3 R = N;
    vec3 V = R;

    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            float D = DistributionGGX(N, H, roughness);

            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);

            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001; 

            float resolution = textureSize(u_EnvironmentMap, 0).x;
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

            prefilteredColor += textureLod(u_EnvironmentMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    imageStore(u_EnvironmentMipImage, unnormalizedTexCoords, vec4(prefilteredColor, 1));
}
