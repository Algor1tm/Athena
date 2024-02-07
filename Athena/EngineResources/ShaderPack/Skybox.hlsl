//////////////////////// Athena Environment Map Shader ////////////////////////

#include "Buffers.hlsli"


struct Vertex
{
    float3 Position;
};

struct Interpolators
{
    float4 OutPosition : SV_Position;
    float3 TexCoords;
};

Interpolators VSMain(Vertex vertex)
{
    Interpolators output;
    
    float4 pos = mul(mul(float4(vertex.Position, 1), u_Camera.RotationView), u_Camera.Projection);
    
    output.OutPosition = pos.xyww;
    output.TexCoords = vertex.Position;
    
    return output;
}


struct Fragment
{
    float4 Color : SV_TARGET0;
};

[[vk::combinedImageSampler]]
TextureCube u_EnvironmentMap : register(t2, space1);
SamplerState u_Sampler : register(s2, space1);


Fragment FSMain(Interpolators input)
{
    Fragment output;
    
    output.Color = u_EnvironmentMap.SampleLevel(u_Sampler, input.TexCoords, u_EnivornmentLOD) * u_EnvironmentIntensity;
    return output;
}
