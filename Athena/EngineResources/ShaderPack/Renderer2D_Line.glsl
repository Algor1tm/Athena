//////////////////////// Athena Renderer2D Line shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout (location = 0) in vec4 a_Position;
layout (location = 1) in vec4 a_Color;

layout (location = 0) out vec4 v_Color;

void main()
{
	v_Color = a_Color;
	gl_Position = a_Position;
}

#version 460 core
#pragma stage : fragment

layout (location = 0) in vec4 v_Color;
layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = v_Color;
}
