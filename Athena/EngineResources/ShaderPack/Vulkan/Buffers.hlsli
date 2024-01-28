
struct CameraData
{
    float4x4 View;
    float4x4 Proj;
};

cbuffer u_CameraData : register(b0, space1)
{
    CameraData u_Camera;
}
