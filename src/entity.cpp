#include "Entity.h"

namespace dw
{
	Entity::Entity(std::string name, Mesh * mesh, glm::vec3 pos) : m_Name(name)
	{
	}

	Entity::~Entity()
	{
	}
}