//////////////////////// Athena Light Culling Shader ////////////////////////

// References:
//   https://github.com/bcrusco/Forward-Plus-Renderer/tree/master


#version 460 core
#pragma stage : compute

#include "Include/Lighting.glslh"
#include "Include/Common.glslh"

layout(local_size_x = LIGHT_TILE_SIZE, local_size_y = LIGHT_TILE_SIZE, local_size_z = 1) in;

layout(set = 1, binding = 5) uniform sampler2D u_SceneDepth;

shared uint s_MinDepthInt;
shared uint s_MaxDepthInt;
shared vec4 s_FrustumPlanes[6];
shared uint s_VisibleLightCount;
shared int s_VisibleLightIndices[MAX_POINT_LIGHT_COUNT_PER_TILE];


void main() 
{
	vec2 uv = (gl_GlobalInvocationID.xy + 0.5) * u_Renderer.InverseViewportSize;
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 tileCount = u_Renderer.ViewportTilesCount.xy;
	uint tileIndex = tileID.y * tileCount.x + tileID.x;

	// Initialize shared values
	if (gl_LocalInvocationIndex == 0) 
	{
		s_MinDepthInt = 0xFFFFFFFF;
		s_MaxDepthInt = 0;
		s_VisibleLightCount = 0;
	}

	barrier();

/*---------------------------------------------------------------------------------
	Step 1: Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
-----------------------------------------------------------------------------------*/
	float maxDepth, minDepth;
	float depth = texture(u_SceneDepth, uv).r;
	depth = LinearizeDepth(depth, u_Camera.FarClip, u_Camera.NearClip);

	uint depthInt = floatBitsToUint(depth);
	atomicMin(s_MinDepthInt, depthInt);
	atomicMax(s_MaxDepthInt, depthInt);

	barrier();

/*---------------------------------------------------------------------------------
	Step 2: One thread should calculate the frustum planes to be used for this tile
-----------------------------------------------------------------------------------*/
	if (gl_LocalInvocationIndex == 0) 
	{
		minDepth = uintBitsToFloat(s_MinDepthInt);
		maxDepth = uintBitsToFloat(s_MaxDepthInt);

		vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileCount);
		vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileCount);

		s_FrustumPlanes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negativeStep.x); // Left
		s_FrustumPlanes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positiveStep.x); // Right
		s_FrustumPlanes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negativeStep.y); // Top
		s_FrustumPlanes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positiveStep.y); // Bottom
		s_FrustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth); // Near
		s_FrustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth); // Far

		for (uint i = 0; i < 4; i++) 
		{
			s_FrustumPlanes[i] *= u_Camera.ViewProjection;
			s_FrustumPlanes[i] /= length(s_FrustumPlanes[i].xyz);
		}

		s_FrustumPlanes[4] *= u_Camera.View;
		s_FrustumPlanes[4] /= length(s_FrustumPlanes[4].xyz);
		s_FrustumPlanes[5] *= u_Camera.View;
		s_FrustumPlanes[5] /= length(s_FrustumPlanes[5].xyz);
	}

	barrier();

/*---------------------------------------------------------------------------------
	Step 3: Cull lights
-----------------------------------------------------------------------------------*/

	// Parallelize the threads against the lights now.
	// Can handle 256 simultaniously. Anymore lights than that and additional passes are performed
	uint lightCount = g_PointLightCount;

	uint threadCount = LIGHT_TILE_SIZE * LIGHT_TILE_SIZE;
	uint passCount = (lightCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++) 
	{
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= lightCount) 
			break;
		
		PointLight light = g_PointLights[lightIndex];

		vec4 position = vec4(light.Position.xyz, 1.0);
		float radius = light.Radius;

		// Sphere frustum intersection test
		float dist = 0.0;
		for (uint j = 0; j < 6; j++) 
		{
			dist = dot(position, s_FrustumPlanes[j]) + radius;

			if (dist <= 0.0) 
				break;
		}

		if (dist > 0.0)
		{
			uint offset = atomicAdd(s_VisibleLightCount, 1);

			if(offset >= MAX_POINT_LIGHT_COUNT_PER_TILE - 1)
				break;

			s_VisibleLightIndices[offset] = int(lightIndex);
		}
	}

	barrier();


/*---------------------------------------------------------------------------------
	Step 4: One thread should fill the global light buffer
-----------------------------------------------------------------------------------*/
	if (gl_LocalInvocationIndex == 0) 
	{
		s_VisibleLightCount = min(s_VisibleLightCount, MAX_POINT_LIGHT_COUNT_PER_TILE);
		u_VisibleLights[tileIndex].LightCount = s_VisibleLightCount;

		for (uint i = 0; i < s_VisibleLightCount; i++) 
			u_VisibleLights[tileIndex].LightIndices[i] = s_VisibleLightIndices[i];
	}
}
