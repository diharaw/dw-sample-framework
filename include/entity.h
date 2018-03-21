#pragma once

#include "Mesh.h"
#include <string>

namespace dw
{
	struct Entity
	{
		std::string m_name;
		glm::vec3 m_position;
		glm::vec3 m_rotation;
		glm::vec3 m_scale;
		glm::mat4 m_transform;
		uint64_t m_material_key;
		Mesh* m_mesh;
	};
}