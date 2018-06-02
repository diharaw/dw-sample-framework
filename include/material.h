#pragma once

#include <unordered_map>
#include <string>
#include <glm.hpp>

// Forward declarations.
struct Texture2D;
class  RenderDevice;

namespace dw
{
	class Material
	{
	public:
		// Material factory methods.
		static Material* load(const std::string& name, const std::string* textures, RenderDevice* device);
		static void unload(Material*& mat);

		// Texture factory methods.
		static Texture2D* load_texture(const std::string& path, RenderDevice* device, bool srgb = false);
		static void unload_texture(Texture2D*& tex, RenderDevice* device);
		
		// Rendering related getters.
		inline Texture2D* texture(const uint32_t& index) { return m_textures[index];  }
		inline glm::vec4  albedo_value()				 { return m_albedo_val;	 }

	private:
		// Private constructor and destructor.
		Material(const std::string& name, const std::string* textures, RenderDevice* device);
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

		// Cached RenderDevice pointer for cleaning up GPU resources.
		RenderDevice* m_device = nullptr;
	};
} // namespace dw
