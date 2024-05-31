//////////////////////// Athena SSR Shader ////////////////////////

// Refernces:
// https://github.com/JoshuaLim007/Unity-ScreenSpaceReflections-URP/blob/main/Shaders/ssr_shader.shader


#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba8, set = 1, binding = 2) uniform image2D u_Output;

layout(set = 1, binding = 3) uniform sampler2D u_HiZBuffer;
layout(set = 1, binding = 4) uniform sampler2D u_SceneNormals;
layout(set = 1, binding = 5) uniform sampler2D u_SceneRoughnessMetalness;

layout(std140, set = 1, binding = 6) uniform u_SSRData
{
	float Intensity;
	uint MaxSteps;
	float MaxRoughness;
	float _Pad0;
	vec4 ProjInfo;
} u_SSR;


const float stepStrideLength = 0.1;
const int binaryStepCount = 16;


bool IsSkybox(float eyeDepth)
{
	return eyeDepth == -u_Camera.FarClip;
}

bool IsOutsideScreen(vec2 uv)
{
	return uv.x >= 1 || uv.x < 0 || uv.y >= 1 || uv.y < 0;
}

vec3 GetViewPos(vec2 uv)
{
    float viewDepth = texture(u_HiZBuffer, uv).r;
    vec2 viewUV = uv * u_SSR.ProjInfo.xy + u_SSR.ProjInfo.zw;
    viewUV *= viewDepth;

    return vec3(viewUV, -viewDepth);
}

vec2 GetUV(vec3 viewPos)
{
	vec4 sampleUV = u_Camera.Projection * vec4(viewPos, 1);
	sampleUV /= sampleUV.w;
	sampleUV.xy = sampleUV.xy * 0.5 + 0.5;

	return sampleUV.xy;
}

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

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec2 uv = (pixelCoords + 0.5) / imageSize(u_Output);

	vec3 viewPos = GetViewPos(uv);

	if(IsSkybox(viewPos.z))
	{
		imageStore(u_Output, pixelCoords, vec4(0.0));
		return;
	}

	float roughness = texture(u_SceneRoughnessMetalness, uv).r;

	if(roughness >= u_SSR.MaxRoughness)
	{
		imageStore(u_Output, pixelCoords, vec4(0.0));
		return;
	}

	vec3 normal = texture(u_SceneNormals, uv).xyz * 2.0 - 1.0;

	vec3 viewDir = normalize(viewPos);
	vec3 reflectionRay = reflect(viewDir, normal);

	float viewReflectDot = clamp(dot(viewDir, reflectionRay), 0, 1);
	float cameraViewReflectDot = clamp(dot(vec3(0, 0, -1), reflectionRay), 0, 1);
	float oneMinusViewReflectDot = sqrt(1 - viewReflectDot);
	float stride = stepStrideLength / oneMinusViewReflectDot;
	float thickness = stride * 2;

	float maxRayLength = u_SSR.MaxSteps * stride;
    float maxDist = mix(min(-viewPos.z, maxRayLength), maxRayLength, cameraViewReflectDot);
    uint numSteps = uint(max(maxDist / stride, 0));

	int hit = 0;
	float maskOut = 1;
	vec3 currentPosition = viewPos;
	vec2 currentUV = uv;

	vec3 rayStep = reflectionRay * stride;
	float depthDelta = 0;


	for(int i = 0; i < numSteps; ++i)
	{
		currentPosition += rayStep;
		vec2 sampleUV = GetUV(currentPosition);

		if(IsOutsideScreen(sampleUV))
			break;

		float sampleDepth = -texture(u_HiZBuffer, sampleUV).r;

		if(abs(viewPos.z - sampleDepth) > 0 && !IsSkybox(sampleDepth))
		{
			depthDelta = sampleDepth - currentPosition.z;

			if(depthDelta > 0 && depthDelta < thickness)
			{
				currentUV = sampleUV;
				hit = 1;
				break;
			}
		}
	}

	int binarySearchSteps = binaryStepCount * hit;

	for(int i = 0; i < binarySearchSteps; ++i)
	{
		rayStep *= .5f;

		if (depthDelta > 0) 
			currentPosition -= rayStep;
		else if (depthDelta < 0) 
			currentPosition += rayStep;
		else 
			break;
		
		currentUV = GetUV(currentPosition);

		float sampleDepth = -texture(u_HiZBuffer, currentUV.xy).r;
		depthDelta = sampleDepth - currentPosition.z;
		float minv = 1 / max((oneMinusViewReflectDot * float(i)), 0.001);

		if (abs(depthDelta) > minv)
		{
			hit = 0;
			break;
		}
	}

	// back face
	if(hit == 1)
	{
		vec3 currentNormal = texture(u_SceneNormals, currentUV).rgb * 2.0 - 1.0;
		float backFaceDot = dot(currentNormal, reflectionRay);
		if(backFaceDot > 0)
			hit = 0;
	}

	if(hit == 1)
	{
		//maskOut = ScreenEdgeMask(currentUV);
		maskOut = 1.0;
		vec3 deltaDir = viewPos - currentPosition;
		float progress = dot(deltaDir, deltaDir) / (maxDist * maxDist);
		progress = smoothstep(0.0, 0.5, 1 - progress);
		maskOut *= progress;
	}
	else
	{
		maskOut = 0.0;
		currentUV = vec2(0.0);
	}

	imageStore(u_Output, pixelCoords, vec4(currentUV, maskOut, 1.0));
}
