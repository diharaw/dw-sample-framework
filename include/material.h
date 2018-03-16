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
		static Material* load(const std::string& path, RenderDevice* device);
		static void unload(Material*& mat);
		~Material();

		inline Texture2D* texture_albedo()	 { return m_albedo_tex;	 }
		inline Texture2D* texture_normal()	 { return m_normal_tex;	 }
		inline Texture2D* texture_metalness() { return m_metalness_tex;  }
		inline Texture2D* texture_roughness() { return m_roughness_tex;	 }
		inline glm::vec4  albedo()			 { return m_albedo_val;	 }
		inline float	  metalness()		 { return m_metalness_val;  }
		inline float	  roughness()		 { return m_roughness_val;  }

	private:
		static std::unordered_map<std::string, Material*> m_cache;

		Texture2D* m_albedo_tex = nullptr;
		Texture2D* m_normal_tex = nullptr;
		Texture2D* m_metalness_tex = nullptr;
		Texture2D* m_roughness_tex = nullptr;
		bool m_alpha;
		glm::vec4 m_albedo_val = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		float m_metalness_val = 0.5f;
		float m_roughness_val = 0.5f;
		RenderDevice* m_device = nullptr; // Temp
	};
}
