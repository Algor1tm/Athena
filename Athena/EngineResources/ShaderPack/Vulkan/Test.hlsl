#include "Buffers.hlsli"

struct Vertex
{
    float2 Position;
    float4 Color;
};

struct Interpolators
{
    float4 Position : SV_POSITION;
    float4 Color;
};

[[vk::push_constant]]
cbuffer MaterialData
{
    float4x4 u_Transform;
};


Interpolators VSMain(Vertex vertex)
{
    Interpolators output;
    
    output.Position = mul(mul(mul(float4(vertex.Position, 0.0, 1.0), u_Transform), u_Camera.View), u_Camera.Proj);
    output.Color = vertex.Color;
    return output;
}


struct Fragment
{
    float4 Color : SV_TARGET0;
};

Fragment FSMain(Interpolators input)
{
    Fragment output;
    
    output.Color = input.Color;
    return output;
}
