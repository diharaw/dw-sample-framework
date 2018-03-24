#pragma once

#include "entity.h"
#include <vector>
#include <string>

class RenderDevice;
struct TextureCube;

namespace dw
{
	class Renderer;

	class Scene
	{
	public:
		static Scene* load(const std::string& file, RenderDevice* device, Renderer* renderer);

		Scene();
		~Scene();

		inline void add_entity(Entity* entity) { m_entities.push_back(entity); }
		inline uint32_t entity_count() { return m_entities.size(); }
		inline Entity** entities() { return &m_entities[0]; }
		inline const char* name() { return m_name.c_str(); }
		inline TextureCube* env_map() { return m_env_map; }
		inline TextureCube* irradiance_map() { return m_irradiance_map; }
		inline TextureCube* prefiltered_map() { return m_prefiltered_map; }
		void save(std::string path);
		
	private:
		RenderDevice* m_device;
		TextureCube* m_env_map;
		TextureCube* m_irradiance_map;
		TextureCube* m_prefiltered_map;
		std::string			 m_name;
		std::vector<Entity*> m_entities;
	};
}