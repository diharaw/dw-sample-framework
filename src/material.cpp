#include <macros.h>
#include <material.h>
#include <utility.h>
#include <logger.h>

namespace dw
{
	std::unordered_map<std::string, Material*> Material::m_cache;
	std::unordered_map<std::string, Texture2D*> Material::m_texture_cache;

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material* Material::load(const std::string& name, const std::string* textures)
	{
		if (m_cache.find(name) == m_cache.end())
		{
			DW_LOG_INFO("Material Asset not in cache. Loading from disk.");

			Material* mat = new Material(name, textures);
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

	Material* Material::load(const std::string& name, int num_textures, Texture2D** textures, glm::vec4 albedo, float roughness, float metalness)
	{
		if (m_cache.find(name) == m_cache.end())
		{
			DW_LOG_INFO("Material Asset not in cache. Loading from disk.");

			Material* mat = new Material();

			for (int i = 0; i < num_textures; i++)
				mat->m_textures[i] = textures[i];

			mat->m_albedo_val = albedo;

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

	bool Material::is_loaded(const std::string& name)
	{
		return m_cache.find(name) != m_cache.end();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Texture2D* Material::load_texture(const std::string& path, bool srgb)
	{
		if (m_texture_cache.find(path) == m_texture_cache.end())
		{
            Texture2D* tex = Texture2D::create_from_files(path, srgb);
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

	void Material::unload_texture(Texture2D*& tex)
	{
		for (auto itr : m_texture_cache)
		{
			if (itr.second == tex)
			{
				m_texture_cache.erase(itr.first);
                DW_SAFE_DELETE(tex);
				return;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material::Material() {}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material::Material(const std::string& name, const std::string* textures)
	{
		for (uint32_t i = 0; i < 16; i++)
		{
			m_textures[i] = nullptr;

			if (!textures[i].empty())
			{
				// First index must always be diffuse/albedo, so SRGB is set to true.
				m_textures[i] = load_texture(textures[i], i == 0 ? true : false);
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Material::~Material()
	{
		for (uint32_t i = 0; i < 16; i++)
		{
			if (m_textures[i])
				unload_texture(m_textures[i]);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
