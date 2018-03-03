#pragma once

#include "Entity.h"
#include <vector>
#include <string>

class RenderDevice;

namespace dw
{
	class Scene
	{
	public:
		static Scene* Load(const std::string& file, RenderDevice* device);
		
		Scene();
		~Scene();

		inline void AddEntity(Entity* entity) { m_Entities.push_back(entity); }
		inline uint32_t EntityCount() { return m_Entities.size(); }
		inline Entity** Entities() { return &m_Entities[0]; }

	private:
		std::string			 m_Name;
		std::vector<Entity*> m_Entities;
	};
}