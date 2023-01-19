#type VERTEX_SHADER

cbuffer Camera : register(b0)
{
    row_major float4x4 u_ViewProjection;
}

struct VertexShaderInput
{
    float3 Position    : a_Position;
    float4 Color       : a_Color;
    float2 TexCoord    : a_TexCoord;
    float TilingFactor : a_TilingFactor;
    float TexIndex     : a_TexIndex;
    int EntityID       : a_EntityID;
};

struct VertexShaderOutput
{
    float4 Position    : SV_POSITION;
    float4 Color       : a_Color;
    float2 TexCoord    : a_TexCoord;
    float TilingFactor : a_TilingFactor;
    nointerpolation float TexIndex : a_TexIndex;
    nointerpolation int EntityID   : a_EntityID;
};


VertexShaderOutput VSMain(VertexShaderInput Input)
{
    VertexShaderOutput Output;
    
    Output.Position = mul(float4(Input.Position, 1.f), u_ViewProjection);
    Output.Color = Input.Color;
    Output.TexCoord = Input.TexCoord;
    Output.TilingFactor = Input.TilingFactor;
    Output.TexIndex = Input.TexIndex;
    Output.EntityID = Input.EntityID;
    
    return Output;
}


#type FRAGMENT_SHADER

struct PixelShaderInput
{
    float4 Position    : SV_POSITION;
    float4 Color       : a_Color;
    float2 TexCoord    : a_TexCoord;
    float TilingFactor : a_TilingFactor;
    nointerpolation float TexIndex : a_TexIndex;
    nointerpolation int EntityID   : a_EntityID;
};

SamplerState Sampler : register(s0);
Texture2D Texture[32] : register(t0);

struct PixelShaderOutput
{
    float4 Color : SV_TARGET0;
    int EntityID : SV_TARGET1;
};

PixelShaderOutput PSMain(PixelShaderInput Input)
{
    PixelShaderOutput Output;
    
    float4 TexColor = Input.Color;

    switch (int(Input.TexIndex))
    {
        case 0:  TexColor *= Texture[ 0].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 1:  TexColor *= Texture[ 1].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 2:  TexColor *= Texture[ 2].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 3:  TexColor *= Texture[ 3].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 4:  TexColor *= Texture[ 4].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 5:  TexColor *= Texture[ 5].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 6:  TexColor *= Texture[ 6].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 7:  TexColor *= Texture[ 7].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 8:  TexColor *= Texture[ 8].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 9:  TexColor *= Texture[ 9].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 10: TexColor *= Texture[10].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 11: TexColor *= Texture[11].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 12: TexColor *= Texture[12].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 13: TexColor *= Texture[13].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 14: TexColor *= Texture[14].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 15: TexColor *= Texture[15].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 16: TexColor *= Texture[16].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 17: TexColor *= Texture[17].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 18: TexColor *= Texture[18].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 19: TexColor *= Texture[19].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 20: TexColor *= Texture[20].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 21: TexColor *= Texture[21].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 22: TexColor *= Texture[22].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 23: TexColor *= Texture[23].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 24: TexColor *= Texture[24].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 25: TexColor *= Texture[25].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 26: TexColor *= Texture[26].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 27: TexColor *= Texture[27].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 28: TexColor *= Texture[28].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 29: TexColor *= Texture[29].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 30: TexColor *= Texture[30].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
        case 31: TexColor *= Texture[31].Sample(Sampler, Input.TexCoord * Input.TilingFactor); break;
    }

    if (TexColor.a == 0.0)
        discard;

    Output.Color = TexColor;
    Output.EntityID = Input.EntityID;
    
    return Output;
}
