#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inPos;
layout (input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput inNormal;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

void main() 
{
	outFragcolor = vec4(subpassLoad(inPos).rgb + subpassLoad(inNormal).rgb, 1.0);
}