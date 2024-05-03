//////////////////////// Athena HBAO-Deinterleave Shader ////////////////////////

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"
#include "Include/HBAO.glslh"

layout(local_size_x = 8, local_size_y = 4) in;

layout(set = 1, binding = 1) uniform sampler2D u_SceneDepth;
layout(r32f, set = 1, binding = 2) uniform writeonly image2DArray u_DepthLayers;


void main()
{
    uvec2 layerPixelCoords = gl_GlobalInvocationID.xy;
    uvec2 pixelCoords = layerPixelCoords * 4;

    /*--------------------------
          x     x    x     x  
             A          B  
          x     x    x     x
                           
          x     x    x     x
             C          D        
          x     x    x     x
    --------------------------*/
    
    vec2 uvA = (vec2(pixelCoords) + 0.5 + 0.5) * u_HBAO.InvResolution;

    vec4 A = textureGather(u_SceneDepth, uvA, 0);
    vec4 B = textureGatherOffset(u_SceneDepth, uvA, ivec2(2, 0), 0);
    vec4 C = textureGatherOffset(u_SceneDepth, uvA, ivec2(0, 2), 0);
    vec4 D = textureGatherOffset(u_SceneDepth, uvA, ivec2(2, 2), 0);

    for(int i = 0; i < 4; ++i)
    {
        A[i] = LinearizeDepth(A[i], u_Camera.FarClip, u_Camera.NearClip);
        B[i] = LinearizeDepth(B[i], u_Camera.FarClip, u_Camera.NearClip);
        C[i] = LinearizeDepth(C[i], u_Camera.FarClip, u_Camera.NearClip);
        D[i] = LinearizeDepth(D[i], u_Camera.FarClip, u_Camera.NearClip);
    }

    /*--------------------------
        Gather:
        x - (0, 1)
        y - (1, 1)
        z - (1, 0)
        w - (0, 0)
    --------------------------*/

    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 0),  vec4(A.w));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 1),  vec4(A.z));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 2),  vec4(B.w));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 3),  vec4(B.z));
                                                           
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 4),  vec4(A.x));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 5),  vec4(A.y));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 6),  vec4(B.x));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 7),  vec4(B.y));
                                                           
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 8),  vec4(C.w));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 9),  vec4(C.z));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 10), vec4(D.w));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 11), vec4(D.z));

    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 12), vec4(C.x));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 13), vec4(C.y));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 14), vec4(D.x));
    imageStore(u_DepthLayers, ivec3(layerPixelCoords, 15), vec4(D.y));
}
