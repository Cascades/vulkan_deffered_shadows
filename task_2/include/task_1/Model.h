#pragma once
#include <filesystem>
#include <glm/glm.hpp>

#include "task_1/Vertex.h"

class Model
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

public:

    float Ns;
    float Ni;
    float d;
    float Tr;
    glm::vec3 Tf;
    float illum;
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    glm::vec3 Ke;
    float specular = 0.1;
    float diffuse = 0.5;
    float ambient = 0.2;
	
	void loadModel(std::filesystem::path const & model_path);

	std::vector<Vertex> const& getVertices() const
	{
        return vertices;
	}

	std::vector<uint32_t> const& getIndices() const
	{
        return indices;
	}
};
