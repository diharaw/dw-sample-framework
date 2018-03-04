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
	std::unordered_map<std::string, Material*> Material::m_Cache;

	Material* Material::Load(const std::string& path, RenderDevice* device)
	{
		if (m_Cache.find(path) == m_Cache.end())
		{
			LOG_INFO("Material Asset not in cache. Loading from disk.");

			Material* mat = new Material(path, device);
			m_Cache[path] = mat;
			return mat;
		}
		else
		{
			LOG_INFO("Material Asset already loaded. Retrieving from cache.");
			return m_Cache[path];
		}	
	}

	void Material::Unload(Material*& mat)
	{
		for (auto itr : m_Cache)
		{
			if (itr.second == mat)
			{
				m_Cache.erase(itr.first);	
				DW_SAFE_DELETE(mat);
				return;
			}
		}
	}

	Material::Material(const std::string& path, RenderDevice* device) : m_Device(device)
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

			if(n == 4)
				desc.format = TextureFormat::R8G8B8A8_UNORM_SRGB;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM_SRGB;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;
			
			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_AlbedoT = device->create_texture_2d(desc);

			if (!m_AlbedoT)
			{
				LOG_ERROR("Failed to load Albedo Map");
			}
		}
		else if (json.find("diffuse_value") != json.end())
		{
			m_AlbedoV.x = json["diffuse_value"]["r"];
			m_AlbedoV.y = json["diffuse_value"]["g"];
			m_AlbedoV.z = json["diffuse_value"]["b"];
			m_AlbedoV.w = json["diffuse_value"]["a"];
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
				desc.format = TextureFormat::R8G8B8A8_UNORM_SRGB;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM_SRGB;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_NormalT = device->create_texture_2d(desc);

			if (!m_NormalT)
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
				desc.format = TextureFormat::R8G8B8A8_UNORM_SRGB;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM_SRGB;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_MetalnessT = device->create_texture_2d(desc);

			if (!m_MetalnessT)
			{
				LOG_ERROR("Failed to load Metalness Map");
			}
		}
		if (json.find("metalness_value") != json.end())
		{
			m_MetalnessF = json["metalness_value"];
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
				desc.format = TextureFormat::R8G8B8A8_UNORM_SRGB;
			else if (n == 3)
				desc.format = TextureFormat::R8G8B8_UNORM_SRGB;
			else if (n == 1)
				desc.format = TextureFormat::R8_UNORM;

			desc.width = x;
			desc.height = y;
			desc.mipmap_levels = 10;

			m_RoughnessT = device->create_texture_2d(desc);

			if (!m_RoughnessT)
			{
				LOG_ERROR("Failed to load Roughness Map");
			}
		}
		if (json.find("roughness_value") != json.end())
		{
			m_RoughnessF = json["roughness_value"];
		}
	}

	Material::~Material()
	{
		if (m_AlbedoT)
			m_Device->destroy(m_AlbedoT);

		if (m_NormalT)
			m_Device->destroy(m_NormalT);

		if (m_MetalnessT)
			m_Device->destroy(m_MetalnessT);

		if (m_RoughnessT)
			m_Device->destroy(m_RoughnessT);
	}
}
