#pragma VERTEX_STAGE

struct VertexShaderInput
{
    float2 Position;
    float4 Color;
};

struct FragmentShaderInput
{
    float4 Position : SV_POSITION;
    float4 Color;
};

cbuffer Camera : register(b0, space0)
{
    float4x4 u_CameraView;
};

FragmentShaderInput VSMain(VertexShaderInput Input)
{
    FragmentShaderInput Output;
    
    Output.Position = mul(float4(Input.Position, 0.0, 1.0), u_CameraView);
    Output.Color = Input.Color;
    return Output;
}

#pragma FRAGMENT_STAGE

struct FragmentShaderInput
{
    float4 Position : SV_POSITION;
    float4 Color;
};

struct FragmentShaderOuput
{
    float4 Color : SV_TARGET0;
};

FragmentShaderOuput FSMain(FragmentShaderInput Input)
{
    FragmentShaderOuput Output;
    
    Output.Color = Input.Color;
    return Output;
}
