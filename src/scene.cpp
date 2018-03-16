#include <scene.h>
#include <utility.h>

#include <json.hpp>
#include <gtc/matrix_transform.hpp>
#include <render_device.h>

namespace dw
{
	Scene* Scene::load(const std::string& file, RenderDevice* device)
	{
		std::string sceneJson;
		assert(Utility::ReadText(file, sceneJson));
		nlohmann::json json = nlohmann::json::parse(sceneJson.c_str());

		Scene* scene = new Scene();

		std::string sceneName = json["name"];
		scene->m_name = sceneName;

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

			Mesh* mesh = Mesh::load(model, device);
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

			scene->m_entities.push_back(newEntity);
		}

		return scene;
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		for (auto entity : m_entities)
		{
			if (entity)
				delete entity;
		}
	}
}