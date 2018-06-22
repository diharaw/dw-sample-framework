#pragma once

#include <unordered_map>
#include <string>
#include <glm.hpp>
#include <ogl.h>
#include <memory>

namespace dw
{
	class Material
	{
	public:
		// Material factory methods.
		static Material* load(const std::string& name, const std::string* textures);
		// Custom factory method for creating a material from provided data.
		static Material* load(const std::string& name, int num_textures, Texture2D** textures, glm::vec4 albedo = glm::vec4(1.0f), float roughness = 0.0f, float metalness = 0.0f);
		static bool is_loaded(const std::string& name);
		static void unload(Material*& mat);

		// Texture factory methods.
        static Texture2D* load_texture(const std::string& path, bool srgb = false);
		static void unload_texture(Texture2D*& tex);
		
		// Rendering related getters.
		inline Texture2D* texture(const uint32_t& index) { return m_textures[index];  }
		inline glm::vec4  albedo_value()				 { return m_albedo_val;	 }

	private:
		// Private constructor and destructor.
		Material();
		Material(const std::string& name, const std::string* textures);
		~Material();

	public:
		// Material cache.
		static std::unordered_map<std::string, Material*> m_cache;

		// Texture cache.
		static std::unordered_map<std::string, Texture2D*> m_texture_cache;

		// Albedo color.
		glm::vec4 m_albedo_val = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

		// Texture list. In the same order as the Assimp texture enums.
		Texture2D* m_textures[16];
	};
} // namespace dw
