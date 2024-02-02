//////////////////////// Athena Composite Shader ////////////////////////

#include "Buffers.hlsli"

struct Vertex
{
    float2 Position;
    float2 TexCoords;
};

struct Interpolators
{
    float4 OutPosition : SV_POSITION;
    float2 TexCoords;
};


Interpolators VSMain(Vertex vertex)
{
    Interpolators output;
    output.OutPosition = float4(vertex.Position, 0, 1);
    output.TexCoords = vertex.TexCoords;
    
    return output;
}

struct Fragment
{
    float4 Color : SV_TARGET0;
};

[[vk::combinedImageSampler]]
Texture2D u_SceneHDRColor : register(t0, space1);
SamplerState u_Sampler : register(s0, space1);


Fragment FSMain(Interpolators input)
{
    Fragment output;
    
    float3 hdrColor = u_SceneHDRColor.Sample(u_Sampler, input.TexCoords).rgb;
    
    float3 color = float3(1.0) - exp(-hdrColor * u_Exposure);
    color = pow(color, float3(1.0 / u_Gamma));
    output.Color = float4(color, 1.0);
    
    return output;
}
