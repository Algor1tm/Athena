
struct CameraData
{
    float4x4 View;
    float4x4 Proj;
};

cbuffer CameraUBO : register(b0, space0)
{
    CameraData u_Camera;
}
