//////////////////////// Athena Pre-Convolution Shader ////////////////////////

// References:
//		https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf

#version 460 core
#pragma stage : compute

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D u_SourceImage;
layout(r11f_g11f_b10f, set = 0, binding = 1) uniform writeonly image2D u_OutputImage;


layout(push_constant) uniform u_Uniforms
{
	uint u_FirstPass;
};

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

	if(bool(u_FirstPass))
	{
		outputColor = texture(u_SourceImage, uv).rgb;
	}
	else
	{
		outputColor = DualFilter(uv, texelSize);
	}

	imageStore(u_OutputImage, pixelCoords, vec4(outputColor, 1.0));
}
