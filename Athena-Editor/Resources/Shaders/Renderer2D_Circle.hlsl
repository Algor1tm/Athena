#type VERTEX_SHADER

cbuffer Camera : register(b0)
{
    row_major float4x4 u_ViewProjection;
}

struct VertexShaderInput
{
    float3 WorldPosition : a_WorldPosition;
    float3 LocalPosition : a_LocalPosition;
    float4 Color         : a_Color;
    float Thickness      : a_Thickness;
    float Fade           : a_Fade;
    int EntityID         : a_EntityID;
};

struct VertexShaderOutput
{
    float4 Position      : SV_POSITION;
    float3 LocalPosition : a_LocalPosition;
    float4 Color         : a_Color;
    float Thickness      : a_Thickness;
    float Fade           : a_Fade;
    nointerpolation int EntityID: a_EntityID;
};


VertexShaderOutput VSMain(VertexShaderInput Input)
{
    VertexShaderOutput Output;
    
    Output.Position = mul(float4(Input.WorldPosition, 1.f), u_ViewProjection);
    Output.LocalPosition = Input.LocalPosition;
    Output.Color = Input.Color;
    Output.Thickness = Input.Thickness;
    Output.Fade = Input.Fade;
    Output.EntityID = Input.EntityID;
    
    return Output;
}


#type FRAGMENT_SHADER

struct PixelShaderInput
{
    float4 Position      : SV_POSITION;
    float3 LocalPosition : a_LocalPosition;
    float4 Color         : a_Color;
    float Thickness      : a_Thickness;
    float Fade           : a_Fade;
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
    
    float distance = 1.0 - length(Input.LocalPosition.xy);
    float circle = smoothstep(0.0, Input.Fade, distance);
    circle *= smoothstep(Input.Thickness + Input.Fade, Input.Thickness, distance);

    if (circle == 0.0)
        discard;

    Output.Color = Input.Color;
    Output.Color.a *= circle;

    Output.EntityID = Input.EntityID;
    
    return Output;
}
