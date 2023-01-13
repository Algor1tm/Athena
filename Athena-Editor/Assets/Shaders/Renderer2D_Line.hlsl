#type VERTEX_SHADER

cbuffer Camera : register(b0)
{
    row_major float4x4 u_ViewProjection;
}

struct VertexShaderInput
{
    float3 Position : a_Position;
    float4 Color    : a_Color;
    int EntityID    : a_EntityID;
};

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : a_Color;
    nointerpolation int EntityID : a_EntityID;
};


VertexShaderOutput VSMain(VertexShaderInput Input)
{
    VertexShaderOutput Output;
    
    Output.Position = mul(float4(Input.Position, 1.f), u_ViewProjection);
    Output.Color = Input.Color;
    Output.EntityID = Input.EntityID;
    
    return Output;
}


#type FRAGMENT_SHADER

struct PixelShaderInput
{
    float4 Position : SV_POSITION;
    float4 Color    : a_Color;
    nointerpolation int EntityID : a_EntityID;
};


struct PixelShaderOutput
{
    float4 Color : SV_TARGET0;
    int EntityID : SV_TARGET1;
};

PixelShaderOutput PSMain(PixelShaderInput Input)
{
    PixelShaderOutput Output;
    
    Output.Color = Input.Color;
    Output.EntityID = Input.EntityID;
    
    return Output;
}
