//////////////////////// Athena SSR-Composite Shader ////////////////////////

// Refernces:
// https://github.com/JoshuaLim007/Unity-ScreenSpaceReflections-URP/blob/main/Shaders/ssr_shader.shader


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
layout(set = 1, binding = 7) uniform sampler2D u_SceneNormals;
layout(set = 1, binding = 8) uniform sampler2D u_SceneRoughnessMetalness;

layout(std140, set = 1, binding = 9) uniform u_SSRData
{
	float Intensity;
	uint MaxSteps;
	float MaxRoughness;
	float _Pad0;
	vec4 ProjInfo;
} u_SSR;

vec3 GetViewPos(vec2 uv)
{
    float viewDepth = texture(u_HiZBuffer, uv).r;
    vec2 viewUV = uv * u_SSR.ProjInfo.xy + u_SSR.ProjInfo.zw;
    viewUV *= viewDepth;

    return vec3(viewUV, -viewDepth);
}

float RGB2Lum(vec3 c)
{
    return dot(c, vec3(0.2126729, 0.7151522, 0.0721750));
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec2 uv = (pixelCoords + 0.5) / imageSize(u_SceneColorOutput);

	vec3 mainColor = imageLoad(u_SceneColorOutput, pixelCoords).rgb;

	vec3 ssrOutput = texture(u_SSROutput, uv).rgb;
	vec2 reflectUV = ssrOutput.xy;
	float maskVal = ssrOutput.z;

	if(maskVal == 0.0)
	{
		imageStore(u_SceneColorOutput, pixelCoords, vec4(mainColor, 1.0));
		return;
	}

	vec3 albedo = texture(u_SceneAlbedo, uv).rgb;
	vec2 roughnessMetalness = texture(u_SceneRoughnessMetalness, uv).rg;
	float roughness = roughnessMetalness.r;
	float metalness = roughnessMetalness.g;
	float smoothness = 1.0 - roughness;

	// Emissive mask
	//float lumin = clamp(RGB2Lum(mainColor) - 1, 0, 1);
    //float luminMask = 1 - lumin;
    //luminMask = pow(luminMask, 5);

	// Fresnel
	vec3 normal = texture(u_SceneNormals, uv).xyz * 2.0 - 1.0;
	vec3 viewPos = GetViewPos(uv);
	vec3 viewDir = normalize(viewPos);
	vec3 reflectionRay = reflect(viewDir, normal);
	vec3 halfWayDir = normalize(viewDir + reflectionRay);
	float normalDotLightDir = max(dot(normal, reflectionRay), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);

	vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

	float normRoughness = roughness / u_SSR.MaxRoughness;
    vec3 fresnel = FresnelSchlickRoughness(NdotV, F0, normRoughness); 

	// specular and diffuse
	vec3 Ks = fresnel;
	vec3 Kd = 1 - Ks;
	Kd *= 1 - metalness;

	// Blur
    const float blurPow = 4;
	float stepS = smoothstep(1.0 - u_SSR.MaxRoughness, 1, smoothness);
	float lod = mix(0.0, PRECONVOLUTION_MIP_LEVEL_COUNT - 1, 1 - pow(stepS, blurPow));
	vec3 reflectedColor = textureLod(u_HiColorBuffer, reflectUV, lod).rgb;

	vec3 lightColor = (albedo / PI * Kd + reflectedColor * Ks) * normalDotLightDir;

	float blend = RGB2Lum(fresnel) * maskVal;
	vec3 result = mix(mainColor, lightColor, blend);

	imageStore(u_SceneColorOutput, pixelCoords, vec4(result, 1.0));
}
