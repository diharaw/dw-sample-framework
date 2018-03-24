#pragma once

#include <macros.h>
#include <glm.hpp>
#include <unordered_map>

class Camera;
class RenderDevice;
struct SamplerState;
struct UniformBuffer;
struct RasterizerState;
struct DepthStencilState;
struct ShaderProgram;
struct Shader;
struct VertexArray;
struct VertexBuffer;
struct InputLayout;
struct Texture2D;

namespace dw
{ 
#define MAX_POINT_LIGHTS 32
	class Material;

	struct PointLight
	{
		glm::vec4 position;
		glm::vec4 color;
	};

	struct DirectionalLight
	{
		glm::vec4 direction;
		glm::vec4 color;
	};

	struct DW_ALIGNED(16) PerFrameUniforms
	{
		glm::mat4 lastViewProj;
		glm::mat4 viewProj;
		glm::mat4 invViewProj;
		glm::mat4 projMat;
		glm::mat4 viewMat;
		glm::vec4 viewPos;
		glm::vec4 viewDir;
	};

	struct DW_ALIGNED(16) PerEntityUniforms
	{
		glm::mat4 mvpMat;
		glm::mat4 modalMat;
		glm::vec4 worldPos;
		uint8_t padding[112];
	};

	struct DW_ALIGNED(16) PerSceneUniforms
	{
		PointLight 		 pointLights[MAX_POINT_LIGHTS];
		DirectionalLight directionalLight;
		int				 pointLightCount;
	};

	struct DW_ALIGNED(16) PerMaterialUniforms
	{
		glm::vec4  albedoValue;
	};

	class Scene;

	class Renderer
	{
	public:
		Renderer(RenderDevice* device);
		~Renderer();
		void set_scene(Scene* scene);
		Scene* scene();
		void render(Camera* camera);
		Shader* load_shader(int type, std::string& path, Material* mat);
		ShaderProgram* load_program(std::string& combined_name, uint32_t count, Shader** shaders);

	private:
		void render_shadow_maps();
		void render_atmosphere();
		void render_scene();
		void render_post_process();

	private:
		Scene* m_scene;
		RenderDevice* m_device;
		SamplerState* m_trilinear_sampler;
		SamplerState* m_bilinear_sampler;
		UniformBuffer* m_per_scene;
		UniformBuffer* m_per_frame;
		UniformBuffer* m_per_material;
		UniformBuffer* m_per_entity;
		RasterizerState* m_standard_rs;
		DepthStencilState* m_standard_ds;
		DepthStencilState* m_atmosphere_ds;
		VertexArray* m_quad_vao;
		VertexBuffer* m_quad_vbo;
		InputLayout* m_quad_layout;
		PerFrameUniforms m_per_frame_uniforms;
		PerSceneUniforms m_per_scene_uniforms;
		PerEntityUniforms m_per_entity_uniforms[1024];
		PerMaterialUniforms m_per_material_uniforms[1024];
		Texture2D* m_brdfLUT;
		std::unordered_map<std::string, ShaderProgram*> m_program_cache;
		std::unordered_map<std::string, Shader*> m_shader_cache;
	};
}
