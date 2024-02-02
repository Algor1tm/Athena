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

cbuffer u_SceneData : register(b1, space1)
{
    float u_Exposure;
    float u_Gamma;
    float u_EnvironmentIntensity;
    float u_EnivornmentLOD;
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

tbuffer u_LightData : register(b2, space1)
{
    DirectionalLight g_DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    int g_DirectionalLightCount;

    PointLight g_PointLights[MAX_POINT_LIGHT_COUNT];
    int g_PointLightCount;
};
