#pragma once

#include "Mesh.h"
#include <string>

namespace dw
{
	class Entity
	{
	public:
		Entity(std::string name, Mesh* mesh, glm::vec3 pos);
		~Entity();

		inline glm::vec3* Position()  { return &m_Position;	 }
		inline glm::mat4* Transform() { return &m_Transform; }

	private:
		std::string m_Name;
		glm::vec3 m_Position = glm::vec3(0);
		glm::mat4 m_Transform = glm::mat4(1);
		Mesh* m_Mesh = nullptr;
	};
}