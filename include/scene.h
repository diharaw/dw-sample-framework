#pragma once

#include "entity.h"
#include <vector>
#include <string>
#include "packed_array.h"

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

		Entity* lookup(const std::string& name);
		void update();

		inline void add_entity(Entity& entity) 
		{ 
			ID id = m_entities.add();
			entity.id = id;
			m_entities.set(id, entity);
		}
		inline Entity& lookup(ID id) { return m_entities.lookup(id); }
		inline void destroy_entity(ID id) { m_entities.remove(id); }
		inline uint32_t entity_count() { return m_entities.size(); }
		inline Entity* entities() { return m_entities.array(); }
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
		PackedArray<Entity, 1024> m_entities;
	};
}