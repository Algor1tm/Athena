//////////////////////// Athena common buffers ////////////////////////

struct CameraData
{
    float4x4 View;
    float4x4 Projection;
    float3 Position;
};

cbuffer u_CameraData : register(b0, space1)
{
    CameraData u_Camera;
}

struct DirectionalLight
{
    float4 Color;
    float3 Direction;
    float Intensity;
};

struct PointLight
{
    float4 Color;
    float3 Position;
    float Intensity;
    float Radius;
    float FallOff;
};

tbuffer u_LightData : register(b1, space1)
{
    DirectionalLight g_DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    int g_DirectionalLightCount;

    PointLight g_PointLights[MAX_POINT_LIGHT_COUNT];
    int g_PointLightCount;
};
