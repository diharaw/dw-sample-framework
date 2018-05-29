#pragma once

#include <macros.h>
#include <glm.hpp>
#include <unordered_map>

class Camera;
class RenderDevice;
struct SamplerState;
struct UniformBuffer;
struct RasterizerState;
struct BlendState;
struct DepthStencilState;
struct ShaderProgram;
struct Shader;
struct VertexArray;
struct VertexBuffer;
struct InputLayout;
struct Texture2D;
struct Framebuffer;

class Shadows;

namespace dw
{ 
#define MAX_POINT_LIGHTS 32
#define MAX_SHADOW_FRUSTUM 8

	class Material;

	struct PointLight
	{
		DW_ALIGNED(16) glm::vec4 position;
		DW_ALIGNED(16) glm::vec4 color;
	};

	struct DirectionalLight
	{
		DW_ALIGNED(16) glm::vec4 direction;
		DW_ALIGNED(16) glm::vec4 color;
	};

	struct ShadowFrustum
	{
		DW_ALIGNED(16) glm::mat4 shadowMatrix;
		DW_ALIGNED(16) float	 farPlane;
	};

	struct PerFrameUniforms
	{
		DW_ALIGNED(16) glm::mat4	 lastViewProj;
		DW_ALIGNED(16) glm::mat4	 viewProj;
		DW_ALIGNED(16) glm::mat4	 invViewProj;
		DW_ALIGNED(16) glm::mat4	 projMat;
		DW_ALIGNED(16) glm::mat4	 viewMat;
		DW_ALIGNED(16) glm::vec4	 viewPos;
		DW_ALIGNED(16) glm::vec4	 viewDir;
		DW_ALIGNED(16) int			 numCascades;
		DW_ALIGNED(16) ShadowFrustum shadowFrustums[MAX_SHADOW_FRUSTUM];
	};

	struct PerEntityUniforms
	{
		DW_ALIGNED(16) glm::mat4 mvpMat;
		DW_ALIGNED(16) glm::mat4 lastMvpMat;
		DW_ALIGNED(16) glm::mat4 modalMat;
		DW_ALIGNED(16) glm::vec4 worldPos;
		uint8_t	  padding[48];
	};

	struct PerSceneUniforms
	{
		DW_ALIGNED(16) PointLight 		pointLights[MAX_POINT_LIGHTS];
		DW_ALIGNED(16) DirectionalLight directionalLight;
		DW_ALIGNED(16) int				pointLightCount;
	};

	struct PerMaterialUniforms
	{
		DW_ALIGNED(16) glm::vec4 albedoValue;
		DW_ALIGNED(16) glm::vec4 metalnessRoughness;
	};

	struct PerFrustumSplitUniforms
	{
		DW_ALIGNED(16) glm::mat4 crop_matrix;
	};

	class Scene;

	class Renderer
	{
	public:
		Renderer(RenderDevice* device, uint16_t width, uint16_t height);
		~Renderer();
		void set_scene(Scene* scene);
		Scene* scene();
		void render(Camera* camera, uint16_t w = 0, uint16_t h = 0, Shadows* shadows = nullptr, Framebuffer* fbo = nullptr);
		Shader* load_shader(int type, std::string& path, Material* mat);
		ShaderProgram* load_program(std::string& combined_name, uint32_t count, Shader** shaders);

		inline PerSceneUniforms* per_scene_uniform() { return &m_per_scene_uniforms; }

	private:
		void create_cube();
		void create_quad();
		void render_shadow_maps(Shadows* shadows);
		void render_atmosphere();
		void render_scene(uint16_t w = 0, uint16_t h = 0, Framebuffer* fbo = nullptr);
		void render_post_process(Shadows* shadows);

	private:
		uint16_t m_width;
		uint16_t m_height;
		Scene* m_scene;
		RenderDevice* m_device;
		SamplerState* m_trilinear_sampler;
		SamplerState* m_bilinear_sampler;
		UniformBuffer* m_per_scene;
		UniformBuffer* m_per_frame;
		UniformBuffer* m_per_material;
		UniformBuffer* m_per_entity;
		UniformBuffer* m_per_frustum_split;
		RasterizerState* m_standard_rs;
		RasterizerState* m_atmosphere_rs;
		DepthStencilState* m_standard_ds;
		DepthStencilState* m_atmosphere_ds;
		BlendState*	  m_standard_bs;
		VertexArray*  m_quad_vao;
		VertexBuffer* m_quad_vbo;
		InputLayout*  m_quad_layout;
		VertexArray*  m_cube_vao;
		VertexBuffer* m_cube_vbo;
		InputLayout*  m_cube_layout;
		PerFrameUniforms m_per_frame_uniforms;
		PerSceneUniforms m_per_scene_uniforms;
		PerEntityUniforms m_per_entity_uniforms[1024];
		PerMaterialUniforms m_per_material_uniforms[1024];
		Shader*		   m_cube_map_vs;
		Shader*		   m_cube_map_fs;
		ShaderProgram* m_cube_map_program;
		Shader*		   m_pssm_vs;
		Shader*		   m_pssm_fs;
		ShaderProgram* m_pssm_program;
		Shader*		   m_quad_vs;
		Shader*		   m_quad_fs;
		ShaderProgram* m_quad_program;
		Texture2D*	   m_brdfLUT;
		Texture2D*	   m_color_buffer;
		Texture2D*	   m_depth_buffer;
		Framebuffer*   m_color_fbo;
		std::unordered_map<std::string, ShaderProgram*> m_program_cache;
		std::unordered_map<std::string, Shader*> m_shader_cache;
	};
}
