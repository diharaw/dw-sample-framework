#include <render_device.h>
#include <macros.h>
#include <material.h>
#include <utility.h>
#include <logger.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace dw
{
	std::unordered_map<std::string, Material*> Material::m_cache;
	std::unordered_map<std::string, Texture2D*> Material::m_texture_cache;

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material* Material::load(const std::string& name, const std::string* textures, RenderDevice* device)
	{
		if (m_cache.find(name) == m_cache.end())
		{
			DW_LOG_INFO("Material Asset not in cache. Loading from disk.");

			Material* mat = new Material(name, textures, device);
			m_cache[name] = mat;
			return mat;
		}
		else
		{
			DW_LOG_INFO("Material Asset already loaded. Retrieving from cache.");
			return m_cache[name];
		}	
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Texture2D* Material::load_texture(const std::string& path, RenderDevice* device, bool srgb)
	{
		if (m_texture_cache.find(path) == m_texture_cache.end())
		{
			Texture2DCreateDesc desc;
			DW_LOG_INFO("Texture Asset not in cache. Loading from disk.");

			std::string texPath = utility::path_for_resource(path);
	
			int x, y, n;
			stbi_uc* data = stbi_load(texPath.c_str(), &x, &y, &n, 0);

			DW_ZERO_MEMORY(desc);

			desc.data = data;

			if (srgb)
			{
				if (n == 4)
					desc.format = TextureFormat::R8G8B8A8_UNORM_SRGB;
				else if (n == 3)
					desc.format = TextureFormat::R8G8B8_UNORM_SRGB;
				else if (n == 1)
					desc.format = TextureFormat::R8_UNORM;
			}
			else
			{
				if (n == 4)
					desc.format = TextureFormat::R8G8B8A8_UNORM;
				else if (n == 3)
					desc.format = TextureFormat::R8G8B8_UNORM;
				else if (n == 1)
					desc.format = TextureFormat::R8_UNORM;
			}

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			Texture2D* tex = device->create_texture_2d(desc);

			m_texture_cache[path] = tex;
			return tex;
		}
		else
		{
			DW_LOG_INFO("Texture Asset already loaded. Retrieving from cache.");
			return m_texture_cache[path];
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Material::unload(Material*& mat)
	{
		for (auto itr : m_cache)
		{
			if (itr.second == mat)
			{
				m_cache.erase(itr.first);	
				DW_SAFE_DELETE(mat);
				return;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Material::unload_texture(Texture2D*& tex, RenderDevice* device)
	{
		for (auto itr : m_texture_cache)
		{
			if (itr.second == tex)
			{
				m_texture_cache.erase(itr.first);
				device->destroy(tex);
				return;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material::Material(const std::string& name, const std::string* textures, RenderDevice* device) : m_device(device)
	{
		for (uint32_t i = 0; i < 16; i++)
		{
			m_textures[i] = nullptr;

			if (!textures[i].empty())
			{
				// First index must always be diffuse/albedo, so SRGB is set to true.
				m_textures[i] = load_texture(textures[i], m_device, i == 0 ? true : false);
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material::~Material()
	{
		for (uint32_t i = 0; i < 16; i++)
		{
			if (m_textures[i])
				unload_texture(m_textures[i], m_device);
		}
	}
} // namespace dw
