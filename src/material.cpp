#include <render_device.h>

#include <macros.h>
#include <material.h>
#include <utility.h>
#include <logger.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <json.hpp>

namespace dw
{
	std::unordered_map<std::string, Material*> Material::m_cache;
	std::unordered_map<std::string, Texture2D*> Material::m_texture_cache;

	Material* Material::load(const std::string& path, RenderDevice* device)
	{
		if (m_cache.find(path) == m_cache.end())
		{
			LOG_INFO("Material Asset not in cache. Loading from disk.");

			Material* mat = new Material(path, device);
			m_cache[path] = mat;
			return mat;
		}
		else
		{
			LOG_INFO("Material Asset already loaded. Retrieving from cache.");
			return m_cache[path];
		}	
	}

	Texture2D* Material::load_texture(const std::string& path, RenderDevice* device, bool srgb)
	{
		if (m_texture_cache.find(path) == m_texture_cache.end())
		{
			Texture2DCreateDesc desc;
			LOG_INFO("Texture Asset not in cache. Loading from disk.");

			std::string texPath = "assets/texture/";
			texPath += path;
			//stbi_set_flip_vertically_on_load(true);
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
			LOG_INFO("Texture Asset already loaded. Retrieving from cache.");
			return m_texture_cache[path];
		}
	}

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

	Material::Material(const std::string& path, RenderDevice* device) : m_device(device), m_alpha(false)
	{
		std::string matJson;
		bool result = Utility::ReadText("assets/" + path, matJson);
		assert(result);
		nlohmann::json json = nlohmann::json::parse(matJson.c_str());

		Texture2DCreateDesc desc;

		if (json.find("diffuse_map") != json.end())
		{
			std::string texPath = json["diffuse_map"];
			m_albedo_tex = load_texture(texPath, device, true);

			if (!m_albedo_tex)
			{
				LOG_ERROR("Failed to load Albedo Map");
			}
		}
		else if (json.find("diffuse_value") != json.end())
		{
			m_albedo_val.x = json["diffuse_value"]["r"];
			m_albedo_val.y = json["diffuse_value"]["g"];
			m_albedo_val.z = json["diffuse_value"]["b"];
			m_albedo_val.w = json["diffuse_value"]["a"];
		}

		if (json.find("normal_map") != json.end())
		{
			std::string texPath = json["normal_map"];
			m_normal_tex = load_texture(texPath, device);

			if (!m_normal_tex)
			{
				LOG_ERROR("Failed to load Normal Map");
			}
		}

		if (json.find("metalness_map") != json.end())
		{
			std::string texPath = json["metalness_map"];
			m_metalness_tex = load_texture(texPath, device);

			if (!m_metalness_tex)
			{
				LOG_ERROR("Failed to load Metalness Map");
			}
		}
		if (json.find("metalness_value") != json.end())
		{
			m_metalness_val = json["metalness_value"];
		}

		if (json.find("roughness_map") != json.end())
		{
			std::string texPath = json["roughness_map"];
			m_roughness_tex = load_texture(texPath, device);

			if (!m_roughness_tex)
			{
				LOG_ERROR("Failed to load Roughness Map");
			}
		}
		if (json.find("roughness_value") != json.end())
		{
			m_roughness_val = json["roughness_value"];
		}
	}

	Material::~Material()
	{
		if (m_albedo_tex)
			unload_texture(m_albedo_tex, m_device);

		if (m_normal_tex)
			unload_texture(m_normal_tex, m_device);

		if (m_metalness_tex)
			unload_texture(m_metalness_tex, m_device);

		if (m_roughness_tex)
			unload_texture(m_roughness_tex, m_device);
	}
}
