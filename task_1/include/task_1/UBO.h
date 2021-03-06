#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
	glm::mat4 light;
	glm::vec4 Ka;
	glm::vec4 Kd;
	glm::vec4 Ks;
	glm::vec4 Ke;
	glm::float32 Ns;
	glm::float32 model_stage_on;
	glm::float32 texture_stage_on;
	glm::float32 lighting_stage_on;
	glm::float32 specular;
	glm::float32 diffuse;
	glm::float32 ambient;
	glm::int32 display_mode;
	glm::vec2 win_dim;
};