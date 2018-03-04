#include "scene.h"
#include "utility.h"

#include <json.hpp>
#include <gtc/matrix_transform.hpp>
#include <render_device.h>

namespace dw
{
	Scene* Scene::Load(const std::string& file, RenderDevice* device)
	{
		std::string sceneJson;
		assert(Utility::ReadText(file, sceneJson));
		nlohmann::json json = nlohmann::json::parse(sceneJson.c_str());

		Scene* scene = new Scene();

		std::string sceneName = json["name"];
		scene->m_Name = sceneName;

		auto entities = json["entities"].array();

		for (auto& entity : entities)
		{
			std::string name = entity["name"];
			std::string model = entity["model"];

			auto positionJson = entity["position"].array();
			glm::vec3 position = glm::vec3(positionJson[0], positionJson[1], positionJson[2]);

			auto scaleJson = entity["scale"].array();
			glm::vec3 scale = glm::vec3(scaleJson[0], scaleJson[1], scaleJson[2]);

			auto rotationJson = entity["rotation"].array();
			glm::vec3 rotation = glm::vec3(rotationJson[0], rotationJson[1], rotationJson[2]);

			Mesh* mesh = Mesh::Load(model, device);
			Entity* newEntity = new Entity(name, mesh, position);

			//newEntity->Position = position;
			//newEntity->Transform = glm::translate(glm::mat4(1.0f), position);

			scene->m_Entities.push_back(newEntity);
		}

		return scene;
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		for (auto entity : m_Entities)
		{
			if (entity)
				delete entity;
		}
	}
}