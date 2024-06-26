//////////////////////// Athena SSR Shader ////////////////////////

// Refernces:
// https://github.com/JoshuaLim007/Unity-ScreenSpaceReflections-URP/blob/main/Shaders/ssr_shader.shader
// https://sugulee.wordpress.com/2021/01/19/screen-space-reflections-implementation-and-optimization-part-2-hi-z-tracing-method/
// GPU Pro 5 - Advanced Rendering Techniques


#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, set = 1, binding = 2) uniform image2D u_Output;

layout(set = 1, binding = 3) uniform sampler2D u_HiZBuffer;
layout(set = 1, binding = 4) uniform sampler2D u_SceneNormals;
layout(set = 1, binding = 5) uniform sampler2D u_SceneRoughnessMetalness;

layout(std140, set = 1, binding = 6) uniform u_SSRData
{
	float Intensity;
	uint MaxSteps;
	float MaxRoughness;
    float ScreenEdgesFade;
    uint ConeTrace;
    uint BackwardRays;
    vec2 _Pad0;
} u_SSR;

#define HIZ_TRACE 1

#define HIZ_START_LEVEL 2           
#define HIZ_MAX_LEVEL HIZ_MIP_LEVEL_COUNT - 1
#define MAX_THICKNESS 0.015

float ScreenEdgeMask(vec2 clipPos) 
{
    float yDif = 1 - abs(clipPos.y);
    float xDif = 1 - abs(clipPos.x);

    if (yDif < 0 || xDif < 0)
        return 0;
    
    float t1 = smoothstep(0.0, 0.2, yDif);
    float t2 = smoothstep(0.0, 0.1, xDif);

    return clamp(t2 * t1, 0, 1);
}

float GetMaxThickness(float sampleDepth)
{
    return MAX_THICKNESS * (1 - sampleDepth);
}

void PrepareTracing(vec2 uv, float depth, vec3 samplePosVS, vec3 rayDirVS, out vec3 outSamplePosTS, out vec3 outRayDirTS, out float outMaxDistance)
{
    vec3 samplePosCS = vec3(uv * 2.0 - 1.0, depth);

    //vec3 rayEndVS = samplePosVS + rayDirVS * 1000.f;
    vec3 rayEndVS = samplePosVS + rayDirVS * -samplePosVS.z;
    rayEndVS /= rayEndVS.z > 0 ? -rayEndVS.z : 1;
    vec4 rayEndCS = u_Camera.Projection * vec4(rayEndVS, 1);
    rayEndCS /= rayEndCS.w;
    rayEndCS.z = 1.0 - rayEndCS.z;
    vec3 rayDirCS = normalize(rayEndCS.xyz - samplePosCS);

    outSamplePosTS = vec3(uv, depth);
    outRayDirTS = rayDirCS;
    outRayDirTS.xy *= vec2(0.5, 0.5);
 
    // Compute the maximum distance to trace before the ray goes outside of the visible area.
    outMaxDistance = 1000;

    if(outRayDirTS.x != 0)
        outMaxDistance = outRayDirTS.x > 0 ? (1 - outSamplePosTS.x) / outRayDirTS.x : -outSamplePosTS.x / outRayDirTS.x;
    if(outRayDirTS.y != 0)
        outMaxDistance = min(outMaxDistance, outRayDirTS.y > 0 ? (1 - outSamplePosTS.y) / outRayDirTS.y : -outSamplePosTS.y / outRayDirTS.y);
    if(outRayDirTS.z != 0)
        outMaxDistance = min(outMaxDistance, outRayDirTS.z > 0 ? (1 - outSamplePosTS.z) / outRayDirTS.z : -outSamplePosTS.z / outRayDirTS.z);
}

void LinearTrace(vec3 samplePosTS, vec3 rayDirTS, float maxTraceDistance, out bool outHit, out vec3 outIntersection)
{
    bool isBackwardRay = rayDirTS.z < 0;
    if(!bool(u_SSR.BackwardRays) && isBackwardRay)
        return;

    vec3 rayEndTS = samplePosTS + rayDirTS * maxTraceDistance;
    
    vec2 viewportSize = textureSize(u_HiZBuffer, 0);
    vec3 dp = rayEndTS.xyz - samplePosTS.xyz;
    ivec2 sampleScreenPos = ivec2(samplePosTS.xy * viewportSize);
    ivec2 endPosScreenPos = ivec2(rayEndTS.xy * viewportSize);
    ivec2 dp2 = endPosScreenPos - sampleScreenPos;
    const int maxDist = max(abs(dp2.x), abs(dp2.y));
    dp /= maxDist;
    
    vec4 rayPosTS = vec4(samplePosTS.xyz + dp, 0);
    vec4 rayStepTS = vec4(dp.xyz, 0);
	vec4 rayStartPos = rayPosTS;

    int hitIndex = -1;
    for(int i = 0; i < maxDist && i < u_SSR.MaxSteps; i++)
    {
	    float depth = texture(u_HiZBuffer, rayPosTS.xy).r;
	    float thickness = rayPosTS.z - depth;

	    if(thickness >= 0 && thickness < GetMaxThickness(depth))
	    {
	        hitIndex = i;
		    break;
	    }
		
        rayPosTS += rayStepTS;
    }

    outHit = hitIndex >= 0;
    outIntersection = vec3(rayStartPos + rayStepTS * hitIndex);
}

vec2 GetCellCount(int mipLevel)
{
    return textureSize(u_HiZBuffer, mipLevel);
}

vec2 GetCell(vec2 pos, vec2 cellCount)
{
    return floor(pos * cellCount);
}

vec3 IntersectDepthPlane(vec3 o, vec3 d, float t)
{
	return o + d * t;
}

vec3 IntersectCellBoundary(vec3 o, vec3 d, vec2 cell, vec2 cellCount, vec2 crossStep, vec2 crossOffset)
{
	vec2 index = cell + crossStep;
	vec2 boundary = index / cellCount;
	boundary += crossOffset;
	
	vec2 delta = (boundary - o.xy) / d.xy;
	float t = min(delta.x, delta.y);
	
	vec3 intersection = IntersectDepthPlane(o, d, t);

	return intersection;
}

float GetMinimumDepthPlane(vec2 p, int mipLevel)
{
    return textureLod(u_HiZBuffer, p, mipLevel).r;
}

bool CrossedCellBoundary(vec2 oldCellIndex, vec2 newCellIndex)
{
	return (oldCellIndex.x != newCellIndex.x) || (oldCellIndex.y != newCellIndex.y);
}

void HiZTrace(vec3 samplePosTS, vec3 rayDirTS, float maxTraceDistance, out bool outHit, out vec3 outIntersection)
{
    const int maxLevel = HIZ_MAX_LEVEL;
    const int startLevel = HIZ_START_LEVEL;
    const int stopLevel = 0;

    bool isBackwardRay = rayDirTS.z < 0;
    if(!bool(u_SSR.BackwardRays) && isBackwardRay)
        return;

    vec2 viewportSize = textureSize(u_HiZBuffer, 0);
    vec2 crossStep = vec2(rayDirTS.x >= 0 ? 1 : -1, rayDirTS.y >= 0 ? 1 : -1);
    vec2 crossOffset = crossStep / viewportSize / 128.0;
    crossStep.xy = clamp(crossStep.xy, 0, 1);

    vec3 ray = samplePosTS;
    float minZ = ray.z;
    float maxZ = ray.z + rayDirTS.z * maxTraceDistance;
    float deltaZ = maxZ - minZ;

    vec3 o = ray;
    vec3 d = rayDirTS * maxTraceDistance;

    vec2 startCellCount = GetCellCount(startLevel);
    vec2 rayCell = GetCell(ray.xy, startCellCount);
    ray = IntersectCellBoundary(o, d, rayCell, startCellCount, crossStep, crossOffset * 128);

    int level = startLevel;
    uint iter = 0;
    float rayDir = isBackwardRay ? -1 : 1;

    while(level >= stopLevel && ray.z * rayDir <= maxZ * rayDir && iter < u_SSR.MaxSteps)
    {
        vec2 cellCount = GetCellCount(level);
        vec2 oldCellIdx = GetCell(ray.xy, cellCount);

        float cellMinZ = GetMinimumDepthPlane((oldCellIdx + 0.5) / cellCount, level);
        vec3 tmpRay = ((cellMinZ > ray.z) && !isBackwardRay) ? IntersectDepthPlane(o, d, (cellMinZ - minZ) / deltaZ) : ray;

        vec2 newCellIdx = GetCell(tmpRay.xy, cellCount);

        float thickness = level == 0 ? (ray.z - cellMinZ) : 0;
        bool crossed = (isBackwardRay && (cellMinZ > ray.z)) || (thickness > GetMaxThickness(cellMinZ)) || CrossedCellBoundary(oldCellIdx, newCellIdx);
        ray = crossed ? IntersectCellBoundary(o, d, oldCellIdx, cellCount, crossStep, crossOffset) : tmpRay;
        level = crossed ? min(maxLevel, level + 1) : level - 1;
        
        ++iter;
    }

    outHit = level < stopLevel;
    outIntersection = ray;
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec2 uv = (pixelCoords + 0.5) / imageSize(u_Output);

    float depth = texture(u_HiZBuffer, uv).r;

	if(depth == 1.0)
	{
		imageStore(u_Output, pixelCoords, vec4(0.0));
		return;
	}

	vec2 roughnessMetalness = texture(u_SceneRoughnessMetalness, uv).rg;
    float roughness = roughnessMetalness.x;
    float metalness = roughnessMetalness.y;

	if(roughness >= u_SSR.MaxRoughness || metalness == 0.0)
	{
		imageStore(u_Output, pixelCoords, vec4(0.0));
		return;
	}

	vec3 normal = texture(u_SceneNormals, uv).xyz * 2.0 - 1.0;

    vec3 samplePosCS = vec3(uv * 2.0 - 1.0, depth);
    vec3 samplePosVS = ViewPositionFromDepth(uv, 1.0 - depth, u_Camera.InverseProjection);

    vec3 viewDir = normalize(samplePosVS);
	vec3 rayDirVS = reflect(viewDir, normal);

    vec3 samplePosTS;
    vec3 rayDirTS;
    float maxTraceDistance;
    PrepareTracing(uv, depth, samplePosVS, rayDirVS, samplePosTS, rayDirTS, maxTraceDistance);

    vec3 intersection;
    bool hit = false;

#if HIZ_TRACE
    HiZTrace(samplePosTS, rayDirTS, maxTraceDistance, hit, intersection);
#else
    LinearTrace(samplePosTS, rayDirTS, maxTraceDistance, hit, intersection);
#endif
    
    vec4 result = vec4(0.0);

    if(hit)
    {
        float depth = textureLod(u_HiZBuffer, intersection.xy, 0).r;

        if(depth < 1.0)   // exclude skybox
        {
            result.xy = intersection.xy;
            result.z = depth;
            result.w = dot(rayDirVS, viewDir);
        }
    }

	imageStore(u_Output, pixelCoords, result);
}
