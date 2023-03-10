#type COMPUTE_SHADER
#version 460

layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube u_Skybox;
layout(r11f_g11f_b10f, binding = 1) uniform imageCube u_PrefiliteredMap;


layout(std140, binding = SCENE_BUFFER_BINDER) uniform SceneData
{
	mat4 u_ViewMatrix;
    mat4 u_ProjectionMatrix;
    vec4 u_CameraPosition;
    float u_NearClip;
	float u_FarClip;
    float u_MipLevel;
	float u_Exposure;
};


float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; //  0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float numerator = a2;
    float denominator = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
	
    return numerator / denominator;
}

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

    const float maxFloat = 3.402823466 * pow(10.0, 1.5);
    float roughness = u_MipLevel;

    vec3 N = direction;    
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024;
    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        if(NdotL > 0.0)
        {
            float D = DistributionGGX(NdotH, roughness);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001; 

            float resolution = textureSize(u_Skybox, 0).x;
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

            prefilteredColor += clamp(textureLod(u_Skybox, L, mipLevel).rgb, 0, maxFloat) * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    imageStore(u_PrefiliteredMap, texelCoord, vec4(prefilteredColor, 1));
}
