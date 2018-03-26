#pragma once

#include "Mesh.h"
#include <string>

struct ShaderProgram;

namespace dw
{
	struct Entity
	{
		std::string m_name;
		glm::vec3 m_position;
		glm::vec3 m_rotation;
		glm::vec3 m_scale;
		glm::mat4 m_transform;
		ShaderProgram* m_program;
		Material* m_override_mat;
		Mesh* m_mesh;
	};
}