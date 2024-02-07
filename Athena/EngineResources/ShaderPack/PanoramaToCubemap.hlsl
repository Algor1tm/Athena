//////////////////////// Athena Panorama To Cubemap Shader ////////////////////////

#include "Common.hlsli"

//Texture2D<float4> u_PanoramaTex : register(b0, space1);

[[vk::image_format("rgba32f")]]
RWTexture2DArray<float4> u_Cubemap : register(b1, space1);


[numthreads(8, 4, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 unnormalizedTexCoords = dispatchThreadId;
    u_Cubemap[unnormalizedTexCoords] = float4(0.0, 1.0, 0, 1.0);
}
