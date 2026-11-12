//////////////////////// Athena SSR-Composite Shader ////////////////////////

// Refernces:
// https://roar11.com/2015/07/screen-space-glossy-reflections/
// GPU Pro 5 - Advanced Rendering Techniques

#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"
#include "Include/PBR.glslh"

layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, set = 1, binding = 2) uniform image2D u_SceneColorOutput;

layout(set = 1, binding = 3) uniform sampler2D u_HiZBuffer;
layout(set = 1, binding = 4) uniform sampler2D u_HiColorBuffer;
layout(set = 1, binding = 5) uniform sampler2D u_SSROutput;
layout(set = 1, binding = 6) uniform sampler2D u_SceneAlbedo;
layout(set = 1, binding = 7) uniform sampler2D u_SceneNormalsEmission;
layout(set = 1, binding = 8) uniform sampler2D u_SceneRoughnessMetalness;

layout(std140, set = 1, binding = 9) uniform u_SSRData
{
	float Intensity;
	uint MaxSteps;
	float MaxRoughness;
    float ScreenEdgesFade;
    uint ConeTrace;
    uint BackwardRays;
    vec2 _Pad0;
} u_SSR;

#define MAX_SPECULAR_POWER 2048


vec3 GetViewPos(vec2 uv, float depth)
{
	float linearDepth = LinearizeDepth(depth, u_Camera.NearClip, u_Camera.FarClip);
    vec2 viewUV = uv * u_Camera.ProjInfo.xy + u_Camera.ProjInfo.zw;
    viewUV *= linearDepth;

    return vec3(viewUV, -linearDepth);
}

float RoghnessToConeAngle(float roughness)
{
	float specularPower = 2 / max(pow(roughness, 2), 0.0001) - 2;

	if(specularPower >= MAX_SPECULAR_POWER)
		return 0;

	const float xi = 0.244;
    float exponent = 1.0 / (specularPower + 1.0);

	return cos(pow(xi, exponent));
}

float IsoscelesTriangleOpposite(float adjacentLength, float coneTheta)
{
    return 2.0 * tan(coneTheta) * adjacentLength;
}

float IsoscelesTriangleInRadius(float a, float h)
{
    float a2 = a * a;
    float fh2 = 4.0 * h * h;
    return (a * (sqrt(a2 + fh2) - a)) / (4.0 * h);
}

vec4 ConeSampleWeightedColor(vec2 samplePos, float mipChannel, float gloss)
{
	vec3 sampleColor = textureLod(u_HiColorBuffer, samplePos, mipChannel).rgb;
    return vec4(sampleColor * gloss, gloss);
}

float IsoscelesTriangleNextAdjacent(float adjacentLength, float incircleRadius)
{
    return adjacentLength - (incircleRadius * 2.0);
}

// TODO: looks bad
vec3 ConeTrace(vec2 uv, vec2 reflectUV, float roughness, out float remainingAlpha)
{
    vec2 viewportSize = textureSize(u_HiZBuffer, 0);
    float coneTheta = RoghnessToConeAngle(roughness) * 0.5;

    vec2 rayStart = uv;
    vec2 rayEnd = reflectUV.xy;

    vec2 deltaP = rayEnd - rayStart;
    float adjacentLength = length(deltaP);
    vec2 adjacentUnit = normalize(deltaP);

    vec4 reflectedColor = vec4(0.0);
    remainingAlpha = 1.0;
	float gloss = 1 - roughness;
    float glossMult = gloss;

    float allMipLevels = textureQueryLevels(u_HiColorBuffer) - 1.0;
    float convolutedMipLevels = PRECONVOLUTION_MIP_LEVEL_COUNT - 1.0;

    for(int i = 0; i < 14; ++i)
    {
        float oppositeLength = IsoscelesTriangleOpposite(adjacentLength, coneTheta);
        float incircleSize = IsoscelesTriangleInRadius(oppositeLength, adjacentLength);

        // get the sample position in screen space
        vec2 samplePos = rayStart + adjacentUnit * (adjacentLength - incircleSize);

        float circleMipLevel = clamp(log2(incircleSize * max(viewportSize.x, viewportSize.y)), 0.0, allMipLevels);
        float mipLevel = mix(0.0, convolutedMipLevels, circleMipLevel / allMipLevels);

        /*
         * Read color and accumulate it using trilinear filtering and weight it.
         * Uses pre-convolved image (color buffer) and glossiness to weigh color contributions.
         * Visibility is accumulated in the alpha channel. Break if visibility is 100% or greater (>= 1.0f).
         */
        vec4 newColor = ConeSampleWeightedColor(samplePos, mipLevel, glossMult);

        remainingAlpha -= newColor.a;

        if(remainingAlpha < 0.0)
            newColor.rgb *= (1.0 - abs(remainingAlpha));

        reflectedColor += newColor;

        if(reflectedColor.a >= 1.0)
            break;

        adjacentLength = IsoscelesTriangleNextAdjacent(adjacentLength, incircleSize);
        glossMult *= gloss;
    }

    return reflectedColor.rgb;
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec2 uv = (pixelCoords + 0.5) / imageSize(u_SceneColorOutput);

	vec4 ssrOutput = texture(u_SSROutput, uv).rgba;

	if(ssrOutput == vec4(0.0))
		return;

    vec4 normalEmission = texture(u_SceneNormalsEmission, uv).xyzw;
    float emission = normalEmission.w;
	vec3 normal = normalEmission.xyz * 2.0 - 1.0;

    if(emission > 0)
        return;

    vec2 reflectUV = ssrOutput.xy;
    float rayHitDepth = ssrOutput.z;
    float rayDotView = ssrOutput.w;
    bool isBackwardRay = rayDotView < 0;

	vec3 mainColor = imageLoad(u_SceneColorOutput, pixelCoords).rgb;

	vec3 albedo = texture(u_SceneAlbedo, uv).rgb;
	vec2 roughnessMetalness = texture(u_SceneRoughnessMetalness, uv).rg;
	float roughness = roughnessMetalness.r;
	float metalness = roughnessMetalness.g;

	// Fresnel
    float depth = texture(u_HiZBuffer, uv).r;
	vec3 viewPos = GetViewPos(uv, depth);
	vec3 viewDir = normalize(viewPos);
	vec3 rayDir = reflect(viewDir, normal);
	vec3 halfWayDir = normalize(viewDir + rayDir);
    float HdotV = max(dot(halfWayDir, viewDir), 0.0);

	vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);
    vec3 fresnel = FresnelSchlickRoughness(HdotV, F0, roughness); 

    float remainingAlpha = 0.0;
    vec3 reflectedColor;

    if(bool(u_SSR.ConeTrace) && !isBackwardRay && roughness != 0.0)
    {
        reflectedColor = ConeTrace(uv, reflectUV, roughness, remainingAlpha);
    }
    else
    {
        float lod = mix(0.0, PRECONVOLUTION_MIP_LEVEL_COUNT - 1, sqrt(roughness));
	    reflectedColor = textureLod(u_HiColorBuffer, reflectUV, lod).rgb;
    }

    vec2 clipPos = reflectUV * 2.0 - 1.0;
    float yDif = 1 - abs(clipPos.y);
    float xDif = 1 - abs(clipPos.x);
    float t1 = smoothstep(0.0, 2.0 * u_SSR.ScreenEdgesFade, yDif);
    float t2 = smoothstep(0.0, u_SSR.ScreenEdgesFade, xDif);
    float fadeOnBorder = clamp(t2 * t1, 0, 1);

    float maxRoughnessFade = 1.0 - clamp(0.1 - (u_SSR.MaxRoughness - roughness), 0.0, 0.1) * 10;
    float fadeOnRoughness = clamp(mix(0.0, 1.0, (1 - roughness) * 4.0), 0, 1) * metalness * maxRoughnessFade;

    float totalFade = u_SSR.Intensity * fadeOnBorder * fadeOnRoughness * (1.0 - clamp(remainingAlpha, 0, 1));
    vec3 result = mix(mainColor, reflectedColor.rgb * fresnel, totalFade);

	imageStore(u_SceneColorOutput, pixelCoords, vec4(result, 1.0));
}
