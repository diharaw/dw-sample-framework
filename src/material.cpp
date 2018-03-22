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

	Material::Material(const std::string& path, RenderDevice* device) : m_device(device), m_alpha(false)
	{
		std::string matJson;
		assert(Utility::ReadText(path, matJson));
		nlohmann::json json = nlohmann::json::parse(matJson.c_str());

		Texture2DCreateDesc desc;

		if (json.find("diffuse_map") != json.end())
		{
			std::string texPath = json["diffuse_map"];
			stbi_set_flip_vertically_on_load(true);
			int x, y, n;

			stbi_uc* data = stbi_load(texPath.c_str(), &x, &y, &n, 0);

			DW_ZERO_MEMORY(desc);

			desc.data = data;

			if (n == 4)
			{
				desc.format = TextureFormat::R8G8B8A8_UNORM_SRGB;
				m_alpha = true;
			}
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM_SRGB;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;
			
			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_albedo_tex = device->create_texture_2d(desc);

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
			stbi_set_flip_vertically_on_load(true);
			int x, y, n;

			stbi_uc* data = stbi_load(texPath.c_str(), &x, &y, &n, 0);

			DW_ZERO_MEMORY(desc);

			desc.data = data;

			if (n == 4)
				desc.format = TextureFormat::R8G8B8A8_UNORM;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_normal_tex = device->create_texture_2d(desc);

			if (!m_normal_tex)
			{
				LOG_ERROR("Failed to load Normal Map");
			}
		}

		if (json.find("metalness_map") != json.end())
		{
			std::string texPath = json["metalness_map"];
			stbi_set_flip_vertically_on_load(true);
			int x, y, n;

			stbi_uc* data = stbi_load(texPath.c_str(), &x, &y, &n, 0);

			DW_ZERO_MEMORY(desc);

			desc.data = data;
			
			if (n == 4)
				desc.format = TextureFormat::R8G8B8A8_UNORM;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_metalness_tex = device->create_texture_2d(desc);

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
			stbi_set_flip_vertically_on_load(true);
			int x, y, n;

			stbi_uc* data = stbi_load(texPath.c_str(), &x, &y, &n, 0);

			DW_ZERO_MEMORY(desc);

			desc.data = data;
			
			if (n == 4)
				desc.format = TextureFormat::R8G8B8A8_UNORM;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_roughness_tex = device->create_texture_2d(desc);

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
			m_device->destroy(m_albedo_tex);

		if (m_normal_tex)
			m_device->destroy(m_normal_tex);

		if (m_metalness_tex)
			m_device->destroy(m_metalness_tex);

		if (m_roughness_tex)
			m_device->destroy(m_roughness_tex);
	}
}
