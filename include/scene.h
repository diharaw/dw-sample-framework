#pragma once

#include "entity.h"
#include <vector>
#include <string>

class RenderDevice;
class Texture2D;

namespace dw
{
	class Scene
	{
	public:
		static Scene* load(const std::string& file, RenderDevice* device);
		
		Scene();
		~Scene();

		inline void add_entity(Entity* entity) { m_entities.push_back(entity); }
		inline uint32_t entity_count() { return m_entities.size(); }
		inline Entity** entities() { return &m_entities[0]; }
		inline const char* name() { return m_name.c_str(); }
		inline Texture2D* env_map() { return m_env_map; }

		void load_env_map(std::string file, RenderDevice* device);

	private:
		Texture2D* m_env_map;
		std::string			 m_name;
		std::vector<Entity*> m_entities;
	};
}