#include <scene.h>
#include <utility.h>

#include <json.hpp>
#include <gtc/matrix_transform.hpp>
#include <render_device.h>
#include <material.h>
#include <macros.h>
#include <trm_loader.h>
#include <renderer.h>

#include <stb_image.h>

namespace dw
{
	Scene* Scene::load(const std::string& file, RenderDevice* device, Renderer* renderer)
	{
		std::string sceneJson;
		assert(Utility::ReadText(file, sceneJson));
		nlohmann::json json = nlohmann::json::parse(sceneJson.c_str());

		Scene* scene = new Scene();
		scene->m_device = device;

		std::string sceneName = json["name"];
		scene->m_name = sceneName;

		std::string envMap = json["environment_map"];
		TextureCube* cube = (TextureCube*)trm::load_image(envMap, TextureFormat::R16G16B16_FLOAT, device);
		scene->m_env_map = cube;

		std::string irradianceMap = json["irradiance_map"];
		cube = (TextureCube*)trm::load_image(envMap, TextureFormat::R16G16B16_FLOAT, device);
		scene->m_irradiance_map = cube;

		std::string prefilteredMap = json["prefiltered_map"];
		cube = (TextureCube*)trm::load_image(envMap, TextureFormat::R16G16B16_FLOAT, device);
		scene->m_prefiltered_map = cube;

		auto entities = json["entities"].array();

		for (auto& entity : entities)
		{
			Material* mat_override = nullptr;

			std::string name = entity["name"];
			std::string model = entity["model"];
			
			if (!entity["material"].is_null())
			{
				std::string material = entity["material"];
				mat_override = Material::load(material, device);
			}

			auto positionJson = entity["position"].array();
			glm::vec3 position = glm::vec3(positionJson[0], positionJson[1], positionJson[2]);

			auto scaleJson = entity["scale"].array();
			glm::vec3 scale = glm::vec3(scaleJson[0], scaleJson[1], scaleJson[2]);

			auto rotationJson = entity["rotation"].array();
			glm::vec3 rotation = glm::vec3(rotationJson[0], rotationJson[1], rotationJson[2]);

			Mesh* mesh = Mesh::load(model, device, mat_override);
			Entity* newEntity = new Entity();

			newEntity->m_name = name;
			newEntity->m_position = position;
			newEntity->m_rotation = rotation;
			newEntity->m_scale = scale;

			glm::mat4 H = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 P = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 B = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			glm::mat4 R = H * P * B;
			glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
			glm::mat4 T = glm::translate(glm::mat4(1.0f), position);

			newEntity->m_transform = T * R * S;

			auto shaderJson = entity["shader"];

			Shader* shaders[2];

			std::string vsFile = shaderJson["vs"];
			shaders[0] = renderer->load_shader(ShaderType::VERTEX, vsFile, nullptr);

			std::string fsFile = shaderJson["fs"];
			shaders[1] = renderer->load_shader(ShaderType::VERTEX, vsFile, nullptr);

			std::string combName = vsFile + fsFile;
			newEntity->m_program = renderer->load_program(combName, 2, &shaders[0]);

			scene->m_entities.push_back(newEntity);
		}

		return scene;
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		m_device->destroy(m_env_map);
		m_device->destroy(m_irradiance_map);
		m_device->destroy(m_prefiltered_map);

		for (auto entity : m_entities)
		{
			if (entity)
			{
				Mesh::unload(entity->m_mesh);
				delete entity;
			}
		}
	}

	void Scene::save(std::string path)
	{

	}
}