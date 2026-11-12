//////////////////////// Athena Pre-Convolution Shader ////////////////////////

// References:
//		https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf

#version 460 core
#pragma stage : compute

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D u_SourceImage;
layout(r11f_g11f_b10f, set = 0, binding = 1) uniform writeonly image2D u_OutputImage;


#define MODE_COPY 1
#define MODE_BLUR_X 2
#define MODE_BLUR_Y 3
#define MODE_DOWNSAMPLE 4

layout(push_constant) uniform u_Uniforms
{
	uint u_Mode;
};

vec3 GaussianBlur7(vec2 uv, vec2 texelSize, ivec2 direction)
{
	vec2 dir = texelSize * direction;

	vec3 result = texture(u_SourceImage, uv + ivec2(-3, -3) * dir).rgb * 0.001;
	result += texture(u_SourceImage, uv + ivec2(-2, -2) * dir).rgb * 0.028;
	result += texture(u_SourceImage, uv + ivec2(-1, -1) * dir).rgb * 0.233;
	result += texture(u_SourceImage, uv + ivec2(0, 0) * dir).rgb * 0.474;
	result += texture(u_SourceImage, uv + ivec2(1, 1) * dir).rgb * 0.233;
	result += texture(u_SourceImage, uv + ivec2(2, 2) * dir).rgb * 0.028;
	result += texture(u_SourceImage, uv + ivec2(3, 3) * dir).rgb * 0.001;

	return result;
}

vec3 GaussianBlur9(vec2 uv, vec2 texelSize, ivec2 direction)
{
	float weights[] = float[](0.2270270270, 0.3162162162, 0.0702702703);
	float offsets[] = float[](1.3846153846, 3.2307692308);

	vec2 dir = texelSize * direction;

	vec3 result = texture(u_SourceImage, uv).rgb * weights[0];
	result += texture(u_SourceImage, uv + offsets[0] * dir).rgb * weights[1];
	result += texture(u_SourceImage, uv - offsets[0] * dir).rgb * weights[1];
	result += texture(u_SourceImage, uv + offsets[1] * dir).rgb * weights[2];
	result += texture(u_SourceImage, uv - offsets[1] * dir).rgb * weights[2];

	return result;
}

vec3 DualFilter(vec2 uv, vec2 texelSize)
{
	vec2 halfPixel = 0.5 * texelSize;

	vec3 result = texture(u_SourceImage, uv).rgb * 4.0;
	result += texture(u_SourceImage, uv - halfPixel.xy).rgb;
	result += texture(u_SourceImage, uv + halfPixel.xy).rgb;
	result += texture(u_SourceImage, uv + vec2(halfPixel.x, -halfPixel.y)).rgb;
	result += texture(u_SourceImage, uv - vec2(halfPixel.x, -halfPixel.y)).rgb;

	return result / 8.0;
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID);
	vec2 texelSize = 1.0 / imageSize(u_OutputImage);
	vec2 uv = (pixelCoords + 0.5) * texelSize;

	vec3 outputColor;

	if(u_Mode == MODE_COPY)
	{
		outputColor = texture(u_SourceImage, uv).rgb;
	}
	else if(u_Mode == MODE_BLUR_X)
	{
		outputColor = GaussianBlur7(uv, texelSize, ivec2(1, 0));	
	}
	else if(u_Mode == MODE_BLUR_Y)
	{
		outputColor = GaussianBlur7(uv, texelSize, ivec2(0, 1));	
	}
	else if(u_Mode == MODE_DOWNSAMPLE)
	{
		outputColor = texture(u_SourceImage, uv).rgb;
		//outputColor = DualFilter(uv, texelSize);
	}

	imageStore(u_OutputImage, pixelCoords, vec4(outputColor, 1.0));
}
