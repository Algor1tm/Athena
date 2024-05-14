//////////////////////// Athena Text shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoords;

struct VertexInterpolators
{
	vec4 Color;
	vec2 TexCoords;
};

layout (location = 0) out VertexInterpolators Interpolators;

void main()
{
	Interpolators.Color = a_Color;
	Interpolators.TexCoords = a_TexCoords;

	gl_Position = a_Position;
}

#version 460 core
#pragma stage : fragment

struct VertexInterpolators
{
	vec4 Color;
	vec2 TexCoords;
};

layout (location = 0) in VertexInterpolators Interpolators;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform sampler2D u_AtlasTexture;

// https://github.com/Chlumsky/msdfgen/tree/master
float screenPxRange() 
{
	const float pxRange = 2.0; // set to distance field's pixel range
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_AtlasTexture, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(Interpolators.TexCoords);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
	vec4 bgColor = vec4(0.0);

	vec3 msd = texture(u_AtlasTexture, Interpolators.TexCoords).rgb;
    float sd = median(msd.r, msd.g, msd.b);

    float screenPxDistance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    vec4 color = mix(bgColor, Interpolators.Color, opacity);

	if (color.a == 0.0)
		discard;
	
	o_Color = color;
}
