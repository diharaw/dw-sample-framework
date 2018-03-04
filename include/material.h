#pragma once

#include <unordered_map>
#include <string>
#include <glm.hpp>

struct Texture2D;
class  RenderDevice;

namespace dw
{
	class Material
	{
	private:
		Material(const std::string& path, RenderDevice* device);

	public:
		static Material* Load(const std::string& path, RenderDevice* device);
		static void Unload(Material*& mat);
		~Material();

		inline Texture2D* TextureAlbedo()	 { return m_AlbedoT;	 }
		inline Texture2D* TextureNormal()	 { return m_NormalT;	 }
		inline Texture2D* TextureMetalness() { return m_MetalnessT;  }
		inline Texture2D* TextureRoughness() { return m_RoughnessT;	 }
		inline glm::vec4  Albedo()			 { return m_AlbedoV;	 }
		inline float	  Metalness()		 { return m_MetalnessF;  }
		inline float	  Roughness()		 { return m_RoughnessF;  }

	private:
		static std::unordered_map<std::string, Material*> m_Cache;

		Texture2D* m_AlbedoT = nullptr;
		Texture2D* m_NormalT = nullptr;
		Texture2D* m_MetalnessT = nullptr;
		Texture2D* m_RoughnessT = nullptr;

		glm::vec4 m_AlbedoV = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		float m_MetalnessF = 0.5f;
		float m_RoughnessF = 0.5f;
		RenderDevice* m_Device = nullptr; // Temp
	};
}
