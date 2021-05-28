#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 light;
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	vec4 Ke;
    float Ns;
	float model_stage_on;
	float texture_stage_on;
	float lighting_stage_on;
    float specular;
	float diffuse;
	float ambient;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float texture_on;
layout(location = 4) out float specularity;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    outNormal = inNormal;

    specularity = ubo.specular;

    fragColor = vec3(ubo.diffuse,ubo.diffuse,ubo.diffuse);

    fragTexCoord = inTexCoord;
    texture_on = int(ubo.texture_stage_on);
}