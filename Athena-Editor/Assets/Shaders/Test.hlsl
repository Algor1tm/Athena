#type VERTEX_SHADER

cbuffer Camera : register(b1)
{
    row_major float4x4 u_ViewProjection;
}

struct VertexShaderInput
{
    float3 Position : a_Position;
    float4 Color : a_Color;
    float2 TexCoord : a_TexCoord;
};

struct VertexShaderOutput
{
    float4 Position : SV_POSITION;
    float4 Color : a_Color;    
    float2 TexCoord : a_TexCoord;
};


VertexShaderOutput VSMain(VertexShaderInput vsInput)
{
    VertexShaderOutput vsOutput;
    
    vsOutput.Color = vsInput.Color;
    vsOutput.Position = mul(float4(vsInput.Position, 1.f), u_ViewProjection);
    vsOutput.TexCoord = vsInput.TexCoord;
    return vsOutput;
}


#type FRAGMENT_SHADER

SamplerState Sampler : register(s0);
Texture2D Texture : register(t0);

struct PixelShaderInput
{
    float4 Position : SV_POSITION;
    float4 Color : a_Color;
    float2 TexCoord : a_TexCoord;
};


float4 PSMain(PixelShaderInput psInput) : SV_TARGET
{
    float4 color;
    
    color = psInput.Color;
    //color = float4(psInput.TexCoord, 0.f, 1.f);
    //color = Texture.Sample(Sampler, psInput.TexCoord);
    
    return color;
}
